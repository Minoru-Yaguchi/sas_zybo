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

#ifndef _IICPS_H_
#define _IICPS_H_

int IICPSread16_16(unsigned int base_addr, unsigned char slv_addr, unsigned short reg_no, unsigned short *data);
int IICPSread8_16(unsigned int base_addr, unsigned char slv_addr, unsigned char reg_no, unsigned short *data);
int IICPSread8_8(unsigned int base_addr, unsigned char slv_addr, unsigned char reg_no, unsigned char *data);

int IICPSwrite16_16(unsigned int base_addr, unsigned char slv_addr, unsigned short reg_no, unsigned short data);
int IICPSread0_8(unsigned int base_addr, unsigned char slv_addr, unsigned char *reg_no);
int IICPSwrite8_16(unsigned int base_addr, unsigned char slv_addr, unsigned char reg_no, unsigned short data);
int IICPSwrite8_8(unsigned int base_addr, unsigned char slv_addr, unsigned char reg_no, unsigned char data);
int IICPSwrite0_8(unsigned int base_addr, unsigned char slv_addr, unsigned char reg_no);


#endif
