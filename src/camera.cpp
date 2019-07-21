#include "camera.h"

static APT_REG apt0[ ] = {
		{0x10, 0x0051},
		{0x11, 0x6004}, {0x12, 0x0003}, // 96MHx
		{0x01, 54 + (1944 - MAX_HEIGHT) /2},
		{0x02, 16 + (2592 - MAX_WIDTH) /2},
		{0x03, MAX_HEIGHT - 1}, {0x04,MAX_WIDTH - 1 },

		{0x09, MAX_HEIGHT - 2 },
		{0x0a, 0x8600},

		{0x49, 0x0000},
		{0x35, 0x0054}, // gain 5x
		{0x10, 0x0053},
		{EORS, 0x0000}
};

static IGC_REG igc0 = {
		MAINMEM_BASEADDR + FRAME0_OFFSET,
		FRAME0_SIZE,
		(FRAME0_NUM - 1),

		2,
		(8 - 1),

		MAX_WIDTH,
		MAX_WIDTH
};


static VDC_REG vdc0 = {
		MAINMEM_BASEADDR + FRAME1_OFFSET,
		FRAME1_SIZE,
		(FRAME1_NUM - 1),

		2,
		(16 - 1),

		MAX_WIDTH,

		0,

		SXGA_HVALID,
		SXGA_HPULSE,
		SXGA_HBPORCH,
		SXGA_HFPORCH,
		SXGA_HPOLARITY,

		SXGA_VVALID,
		SXGA_VPULSE,
 		SXGA_VBPORCH,
 		SXGA_VFPORCH,
		SXGA_VPOLARITY
};

static volatile unsigned int *sw;
static volatile unsigned int *led;
static volatile unsigned int *vmux;
static volatile unsigned int *imux;
static volatile unsigned int *vfreq;
static volatile unsigned int *ifreq;
static volatile unsigned int *ci0;
static volatile unsigned int *cnt;

#define USCNT	25 // 25MHz
#define BPP		4

void CameraDev::setAddr(void){
	int fd0, fd1, fd2, fd3, fd4, fd5, fd6, fd7, fd8, fd9;

	sw = static_cast<volatile unsigned int *>(getSpace( ATRJ_SW_BASEADDR, 32, &fd0 ));
	led = static_cast<volatile unsigned int *>(getSpace( ATRJ_LED_BASEADDR, 32, &fd1 ));
	vmux = static_cast<volatile unsigned int *>(getSpace( ATRJ_VMUX_BASEADDR, 32, &fd2 ));
	imux = static_cast<volatile unsigned int *>(getSpace( ATRJ_IMUX_BASEADDR, 32, &fd3 ));
	vfreq = static_cast<volatile unsigned int *>(getSpace( ATRJ_VFREQ_BASEADDR, 32, &fd4 ));
	ifreq = static_cast<volatile unsigned int *>(getSpace( ATRJ_IFREQ_BASEADDR, 32, &fd5 ));
	ci0 = static_cast<volatile unsigned int *>(getSpace( ATRJ_CI0_BASEADDR, 32, &fd6 ));
	cnt = static_cast<volatile unsigned int *>(getSpace( ATRJ_FCNT_BASEADDR, 32, &fd7 ));

	this->frame0 = static_cast<unsigned int *>(getSpace( MAINMEM_BASEADDR + FRAME0_OFFSET,  FRAME0_SIZE, &fd8 ));
	this->frame1 = static_cast<unsigned int *>(getSpace( MAINMEM_BASEADDR + FRAME1_OFFSET,  FRAME1_SIZE, &fd9 ));
}

void CameraDev::initCameraDev(void){
	setAddr();

	setMULT( ATRJ_CLKGEN_BASEADDR, 6 ); // For MBUS 200MHz
	setDIV( ATRJ_CLKGEN_BASEADDR, 1 );
	setO1DIV( ATRJ_CLKGEN_BASEADDR, 11 ); // for SXGA
	setO2DIV( ATRJ_CLKGEN_BASEADDR, 8 ); // for 720P

	changeCLK( ATRJ_CLKGEN_BASEADDR );

	APTregSet( 0, ATRJ_APTIIC_BASEADDR, MT9P031_ADDR, apt0, 0 );
	cleanVDC32frame( 1, FRAME1_OFFSET );

	*imux = 0x10; // EX in(24bit RGB w/CI) / No Conv out
	*vmux = 0x00; // No Ex in(24bit RGB) / No Conv out

	ci0[ XCI_PIPE_AXILITES_ADDR_MODE_DATA / 4 ] = CI_MODE;
	ci0[ XCI_PIPE_AXILITES_ADDR_SFT_DATA / 4 ] = ROUND_BITS;

	setupVDCreg( ATRJ_VDC0_BASEADDR, &vdc0 );
	setupIGCreg( ATRJ_IGC0_BASEADDR, &igc0 );

	startVDC( ATRJ_VDC0_BASEADDR );
	startIGC( ATRJ_IGC0_BASEADDR );

}

void CameraDev::readCamera(unsigned char* ptr){
	uint12 x, y;
	uint24 pix;

	/* RGBAの結果を一時領域にコピー(次フレームへの影響を抑えるため) */
	memcpy(this->tmpframe, this->getframe0(), FRAME0_SIZE);

	/* RGBA → RGBに変換 */
	for( y = 0; y < image_height; y++ ) {
		for( x = 0 ; x < image_width; x++ ) {
			pix = this->tmpframe[ x + y * MAX_WIDTH ];
			ptr[(y * image_width + x) * 3 + 0] = ((pix >>  0) & 0xff);
			ptr[(y * image_width + x) * 3 + 1] = ((pix >>  8) & 0xff);
			ptr[(y * image_width + x) * 3 + 2] = ((pix >> 16) & 0xff);
		}
	}
	return;
}

void CameraDev::setDisplay(unsigned char* ptr){
	uint12 x, y;
	unsigned int* sadr = this->getframe1();
	for( y = 0; y < image_height; y++ ) {
		for( x = 0 ; x < image_width; x++ ) {
			sadr[ x + y * MAX_WIDTH ] = ((unsigned int)ptr[(y * image_width + x) * 3 + 0] << 0) 
									  | ((unsigned int)ptr[(y * image_width + x) * 3 + 1] << 8) 
									  | ((unsigned int)ptr[(y * image_width + x) * 3 + 2] << 16);
		}
	}
	return;
}

size_t CameraDev::getImageWidth(){
	return this->image_width;
};
size_t CameraDev::getImageHeight(){
	return this->image_height;
};

unsigned int* CameraDev::getframe0(){
	return frame0 + ofst;
}
unsigned int* CameraDev::getframe1(){
	return frame1 + ofst;
}

CameraDev::CameraDev(){
	initCameraDev();
	this->tmpframe = new unsigned int[MAX_WIDTH * MAX_HEIGHT];
};
CameraDev::~CameraDev(){
	delete this->tmpframe;
};
