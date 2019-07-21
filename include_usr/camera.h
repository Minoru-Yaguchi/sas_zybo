#ifndef __CAMERA_HEAD__
#define __CAMERA_HEAD__
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include"xparameters.h"
#include "mysystem.h"
#include "fir.h"

extern "C" {
	#include "vdc.h"
	#include "igc.h"
	#include "vparam.h"
	#include "vclk.h"
	#include "apt.h"
	#include "xci_pipe_hw.h"
	extern void *getSpace(unsigned int addr, unsigned int size, int *fd);
}

class CameraDev{
public:
    /* 画像の主走査サイズ取得 */
    size_t getImageWidth();

    /* 画像の副走査サイズ取得 */
    size_t getImageHeight();

    /* カメラ画像をダンプする */
    void readCamera(unsigned char* ptr);

    /* ディスプレイバッファに画像書き込み */
    void setDisplay(unsigned char* ptr);

    CameraDev();
    ~CameraDev();
private:
    /* 画像サイズ */
    size_t image_width  = MAX_WIDTH;
    size_t image_height = MAX_HEIGHT;

    /* メモリ領域上のオフセット */
    int ofst = 0;

    /* カメラ用バッファ領域 */
    unsigned int *frame0;
    /* ディスプレイ用バッファ領域 */
    unsigned int *frame1;

    /* RGBA読み込み用の一時バッファ */
    unsigned int *tmpframe;

    void setAddr(void);

    /* カメラデバイスの初期化 */
    void initCameraDev(void);
    unsigned int* getframe0();
    unsigned int* getframe1();

};

#endif //__CAMERA_HEAD__