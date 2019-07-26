#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include "hog_soft.h"
#include "msgQLib.h"

#define BLOCK_SIZE 3
#define ORIENTATION 9
#define CELL_SIZE 5
#define PI 3.14159265359
#define COL_RECT 0xFF
#define LINE_WIDTH 0x2
#define ORIENTATION 9
#define CELL_SIZE 5
#define LOOP_COUNT 20

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
int recog_ok = false;
extern int progressing;

/* HOG用のRGB→Gray変換 */
void convertRGB2Gray(unsigned char* src, unsigned char* dst, size_t width, size_t height)
{
	/* Y  =  0.257R + 0.504G + 0.098B + 16 */
	for(size_t y=0; y<height; y++)
	for(size_t x=0; x<width ; x++){
		float rf = static_cast<float>(src[(x + y * width) * 3 + 0]);
		float gf = static_cast<float>(src[(x + y * width) * 3 + 1]);
		float bf = static_cast<float>(src[(x + y * width) * 3 + 2]);
		float yf = 0.257f * rf + 0.504f * gf + 0.098 * bf + 16.0f;
		dst[x + y * width] = static_cast<unsigned char>(yf);
	}
}

int CompHistogram(unsigned char *img, ACCURACY *cell_hist, int cols, int rows){

    for( int y=1; y<((int)(rows / CELL_SIZE)*CELL_SIZE)-1; y++){
        for( int x=1; x<((int)(cols/CELL_SIZE)*CELL_SIZE)-1; x++){
            int center = y*cols + x;
            // 縦横方向の差分
            int xDelta = img[center - 1] - img[center + 1] ;
            int yDelta = img[center - cols] - img[center + cols] ;
            // 勾配強度の算出
            ACCURACY mag = sqrt((ACCURACY)xDelta * (ACCURACY)xDelta + (ACCURACY)yDelta * (ACCURACY)yDelta) ;
            // 勾配方向の算出(ラジアンから角度へ変換)
            ACCURACY grad = ( atan2((ACCURACY)yDelta, (ACCURACY)xDelta)*180.0) / PI ;

            if(grad < 0.0){
                grad += 360.0 ; // 符号が負である場合は反転
            }
            if(grad >= 180.0){
                grad -= 180.0;  // 0 ~ 360度から 0 ~ 180度に変換
            }
            grad = grad/20;

            // ヒストグラムに蓄積
            int p = (int)((int)(y/CELL_SIZE) * (int)(cols/CELL_SIZE)*ORIENTATION + (int)( x/CELL_SIZE)*ORIENTATION + (int)grad);
            cell_hist[p] += mag;
        }
    }

    return 0;
}

int CompHOG(ACCURACY *pHOGFeatures, ACCURACY *cell_hist, int Cell_Y, int Cell_X){

    // 特徴量の算出と正規化
    for( int y=0; y<Cell_Y; y++){
        for( int x=0; x<Cell_X; x++){
            ACCURACY sum_mag = 0.0;
            // 正規化のためヒストグラムの二乗の総和を算出
            for( int j=0; j<BLOCK_SIZE; j++){
                for( int i=0; i<BLOCK_SIZE; i++){
                    for( int k=0; k<ORIENTATION; k++){
                        int p = (int)((y)*Cell_X * ORIENTATION + (x) * ORIENTATION + k);
                        if( cell_hist[p] < 200.0) cell_hist[p] = 0.0 ;
                        //printf("cell_hist[%d] = %f\n", p,cell_hist[p] ) ;
                        sum_mag += cell_hist[p] * cell_hist[p];
                    }
                }
            }
            
            sum_mag = sqrt(sum_mag);
            // 特徴量の正規化
            for( int k=0; k<ORIENTATION; k++){
                int p = (int)(y * Cell_X * ORIENTATION + x * ORIENTATION + k);
                if( sum_mag == 0 ){
                    pHOGFeatures[p] = 0;
                }
                else{
                    pHOGFeatures[p] = cell_hist[p] / sum_mag ;
                }

                //printf("%d:%d:%d = %f,%f\n",x,y,k, pHOGFeatures[p], cell_hist[p]);
            }
        }
    }

    return 0;
}

