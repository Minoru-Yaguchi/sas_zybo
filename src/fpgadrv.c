#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <jpeglib.h>
#include <signal.h>
#include "fpgadrv.h"
#include "module/fpgamod.h"
#include "camera.h"
#include "hog_soft.h"
#include "msgQLib.h"

#define __CALC_HOG__		// 人物検知有効フラグ
#define __OUTPUT_FILE__	// カメラ画像ダンプフラグ

/* 人物検知用のパラメータ */
#define ORIENTATION 9
#define CELL_SIZE 5
#define LOOP_COUNT 20
#define BASE_ADDR1 0x43C40000
#define MAP_LENG  0x00040000

#define STARTHOG 0
//#define GETIMAGE 1


static size_t Cell_X;
static size_t Cell_Y;
static unsigned short *HogBin;
static int *ada_list_x;
static int *ada_list_y;
static int *ada_list_h;
static float *ada_list_w;
static int width_img;
static int height_img;
unsigned char* colptr=NULL;
CameraDev* cdev;
int fd=0;
MSG_Q_ID fpgamsg;
unsigned int* hogbase;

#define FPGA_STARTDETECT 0
#define HOGCOUNT 3
extern size_t calcResize(size_t size, size_t pat);
extern void resize_image(unsigned char* src, unsigned char* dst, size_t width, size_t height, size_t width_res, size_t height_res, size_t pat);
int exec_adaboost(char data);
extern void sas_msgSend(int num);

void* exec_humandetect(void* arg);
void* adaboostthread(void* arg);
int dataset();

static int detectstatus=2;

void writeJpegFormat(unsigned char* src, size_t width, size_t height, char* filename){
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	FILE *fp = fopen(filename, "wb");
	jpeg_stdio_dest(&cinfo, fp);
	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, 75, TRUE);

	jpeg_start_compress(&cinfo, TRUE);

	JSAMPARRAY img = (JSAMPARRAY) malloc(sizeof(JSAMPROW) * height);
	for (size_t i = 0; i < height; i++) {
		img[i] = (JSAMPROW) malloc(sizeof(JSAMPLE) * 3 * width);
		for (size_t j = 0; j < width; j++) {
			img[i][j*3 + 0] = src[(i*width + j)*3 + 0];
			img[i][j*3 + 1] = src[(i*width + j)*3 + 1];
			img[i][j*3 + 2] = src[(i*width + j)*3 + 2];
		}
	}
	jpeg_write_scanlines(&cinfo, img, height);
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
}


void* adaboostthread(void* arg){
	int ret;
	int count=STARTHOG;
	
	while(1){
		ret = ioctl(fd, fpga_get_hog, 0);
		if( ret == -1 )
		{
			printf("%s\n",strerror(errno));
			perror("ioctl");
			close(fd);
			return NULL;
		}
		if(detectstatus == 0) continue;
		if(exec_adaboost(count)){
			printf("%s(human detect)\n", __func__);
			sas_msgSend(0);
			detectstatus=0;
			count=STARTHOG;
		}
		else{
			if(count > HOGCOUNT){
				printf("%s(human not detect)\n", __func__);
				sas_msgSend(1);
				detectstatus=0;
				count=STARTHOG;
			}else{
				count++;
				int ionum=count+0x20;
				printf("execadaboost (count=%d, ionum=%d)\n", count, ionum);
				ret = ioctl(fd, ionum, 0);
				if( ret == -1 )
				{
					printf("%s\n",strerror(errno));
					perror("ioctl");
					close(fd);
					return NULL;
				}
			}
		}
	}
}

void start_humandetect(){
	char sendmsg = FPGA_STARTDETECT;
	msgQSend(fpgamsg, &sendmsg, sizeof(sendmsg));
}

