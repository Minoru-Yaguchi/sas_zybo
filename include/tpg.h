/*
 * tpg.h
 *
 *  Created on: 2014/04/04
 *      Author: kirk
 */

#ifndef TPG_H_
#define TPG_H_

#define TPG_PT_MASK	0x03
#define TPG_THRU_MASK	0x04
#define TPG_SKIP_MASK	0x08
#define TPG_DATA_MASK	0x30

#define TPG_VERTICAL	0
#define TPG_HORIZONTAL	1
#define TPG_SCALE		2

#define TPG_8		0
#define TPG_16		1
#define TPG_24		2

typedef struct {
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
	unsigned char op_mode;
	unsigned char thru_mode;
	unsigned char skip_mode;
	unsigned char data_mode;
} TPG_REG;

void initTPG(unsigned int badr, TPG_REG *param);
void startTPG(unsigned int badr);
void stopTPG(unsigned int badr);

#endif /* TPG_H_ */
