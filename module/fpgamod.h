#define fpga_detect_human_hog10 0x20
#define fpga_detect_human_hog12 0x21
#define fpga_detect_human_hog14 0x22
#define fpga_detect_human_hog16 0x23
#define fpga_detect_human_hog18 0x24
#define fpga_take_picture 0x11
#define fpga_exec_reset 0x12
#define fpga_get_flag 0x13
#define fpga_initsetting 0x14
#define fpga_get_hog 0x15

struct fpga_data{
	char pat;
};

