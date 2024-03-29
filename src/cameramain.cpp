#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys/time.h"
#include "camera.h"
#include "hog_soft.h"
#include "mysystem.h"
extern "C" {
	#include "vparam.h"
}
#include <jpeglib.h>
#include <string>
#include "msgQLib.h"
#include "aws.h"

#define __CALC_HOG__		// 人物検知有効フラグ
#define __OUTPUT_FILE__	// カメラ画像ダンプフラグ
#define __VIDEO_OUT__		// HDMIビデオ出力フラグ

/* 人物検知用のパラメータ */
#define ORIENTATION 9
#define CELL_SIZE 5
#define LOOP_COUNT 20

extern CameraDev* cdev;

extern MSG_Q_ID sas_msg;
MSG_Q_ID cam_msg;
enum msg_num{
	detect_result = 0,
	calculate_result,
	recognize_result,
	taking_now,
	open_result,
	close_result,
	start_request,
	disappear_object = 9
};
extern int recog_ok;
int taking = 0;
int progressing = 0;

/* DDRのメモリ領域 */
extern unsigned int *frame0;
extern unsigned int *frame1;

void* cameramain(void* arg);

void writeJpegFormat(unsigned char* src, size_t width, size_t height, std::string filename){
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	FILE *fp = fopen(filename.c_str(), "wb");
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

int start_camera(void) {
	int ret = 0;
	pthread_t pthread;
	cam_msg = msgQCreate(10, 10);
	if (!cam_msg) {
		printf("cam_msg create error...\n");
		return -1;
	}

	ret = pthread_create(&pthread, NULL, &cameramain, NULL);
	if (ret) {
		printf("camera thread create error...\n");
		return -1;
	}

	return ret;
}

int ishuman() {
	char buf[10] = {0};
	// 処理中はメッセージ送信しない
	if (!progressing) {
		recog_ok = false;
		buf[start_request] = true;
		int ret = msgQSend(cam_msg, buf, sizeof(buf));
		if (ret) {
			printf("ishuman msg send error\n");
			return -1;
		}
	}
	return 0;
}

#if 0
int take_picture() {
	taking = true;
	return 0;
}
#else
extern 
int take_picture() {
	static int num = 0;
	int width_img  = cdev->getImageWidth();		// 主走査サイズを取得
	int height_img = cdev->getImageHeight();	// 副走査サイズを取得
	unsigned char* colptr = new unsigned char[width_img * height_img * 3];

	// ベストショット画像のJPEG保存
	time_t timer;
	struct tm *local;
	char datetime[20];
	timer = time(NULL);
	local = localtime(&timer);
	strftime(datetime, sizeof(datetime), "%Y%m%d%H%M%S", local);
	std::string dt(datetime, 14);
	cdev->readCamera(colptr);
//	std::string outfilename = "test_col_" + std::to_string(num) + ".jpg";
	std::string outfilename = dt + ".jpg";
	writeJpegFormat(colptr, width_img, height_img, "./output/" + outfilename);

	// AWSのS3へアップロード
	if(!upload_S3_bucket(outfilename)){
		std::cerr << "image upload to s3 is error..." << std::endl;
		return -1;
	}else{
//		std::cout << "image upload to s3 is end  !!!" << std::endl;
	}

	// AWS Rekognitionで顔認証
	picojson::object obj;
	if(!recognition_face_detect(outfilename, obj)){
		std::cerr << "face detect is error..." << std::endl;
		return -1;
	}else{
		picojson::object pboj =  obj["payloads"].get<picojson::object>();           // 顔認証結果のみ抜き出し
		picojson::array farry = pboj["FaceMatches"].get<picojson::array>();         // 顔認証結果のみ抜き出し
		size_t face_num = farry.size();

		std::cout << "*****************************************************" << std::endl;
		std::cout << "********************* 顔認証結果 *********************" << std::endl;
		std::cout << "*****************************************************" << std::endl;
		if(face_num == 0){
			std::cout << "不審者が入っています！！！" << std::endl;
		}else{
			for(size_t i=0; i<face_num; i++){
				picojson::object fmobj = farry[i].get<picojson::object>();
				picojson::object fobj = fmobj["Face"].get<picojson::object>();
				std::string name = fobj["ExternalImageId"].get<std::string>();
				std::cout << name << " が向かってきています。ご準備ください。" << std::endl;
			}
		}
		std::cout << "*****************************************************" << std::endl;
	}
	num++;
	taking = true;
	return 0;
}
#endif

int change_recog() {
	recog_ok = true;
	return 0;
}

void* cameramain(void* arg)
{
	char buf[10] = {0};
	/*********************************************/
	/* カメラ制御インスタンス生成 + デバイスの初期化 */
	/*********************************************/
	cdev = new CameraDev();
	int width_img  = cdev->getImageWidth();		// 主走査サイズを取得
	int height_img = cdev->getImageHeight();	// 副走査サイズを取得
	printf("width_img = %d : height_img = %d\n", width_img, height_img);
	unsigned char* colptr = new unsigned char[width_img * height_img * 3];

	/*****************************************/
	/* HOG + 人物検知の前処理 + データ読み出し  */
	/* SW処理用のため、暫定処理                */
	/*****************************************/
    // 弱認識器のパラメータ読み込み
    int *ada_list_x;
    int *ada_list_y;
    int *ada_list_h;
    float *ada_list_w;

    ada_list_x = (int *)calloc(LOOP_COUNT * sizeof(int), 1);
    ada_list_y = (int *)calloc(LOOP_COUNT * sizeof(int), 1);
    ada_list_h = (int *)calloc(LOOP_COUNT * sizeof(int), 1);
    ada_list_w = (float *)calloc(LOOP_COUNT * sizeof(float), 1);

    FILE *rfp;
    if(NULL == (rfp=fopen("./dat/weak_data.dat", "a+"))){
        printf("Can not open the outfile.\n");
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
    }
    fclose(rfp);

	// 画像全体のブロック数を計算 *************************************************************************************
	size_t Cell_X = static_cast<size_t>(width_img  / CELL_SIZE) ;
	size_t Cell_Y = static_cast<size_t>(height_img / CELL_SIZE) ;
	unsigned char* gryptr = new unsigned char[width_img * height_img * 1];

	/* Hog用領域は最大サイズで確保して使いまわす */
	unsigned char* gryptr_res = new unsigned char[width_img * height_img];
	ACCURACY *cell_hist = new ACCURACY[Cell_X * Cell_Y * ORIENTATION];
	ACCURACY *pHOGFeatures = new ACCURACY[Cell_X * Cell_Y * ORIENTATION];
	unsigned short *HogBin = new unsigned short[Cell_X * Cell_Y];


#if 0
	/* 処理時間測定用 */
    struct timeval st0, et0;
#else
	/* 処理時間測定用 */
	struct timeval st0;
#endif
	static int num=0;

	while( 1 ) {
		if (!progressing) {
			// 人物検知開始はメッセージ受信をトリガとする
			int ret = msgQReceive(cam_msg, buf, sizeof(buf));
			if (ret == -1) {
				printf("cam msg receive error ret=%d\n", ret);
				break;
			}
			progressing = buf[start_request];
			printf("==================== HOG START!! ======================\n");
		}
		gettimeofday(&st0, NULL);

		/* ★★★　カメラ画像のスナップショット取得処理　★★★*/
		/* 結果格納領域は事前に確保しておく                  */
		cdev->readCamera(colptr);

		/*****************************************/
		/* ここから SWでの HOG + 人物検知処理      */
		/*****************************************/
#ifdef __CALC_HOG__
		/* 人物検知処理　*/
		/* RGB→Gray   　*/
		convertRGB2Gray(colptr, gryptr, static_cast<size_t>(width_img), static_cast<size_t>(height_img));

		/* 人物検知処理 (今は暫定でモニタ画面に矩形を上書きしている　*/
		/* 最終的にここはHWになるので、暫定で実装                  */
		detectpeople(colptr, gryptr, gryptr_res, cell_hist, pHOGFeatures, HogBin, width_img, height_img, ada_list_x, ada_list_y, ada_list_h, ada_list_w, 2);
		detectpeople(colptr, gryptr, gryptr_res, cell_hist, pHOGFeatures, HogBin, width_img, height_img, ada_list_x, ada_list_y, ada_list_h, ada_list_w, 4);
		detectpeople(colptr, gryptr, gryptr_res, cell_hist, pHOGFeatures, HogBin, width_img, height_img, ada_list_x, ada_list_y, ada_list_h, ada_list_w, 6);
		detectpeople(colptr, gryptr, gryptr_res, cell_hist, pHOGFeatures, HogBin, width_img, height_img, ada_list_x, ada_list_y, ada_list_h, ada_list_w, 8);
#endif	//__CALC_HOG__

#ifdef __VIDEO_OUT__
		/*  人物検知結果を描画用メモリ領域に書き込む */
		cdev->setDisplay(colptr);
#endif	//__VIDEO_OUT__

		/*  出力結果の出力用 */
#ifdef __OUTPUT_FILE__
		if (taking) {
			writeJpegFormat(colptr, width_img, height_img, "./output/test_col_" + std::to_string(num) + ".jpg");
			num++;
			taking = false;
		}
#endif 	//__OUTPUT_FILE__

#if 0
		/* 各フレームごとに処理時間を表示 */
		gettimeofday(&et0, NULL);
		double time = (et0.tv_sec - st0.tv_sec) + (et0.tv_usec - st0.tv_usec)*1.0E-6;
		float  fps = 1.0f / time;
		printf("Flame Process Time : %5.3lf [s]\tFPS : %5.3f [fps]\n", time, fps);
#endif
	}
	
	/* 確保領域のメモリ開放 */
	delete gryptr; gryptr = nullptr;
	delete cell_hist; cell_hist = nullptr;
	delete pHOGFeatures; pHOGFeatures = nullptr;
	delete HogBin; HogBin = nullptr;
	delete gryptr_res; gryptr_res = nullptr;

	return 0;
}
