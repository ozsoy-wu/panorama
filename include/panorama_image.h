#ifndef __PANORAMA_IMAGE_H__
#define __PANORAMA_IMAGE_H__

#include "panorama.h"

typedef enum BUF_TYPE_E {
	BUF_TYPE_COPY_DELETE = 0,
	BUF_TYPE_COPY_NODELETE,
	BUF_TYPE_NOCOPY_DELETE,
	BUF_TYPE_NOCOPY_NODELETE,
	BUF_TYPE_NOBUF
} BUF_TYPE;

typedef struct Image_S
{
	int w;				/* 图片分辨率宽度*/
	int h;				/* 图片分辨率高度*/
	int colorDeep;		/* 图片颜色深度 */
	IMG_FORMAT imgFmt;	/* 图片格式*/
	int selfNeedFree;	/* Image自身是否需要调用free释放 */
	int dataNeedFree;	/* data指针是否需要由Image自身释放 */
	int dataBlocks;		/* 图片内存块数量 */
	int dataSize[3];		/* data指针数据大小*/
	unsigned char *data[3];	/* data指针，指向图片数据*/
} Image;


#define IMAGE_Y_PTR(iPtr) ((unsigned char *)(iPtr)->data[0])
#define IMAGE_U_PTR(iPtr) ({\
	int ysize = (iPtr)->w* (iPtr)->h; \
	unsigned char *res; \
	if ((iPtr)->dataBlocks == 1) { \
		res = (unsigned char *)(iPtr)->data[0] + ysize; \
	} else if ((iPtr)->dataBlocks >= 2) { \
		res= (unsigned char *)(iPtr)->data[1]; \
	} \
	res; \
})
#define IMAGE_V_PTR(iPtr) ({\
	int ysize = (iPtr)->w * (iPtr)->h; \
	int usize = ((iPtr)->w / 2) * ((iPtr)->h / 2); \
	unsigned char *res; \
	if ((iPtr)->dataBlocks == 1) { \
		res = (unsigned char *)(iPtr)->data[0] + ysize + usize; \
	} else if ((iPtr)->dataBlocks == 2) { \
		res= (unsigned char *)(iPtr)->data[1] + usize; \
	} \
	else if ((iPtr)->dataBlocks == 3) { \
		res= (unsigned char *)(iPtr)->data[3]; \
	} \
	res; \
})

int imageConstruct(Image **imgPtr, char **buf, int *bufSize, int bufCnt,
	int imgWidth, int imgHeight, IMG_FORMAT format, int bufType);
int imageDestruct(Image **imgPtr);
int distortCalcK1K2(double distortLevel, int W, int H, double *k1, double *k2);
int calcK1(double *k1);
int imageUndistort(double k, double k2, Image *src, Image **dst);

#endif // __PANORAMA_IMAGE_H__