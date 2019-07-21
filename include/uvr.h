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

#ifndef UVR_H_
#define UVR_H_

#define UVR_REG_SIZE	0x10

typedef struct _uvfdef {
	unsigned char minor;
	unsigned char major;
	unsigned char option;
	unsigned char board;
} UVRDEF;

#define DEF1_REG	0x0
#define DEF2_REG	0x1
#define USER_REG	0x2
#define COUNTER_REG	0x3

#define MYBOARD_ZC706	0
#define MYBOARD_ZED	1
#define MYBOARD_ZC702	2
#define MYBOARD_CUSTOM	3

static char *board_name[] = {
		"ZC706",
		"ZED",
		"ZC702",
		"CUSTOM"
};

#define IMAGER_APTFMC		0
#define IMAGER_APTPMOD		1
#define IMAGER_CUSTOM		2

static char *imager_name[] = {
		"Aptina via FMC",
		"Aptina via PMOD",
		"CUSTOM"
};

void initCount(unsigned int badr);
unsigned int getCount(unsigned int badr);
unsigned int getMajor(unsigned int badr);
unsigned int getMinor(unsigned int badr);
unsigned int getBoard(unsigned int badr);
unsigned int getOption(unsigned int badr);


#endif /* UVR_H_ */

