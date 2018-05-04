#ifndef __PANORAMA_MATRIX_H__
#define __PANORAMA_MATRIX_H__

#include "panorama.h"

typedef enum RESIZE_INTERPOLATION_METHOD {
	INTER_NEAREST = 0,
	INTER_LINEAR,
	INTER_AREA,
	INTER_CUBIC,
	INTER_LANCZOS4
} INTERPOLATION_METHOD;

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
} Mat;


#define MAT_ROW_PTR(matPtr, row) ((unsigned char *)((matPtr)->data) + ((row) * (matPtr)->step))
#define MAT_AT_COOR(matPtr, row, col) ((unsigned char *)((matPtr)->data) + ((row) * (matPtr)->step) + (col) * (matPtr)->elemSize * (matPtr)->channel)

int matIntegral(Mat *src, Mat *sum);

int matConstruct(Mat **matPtr, int cols, int rows, int channel, int elemSize1, unsigned char *dataPtr);
int matDestruct(Mat **matPtr);
int matResize(Mat *src, Mat *dst, double fx, double fy, INTERPOLATION_METHOD method);

#endif // __PANORAMA_MATRIX_H__
