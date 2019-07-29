#gcc main.c msgQLib.c fpgadrv.c -lrt -lpthread
obj-m:= fpgamod.o
all:
	make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -C /home/ubuntu/work/peta/kernel/linux-xlnx-master M=$(PWD) modules