int init_fpga()
{
	pthread_t pthread;
	int ret;
	
	if((fd = open("/dev/fpga", O_RDWR|O_CLOEXEC, 0644)) == -1){
		perror("open");
		return -1;
	}

	/*********************************************/
	/* カメラ制御インスタンス生成 + デバイスの初期化 */
	/*********************************************/
	cdev = new CameraDev();
	width_img  = cdev->getImageWidth();		// 主走査サイズを取得
	height_img = cdev->getImageHeight();	// 副走査サイズを取得
	printf("width_img = %d : height_img = %d\n", width_img, height_img);
	colptr = new unsigned char[width_img * height_img * 3];

	/*****************************************/
	/* HOG + 人物検知の前処理 + データ読み出し  */
	/* SW処理用のため、暫定処理                */
	/*****************************************/
	// 弱認識器のパラメータ読み込み
	ada_list_x = (int *)calloc(LOOP_COUNT * sizeof(int), 1);
	ada_list_y = (int *)calloc(LOOP_COUNT * sizeof(int), 1);
	ada_list_h = (int *)calloc(LOOP_COUNT * sizeof(int), 1);
	ada_list_w = (float *)calloc(LOOP_COUNT * sizeof(float), 1);

	FILE *rfp;
	if(NULL == (rfp=fopen("./dat/weak_data.dat", "a+"))){
		printf("Can not open the outfile.\n");
		close(fd);
		return 0;
	}

	int dummy;
	size_t read_size = 0;
	for(int num = 0; num < LOOP_COUNT; num++){
		read_size += fread(&dummy, sizeof(int), 1, rfp);
		read_size += fread(&ada_list_x[num], sizeof(int), 1, rfp);
		read_size += fread(&ada_list_y[num], sizeof(int), 1, rfp);
		read_size += fread(&ada_list_h[num], sizeof(int), 1, rfp);
		read_size += fread(&ada_list_w[num], sizeof(float), 1, rfp);
		read_size += fread(&dummy, sizeof(int), 1, rfp);
		read_size += fread(&dummy, sizeof(int), 1, rfp);
		read_size += fread(&dummy, sizeof(int), 1, rfp);
    	printf("%d,%d,%f\n", ada_list_x[num],ada_list_y[num],ada_list_w[num]);
	}
	fclose(rfp);
	
	// 画像全体のブロック数を計算 *************************************************************************************
	Cell_X = (size_t)(width_img  / CELL_SIZE) ;
	Cell_Y = (size_t)(height_img / CELL_SIZE) ;

	/* Hog用領域は最大サイズで確保して使いまわす */
	HogBin = (unsigned short*)malloc(Cell_X * Cell_Y * sizeof(short));
	printf("Cell_X=%d, Cell_Y=%d, HogBin=%p\n", Cell_X, Cell_Y, HogBin);
	
	/* ★★★　カメラ画像のスナップショット取得処理　★★★*/
	/* 結果格納領域は事前に確保しておく                  */
	cdev->readCamera(colptr);

	fpgamsg = msgQCreate(10,10);
	if(fpgamsg == NULL){
		printf("fpgamsg msgQCreate error\n");
		close(fd);
		return -1;
	}

	ret = pthread_create(&pthread, NULL, &exec_humandetect, NULL);
	if (ret) {
		printf("exec_humandetect thread create error...\n");
		close(fd);
		return -1;
	}

	ret = pthread_create(&pthread, NULL, &adaboostthread, NULL);
	if (ret) {
		printf("adaboostthread thread create error...\n");
		close(fd);
		return -1;
	}

	int memfd;
	memfd = open( "/dev/mem", O_RDWR );
	if( memfd == -1 ){
		printf( "Can't open /dev/mem.\n" );
		return -1 ;
	}
	hogbase = (unsigned int *)mmap( NULL, MAP_LENG,
								PROT_READ | PROT_WRITE,
							   MAP_SHARED, memfd,
							   BASE_ADDR1 & 0xFFFF0000);
	if( hogbase == MAP_FAILED ){
		printf( "Error: mmap()\n" );
		return -1 ;
	}
	printf("%s end\n", __func__);
	
	return 0;
}

