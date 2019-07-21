#if !defined(__HOG_DETECT_H__)
#define __HOG_DETECT_H__
typedef float ACCURACY;

void convertRGB2Gray(unsigned char* src, unsigned char* dst, size_t width, size_t height);
void detectpeople(unsigned char* colptr, unsigned char* gryptr, unsigned char* gryptr_res, ACCURACY *cell_hist, ACCURACY *pHOGFeatures, unsigned short *HogBin,
    size_t width_img, size_t height_img,int *ada_list_x,int *ada_list_y,int *ada_list_h,float *ada_list_w, int pat);

#endif