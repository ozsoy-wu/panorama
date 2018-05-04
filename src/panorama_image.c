/******************************************************************************
 * Copyright (c) 2015-2018 TP-Link Technologies CO.,LTD.
 *
 * 文件名称:		panorama_image.c
 * 版           本:	1.0
 * 摘           要:	图像表示及操作
 * 作           者:	wupimin<wupimin@tp-link.com.cn>
 * 创建时间:		2018-04-28
 ******************************************************************************/

#include <stdio.h>

#include "panorama_utils.h"
#include "panorama_log.h"
#include "panorama_features2d.h"
#include "panorama_image.h"

int imageConstruct(Image **imgPtr, char **buf, int *bufSize, int bufCnt,
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
		if (!(*imgPtr))
		{
			return PANORAMA_ERROR;
		}
		memset(*imgPtr, 0, sizeof(Image));
		(*imgPtr)->selfNeedFree = 1;
	}
	else
	{
		(*imgPtr)->selfNeedFree = 0;
		(*imgPtr)->w = imgWidth;
		(*imgPtr)->h = imgHeight;
		(*imgPtr)->imgFmt = format;
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
				Log(LOG_ERROR, "buf[%d]=%s, bufSize[i]=%d\n", buf[i] ? "NOT null" : "NULL", bufSize[i]);
				goto err;
			}

			(*imgPtr)->dataSize[i] = bufSize[i];
			(*imgPtr)->data[i] = NULL;
			newP = (unsigned char *)lMalloc(unsigned char, bufSize[i]);
			if (!newP)
			{
				Log(LOG_ERROR, "malloc failed\n");
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
			Log(LOG_ERROR, "malloc failed\n");
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
			(*imgPtr)->selfNeedFree = 0;
			FREE(*imgPtr);
		}
	}

	return PANORAMA_ERROR;
}

int imageDestruct(Image **imgPtr)
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
			(*imgPtr)->selfNeedFree = 0;
			FREE(*imgPtr);
		}
	}

	return PANORAMA_OK;
}

