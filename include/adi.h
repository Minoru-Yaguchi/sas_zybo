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

#ifndef ADI_H_
#define ADI_H_

typedef struct {
	unsigned char addr;
	unsigned char data;
} ADV7511_REG;

#define ADV7511_REG_START	0
#define ADV7511_REG_NUM	256

#define EORS8	0xff

void ADIregSet(int sel, unsigned int baseaddr, unsigned int iicaddr, ADV7511_REG *p);
void ADIregCheck(int sel, unsigned int baseaddr, unsigned int iicaddr, ADV7511_REG *p);
void ADIregDump(int sel, unsigned int baseaddr, unsigned int iicaddr, int reg_sta, int reg_num);

#endif /* ADI_H_ */
