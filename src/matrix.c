#include <stdio.h>

#include "utils.h"
#include "matrix.h"

#ifdef DEBUG_FUNC
void printMatrix(void *entity)
{
	int *p;
	Mat *mat = (Mat *)entity;
	printf("Mat cols=%d, rows=%d, ", mat->cols, mat->rows);
	printf("channel=%d, elemSize1=%d, elemSize=%d, step=%d, totalSize=%d\n",
		mat->channel, mat->elemSize1, mat->elemSize, mat->step, mat->totalSize);

	int i, j;
	for (i = 0; i < mat->rows; i++)
	{
		for (j = 0; j < mat->cols; j++)
		{
			p = (int *)MAT_AT_COOR(mat, i, j);

			printf("%d, ", *p);
			//printf("%f, ", (char)mat->data +mat->step * i + mat->elemSize * j);
		}

		printf("\n");
	}
}
#endif

int constructMat(Mat *matPtr, int cols, int rows, int channel, int elemSize1, unsigned char *dataPtr)
{
	if (!matPtr)
	{
		matPtr = tMalloc(Mat);
		if (!matPtr)
		{
			return PANORAMA_ERROR;
		}
		memset(matPtr, 0, sizeof(Mat));
		matPtr->selfNeedFree = 1;
	}
	else
	{
		matPtr->selfNeedFree = 0;
	}

	matPtr->cols = cols;
	matPtr->rows = rows;
	matPtr->channel = channel;
	matPtr->elemSize1 = elemSize1;
	matPtr->elemSize = elemSize1 * channel;
	matPtr->step = elemSize1 * channel * cols;
	matPtr->totalSize = matPtr->step * rows;

#ifdef DEBUG_FUNC
	matPtr->print = printMatrix;
#endif

	if (!dataPtr)
	{
		matPtr->data = lMalloc(unsigned char, matPtr->totalSize);
		if (matPtr->data == NULL)
		{
			if (matPtr->selfNeedFree)
			{
				matPtr->selfNeedFree = 0;
				FREE(matPtr);
			}
			return PANORAMA_ERROR;
		}
		memset(matPtr->data, 0 , matPtr->totalSize);
		matPtr->dataNeedFreeByMat = 1;
	}
	else
	{
		matPtr->data = dataPtr;
		matPtr->dataNeedFreeByMat = 0;
	}

	return PANORAMA_OK;
}

int destructMat(Mat *matPtr)
{
	if (matPtr)
	{
		if (matPtr->dataNeedFreeByMat)
		{
			FREE(matPtr->data);
		}
		if (matPtr->selfNeedFree)
		{
			FREE(matPtr);
		}
	}

	return PANORAMA_OK;
}

/*
* 计算矩阵积分
* 结果保存到sum中
* sum矩阵需要使用int类型
*/
int integral(Mat *src, Mat *sum)
{
	int i, j, dx, dy, srcW, srcH, dstW, dstH;
	unsigned char *pSrc = NULL;
	int *pDst = NULL;
	int *pDst1 = NULL;
	int *pDst2 = NULL;
	int *pDst3 = NULL;
	int tmp;

	if (!src || !sum || !src->data || !sum->data)
	{
		return PANORAMA_ERROR;
	}

	pSrc = (unsigned char *)src->data;
	pDst = (int *)sum->data;
	srcW = src->cols;
	srcH = src->rows;
	dstW = sum->cols = src->cols + 1;
	dstH = sum->rows = src->rows + 1;

	memset(pDst, 0, sum->cols * sum->rows * sizeof(int));

	for (i = 0; i < src->rows; i++)
	{
		dx = i + 1;
		for (j = 0; j < src->cols; j++)
		{
			dy = j + 1;

			pSrc = MAT_AT_COOR(src, i, j);
			pDst1 = (int *)MAT_AT_COOR(sum, dx-1, dy);
			pDst2 =  (int *)MAT_AT_COOR(sum, dx, dy-1);
			pDst3 =  (int *)MAT_AT_COOR(sum, dx-1, dy-1);
			pDst =  (int *)MAT_AT_COOR(sum, dx, dy);
			
			*pDst = (int)*pSrc + *pDst1 + *pDst2 - *pDst3;
		}
	}

	return PANORAMA_OK;
}
