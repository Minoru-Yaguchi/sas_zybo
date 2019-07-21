/*
 * vclk.h
 *
 *  Created on: 2013/07/21
 *      Author: Kirk
 */

#ifndef VCLK_H_
#define VCLK_H_

#define VCLK_REGS_BYTE	32

#define CTRLREG		0
#define MULTREG		1
#define DIVREG		2
#define O1DIVREG	3
#define O2DIVREG	4

#define CLKFREQ		6
#define DIVVAL		7

#define MULT_MASK			0xff000000
#define DIV_MASK			0x00ff0000
#define O1DIV_MASK			0x0000ff00
#define O2DIV_MASK			0x000000ff

#define MIN_M	1
#define MAX_M	64
#define MIN_D	1
#define MAX_D	64
#define MIN_O	1
#define MAX_O	128

#define STATUS_BIT_MASK	0x80000000
#define CHANGE_BIT_MASK	0x00000001

int changeCLK(unsigned int badr);

int setMULT(unsigned int badr, unsigned int val);
int setO1DIV(unsigned int badr, unsigned int val);
int setO2DIV(unsigned int badr, unsigned int val);
int setDIV(unsigned int badr, unsigned int val);

int getCLKFREQ(unsigned int badr);
int getDIVVAL(unsigned int badr);
int getMULT(unsigned int badr);
int getDIV(unsigned int badr);
int getO1DIV(unsigned int badr);
int getO2DIV(unsigned int badr);

#endif /* VCLK_H_ */
