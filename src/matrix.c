#include <stdio.h>

#include "utils.h"
#include "matrix.h"

#ifdef DEBUG_FUNC
void printMatrix(void *entity)
{
	Mat *mat = (Mat *)entity;
	printf("Mat cols=%d, rows=%d\n", mat->cols, mat->rows);
	printf("Mat channel=%d, elemSize1=%d, elemSize=%d, step=%d, totalSize=%d\n",
		mat->channel, mat->elemSize1, mat->elemSize, mat->step, mat->totalSize);
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
			dy = i + 1;
			pDst[dy * dstW + dx] = pSrc[i * srcW + j] + pDst[(dy - 1) * dstW + dx]
				+ pDst[dy * dstW + dx - 1] - pDst[(dy - 1) * dstW + dx - 1];
		}
	}


	return PANORAMA_OK;
}
