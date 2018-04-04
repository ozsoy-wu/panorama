#ifndef __PANORAMA_MATRIX_H__
#define __PANORAMA_MATRIX_H__

#include <sys/queue.h>
#include "panorama.h"

//typedef (unsigned char) dataType;
#define dataType char

typedef struct Image_S
{
	int w;				/* 图片分辨率宽度*/
	int h;				/* 图片分辨率高度*/
	int colorDeep;			/* 图片颜色深度 */
	IMG_FORMAT imgFmt;	/* 图片格式*/
	int needFree;			/* data指针是否需要由库释放 */
	int dataBlocks;		/* 图片内存块数量 */
	int dataSize[3];			/* data指针数据大小*/
	dataType *data[3];	/* data指针，指向图片数据*/

	TAILQ_ENTRY(Image_S) entries;
} Image;

typedef struct Matrix_S
{
	int cols;
	int rows;
	int channel;	/* 每个元素的通道数，对于YUV数据channel=1，对于RGB数据channel=3 */
	int elemSize1;	/* 每个元素单通道的数据大小，字节为单位 */
	int elemSize;	/* 每个元素所有通道的数据大小，字节为单位 elemSize=elemSize1*channel */
	int step;		/* 每一行的数据总字节数，step =elemSize *channel*cols */
	int totalSize;	/* data数据总字节数，totalSize = step * rows */
	int selfNeedFree;	/* Mat自身是否需要调用free释放 */
	int dataNeedFreeByMat;	/* data指针是否需要由Mat自身释放 */
	unsigned char *data; // TODO 图片

#ifdef DEBUG_FUNC
	void (* print)(void *entity);
#endif
} Mat;

#define MAT_ROW_PTR(matPtr, row) ((unsigned char *)((matPtr)->data) + ((row) * (matPtr)->step))
#define MAT_AT_COOR(matPtr, row, col) ((unsigned char *)((matPtr)->data) + ((row) * (matPtr)->step) + (col) * (matPtr)->elemSize * (matPtr)->channel)

int integral(Mat *src, Mat *sum);

int constructMat(Mat *matPtr, int cols, int rows, int channel, int elemSize1, unsigned char *dataPtr);
int destructMat(Mat *matPtr);
int resizeMat();



/*
#define NEW_MAT(matPtr, cols, rows, channel, elemSize, dataPtr) ({\
	int ret = PANORAMA_OK;\
	(matPtr)->cols=(cols);\
	(matPtr)->rows=(rows);\
	(matPtr)->channel=(channel);\
	(matPtr)->elemSize=(elemSize);\
	(matPtr)->step= (matPtr)->elemSize * (matPtr)->channel * (matPtr)->cols;\
	(matPtr)->totalSize= (matPtr)->step * (matPtr)->rows;\
	if (dataPtr) {\
		(matPtr)->dataNeedFreeByMat = 0;\
		(matPtr)->data = dataPtr;\
	}\
	else {\
		(matPtr)->dataNeedFreeByMat = 1;\
		(matPtr)->data = lMalloc(unsigned char, (matPtr)->totalSize)\
		if ((matPtr)->data == NULL)\
		{\
			ret = PANORAMA_ERROR;\
		}\
	}\
	ret;
})

#define FREE_MAT(matPtr)
*/

#endif // __PANORAMA_MATRIX_H__
