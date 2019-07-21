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

#ifndef IGC_H_
#define IGC_H_

typedef struct {
	unsigned int frame_offset;
	unsigned int frame_size;
	unsigned short fsync_num;
	unsigned char data_mode;
	unsigned char burst_len;
	unsigned int max_width;
	unsigned short hvalid;
} IGC_REG;

void setIGCdefparam(unsigned int w, unsigned int h, unsigned int u);

void startIGC(unsigned int badr);
void stopIGC(unsigned int badr);
void setupIGCreg(unsigned int base_addr, IGC_REG *param);
void printIGCregs(void);
void fsyncIGCon(unsigned int badr);
void fsyncIGCoff(unsigned int badr);
void fsyncIGCset(unsigned int badr, unsigned int n);
unsigned int fsyncIGCget(unsigned int badr);

#endif /* IGC_H_ */