/* 矫正 */
int imageUndistort(double k, double k2, Image *src, Image **dstImg)
{
	int ret;
	int i, j, subi, subj;
	int scol, srow;
	double halfW, halfH;
	int upx, upy, downx, downy;
	int dstW, dstH;
	int dstHalfW, dstHalfH;
	double dx, dy;
	double dispower2, dispower4;
	double projectionX, projectionY; // 以中心为原点之后的原始图像素坐标
	Point p0, p1, tmp;
	double p1R2, p1R4;
	unsigned char *srcPtr;
	unsigned char *srcuPtr;
	unsigned char *srcvPtr;
	unsigned char *ptr0;
	unsigned char *uptr;
	unsigned char *vptr;
	Mat *ySrc = NULL;
	Mat *uSrc = NULL;
	Mat *vSrc = NULL;
	Mat *yDst = NULL;
	Mat *uDst = NULL;
	Mat *vDst = NULL;

	if (NULL == src)
	{
		return PANORAMA_ERROR;
	}

	scol = src->w;
	srow = src->h;
	halfW = src->w / 2;
	halfH = src->h / 2;

	/* 计算原始图像矫正后的分辨率 */
	p0.x = 0;
	p0.y = 0;
	p1.x = halfW;
	p1.y = halfH;
	p1R2 = pointDisPower2(&p0, &p1);
	p1R4 = p1R2 * p1R2;

	/* 矫正后的图像中心水平线宽度 */
	dstW = 2 * ceil(CORRECT_COOR(p1.x, k, p1R2, k2, p1R4));
	dstH = 2 * ceil(CORRECT_COOR(p1.y, k, p1R2, k2, p1R4));
	dstHalfW = dstW / 2;
	dstHalfH = dstH / 2;

	/* 后续步骤需要用到该数据，由deinit释放 */
	ret = imageConstruct(dstImg, NULL, NULL, 0, dstW, dstH, src->imgFmt, BUF_TYPE_NOBUF);
	if (PANORAMA_OK != ret)
	{
		ret = PANORAMA_ERROR;
		goto clean;
	}

	ret = matConstruct(&ySrc, scol, srow, 1, sizeof(unsigned char), IMAGE_Y_PTR(src));
	if (PANORAMA_OK != ret)
	{
		ret = PANORAMA_ERROR;
		goto clean;
	}

	ret = matConstruct(&uSrc, halfW, halfH, 1, sizeof(unsigned char), IMAGE_U_PTR(src));
	if (PANORAMA_OK != ret)
	{
		ret = PANORAMA_ERROR;
		goto clean;
	}

	ret = matConstruct(&vSrc, halfW, halfH, 1, sizeof(unsigned char), IMAGE_V_PTR(src));
	if (PANORAMA_OK != ret)
	{
		ret = PANORAMA_ERROR;
		goto clean;
	}

	ret = matConstruct(&yDst, dstW, dstH, 1, sizeof(unsigned char), IMAGE_Y_PTR(*dstImg));
	if (PANORAMA_OK != ret)
	{
		ret = PANORAMA_ERROR;
		goto clean;
	}

	ret = matConstruct(&uDst, dstHalfW, dstHalfH, 1, sizeof(unsigned char), IMAGE_U_PTR(*dstImg));
	if (PANORAMA_OK != ret)
	{
		ret = PANORAMA_ERROR;
		goto clean;
	}

	ret = matConstruct(&vDst, dstHalfW, dstHalfH, 1, sizeof(unsigned char), IMAGE_V_PTR(*dstImg));
	if (PANORAMA_OK != ret)
	{
		ret = PANORAMA_ERROR;
		goto clean;
	}

	for (j = 0; j < src->h; j++)
	{
		for (i = 0; i < src->w; i++)
		{
			srcPtr = (unsigned char *)MAT_AT_COOR(ySrc, j, i);
			srcuPtr = (unsigned char *)MAT_AT_COOR(uSrc, j/2, i/2);
			srcvPtr = (unsigned char *)MAT_AT_COOR(vSrc, j/2, i/2);

			projectionX = i - halfW;
			projectionY = j - halfH;
			tmp.x = projectionX;
			tmp.y = projectionY;
			dispower2 = pointDisPower2(&p0, &tmp);
			dispower4 = dispower2 *dispower2;
			dx = CORRECT_COOR(projectionX, k, dispower2, k2, dispower4);
			dy = CORRECT_COOR(projectionY, k, dispower2, k2, dispower4);

			upx = ceil(dx);
			downx = floor(dx);
			upy = ceil(dy);
			downy = floor(dy);

			upx += dstHalfW;
			upy += dstHalfH;
			downx += dstHalfW;
			downy += dstHalfH;

			for (subi = downy; subi <= upy; subi++)
			{
				for (subj = downx; subj <= upx; subj++)
				{
					if (subi < 0 || subi >= dstH ||
						subj < 0 || subj >= dstW)
					{
						continue;
					}

					// Y
					ptr0 = (unsigned char *)MAT_AT_COOR(yDst, subi, subj);
					*ptr0 = *srcPtr;

					// U
					uptr = (unsigned char *)MAT_AT_COOR(uDst, subi/2, subj/2);
					*uptr = *srcuPtr;

					// V
					vptr = (unsigned char *)MAT_AT_COOR(vDst, subi/2, subj/2);
					*vptr = *srcvPtr;
				}
			}
		}
	}

clean:
	matDestruct(&vDst);
	matDestruct(&uDst);
	matDestruct(&yDst);
	matDestruct(&vSrc);
	matDestruct(&uSrc);
	matDestruct(&ySrc);

	return ret;
}

