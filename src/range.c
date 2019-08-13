/*
    SAS5 システム設計基礎パート
    測距センサー用プログラム
 */

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include "vl53l0x_def.h"
#include "msgQLib.h"
#include <pthread.h>

#define MODE_RANGE		0
#define MODE_XTAKCALIB	1
#define MODE_OFFCALIB	2
#define MODE_HELP		3
#define MODE_PARAMETER  6


//******************************** IOCTL definitions
#define VL53L0X_IOCTL_INIT			_IO('p', 0x01)
#define VL53L0X_IOCTL_XTALKCALB		_IOW('p', 0x02, unsigned int)
#define VL53L0X_IOCTL_OFFCALB		_IOW('p', 0x03, unsigned int)
#define VL53L0X_IOCTL_STOP			_IO('p', 0x05)
#define VL53L0X_IOCTL_SETXTALK		_IOW('p', 0x06, unsigned int)
#define VL53L0X_IOCTL_SETOFFSET		_IOW('p', 0x07, int8_t)
#define VL53L0X_IOCTL_GETDATAS		_IOR('p', 0x0b, VL53L0X_RangingMeasurementData_t)
#define VL53L0X_IOCTL_PARAMETER		_IOWR('p', 0x0d, struct stmvl53l0x_parameter)

//modify the following macro accoring to testing set up
#define OFFSET_TARGET		100//200
#define XTALK_TARGET		600//400
#define NUM_SAMPLES			20//20

typedef enum {
	OFFSET_PAR = 0,
	XTALKRATE_PAR = 1,
	XTALKENABLE_PAR = 2,
	GPIOFUNC_PAR = 3,
	LOWTHRESH_PAR = 4,
	HIGHTHRESH_PAR = 5,
	DEVICEMODE_PAR = 6,
	INTERMEASUREMENT_PAR = 7,
	REFERENCESPADS_PAR = 8,
	REFCALIBRATION_PAR = 9,
} parameter_name_e;
/*
 *  IOCTL parameter structs
 */
struct stmvl53l0x_parameter {
	uint32_t is_read; //1: Get 0: Set
	parameter_name_e name;
	int32_t value;
	int32_t value2;
	int32_t status;
};

/* for SAS */
#define MAX_RANGE				(70 * 10)		/* 70cm */
#define MIN_RANGE				(50)			/* 5cm*/
#define BESTSHOT_UPPER_LIMIT	(20 * 10 + 20)	/* 20cm + 2cm */
#define BESTSHOT_LOWER_LIMIT	(20 * 10 - 20)	/* 20cm - 2cm */
enum range_state {
	initial = 0,
	detect = 1,
	speed,
	dooropen,
	doorclose
};
int range_status = initial;
#define ON 1
#define OFF 0
#define SAMPLE_NUM 3
#define SAMPLE_INT 20
#define MAX_NEGATIVE 5
#define TOLERANCE 5
#define AVERAGE_NUM 5
#define OPEN_WAIT_TIME (4000 / SAMPLE_INT)
#define ULTRA_SPEED		500
#define HIGH_SPEED		300
#define NORMAL_SPPED	50
#define SLOW_SPEED		20
#define GATE_DISTANCE	250
static void * ranging(void * arg);
static int initial_flag = 0;
static int mode = 0;
static int open_timing = 0;
static int picture = 0;
static int has_taken = 0;
int motor_initialize(void);
int motor_open(void);
int motor_close(void);
int motor_set_ultraspeed();
int motor_set_highspeed();
int motor_set_normalspeed();
int motor_set_slowspeed();
extern MSG_Q_ID sas_msg;
enum msg_num{
	detect_result = 0,
	calculate_result,
	recognize_result,
	taking_now,
	open_result,
	close_result,
	disappear_object = 9
};

int start_detectdistance(void) {
	int ret = 0;
	pthread_t pthread;
	printf("start_detectdistance\n");

	if (initial_flag == OFF) {
		ret = motor_initialize();
		if (ret) {
			printf("motor initialize failed...\n");
			return -1;
		}
		
		ret = pthread_create(&pthread, NULL, &ranging, NULL);
		if (ret) {
			printf("thread create error...\n");
			return -1;
		}

		initial_flag = ON;
	}

	range_status = detect;
	return ret;
}

int detect_speed() {
	int ret = 0;
	printf("detect_speed\n");

	range_status = speed;

	return ret;
}

int prepare_picture() {
	int ret = 0;
	printf("prepare_picture\n");

	if (!has_taken) {
		picture = ON;
	}

	return ret;
}

