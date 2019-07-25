#include <stdio.h>
#include "msgQLib.h"

#define ThreashC 0
#define ThreashD 0

int start_camera(void);
int start_detectdistance(void);
int detect_speed();
int ishuman();
int prepare_picture();
int take_picture();
int open_door();
int close_door();
MSG_Q_ID sas_msg;
int taking_picture = 0;

enum ad_status{
	initialize = 0,
	start_process,
	wait_detect,
	wait_calculate,
	approval_open,
	wait_close,
};
enum msg_num{
	detect_result = 0,
	calculate_result,
	recognize_result,
	taking_now,
	open_result,
	close_result,
	disappear_object = 9
};

int main(void)
{
	enum ad_status status = initialize;
	int ret = 0;
	char buf[10] = {0};
	sas_msg = msgQCreate(10, 10);
	if (!sas_msg) {
		printf("msg create error...\n");
		return -1;
	}

	// カメラスタート(いずれstart_process時に実行するようにする)
	start_camera();
	
	while(1){
		switch(status){
		case initialize:		start_detectdistance();			break;					// 初期化および物体検知待ち
		case start_process:		detect_speed();	ishuman();		break;					// 処理開始
		case wait_detect:										break;					// 何もしない
		case wait_calculate:	prepare_picture();				break;					// ベストショット取得準備
		case approval_open:		prepare_picture(); open_door();	break;					// ベストショット取得準備＆ドア開
		case wait_close:		close_door();					break;					// ドア閉
		default:	printf("abnormal status(%d)\n", status);	return -1;
		}

		// メッセージは状態遷移に伴う関数実行後に待つ(初回initializeを実行するため)
		ret = msgQReceive(sas_msg, buf, sizeof(buf));
		if (ret == -1) {
			printf("main msg receive error ret=%d\n", ret);
			return ret;
		}

		for (int i = 0; i < 10; i++) {
			printf("message received buf[%d] = %d\n", i, buf[i]);
		}

		// 測距センサー検知無し
		if (buf[disappear_object] == 1) {
			status = initialize;					// 初期状態へ
		} else {
			// 状態遷移
			switch (status) {
				// 初期状態
				case initialize:
					// 測距センサー検知あり
					if (buf[detect_result]) {
						printf("initialize detect sensor\n");
						status = start_process;		// 処理開始へ
					} else {
						printf("not detected...\n");
						status = initialize;		// 初期状態へ
					}
					break;
				// 処理開始
				case start_process:
					// ドア開距離計算＆人物検知完了(多分同時に来るケース無し)
					if (buf[calculate_result] && buf[recognize_result]) {
						printf("calc ok and human exist!\n");
						status = approval_open;		// ドア開許可
					// ドア開距離計算が先に完了
					} else if (buf[calculate_result]) {
						printf("calc ok\n");
						status = wait_detect;		// 人物検知待ち
					// 人物検知が先に完了
					} else if (buf[recognize_result]) {
						printf("human exist\n");
						status = wait_calculate;	// ドア開距離計算待ち
					} else {
						printf("other status...\n");
					}
					break;
				// 人物検知待ち
				case wait_detect:
					if (buf[recognize_result]) {
						printf("human exist\n");
						status = approval_open;		// ドア開許可
					}
					break;
				// ドア開距離計算待ち
				case wait_calculate:
					if (buf[calculate_result]) {
						printf("calc ok\n");
						status = approval_open;		// ドア開許可
					}
					if (buf[taking_now]) {
						printf("take now!\n");
						taking_picture = true;		// ベストショット取得する(状態遷移外)
					}
					break;
				// ドア開許可
				case approval_open:
					if (buf[open_result]) {
						printf("door opened!\n");
						status = wait_close;		// ドア閉指示
					}
					if (buf[taking_now]) {
						printf("take now\n");
						taking_picture = true;		// ベストショット取得する(状態遷移外)
					}
					break;
				// ドア閉指示
				case wait_close:
					if (buf[close_result]) {
						printf("door closed\n");
						status = initialize;		// 初期状態へ
					}
					if (buf[taking_now]) {
						printf("take now\n");
						taking_picture = true;		// ベストショット取得する(状態遷移外)
					}
					break;
				default:
					printf("abnormal message(%d)\n", status);
					status = initialize;
					break;
			}
		}
		// ベストショット取得(状態遷移外)
		if (taking_picture) {
			take_picture();
			taking_picture = false;
		}
	}

	return 0;
}