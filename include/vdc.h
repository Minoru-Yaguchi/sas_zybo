// ***************************************************************************
// **       Copyright (c) 2008 - 2014 ATRJ LLC.  All rights reserved.       **
// **                                                                       **
// ** Advanced Technology Research Japan, LLC.(ATRJ)                        **
// ** ATRJ IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS"           **
// ** BY PROVIDING THIS DESIGN, CODE, OR INFORMATION AS ONE POSSIBLE        **
// ** IMPLEMENTATION OF THIS FEATURE, APPLICATION OR STANDARD, ATRJ IS      **
// ** MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION IS FREE FROM        **
// ** ANY CLAAPT OF INFRINGEMENT, AND YOU ARE RESPONSIBLE FOR OBTAINING     **
// ** ANY RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION.  ATRJ EXPRESSLY   **
// ** DISCLAAPT ANY WARRANTY WHATSOEVER WITH RESPECT TO THE ADEQUACY        **
// ** OF THE IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES    **
// ** OR REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE FROM CLAAPT OF    **
// ** INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS       **
// ** FOR A PARTICULAR PURPOSE.                                             **
// **                                                                       **
// ***************************************************************************

#ifndef VDC_H_
#define VDC_H_

#define VDC_REG_NUM		8
#define VDC_REGS_BYTE	(VDC_REG_NUM * 4)

#define VDC_START_BIT 0x80000000
#define VDC_CLK_BIT 0x01
#define VDC_SYNC_BIT 0x02
#define VDC_TEST_BIT 0x04

typedef struct {
	unsigned int frame_offset;
	unsigned int frame_size;
	unsigned short fsync_num;
	unsigned char data_mode;
	unsigned char burst_len;
	unsigned int max_width;

	unsigned int op_mode;

	unsigned short hvalid;
	unsigned char hpulse;
	unsigned char hbporch;
	unsigned char hfporch;
	unsigned char hpolarity;
	unsigned short vvalid;
	unsigned char vpulse;
	unsigned char vbporch;
	unsigned char vfporch;
	unsigned char vpolarity;
} VDC_REG;

#define C_RED8		0x03
#define C_GREEN8	0x0c
#define C_BLUE8		0x30
#define C_THRU8		0x80

#if 0
static unsigned char col8[] = {
	0,
	C_RED8,
	C_GREEN8,
	C_RED8 | C_GREEN8,
	C_BLUE8,
	C_RED8 | C_BLUE8,
	C_GREEN8 | C_BLUE8,
	C_RED8 | C_GREEN8 | C_BLUE8
};
#endif

#define C_RED16		0x001f
#define C_GREEN16	0x03e0
#define C_BLUE16	0x7c00
#define C_THRU16	0x8000

#if 0
static unsigned short col16[] = {
	0,
	C_RED16,
	C_GREEN16,
	C_RED16 | C_GREEN16,
	C_BLUE16,
	C_RED16 | C_BLUE16,
	C_GREEN16 | C_BLUE16,
	C_RED16 | C_GREEN16 | C_BLUE16
};
#endif

#define C_RED32		0x000000ff
#define C_GREEN32	0x0000ff00
#define C_BLUE32	0x00ff0000
#define C_THRU32	0x80000000

#if 0
static unsigned int col32[] = {
	0,
	C_RED32,
	C_GREEN32,
	C_RED32 | C_GREEN32,
	C_BLUE32,
	C_RED32 | C_BLUE32,
	C_GREEN32 | C_BLUE32,
	C_RED32 | C_GREEN32 | C_BLUE32
};
#endif

void setVDCdefparam(unsigned int w, unsigned int h, unsigned int u);

void setupVDCreg(unsigned int base_addr, VDC_REG *param);
void printVDCreg(unsigned int base_addr, int opmode);

void startVDC(unsigned int badr);
void fsyncVDCon(unsigned int badr);
void fsyncVDCoff(unsigned int badr);
void changeVDCframe(unsigned int badr, unsigned int fo);

void drawVDC8pattern(int k, unsigned int fo, int w, int h);
void drawVDC8box(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned char col, unsigned int fo);
void cleanVDC8frame(unsigned char col, unsigned int fo);
void putcVDC8(int x, int y, unsigned char cn, int color, int scale, unsigned int fo);
void putsVDC8(int x, int y, unsigned char *str, int color, int scale, unsigned int fo);

void drawVDC16pattern(int k, unsigned int fo, int w, int h);
void drawVDC16box(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned short col, unsigned int fo);
void cleanVDC16frame(unsigned short col, unsigned int fo);
void putcVDC16(int x, int y, unsigned char cn, int color, int scale, unsigned int fo);
void putsVDC16(int x, int y, unsigned char *str, int color, int scale, unsigned int fo);

void drawVDC32pattern(int k, unsigned int fo, int w, int h);
void drawVDC32box(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned int col, unsigned int fo);
void cleanVDC32frame(unsigned int col, unsigned int fo);
void putcVDC32(int x, int y, unsigned char cn, int color, int scale, unsigned int fo);
void putsVDC32(int x, int y, unsigned char *str, int color, int scale, unsigned int fo);
void drawVDC32line(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, unsigned int col, unsigned int fo);

#define MAX_STRING	256
#define CHAR_WIDTH	6
#define CHAR_HEIGHT	8

#endif /* VDC_H_ */

