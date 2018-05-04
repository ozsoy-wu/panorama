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
	int channel;	/* ÿ��Ԫ�ص�ͨ����������YUV����channel=1������RGB����channel=3 */
	int elemSize1;	/* ÿ��Ԫ�ص�ͨ�������ݴ�С���ֽ�Ϊ��λ */
	int elemSize;	/* ÿ��Ԫ������ͨ�������ݴ�С���ֽ�Ϊ��λ elemSize=elemSize1*channel */
	int step;		/* ÿһ�е��������ֽ�����step =elemSize *channel*cols */
	int totalSize;	/* data�������ֽ�����totalSize = step * rows */
	int selfNeedFree;	/* Mat�����Ƿ���Ҫ����free�ͷ� */
	int dataNeedFreeByMat;	/* dataָ���Ƿ���Ҫ��Mat�����ͷ� */
	unsigned char *data; // TODO ͼƬ
} Mat;


#define MAT_ROW_PTR(matPtr, row) ((unsigned char *)((matPtr)->data) + ((row) * (matPtr)->step))
#define MAT_AT_COOR(matPtr, row, col) ((unsigned char *)((matPtr)->data) + ((row) * (matPtr)->step) + (col) * (matPtr)->elemSize * (matPtr)->channel)

int matIntegral(Mat *src, Mat *sum);

int matConstruct(Mat **matPtr, int cols, int rows, int channel, int elemSize1, unsigned char *dataPtr);
int matDestruct(Mat **matPtr);
int matResize(Mat *src, Mat *dst, double fx, double fy, INTERPOLATION_METHOD method);

#endif // __PANORAMA_MATRIX_H__