int exec_adaboost(char data){
	char pat;
	if(data == 0) pat = 1;
	else pat = data * 2;

	size_t width_hog = calcResize(width_img, pat);
	size_t height_hog = calcResize(720, pat);
	size_t Cell_X_pat = (size_t)(width_hog  / CELL_SIZE);
	size_t Cell_Y_pat = (size_t)(height_hog / CELL_SIZE);
    struct timeval st0, et0;
	int kameiresult=0;
	Cell_X_pat = Cell_X;
	Cell_Y_pat = Cell_Y;

	gettimeofday(&st0, NULL);
    for(unsigned int y = 0; y < Cell_Y; y++){
    	for(unsigned int x = 0; x < Cell_X; x++){
    		HogBin[ y * 256 + x ] = (unsigned short)hogbase[ y * 256 + x ];
    	}
    }
#ifdef GETIMAGE
	{
	int kamei;
	kamei=open("./output/hog.raw", O_CREAT|O_WRONLY|O_TRUNC);
	int datasize=Cell_X_pat*Cell_Y_pat*sizeof(unsigned short);
	unsigned char* kameiptr=(unsigned char*)HogBin;
	write(kamei, kameiptr, datasize);
	close(kamei);
	}    // 特徴量データを画像データとしてdump *****************************************************************************
    char testFile[ORIENTATION][32]
				   = {     "plane0.raw", "plane1.raw", "plane2.raw",
						   "plane3.raw", "plane4.raw", "plane5.raw",
						   "plane6.raw", "plane7.raw", "plane8.raw"
				   	 };

    printf("output filename (FPGA):\n") ;

    for( int k=0; k<ORIENTATION; k++){
    	printf("       %d: %s\n", k, testFile[k]) ;
    }

    FILE *fpo[ORIENTATION];
    for( int k=0; k<ORIENTATION; k++){
    	if((fpo[k] = fopen(testFile[k], "wb"))==NULL){
    		printf( "File Open Error : %s\n", testFile[k]);
    		return -1;
    	}
	    else{
	    	printf(          "File Open Succeed. File Name : %s\n", testFile[k]);
	    }
    }

    unsigned char wrData;
    // Hog結果をファイル書き込み ******************************************************************
    for( int y=0; y<Cell_Y; y++){
    	for( int x=0; x<Cell_X; x++){
    		for( int k=0; k<ORIENTATION; k++ ){
    			if( ((HogBin[y*Cell_X + x] >> k) & 0x00000001) == 0x1 ){ wrData = 0xFF; } else{ wrData = 0x00; }
    			fwrite(&wrData, sizeof(unsigned char),1,fpo[k]);
    		}
    	}
    }
    for( int i=0; i<ORIENTATION; i++){
    	fclose(fpo[i]);
    }
#endif
	gettimeofday(&et0, NULL);
	double time = (et0.tv_sec - st0.tv_sec) + (et0.tv_usec - st0.tv_usec)*1.0E-6;
	printf("HogDataRead Time : %5.3lf [s]\n", time);

	gettimeofday(&st0, NULL);
	// 認識 ***********************************************************************
	for(size_t y = 0; y < Cell_Y_pat; y++){
		for(size_t x = 0; x < Cell_X_pat; x++){
			float weight = 0.0;
			if(((Cell_Y_pat - 25 - y) >= 0) && ((Cell_X_pat - 12 - x) >= 0) ){
				for(int m = 0; m < LOOP_COUNT; m++){
					if((HogBin[(y + ada_list_y[m]) * Cell_X_pat + (x + ada_list_x[m])] & ada_list_h[m]) > 0){
						weight += ada_list_w[m];
					}else{
						weight -= ada_list_w[m];
					}
				}
			}
#if 1
#ifdef GETIMAGE
			if(weight > 1.3) {
				printf("human detect(pat=%d)\n", pat);
				extern void drawRectangle( unsigned char *img, int width, int height, int px, int py, size_t pat );
				drawRectangle( colptr, width_img, height_img, x, y, pat );
				kameiresult=1;
			}
#else
			if(weight > 1.3){
				printf("human detect(pat=%d)\n", pat);
				gettimeofday(&et0, NULL);
				time = (et0.tv_sec - st0.tv_sec) + (et0.tv_usec - st0.tv_usec)*1.0E-6;
				printf("AdaBoost Time : %5.3lf [s]\n", time);
				return 1;
			}
#endif
#endif
		}
    }
#ifdef GETIMAGE
	writeJpegFormat(colptr, width_img, height_img, "./output/result.jpg");
#endif
	gettimeofday(&et0, NULL);
	time = (et0.tv_sec - st0.tv_sec) + (et0.tv_usec - st0.tv_usec)*1.0E-6;
	printf("AdaBoost Time : %5.3lf [s]\n", time);
	printf("human notdetect(pat=%d)\n", pat);

	if(kameiresult==1) return 1;
	return 0;
}

void* exec_humandetect(void* args){
	char recvmsg;
	struct timeval st0, et0;
	double time;

	while(1){
		msgQReceive(fpgamsg, &recvmsg, sizeof(recvmsg));
		switch(recvmsg){
		case FPGA_STARTDETECT: // 等倍
			cdev->readCamera(colptr);
			cdev->setDisplay(colptr);
#ifdef GETIMAGE
			static int val=0;
			{
				char path[128]={0};
				sprintf(path, "./output/camera%d.raw", val);
				val++;
			int kamei=open(path, O_CREAT|O_WRONLY|O_TRUNC);
			int datasize=width_img*height_img*3;
			char* kameiptr=(char*)kamei;
			do{
				int writesize=write(kamei, colptr, datasize);
				datasize-=writesize;
				kameiptr+=writesize;
			}while(datasize>0);
			close(kamei);
			}
			writeJpegFormat(colptr, width_img, height_img, "./output/camera.jpg");
#endif
			gettimeofday(&st0, NULL);
			if(detectstatus == 2 || detectstatus == 1) continue;
			detectstatus=1;
			printf("%s\n", __func__);
			ioctl(fd, fpga_detect_human_hog10, 0);
			gettimeofday(&et0, NULL);
			time = (et0.tv_sec - st0.tv_sec) + (et0.tv_usec - st0.tv_usec)*1.0E-6;
			printf("FPGA DataSet Time : %5.3lf [s]\n", time);
			break;
		default:
			printf("unexpected msg(%d)\n", recvmsg);
			break;
		}
	}

	return 0;
}