void drawRectangle( unsigned char *img, int width, int height, int px, int py, size_t pat ){
    // 横線
    for( size_t y=(py*CELL_SIZE)*pat; y<(py*CELL_SIZE + LINE_WIDTH)*pat; y++){
        for( size_t x=(px*CELL_SIZE)*pat; x<(px*CELL_SIZE + 12*5)*pat; x++ ){
            img[(y*width + x)*3 + 0] = COL_RECT ;
            img[(y*width + x)*3 + 1] = 0x00 ;
            img[(y*width + x)*3 + 2] = 0x00 ;
        }
    }
    for( size_t y=(py*CELL_SIZE + 25*5 - LINE_WIDTH)*pat; y<(py*CELL_SIZE + 25*5)*pat; y++){
        for( size_t x=(px*CELL_SIZE)*pat; x<(px*CELL_SIZE + 12*5)*pat; x++ ){
            img[(y*width + x)*3 + 0] = COL_RECT ;
            img[(y*width + x)*3 + 1] = 0x00 ;
            img[(y*width + x)*3 + 2] = 0x00 ;
        }
    }
    // 縦線
    for( size_t y=(py*CELL_SIZE)*pat; y<(py*CELL_SIZE+25*5)*pat; y++){
        for( size_t x=(px*CELL_SIZE)*pat; x<(px*CELL_SIZE + LINE_WIDTH)*pat; x++){
            img[(y*width + x)*3 + 0] = COL_RECT ;
            img[(y*width + x)*3 + 1] = 0x00 ;
            img[(y*width + x)*3 + 2] = 0x00 ;
        }
    }
    for( size_t y=(py*CELL_SIZE)*pat; y<(py*CELL_SIZE+25*5)*pat; y++){
        for( size_t x=(px*CELL_SIZE + 12*5 - LINE_WIDTH)*pat; x<(px*CELL_SIZE + 12*5)*pat; x++){
            img[(y*width + x)*3 + 0] = COL_RECT ;
            img[(y*width + x)*3 + 1] = 0x00 ;
            img[(y*width + x)*3 + 2] = 0x00 ;
        }
    }

    return ;
}

size_t calcResize(size_t size, size_t pat)
{
	float sizef = static_cast<float>(size) / static_cast<float>(pat);
	return static_cast<size_t>(sizef);
}

void resize_image(unsigned char* src, unsigned char* dst, size_t width, size_t height, size_t width_res, size_t height_res, size_t pat)
{
	for(size_t sy=0, dy=0; dy<height; sy+=pat, dy++)
	for(size_t sx=0, dx=0; dx<width;  sx+=pat, dx++){
		dst[dy * width_res + dx] = src[sy * width + sx];
	}
}

void detectpeople(unsigned char* colptr, unsigned char* gryptr, unsigned char* gryptr_res, ACCURACY *cell_hist, ACCURACY *pHOGFeatures, unsigned short *HogBin,
    size_t width_img, size_t height_img,int *ada_list_x,int *ada_list_y,int *ada_list_h,float *ada_list_w, int pat){

	unsigned char wrData;

	size_t width_hog  = calcResize(width_img, pat);
	size_t height_hog = calcResize(height_img, pat);
	size_t Cell_X = static_cast<size_t>(width_hog  / CELL_SIZE) ;
	size_t Cell_Y = static_cast<size_t>(height_hog / CELL_SIZE) ;

    /* resize image */
    resize_image(gryptr, gryptr_res, width_img, height_img, width_hog, height_hog, pat);

    // ヒストグラム生成 **********************************************************************************************
    // ヒストグラムの領域確保
    memset(cell_hist, 0x00, sizeof(ACCURACY) * Cell_X * Cell_Y * ORIENTATION);
    CompHistogram( gryptr_res, cell_hist, width_hog, height_hog);

    // 正規化された特徴量の領域確保 ***********************************************************************************
    CompHOG( pHOGFeatures, cell_hist, Cell_Y, Cell_X );

    // HOG結果のコピー
    for( size_t y=0; y<Cell_Y; y++ ){
        for( size_t x=0; x<Cell_X; x++){
            HogBin[y*Cell_X + x] = 0x0000 ;
            for( size_t k=0; k<ORIENTATION; k++){
                if(pHOGFeatures[ y * Cell_X * ORIENTATION + x * ORIENTATION + k]>=0.15){
                    wrData = 0xFF;
                    HogBin[y*Cell_X + x] = ( HogBin[y*Cell_X + x] | (( (unsigned short)wrData & 0x0001 ) << k) ) ;
                }else{
                    wrData = 0x00;
                }
            }
        }
    }

    // 認識 ***********************************************************************
    for(size_t y = 0; y < Cell_Y; y++){
        for(size_t x = 0; x < Cell_X; x++){
            float weight = 0.0;
            if(((Cell_Y - 25 - y) >= 0) && ((Cell_X - 12 - x) >= 0) ){
                for(int m = 0; m < LOOP_COUNT; m++){
                    if((HogBin[(y + ada_list_y[m]) * Cell_X + (x + ada_list_x[m])] & ada_list_h[m]) > 0){
                        weight += ada_list_w[m];
                    }else{
                        weight -= ada_list_w[m];
                    }
                }
            }
            if(weight > 1.0) {
                drawRectangle( colptr, width_img, height_img, x, y, pat );

                // 人物検知OK 暫定でpatが8の場合のみ メッセージを送るのは1回のみにする
                if (pat == 2 && !recog_ok) {
                    char buf[10] = {0};
                    buf[recognize_result] = true;
                    int ret = msgQSend(sas_msg, buf, sizeof(buf));
                    if (ret) {
                        printf("ranging speed msg send error\n");
                        return;
                    }
                    recog_ok = true;
                    progressing = false;
                    printf("+++++++++ RECOGNIZE OK!! ++++++++++\n");
                }
            }
        }
    }
    return;
}

