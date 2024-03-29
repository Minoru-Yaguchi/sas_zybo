#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <asm/irq.h>
#include <linux/signal.h> 
#include <linux/sched.h> 
#include <linux/sched/signal.h> 
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <asm/io.h>
#include "fpgamod.h"

#define DEV_NAME "fpga"

unsigned long g_pid = 0;

static int fpga_major = 221;

static int irq = 48; // IRQ�ԍ�
static void* res_regs;
static void* dummy_regs;
wait_queue_head_t my_wait;
int wait_status=0;

char res10[68]={
	0x00, 0x00, 0x00, 0xFF, 0x00, 0x05, 0xD0, 0x02, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
char res12[68]={
	0x00, 0x00, 0x01, 0xFF, 0x80, 0x02, 0xD0, 0x02, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
char res14[68]={
	0x00, 0x00, 0x03, 0xFF, 0x40, 0x01, 0xD0, 0x02, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
char res16[68]={
	0x00, 0x00, 0x05, 0xFF, 0xD5, 0x00, 0xD0, 0x02, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
char res18[68]={
	0x00, 0x00, 0x07, 0xFF, 0xA0, 0x00, 0xD0, 0x02, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

module_param(fpga_major, int, S_IRUGO);
MODULE_LICENSE("GPL");

irqreturn_t detect_human_handler(int irq, void *dev_id){
	wait_status = 1;
	wake_up_interruptible(&my_wait);
	*(unsigned int*)(dummy_regs+0x40) = 0x1; // ���荞�݃N���A
	*(unsigned char*)(res_regs+0x34) = 0; // ���荞�ݒ�~
	return IRQ_HANDLED;
}

static long fpga_ioctl(struct file *file,
					unsigned int cmd, unsigned long arg){
	int i;

	switch(cmd){
	case fpga_initsetting:
		g_pid=arg;
		break;
	case fpga_get_hog:
		printk("fpga_get_hog\n");
		if(wait_status == 0){
			wait_event_interruptible(my_wait, wait_status==1);
		}else{
			*(unsigned int*)(dummy_regs+0x40) = 0x1; // ���荞�݃N���A
			*(unsigned char*)(res_regs+0x34) = 0; // ���荞�ݒ�~
			wait_status = 0;
		}
		break;
	case fpga_detect_human_hog10:
		msleep(100);
		printk("fpga_detect_human_hog10\n");
		wait_status=0;
		//FPGA�ɐl�����o�v�����o��
		for(i=0; i<68; i++){
			*(unsigned char*)(res_regs+i) = res10[i];
		}
		break;
	case fpga_detect_human_hog12:
		msleep(100);
		printk("fpga_detect_human_hog12\n");
		wait_status=0;
		//FPGA�ɐl�����o�v�����o��
		for(i=0; i<68; i++){
			*(unsigned char*)(res_regs+i) = res12[i];
		}
		break;
	case fpga_detect_human_hog14:
		msleep(100);
		printk("fpga_detect_human_hog14\n");
		wait_status=0;
		//FPGA�ɐl�����o�v�����o��
		for(i=0; i<68; i++){
			*(unsigned char*)(res_regs+i) = res14[i];
		}
		break;
	case fpga_detect_human_hog16:
		msleep(100);
		printk("fpga_detect_human_hog16\n");
		wait_status=0;
		//FPGA�ɐl�����o�v�����o��
		for(i=0; i<68; i++){
			*(unsigned char*)(res_regs+i) = res16[i];
		}
		break;
	case fpga_detect_human_hog18:
	msleep(100);
		printk("fpga_detect_human_hog18\n");
		wait_status=0;
		//FPGA�ɐl�����o�v�����o��
		for(i=0; i<68; i++){
			*(unsigned char*)(res_regs+i) = res18[i];
		}
		break;
	default:
		printk("unknown command(%d)\n", cmd);
		break;
	}
	return 0;
}

static struct file_operations fpga_fops = {
	.unlocked_ioctl = fpga_ioctl,
	.compat_ioctl = fpga_ioctl,
};

static int fpga_init(void)
{
	int ret;
	
	printk("fpga_init\n");
	init_waitqueue_head(&my_wait);
	msleep(1);

	if ((ret = register_chrdev(fpga_major, DEV_NAME, &fpga_fops)) < 0) {
		printk("register_chrdev faild: %s\n", DEV_NAME);
		return -EIO;
	}

	if( (res_regs = ioremap(0x43C80000, 0x10000)) == NULL ){
		printk("mmap error[0x43C80000]\n");
		unregister_chrdev(fpga_major, DEV_NAME);
		return -1;
	}

	*(unsigned char*)(res_regs+0x32) = 1; // ���荞�ݒ�~
	printk("res_regs=%x\n", *(unsigned char*)(res_regs+0x32));
	msleep(1);
	
	if( (dummy_regs = ioremap(0x43C30000, 0x10000)) == NULL ){
		printk("mmap error[0x43C30000]\n");
		iounmap(res_regs);
		unregister_chrdev(fpga_major, DEV_NAME);
		return -1;
	}

	*(unsigned int*)(dummy_regs+0x44) = 0; // ���荞�ݗL��
	msleep(10);
	*(unsigned int*)(res_regs+0x34) = 0; // ���荞�ݗL��
	msleep(10);

	ret = request_irq(irq, detect_human_handler, IRQF_SHARED, DEV_NAME, (void*)detect_human_handler);
	if(ret < 0){
		printk("request_irq error\n");
		unregister_chrdev(fpga_major, DEV_NAME);
		iounmap(res_regs);
		iounmap(dummy_regs);
		return -1;
	}

	return 0;
}

static void fpga_exit(void)
{
	printk("fpga_exit\n");
	free_irq(irq, (void*)detect_human_handler);
	unregister_chrdev(fpga_major, DEV_NAME);
	iounmap(res_regs);
	iounmap(dummy_regs);
}

module_init(fpga_init);
module_exit(fpga_exit);

