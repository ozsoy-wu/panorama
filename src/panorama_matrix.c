/******************************************************************************
 * Copyright (c) 2015-2018 TP-Link Technologies CO.,LTD.
 *
 * 文件名称:		panorama_matrix.c
 * 版           本:	1.0
 * 摘           要:	二维矩阵功能
 * 作           者:	wupimin<wupimin@tp-link.com.cn>
 * 创建时间:		2018-04-28
 ******************************************************************************/

#include <stdio.h>

#include "panorama_utils.h"
#include "panorama_matrix.h"

int matConstruct(Mat **matPtr, int cols, int rows, int channel, int elemSize1, unsigned char *dataPtr)
{
	if (!matPtr)
	{
		return PANORAMA_ERROR;
	}

	if (!(*matPtr))
	{
		*matPtr = tMalloc(Mat);
		if (!*matPtr)
		{
			return PANORAMA_ERROR;
		}
		memset(*matPtr, 0, sizeof(Mat));
		(*matPtr)->selfNeedFree = 1;
	}
	else
	{
		(*matPtr)->selfNeedFree = 0;
	}

	(*matPtr)->cols = cols;
	(*matPtr)->rows = rows;
	(*matPtr)->channel = channel;
	(*matPtr)->elemSize1 = elemSize1;
	(*matPtr)->elemSize = elemSize1 * channel;
	(*matPtr)->step = elemSize1 * channel * cols;
	(*matPtr)->totalSize = (*matPtr)->step * rows;

	if (!dataPtr)
	{
		(*matPtr)->data = lMalloc(unsigned char, (*matPtr)->totalSize);
		if ((*matPtr)->data == NULL)
		{
			if ((*matPtr)->selfNeedFree)
			{
				(*matPtr)->selfNeedFree = 0;
				FREE(*matPtr);
			}
			return PANORAMA_ERROR;
		}
		memset((*matPtr)->data, 0 , (*matPtr)->totalSize);
		(*matPtr)->dataNeedFreeByMat = 1;
	}
	else
	{
		(*matPtr)->data = dataPtr;
		(*matPtr)->dataNeedFreeByMat = 0;
	}

	return PANORAMA_OK;
}

int matDestruct(Mat **matPtr)
{
	if (matPtr && *matPtr)
	{
		if ((*matPtr)->dataNeedFreeByMat)
		{
			(*matPtr)->dataNeedFreeByMat = 0;
			FREE((*matPtr)->data);
		}
		if ((*matPtr)->selfNeedFree)
		{
			(*matPtr)->selfNeedFree = 0;
			FREE(*matPtr);
		}
	}

	return PANORAMA_OK;
}

int matResize(Mat *src, Mat *dst, double fx, double fy, INTERPOLATION_METHOD method)
{
	if (!src || !dst || !src->data || !dst->data)
	{
		return PANORAMA_ERROR;
	}

	if (INTER_NEAREST == method)
	{
		int sx, sy, xOfSrc;
		int y, x, pix_size = src->elemSize;
		unsigned char *D = NULL;
		unsigned char *S = NULL;
		double ifx = 1./fx;
		double ify = 1./fy;

		for (y = 0; y < dst->rows; y++)
		{
			D = (unsigned char *)MAT_ROW_PTR(dst, y);
			sy = MIN(floor(y*ify), src->rows - 1);
			S = (unsigned char *)MAT_ROW_PTR(src, y);

			switch (pix_size)
			{
			case 1:
				for( x = 0; x <= dst->cols - 2; x += 2 )
				{
					// TODO, 可优化，将xofsrc记录到数组，可避免重复计算
					sx = (int)floor(x * ifx);
					xOfSrc = MIN(sx, src->cols - 1) * pix_size;
					D[x] = S[xOfSrc];

					sx = floor((x + 1) * ifx);
					xOfSrc = MIN(sx, src->cols - 1) * pix_size;
					D[x+1] = S[xOfSrc];
				}

				for( ; x < dst->cols; x++ )
				{
					sx = floor(x * ifx);
					xOfSrc = MIN(sx, src->cols - 1) * pix_size;
					D[x] = S[xOfSrc];
				}
				break;
			}
		}
	}
	return PANORAMA_OK;

}

/*
* 计算矩阵积分
* 结果保存到sum中
* sum矩阵需要使用int类型
*/
int matIntegral(Mat *src, Mat *sum)
{
	int i, j, dx, dy;
	unsigned char *pSrc = NULL;
	int *pDst = NULL;
	int *pDst1 = NULL;
	int *pDst2 = NULL;
	int *pDst3 = NULL;

	if (!src || !sum || !src->data || !sum->data)
	{
		return PANORAMA_ERROR;
	}

	pSrc = (unsigned char *)src->data;
	pDst = (int *)sum->data;

	memset(pDst, 0, sum->totalSize);

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
