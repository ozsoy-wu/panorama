#ifndef __PANORAMA_MATRIX_H__
#define __PANORAMA_MATRIX_H__

#include <sys/queue.h>
#include "panorama.h"

typedef enum RESIZE_INTERPOLATION_METHOD {
	INTER_NEAREST = 0,
	INTER_LINEAR,
	INTER_AREA,
	INTER_CUBIC,
	INTER_LANCZOS4
} INTERPOLATION_METHOD;

typedef enum BUF_TYPE_E {
	BUF_TYPE_COPY_DELETE = 0,
	BUF_TYPE_COPY_NODELETE,
	BUF_TYPE_NOCOPY_DELETE,
	BUF_TYPE_NOCOPY_NODELETE,
	BUF_TYPE_NOBUF
} BUF_TYPE;

typedef struct Image_S
{
	int w;				/* ͼƬ�ֱ��ʿ��*/
	int h;				/* ͼƬ�ֱ��ʸ߶�*/
	int colorDeep;		/* ͼƬ��ɫ��� */
	IMG_FORMAT imgFmt;	/* ͼƬ��ʽ*/
	int selfNeedFree;	/* Image�����Ƿ���Ҫ����free�ͷ� */
	int dataNeedFree;	/* dataָ���Ƿ���Ҫ��Image�����ͷ� */
	int dataBlocks;		/* ͼƬ�ڴ������ */
	int dataSize[3];		/* dataָ�����ݴ�С*/
	unsigned char *data[3];	/* dataָ�룬ָ��ͼƬ����*/
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
} Mat;

#define MAT_ROW_PTR(matPtr, row) ((unsigned char *)((matPtr)->data) + ((row) * (matPtr)->step))
#define MAT_AT_COOR(matPtr, row, col) ((unsigned char *)((matPtr)->data) + ((row) * (matPtr)->step) + (col) * (matPtr)->elemSize * (matPtr)->channel)

int integral(Mat *src, Mat *sum);

int constructImage(Image **imgPtr, char **buf, int *bufSize, int bufCnt,
	int imgWidth, int imgHeight, IMG_FORMAT format, int copy);
int destructImage(Image **imgPtr);

int constructMat(Mat **matPtr, int cols, int rows, int channel, int elemSize1, unsigned char *dataPtr);
int destructMat(Mat **matPtr);
int resizeMat(Mat *src, Mat *dst, double fx, double fy, INTERPOLATION_METHOD method);

#endif // __PANORAMA_MATRIX_H__