int distortCalcK1K2(double distortLevel, int W, int H, double *k1, double *k2)
{
	double halfW = W / 2;
	double halfH = H / 2;

	// 原点
	Point p0;
	p0.x = 0;
	p0.y = 0;

	int calcCnt = 0;
	double vk1 = 0.;
	double vk2 = 0.;
	double vk1Left, vk1Right;

	Point endp1, endp2, midp;
	double p1R2, p1R4;
	double p2R2, p2R4;
	double mpR2, mpR4;
	double lineDisNumerator, p0DisNumerator;
	double lineDisDenominator;
	double xb, yb, xe, ye, xm, ym;
	double curD;

	// 端点赋值
	endp1.x = -halfW;
	endp2.x = halfW;
	endp1.y = endp2.y = halfH * (1 + distortLevel);
	midp.x = 0;
	midp.y = halfH;

	// 计算距离原点距离平方
	p1R2 = pointDisPower2(&p0, &endp1);
	p2R2 = pointDisPower2(&p0, &endp2);
	p1R4 = p1R2 * p1R2;
	p2R4 = p2R2 * p2R2;
	mpR2 = pointDisPower2(&p0, &midp);
	mpR4 = mpR2 * mpR2;

	Dbg("endp1=[%10.20f, %10.20f], endp2=[%10.20f, %10.20f], midp=[%10.20f, %10.20f]\n",
			endp1.x, endp1.y, endp2.x, endp2.y, midp.x, midp.y);

	calcCnt = 0;
	vk1Left = 0;
	vk1Right = 1;
	while (vk1Left < vk1Right)
	{
		calcCnt++;

		vk1 = (vk1Left + vk1Right) / 2;

		// 校正后直线端点
		xb = CORRECT_COOR(endp1.x, vk1, p1R2, vk2, p1R4);
		yb = CORRECT_COOR(endp1.y, vk1, p1R2, vk2, p1R4);
		xe = CORRECT_COOR(endp2.x, vk1, p2R2, vk2, p2R4);
		ye = CORRECT_COOR(endp2.y, vk1, p2R2, vk2, p2R4);
		xm = CORRECT_COOR(midp.x, vk1, mpR2, vk2, mpR4);
		ym = CORRECT_COOR(midp.y, vk1, mpR2, vk2, mpR4);

		// 计算原点与直线的关系
		p0DisNumerator = ((ye - yb) * p0.x -
				(xe - xb) * p0.y +
				((xe - xb) * yb - (ye - yb) * xb));

		// 计算直线距离的分母
		lineDisDenominator = sqrt((ye - yb) * (ye - yb) + (xe - xb) * (xe - xb));
		lineDisNumerator = ((ye - yb) * xm -
				(xe - xb) * ym +
				((xe - xb) * yb - (ye - yb) * xb));

//		Dbg("numerator=%10.20f, de=%10.20f\n", lineDisNumerator, lineDisDenominator);
		curD = lineDisNumerator / lineDisDenominator;

		if (fabs(curD) <= 1e-15)
		{
			Dbg("curD = %10.20f\n", curD);
			break;
		}

		if (calcCnt >= 5000)
		{
			Dbg("reach max count %d\n", calcCnt);
			break;
		}

		// 桶形畸变
		if (distortLevel < 0)
		{
			if (SAME_SIDE_WITH_P0(p0DisNumerator, lineDisNumerator))
			{
				vk1Right = vk1;
			}
			else
			{
				vk1Left = vk1;
			}
		}
		// 枕形畸变
		else
		{
			if (SAME_SIDE_WITH_P0(p0DisNumerator, lineDisNumerator))
			{
				vk1Left = vk1;
			}
			else
			{
				vk1Right = vk1;
			}
		}
	}

	*k1 = vk1;
	*k2 = vk2;

	return PANORAMA_OK;
}