int open_door() {
	int ret = 0;
	printf("open_door %d\n", open_timing);

	range_status = dooropen;

	return ret;
}

int close_door(void) {
	int ret = 0;
	printf("close_door\n");

	range_status = doorclose;

	return ret;
}

static void * ranging(void * arg)
{
	int fd;
	int ret;
	VL53L0X_RangingMeasurementData_t range_datas;
	struct stmvl53l0x_parameter parameter;
	unsigned int targetDistance=0;
	int i = 0;
	char buf[10];
	uint8_t detect_count = 0;
	int sample_count = 0;
	int sample_prev = 0;
	int sample_diff = 0;
	int average = 0;
	int average_count = 0;
	int micro_average = 0;
	int negative_count = 0;
	int open_wait = 0;

	fd = open("/dev/stmvl53l0x_ranging",O_RDWR | O_SYNC);
	mode = MODE_XTAKCALIB;
	if (fd <= 0)
	{
		fprintf(stderr,"Error open stmvl53l0x_ranging device: %s\n", strerror(errno));
		return NULL;
	}
	// 処理開始前に念のためSTOP
	if (ioctl(fd, VL53L0X_IOCTL_STOP , NULL) < 0) {
		fprintf(stderr, "Error: Could not perform VL53L0X_IOCTL_STOP : %s\n", strerror(errno));
		close(fd);
		return NULL;
	}
	// xtalkキャリブレーション？
	if (mode == MODE_XTAKCALIB)
	{
		unsigned int XtalkInt = 0;
		uint8_t XtalkEnable = 0;
		fprintf(stderr, "xtalk Calibrate place black target at %dmm from glass===\n",XTALK_TARGET);
		// to xtalk calibration 
		targetDistance = XTALK_TARGET;
		if (ioctl(fd, VL53L0X_IOCTL_XTALKCALB , &targetDistance) < 0) {
			fprintf(stderr, "Error: Could not perform VL53L0X_IOCTL_XTALKCALB : %s\n", strerror(errno));
			close(fd);
			return NULL;
		}
		// to get xtalk parameter
		parameter.is_read = 1;
		parameter.name = XTALKRATE_PAR;
		if (ioctl(fd, VL53L0X_IOCTL_PARAMETER , &parameter) < 0) {
			fprintf(stderr, "Error: Could not perform VL53L0X_IOCTL_PARAMETER : %s\n", strerror(errno));
			close(fd);
			return NULL;
		}
		XtalkInt = (unsigned int)parameter.value;
		parameter.name = XTALKENABLE_PAR;
		if (ioctl(fd, VL53L0X_IOCTL_PARAMETER , &parameter) < 0) {
			fprintf(stderr, "Error: Could not perform VL53L0X_IOCTL_PARAMETER : %s\n", strerror(errno));
			close(fd);
			return NULL;
		}
		XtalkEnable = (uint8_t)parameter.value;
		fprintf(stderr, "VL53L0 Xtalk Calibration get Xtalk Compensation rate in fixed 16 point as %u, enable:%u\n",XtalkInt,XtalkEnable);
		
		for(i = 0; i <= NUM_SAMPLES; i++)
		{
			usleep(30 *1000); /*100ms*/
					// to get all range data
			ioctl(fd, VL53L0X_IOCTL_GETDATAS,&range_datas);	
		}
		fprintf(stderr," VL53L0 DMAX calibration Range Data:%d,  signalRate_mcps:%d\n",range_datas.RangeMilliMeter, range_datas.SignalRateRtnMegaCps);
		// get rangedata of last measurement to avoid incorrect datum from unempty buffer 
	}

	// オフセットキャリブレーション？
	if (mode == MODE_OFFCALIB) {
		int offset=0;
		uint32_t SpadCount=0;
		uint8_t IsApertureSpads=0;
		uint8_t VhvSettings=0,PhaseCal=0;

		// to xtalk calibration 
		targetDistance = OFFSET_TARGET;
		if (ioctl(fd, VL53L0X_IOCTL_OFFCALB , &targetDistance) < 0) {
			fprintf(stderr, "Error: Could not perform VL53L0X_IOCTL_OFFCALB : %s\n", strerror(errno));
			close(fd);
			return NULL;
		}
		// to get current offset
		parameter.is_read = 1;
		parameter.name = OFFSET_PAR;
		if (ioctl(fd, VL53L0X_IOCTL_PARAMETER, &parameter) < 0) {
			fprintf(stderr, "Error: Could not perform VL53L0X_IOCTL_PARAMETER : %s\n", strerror(errno));
			close(fd);
			return NULL;
		}
		offset = (int)parameter.value;
		fprintf(stderr, "get offset %d micrometer===\n",offset);
		
		parameter.name = REFCALIBRATION_PAR;
		if (ioctl(fd, VL53L0X_IOCTL_PARAMETER, &parameter) < 0) {
			fprintf(stderr, "Error: Could not perform VL53L0X_IOCTL_PARAMETER : %s\n", strerror(errno));
			close(fd);
			return NULL;
		}
		VhvSettings = (uint8_t) parameter.value;
		PhaseCal=(uint8_t) parameter.value2;
		fprintf(stderr, "get VhvSettings is %u ===\nget PhaseCas is %u ===\n", VhvSettings,PhaseCal);
		
		parameter.name =REFERENCESPADS_PAR;
		if (ioctl(fd, VL53L0X_IOCTL_PARAMETER, &parameter) < 0) {
			fprintf(stderr, "Error: Could not perform VL53L0X_IOCTL_PARAMETER : %s\n", strerror(errno));
			close(fd);
			return NULL;
		}
		SpadCount = (uint32_t)parameter.value;
		IsApertureSpads=(uint8_t) parameter.value2;
		fprintf(stderr, "get SpadCount is %d ===\nget IsApertureSpads is %u ===\n", SpadCount,IsApertureSpads);
	}

	// 初期化	
	if (ioctl(fd, VL53L0X_IOCTL_INIT , NULL) < 0) {
		fprintf(stderr, "Error: Could not perform VL53L0X_IOCTL_INIT : %s\n", strerror(errno));
		close(fd);
		return NULL;
	}

	// データ取得開始
	while (1)
	{
		// 測距間隔はSAMPLE_INT
		usleep(SAMPLE_INT * 1000);
		ioctl(fd, VL53L0X_IOCTL_GETDATAS,&range_datas);

		// 何かいた？
		if (range_status == detect && (MIN_RANGE < range_datas.RangeMilliMeter) && (range_datas.RangeMilliMeter < MAX_RANGE)) {
			detect_count++;
			if (detect_count > 5) {
				printf("achieved five count. go to confirming human mode!\n");
				buf[detect_result] = true;
				ret = msgQSend(sas_msg, buf, sizeof(buf));
				if (ret) {
					printf("ranging detect msg send error\n");
					return NULL;
				}
				// 処理後もまだステータスが変わっていない場合はinitialへ(状態の入れ子防止)
				if (range_status == detect) {
					range_status = initial;
				}
				buf[0] = 0;
				detect_count = 0;
			}
		} else {
			detect_count = 0;
		}

		// ドア開タイミング計算
		// 本モードでは測距データを一定区間サンプリングし、平均移動距離からドアを開けるタイミング(距離)を決定する
		// ミクロで見るとセンサー値が前後するため、数区間(AVERAGE_NUM分)の平均値を1サンプルとする
		if (range_status == speed) {
			if (average_count < AVERAGE_NUM) {
				micro_average += range_datas.RangeMilliMeter;
				average_count++;
			}
			// サンプル区間の平均が取れたら平均移動距離計算に投入する
			if (average_count == AVERAGE_NUM) {
				micro_average /= AVERAGE_NUM;
				average_count = 0;
				if (micro_average > 4000) {
					printf("irregular value...\n");
					sample_count--;
					negative_count++;
					// ある程度の区間(MAX_NEGATIVE分)以上前進がなかった場合は計算やり直し
					if (negative_count > MAX_NEGATIVE) {
						sample_count = -1;
						negative_count = 0;
						sample_diff = 0;
					}
				}
				else if (sample_count == 0) {
					sample_prev = micro_average;
					printf("sample start!\n");
				} else if (sample_count > 0) {
					// ひとつ前のサンプルとの差分を蓄積
					if (sample_prev > micro_average) {
						printf("sample count%d\n", sample_count);
						sample_diff += (sample_prev - micro_average);
						sample_prev = micro_average;
						negative_count = 0;
					} else {
						printf("negative direction! prev=%d crnt=%d\n", sample_prev, micro_average);
						negative_count++;
						sample_count--;
						// ある程度の区間(MAX_NEGATIVE分)以上前進がなかった場合は計算やり直し
						if (negative_count > MAX_NEGATIVE) {
							sample_count = -1;
							negative_count = 0;
							sample_diff = 0;
						}
					}
				} else {
					printf("??? %d\n", sample_count);
				}
				printf("sample data[%d] = %d  diff = %d\n", sample_count, micro_average, sample_diff);
				sample_count++;
			}
			// 所定のデータ量蓄積出来たら平均値を計算
			if (sample_count == SAMPLE_NUM) {
				average = sample_diff / (SAMPLE_NUM - 1);

				// 平均移動距離からモーター速度を段階的に設定
				if (average > ULTRA_SPEED) {
					motor_set_ultraspeed();
				} else if (average > HIGH_SPEED) {
					motor_set_highspeed();
				} else if (average > NORMAL_SPPED) {
					motor_set_normalspeed();
				} else {
					motor_set_slowspeed();
				}
				buf[calculate_result] = true;
				open_timing = ((1000 / (SAMPLE_INT * AVERAGE_NUM)) * average);		  // 到達1秒前に開けるので平均移動距離から1秒間で移動する距離を算出
				// ゲートより手前になってしまったらゲートまでの距離を設定
				if (open_timing < GATE_DISTANCE) {
					open_timing = GATE_DISTANCE;
				}
				printf("average = %d\n", average);
				ret = msgQSend(sas_msg, buf, sizeof(buf));
				if (ret) {
					printf("ranging speed msg send error\n");
					return NULL;
				}
				// 処理後もまだステータスが変わっていない場合はinitialへ(状態の入れ子防止)
				if (range_status == speed) {
					range_status = initial;
				}
				buf[calculate_result] = 0;
				sample_count = 0;
			}
		}

		// 所定位置まで来ていたらドアを開ける
		// 既に所定位置を過ぎていてもドアを開ける
		if (range_status == dooropen) {
//			if ((open_timing - TOLERANCE) < range_datas.RangeMilliMeter && (range_datas.RangeMilliMeter < open_timing + TOLERANCE)) {
			if (range_datas.RangeMilliMeter < open_timing) {
				if (!open_wait) {
					motor_open();
					open_wait++;
				}
			}
#if 0
			sleep(5);			// ドアが開き切るまでwait
#else
			// ドアが開ききるまでメッセージを送信しない
			if (open_wait != 0) {
				open_wait++;
				int delay = 0;
				if (average > ULTRA_SPEED) {
					delay = OPEN_WAIT_TIME / 4;
				} else if (average > HIGH_SPEED) {
					delay = OPEN_WAIT_TIME / 2;
				} else {
					delay = OPEN_WAIT_TIME;
				}
				if (open_wait >= delay) {
					buf[open_result] = true;
					ret = msgQSend(sas_msg, buf, sizeof(buf));
					if (ret) {
						printf("ranging door open msg send error\n");
						return NULL;
					}
					// 処理後もまだステータスが変わっていない場合はinitialへ(状態の入れ子防止)
					if (range_status == dooropen) {
						range_status = initial;
					}
					buf[open_result] = 0;
					open_wait = 0;
				}
			}
#endif
		}

		// ドア開中に人がいなくなったらドアを閉める
		if (range_status == doorclose && ((MIN_RANGE > range_datas.RangeMilliMeter) || (range_datas.RangeMilliMeter > MAX_RANGE))) {
			motor_close();
			buf[close_result] = true;
			ret = msgQSend(sas_msg, buf, sizeof(buf));
			if (ret) {
				printf("ranging door close msg send error\n");
				return NULL;
			}
			
			// 処理後もまだステータスが変わっていない場合はinitialへ(状態の入れ子防止)
			if (range_status == doorclose) {
				range_status = initial;
			}
			buf[close_result] = 0;
			has_taken = OFF;
			picture = OFF;
		}

#if 0
		// 写真準備指示がある場合、ベストショット距離かどうかを監視し、範囲内に来たら通知する
		if (picture && (BESTSHOT_LOWER_LIMIT < range_datas.RangeMilliMeter && range_datas.RangeMilliMeter < BESTSHOT_UPPER_LIMIT)) {
#else
		// 写真準備指示がある場合、ベストショット距離より手前にいれば通知する
		if (picture && (range_datas.RangeMilliMeter < BESTSHOT_UPPER_LIMIT)) {
#endif
			buf[taking_now] = true;
			ret = msgQSend(sas_msg, buf, sizeof(buf));
			if (ret) {
				printf("taking now msg send error\n");
				return NULL;
			}
			picture = OFF;
			has_taken = ON;
			buf[taking_now] = 0;
		}
#if 0
		fprintf(stderr," VL53L0 Range Data:%d, error status:0x%x, signalRate_mcps:%d, Amb Rate_mcps:%d  %d\n",
				range_datas.RangeMilliMeter, range_datas.RangeStatus, range_datas.SignalRateRtnMegaCps, range_datas.AmbientRateRtnMegaCps, range_status);
#else
		//printf("%d\n", range_datas.RangeMilliMeter);
#endif

	}
	close(fd);
	return NULL;
}
