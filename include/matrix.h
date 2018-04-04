#ifndef __PANORAMA_MATRIX_H__
#define __PANORAMA_MATRIX_H__

#include <sys/queue.h>
#include "panorama.h"

//typedef (unsigned char) dataType;
#define dataType char

typedef struct Image_S
{
	int w;				/* ͼƬ�ֱ��ʿ��*/
	int h;				/* ͼƬ�ֱ��ʸ߶�*/
	int colorDeep;			/* ͼƬ��ɫ��� */
	IMG_FORMAT imgFmt;	/* ͼƬ��ʽ*/
	int needFree;			/* dataָ���Ƿ���Ҫ�ɿ��ͷ� */
	int dataBlocks;		/* ͼƬ�ڴ������ */
	int dataSize[3];			/* dataָ�����ݴ�С*/
	dataType *data[3];	/* dataָ�룬ָ��ͼƬ����*/

	TAILQ_ENTRY(Image_S) entries;
} Image;

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