#if 0
int main(int argc, char **argv){
    printf("Expect data make!\n");
    
    if((argv[1]==NULL)||(argv[2]==NULL)){
        printf("Usage : hog_test.exe [In image] width, height\n");
        printf("            [In image] is raw data (gray scale)\n");
        printf("        Output Files :\n");
        printf("            plane*.raw : *:0 ~ 8  plane data (binary) 9 orientation.\n");
        printf("            result.raw : result image.\n");
            return -1;
        }

    int width = atoi(argv[2]);
    int height = atoi(argv[3]);
    // 入力画像ファイル処理 *******************************************************************************************
    // 入力ファイル 1plane 8bit : YUYV 8bitが理想
    FILE *fpi ;
    unsigned char *inimg = (unsigned char *)malloc(sizeof(unsigned char) * width * height);
    
    if( (fpi = fopen(argv[1], "rb"))!=NULL ){
        for( int h=0; h<height; h++ ){
    	    for( int w=0; w<width; w++ ){
                inimg[h*width + w] = fgetc(fpi);
                //fread( inimg + h*width+w, sizeof(unsigned char), 1, fpi );
            }
        }
    }
    else{
        printf("File Open Error! : %s\n", argv[1]);
    }

    fclose(fpi);

    struct timeval st0, st1, et0, et1;

    /* 処理時間測定（HOG START) */
    // 画像全体のブロック数を計算 *************************************************************************************
    int Cell_X = width / CELL_SIZE ;
    int Cell_Y = height / CELL_SIZE ;
    printf( "Cell_X : %d, Cell_Y : %d\n", Cell_X, Cell_Y);

    // ヒストグラム生成 **********************************************************************************************
    // ヒストグラムの領域確保
    gettimeofday(&st0, NULL);
    ACCURACY *cell_hist = (ACCURACY *)malloc(sizeof(ACCURACY) * Cell_X * Cell_Y * ORIENTATION) ;
    CompHistogram( inimg, cell_hist, width, height );

    // 正規化された特徴量の領域確保 ***********************************************************************************
    ACCURACY *pHOGFeatures = (ACCURACY *)malloc(sizeof(ACCURACY) * Cell_X * Cell_Y * ORIENTATION) ;
    CompHOG( pHOGFeatures, cell_hist, Cell_Y, Cell_X );

    /* 処理時間測定（HOG END) */
    gettimeofday(&et0, NULL);

    // 特徴量データを画像データとしてdump *****************************************************************************
      char testFile[ORIENTATION][32]
				   = {     "plane0.raw", "plane1.raw", "plane2.raw",
						   "plane3.raw", "plane4.raw", "plane5.raw",
						   "plane6.raw", "plane7.raw", "plane8.raw"
				   	 };

    printf("output filename:\n") ;

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
    
    // HOG結果のコピー
    unsigned short tempData ;
    unsigned char wrData;
    unsigned short *HogBin = (unsigned short *)malloc(sizeof(unsigned short) * Cell_X * Cell_Y);

    for( int y=0; y<Cell_Y; y++ ){
        for( int x=0; x<Cell_X; x++){
            HogBin[y*Cell_X + x] = 0x0000 ;
            for( int k=0; k<ORIENTATION; k++){
                // 特徴量のバイナリ化
                //printf("pHOGFeatures = %f\n", pHOGFeatures[y * Cell_X * ORIENTATION + x * ORIENTATION + k]);
                if(pHOGFeatures[ y * Cell_X * ORIENTATION + x * ORIENTATION + k]>=0.15){
                    wrData = 0xFF; //printf("wrData!!!!");
                    HogBin[y*Cell_X + x] = ( HogBin[y*Cell_X + x] | (( (unsigned short)wrData & 0x0001 ) << k) ) ;
                }else{
                    wrData = 0x00;
                }
                fwrite(&wrData, sizeof(unsigned char), 1, fpo[k]);
            }
        }
    }
    for( int i=0; i<ORIENTATION; i++){
        fclose(fpo[i]);
    }

    // 認識

    // 弱認識器の読み込み
    int *ada_list_x;
    int *ada_list_y;
    int *ada_list_h;
    float *ada_list_w;

    ada_list_x = (int *)calloc(LOOP_COUNT * sizeof(int), 1);
    ada_list_y = (int *)calloc(LOOP_COUNT * sizeof(int), 1);
    ada_list_h = (int *)calloc(LOOP_COUNT * sizeof(int), 1);
    ada_list_w = (float *)calloc(LOOP_COUNT * sizeof(float), 1);

    FILE *rfp;
    if(NULL == (rfp=fopen("./weak_data.dat", "a+"))){
        printf("Can not open the outfile.\n");
        return 0;
    }

    int dummy;
    for(int num = 0; num < LOOP_COUNT; num++){
        fread(&dummy, sizeof(int), 1, rfp);
        fread(&ada_list_x[num], sizeof(int), 1, rfp);
        fread(&ada_list_y[num], sizeof(int), 1, rfp);
        fread(&ada_list_h[num], sizeof(int), 1, rfp);
        fread(&ada_list_w[num], sizeof(float), 1, rfp);
        fread(&dummy, sizeof(int), 1, rfp);
        fread(&dummy, sizeof(int), 1, rfp);
        fread(&dummy, sizeof(int), 1, rfp);
        printf("%d,%d,%f\n", ada_list_x[num],ada_list_y[num],ada_list_w[num]);
    }
    fclose(rfp);

    /* 処理時間測定（Ada START) */
    gettimeofday(&st1, NULL);

    // 認識 ***********************************************************************
    for(int y = 0; y < Cell_Y; y++){
        for(int x = 0; x < Cell_X; x++){
            float weight = 0.0;
            if(((Cell_Y - 25 - y) >= 0) && ((Cell_X - 12 - x) >= 0) ){
                for(int m = 0; m < LOOP_COUNT; m++){
                    if((HogBin[(y + ada_list_y[m]) * Cell_X + (x + ada_list_x[m])] & ada_list_h[m]) > 0){
                        weight += ada_list_w[m];
                        //printf("%d: %04x:%04x = %f\n", m, HogBin[(y + ada_list_y[m]) * Cell_X + (x + ada_list_x[m])] , ada_list_h[m], ada_list_w[m]);
                    }else{
                        weight -= ada_list_w[m];
                    }
                }
                //printf("?: %d:%d = %f\n", x,y,weight);
            }
            if(weight > 0.5) {
//                drawRectangle( inimg, width, height, x, y );
//                printf("?: %d:%d = %f\n", x,y,weight);
                //cv::rectangle(Image, cv::Point(x*CELL_SIZE,y*CELL_SIZE), cv::Point(x*CELL_SIZE+12*5, y*CELL_SIZE+25*5), cv::Scalar(0,0,200), 2, 2);
            }
        }
    }
    /* 処理時間測定（Ada END) */
    gettimeofday(&et1, NULL);

    printf("time scale ------------------------------------\n");
    printf("Hog Time\t%lfs\n", (et0.tv_sec - st0.tv_sec) + (et0.tv_usec - st0.tv_usec)*1.0E-6);
    printf("Ada Time\t%lfs\n", (et1.tv_sec - st1.tv_sec) + (et1.tv_usec - st1.tv_usec)*1.0E-6);

    // 認識結果をファイル出力 ***********************************************************
    FILE *fpr;
    char *result_file = "result.raw" ;

    if((fpr = fopen(result_file, "wb"))==NULL){
        printf( "File Open Error : %s\n", result_file );
        return -1;
    }
	else{
        printf( "File Open Succeed. File Name : %s\n", result_file );
    }

    for( int y=0; y<height; y++ ){
        for( int x=0; x<width; x++){
            fwrite(inimg + y*width + x, sizeof(unsigned char), 1, fpr);
        }
    }

    fclose(fpr);

    // リソース開放 *********************************************************************
    free(inimg);
    free(cell_hist);
    free(pHOGFeatures);
    free(HogBin);

	return 0;
}
#endif