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

int constructImage(Image **imgPtr, char **buf, int *bufSize, int bufCnt,
	int imgWidth, int imgHeight, IMG_FORMAT format, int bufType)
{
	int i;
	int okIdx = -1;
	int imgSize = 0;
	unsigned char *newP = NULL;

	if (!imgPtr)
	{
		return PANORAMA_ERROR;
	}

	if (!(*imgPtr))
	{
		*imgPtr = tMalloc(Image);
		if (!*imgPtr)
		{
			return PANORAMA_ERROR;
		}
		memset(*imgPtr, 0, sizeof(Image));
		(*imgPtr)->selfNeedFree = 1;
	}
	else
	{
		(*imgPtr)->selfNeedFree = 0;
	}


	(*imgPtr)->w = imgWidth;
	(*imgPtr)->h = imgHeight;
	(*imgPtr)->imgFmt = format;

	if (BUF_TYPE_COPY_NODELETE == bufType)
	{
		(*imgPtr)->dataNeedFree = 1;
		(*imgPtr)->dataBlocks = bufCnt;

		for (i = 0; i < bufCnt; i++)
		{
			if (!buf[i] || bufSize[i] <= 0)
			{
				goto err;
			}

			(*imgPtr)->dataSize[i] = bufSize[i];
			(*imgPtr)->data[i] = NULL;
			newP = (unsigned char *)lMalloc(unsigned char, bufSize[i]);
			if (!newP)
			{
				goto err;
			}
			memset(newP, 0, bufSize[i]);
			memcpy(newP, buf[i], bufSize[i]);

			(*imgPtr)->data[i] = newP;
			okIdx = i;
		}
	}
	else if (BUF_TYPE_NOCOPY_NODELETE == bufType)
	{
		(*imgPtr)->dataNeedFree = 0;
		(*imgPtr)->dataBlocks = bufCnt;
		for (i = 0; i < bufCnt; i++)
		{
			(*imgPtr)->dataSize[i] = bufSize[i];
			(*imgPtr)->data[i] = (unsigned char *)buf[i];
		}
	}
	else if (BUF_TYPE_NOCOPY_DELETE == bufType)
	{
		(*imgPtr)->dataNeedFree = 1;
		(*imgPtr)->dataBlocks = bufCnt;
		for (i = 0; i < bufCnt; i++)
		{
			(*imgPtr)->dataSize[i] = bufSize[i];
			(*imgPtr)->data[i] = (unsigned char *)buf[i];
		}
	}
	else if (BUF_TYPE_NOBUF == bufType)
	{
		(*imgPtr)->dataNeedFree = 1;
		(*imgPtr)->dataBlocks = 1;

		if (IMG_FMT_YUV420P_I420 == format ||
			IMG_FMT_YUV420P_YV12 == format ||
			IMG_FMT_YUV420SP_NV12 == format ||
			IMG_FMT_YUV420SP_NV21 == format)
		{
			imgSize = imgWidth * imgHeight * 3 / 2;
		}
		else
		{
			imgSize = imgWidth * imgHeight;
		}

		newP = (unsigned char *)lMalloc(unsigned char, imgSize);
		if (!newP)
		{
			goto err;
		}
		memset(newP, 0, imgSize);

		(*imgPtr)->dataSize[0] = imgSize;
		(*imgPtr)->data[0] = newP;
	}

	return PANORAMA_OK;

err:

	if ((*imgPtr))
	{
		if ((*imgPtr)->dataNeedFree)
		{
			for (i = 0; i <= okIdx; i++)
			{
				FREE((*imgPtr)->data[i]);
			}
			(*imgPtr)->dataNeedFree = 0;
		}

		if ((*imgPtr)->selfNeedFree)
		{
			FREE(*imgPtr);
			(*imgPtr)->selfNeedFree = 0;
		}
	}

	return PANORAMA_ERROR;
}

int destructImage(Image **imgPtr)
{
	int i;
	if (imgPtr && *imgPtr)
	{
		if ((*imgPtr)->dataNeedFree)
		{
			for (i = 0; i < 3; i++)
			{
				FREE((*imgPtr)->data[i]);
			}
			(*imgPtr)->dataNeedFree = 0;
		}

		if ((*imgPtr)->selfNeedFree)
		{
			FREE(*imgPtr);
			(*imgPtr)->selfNeedFree = 0;
		}
	}

	return PANORAMA_OK;
}

int constructMat(Mat **matPtr, int cols, int rows, int channel, int elemSize1, unsigned char *dataPtr)
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

#ifdef DEBUG_FUNC
	(*matPtr)->print = printMatrix;
#endif

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

int destructMat(Mat **matPtr)
{
	if (matPtr && *matPtr)
	{
		if ((*matPtr)->dataNeedFreeByMat)
		{
			FREE((*matPtr)->data);
		}
		if ((*matPtr)->selfNeedFree)
		{
			FREE(*matPtr);
			*matPtr = NULL;
		}
	}

	return PANORAMA_OK;
}

int resizeMat(Mat *src, Mat *dst, double fx, double fy, INTERPOLATION_METHOD method)
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
int integral(Mat *src, Mat *sum)
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
