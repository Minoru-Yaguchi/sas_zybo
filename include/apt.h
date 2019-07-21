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

#ifndef APT_H_
#define APT_H_

typedef struct {
	unsigned short addr;
	unsigned short data;
} APT_REG;


#define MT9P031_REG_START	0
#define MT9P031_REG_NUM	256

#define MT9T031_REG_START	0
#define MT9T031_REG_NUM	256

#define MT9D112_REG_START	0x3000
#define MT9D112_REG_NUM	0x6c0

#define MT9P031_ADDR	(0xBA / 2)
#define MT9T031_ADDR	(0xBA / 2)
#define MT9D112_ADDR	(0x78 / 2)

#define EORS	0xffff

void APTregSet(int sel, unsigned int baseadr, int iicaddr, APT_REG *p, int mode);
void APTregCheck(int sel, unsigned int baseaddr, int iicaddr, APT_REG *p, int mode);
void APTregDump(int sel, unsigned int baseaddr, int iicaddr, int reg_sta, int reg_num, int mode);


#endif /* APT_H_ */
