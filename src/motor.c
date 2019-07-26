#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "spitest.h"

int spi_fd = 0;

int motor_initialize()
{
	unsigned int spibuf = 0;
	int ret = 0;

	spi_fd = open("/dev/spidev0.0", O_RDWR);
	
	if (spi_fd < 0)
	{
		printf("SPI open error... %d\n", errno);
		return -1;
	}
	else
	{
		printf("SPI open success!!\n");
	}

	/* set speed */
	spibuf  = 0x000000FF & (MNEMONIC_SET_PARAM | REGISTER_MAX_SPEED);
	spibuf |= 0x00FFFF00 & (MOTOR_SET_SPEED << 8);
	ret = write(spi_fd, &spibuf, 3);
	/* set step */
	spibuf  = 0x000000FF & (MNEMONIC_SET_PARAM | REGISTER_FS_SPD);
	spibuf |= 0x00FFFF00 & (MOTOR_SET_STEP << 8);
	ret = write(spi_fd, &spibuf, 3);
	/* set holding speed kval */
	spibuf  = 0x000000FF & (MNEMONIC_SET_PARAM | REGISTER_KVAL_HOLD);
	spibuf |= 0x0000FF00 & (MOTOR_SET_HOLD_KVAL << 8);
	ret = write(spi_fd, &spibuf, 2);
	/* set constant speed kval */
	spibuf  = 0x000000FF & (MNEMONIC_SET_PARAM | REGISTER_KVAL_RUN);
	spibuf |= 0x0000FF00 & (MOTOR_SET_CONSTANT_KVAL << 8);
	ret = write(spi_fd, &spibuf, 2);
	/* set acceralation starting kval */
	spibuf  = 0x000000FF & (MNEMONIC_SET_PARAM | REGISTER_KVAL_ACC);
	spibuf |= 0x0000FF00 & (MOTOR_SET_ACC_KVAL << 8);
	ret = write(spi_fd, &spibuf, 2);
	/* set deceralation starting kval */
	spibuf  = 0x000000FF & (MNEMONIC_SET_PARAM | REGISTER_KVAL_DEC);
	spibuf |= 0x0000FF00 & (MOTOR_SET_DEC_KVAL << 8);
	ret = write(spi_fd, &spibuf, 2);

	if (ret == -1) {
		return ret;
	} else {
		ret = 0;
	}

    return ret;
}

void motor_open() {
	unsigned int spibuf = 0;
	int ret = 0;

    /* open (moving 90 positive degree) */
    spibuf  = 0x000000FF & MNEMONIC_GOTO;
    spibuf |= 0xFFFFFF00 & (MOTOR_SET_MARK_P90DEG << 8);
    // spibuf  = 0x000000FF & MOTOR_GOTO_FORWARD;
    // spibuf |= 0xFFFFFF00 & (0x0007D0 << 8);
    ret = write(spi_fd, &spibuf, 4);

	if (ret == -1) {
		printf("spi error\n");
	}

    return;
}

void motor_close() {
    unsigned int spibuf = 0;
    int ret = 0;

    /* close (moving home position) */
    spibuf = 0x000000FF & MOTOR_COMMAND_GO_HOME;
    ret = write(spi_fd, &spibuf, 1);

	if (ret == -1) {
		printf("spi error\n");
	}

    return;
}

void motor_set_ultraspeed() {
	unsigned int spibuf = 0;
	int ret = 0;
	printf("ultra speed!\n");

	/* set ultra speed */
	spibuf  = 0x000000FF & (MNEMONIC_SET_PARAM | REGISTER_MAX_SPEED);
	spibuf |= 0x00FFFF00 & ((MOTOR_SET_SPEED * 4) << 8);
	ret = write(spi_fd, &spibuf, 3);

	if (ret == -1) {
		printf("spi error\n");
	}

	return;
}

void motor_set_highspeed() {
	unsigned int spibuf = 0;
	int ret = 0;
	printf("high speed!\n");

	/* set high speed */
	spibuf  = 0x000000FF & (MNEMONIC_SET_PARAM | REGISTER_MAX_SPEED);
	spibuf |= 0x00FFFF00 & ((MOTOR_SET_SPEED * 2) << 8);
	ret = write(spi_fd, &spibuf, 3);

	if (ret == -1) {
		printf("spi error\n");
	}

	return;
}

void motor_set_normalspeed() {
	unsigned int spibuf = 0;
	int ret = 0;
	printf("normal speed!\n");

	/* set normal speed */
	spibuf  = 0x000000FF & (MNEMONIC_SET_PARAM | REGISTER_MAX_SPEED);
	spibuf |= 0x00FFFF00 & ((MOTOR_SET_SPEED) << 8);
	ret = write(spi_fd, &spibuf, 3);

	if (ret == -1) {
		printf("spi error\n");
	}

	return;
}

void motor_set_slowspeed() {
	unsigned int spibuf = 0;
	int ret = 0;
	printf("slow speed!\n");

	/* set slow speed */
	spibuf  = 0x000000FF & (MNEMONIC_SET_PARAM | REGISTER_MAX_SPEED);
	spibuf |= 0x00FFFF00 & ((MOTOR_SET_SPEED / 2) << 8);
	ret = write(spi_fd, &spibuf, 3);

	if (ret == -1) {
		printf("spi error\n");
	}

	return;
}