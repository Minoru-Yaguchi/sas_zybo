/*
 * fir.h
 *
 *  Created on: 2014/05/21
 *      Author: kirk
 */

#ifndef FIR_H_
#define FIR_H_

#ifndef __SYNTHESIS__

typedef unsigned char uint1;
typedef unsigned char uint2;
typedef unsigned char uint3;
typedef unsigned char uint4;
typedef unsigned char uint5;
typedef unsigned char uint6;
typedef unsigned char uint7;
typedef unsigned char uint8;
typedef unsigned short uint9;
typedef unsigned short uint10;
typedef unsigned short uint11;
typedef unsigned short uint12;
typedef unsigned short uint13;
typedef unsigned short uint14;
typedef unsigned short uint15;
typedef unsigned short uint16;

typedef unsigned int uint24;
typedef unsigned int uint32;

typedef signed char int6;
typedef signed short int15;

#else

#include "ap_cint.h"

#endif

#define RAW_SIZE	3
#define COL_SIZE	RAW_SIZE
#define BLOCK_SIZE	(RAW_SIZE * COL_SIZE)

int fir_org(
	unsigned char *sadr, unsigned int *dadr,
	uint12 width, uint12 height, uint12 max_width
);

int fir_org(
	unsigned int *sadr, unsigned int *dadr,
	uint12 width, uint12 height, uint12 max_width
);

#endif /* FIR_H_ */