int calcK1(double *k1)
{
	int imgW = 1280;
	int imgH = 720;
	double halfImgW = imgW / 2;
	double halfImgH = imgH / 2;

	// 原点
	Point p0;
	p0.x = 0;
	p0.y = 0;

	int hpcnt = 15;
	double hpx[100] = {
		30, 27, 25, 23, 20, 17, 17, 16, 16, 16, 17, 18, 22, 25, 30};
	double hpy[100] = {
		34, 70, 105, 142, 179, 216, 254, 292, 329, 367, 406, 444, 518, 592, 663};

	Point hp[100];

	// 赋值
	int i;
	for (i = 0; i < hpcnt; i++)
	{
		hp[i].x = hpx[i] - halfImgW;
		hp[i].y = hpy[i] - halfImgH;
	}

	int calcCnt = 0;
	double vk1, vk2;
	double finalhk1;
	double vk1Left, vk1Right;

	Point endp1, endp2;
	double p1R2, p1R4;
	double p2R2, p2R4;
	double mpR2, mpR4;
	double lineDisNumerator, p0DisNumerator;
	double lineDisDenominator;
	double xb, yb, xe, ye, xm, ym;
	double totalD, curD;

	// 端点赋值
	endp1.x = hp[0].x;
	endp1.y = hp[0].y;
	endp2.x = hp[hpcnt-1].x;
	endp2.y = hp[hpcnt-1].y;


	// 计算距离原点距离平方
	p1R2 = pointDisPower2(&p0, &endp1);
	p2R2 = pointDisPower2(&p0, &endp2);
	p1R4 = p1R2 * p1R2;
	p2R4 = p2R2 * p2R2;

	calcCnt = 0;
	vk1Left = 0;
	vk1Right = 10;
	vk2 = 0; // 不计算k2
	while (vk1Left < vk1Right)
	{
		calcCnt++;
		totalD = 0.;

		vk1 = (vk1Left + vk1Right) / 2;

		// 校正后直线端点
		xb = CORRECT_COOR(endp1.x, vk1, p1R2, vk2, p1R4);
		yb = CORRECT_COOR(endp1.y, vk1, p1R2, vk2, p1R4);
		xe = CORRECT_COOR(endp2.x, vk1, p2R2, vk2, p2R4);
		ye = CORRECT_COOR(endp2.y, vk1, p2R2, vk2, p2R4);

		// 计算原点与直线的关系
		p0DisNumerator = ((ye - yb) * p0.x -
				(xe - xb) * p0.y +
				((xe - xb) * yb - (ye - yb) * xb));

		// 计算直线距离的分母
		lineDisDenominator = sqrt((ye - yb) * (ye - yb) + (xe - xb) * (xe - xb));
		for (i = 1; i < hpcnt - 1; i++)
		{

			mpR2 = pointDisPower2(&p0, &hp[i]);
			mpR4 = mpR2 * mpR2;
			xm = CORRECT_COOR(hp[i].x, vk1, mpR2, vk2, mpR4);
			ym = CORRECT_COOR(hp[i].y, vk1, mpR2, vk2, mpR4);
			//xm = hp[i].x;
			//ym = hp[i].y;
			lineDisNumerator = ((ye - yb) * xm -
				(xe - xb) * ym +
				((xe - xb) * yb - (ye - yb) * xb));
			/*
			curD = (lineDisNumerator) / lineDisDenominator;
			totalD += curD;
			*/
			curD = fabs(lineDisNumerator) / lineDisDenominator;

			if (SAME_SIDE_WITH_P0(p0DisNumerator, lineDisNumerator))
			{
				totalD -= curD;
			}
			else
			{
				totalD += curD;
			}
		}

		finalhk1 = vk1;
		if (fabs(totalD) <= 1e-20)
		{
			break;
		}

		if (calcCnt >= 5000)
		{
			break;
		}

		// 默认为桶形畸变
		// k1太大，导致矫正点与原点在同一侧
		if (totalD < 0)
		{
			vk1Right = vk1;
		}
		else
		{
			vk1Left = vk1;
		}
	}

	*k1 = finalhk1;

	return PANORAMA_OK;
}


