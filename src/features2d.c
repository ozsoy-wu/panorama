#include <math.h>

#include "panorama.h"
#include "features2d.h"
#include "utils.h"
#include "log.h"

int distortCalcK1K2(double distortLevel, int W, int H, double *k1, double *k2)
{
	int j;
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
	double p1R2, p2R2;
	double p1R4, p2R4;
	double lineDisNumerator, p0DisNumerator;
	double lineDisDenominator;
	double xb, yb, xe, ye, xm, ym;
	double totalD, curD;

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
		xm = CORRECT_COOR(midp.x, vk1, p2R2, vk2, p2R4);
		ym = CORRECT_COOR(midp.y, vk1, p2R2, vk2, p2R4);

		// 计算原点与直线的关系
		p0DisNumerator = ((ye - yb) * p0.x -
				(xe - xb) * p0.y +
				((xe - xb) * yb - (ye - yb) * xb));

		// 计算直线距离的分母
		lineDisDenominator = sqrt((ye - yb) * (ye - yb) + (xe - xb) * (xe - xb));
		lineDisNumerator = ((ye - yb) * xm -
				(xe - xb) * ym +
				((xe - xb) * yb - (ye - yb) * xb));
		curD = lineDisNumerator / lineDisDenominator;

		if (fabs(curD) <= 1e-15)
		{
			break;
		}

		if (calcCnt >= 5000)
		{
			break;
		}

		// 桶形畸变
		if (distortLevel < 0)
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
		// 枕形畸变
		else
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
	}

	*k1 = vk1;
	*k2 = vk2;

	return PANORAMA_OK;
}

int calcK1(double *k1)
{
	int j;
	int imgW = 1280;
	int imgH = 720;
	double halfImgW = imgW / 2;
	double halfImgH = imgH / 2;

	// 原点
	Point p0;
	p0.x = 0;
	p0.y = 0;

	int vpcnt = 11;
	double vpx[100] = {41, 38, 35, 28, 26, 25, 22, 17, 14, 13, 12};
	double vpy[100] = {13, 34, 36, 115, 144, 155, 187, 259, 297, 322, 359};

	int hpcnt = 15;
	double hpx[100] = {
		30, 27, 25, 23, 20, 17, 17, 16, 16, 16, 17, 18, 22, 25, 30};
	double hpy[100] = {
		34, 70, 105, 142, 179, 216, 254, 292, 329, 367, 406, 444, 518, 592, 663};

	Point hp[100];
	Point vp[100];

	// 赋值
	int i;
	for (i = 0; i < hpcnt; i++)
	{
		hp[i].x = hpx[i] - halfImgW;
		hp[i].y = hpy[i] - halfImgH;
	}
	for (i = 0; i < vpcnt; i++)
	{
		vp[i].x = vpx[i] - halfImgW;
		vp[i].y = vpy[i] - halfImgH;
	}

	int calcCnt = 0;
	double vk1, vk2;
	double finalhk1, finalvk1, finalk1;
	double vk1Left, vk1Right;

	Point endp1, endp2;
	double p1R2, p2R2;
	double p1R4, p2R4;
	double lineDisNumerator, p0DisNumerator;
	double lineDisDenominator;
	double xb, yb, xe, ye, xm, ym;
	double totalD, curD;
	double lastDis = -1;
	double thresh;

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
	thresh = 10;
	vk1Left = 0;
	vk1Right = 10;
	vk2 = 0; // 暂不计算k2
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
			//xm = CORRECT_COOR(hp[i].x, vk1, p2R2, vk2, p2R4);
			//ym = CORRECT_COOR(hp[i].y, vk1, p2R2, vk2, p2R4);
			xm = hp[i].x;
			ym = hp[i].y;
			lineDisNumerator = ((ye - yb) * xm -
				(xe - xb) * ym +
				((xe - xb) * yb - (ye - yb) * xb));
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
		if (fabs(totalD) <= 1e-10)
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


#if 0
	// ================== 计算第二组坐标 =========================
	// 端点赋值
	endp1.x = vp[0].x;
	endp1.y = vp[0].y;
	endp2.x = vp[vpcnt-1].x;
	endp2.y = vp[vpcnt-1].y;


	// 计算距离原点距离平方
	p1R2 = pointDisPower2(&p0, &endp1);
	p2R2 = pointDisPower2(&p0, &endp2);
	p1R4 = p1R2 * p1R2;
	p2R4 = p2R2 * p2R2;

	calcCnt = 0;
	thresh = 10;
	vk1Left = 0;
	vk1Right = 10;
	while (vk1Left < vk1Right)
	{
		calcCnt++;
		totalD = 0.;

		vk1 = (vk1Left + vk1Right) / 2;

		// 校正后直线端点
		xb = endp1.x * (1 + vk1 * p1R2);
		yb = endp1.y * (1 + vk1 * p1R2);
		xe = endp2.x * (1 + vk1 * p2R2);
		ye = endp2.y * (1 + vk1 * p2R2);

		// 计算直线距离的分母
		lineDisDenominator = sqrt((ye - yb) * (ye - yb) + (xe - xb) * (xe - xb));
		for (i = 1; i < vpcnt - 1; i++)
		{
			lineDisNumerator = ((ye - yb) * vp[i].x -
				(xe - xb) * vp[i].y +
				((xe - xb) * yb - (ye - yb) * xb));
			curD = lineDisNumerator / lineDisDenominator;
			totalD += curD;
		}
		printf("interator%d, totalD=%10.15f, vk1=%10.20f\n", calcCnt, totalD, vk1);
	
		finalvk1 = vk1;
		if (fabs(totalD) <= 1e-10)
		{
			break;
		}
		else if (totalD > 0)
		{
			vk1Right = vk1;
		}
		else if (totalD < 0)
		{
			vk1Left = vk1;
		}

		if (calcCnt >= 5000)
		{
			break;
		}
	}

	finalk1 = (finalhk1 + finalvk1 ) / 2;
#endif

	*k1 = finalhk1;

	printf("final k1 = %10.20f\n", *k1);

	return PANORAMA_OK;
}

/* 矫正 */
int undistort(double k, double k2, Image *src, Image **dstImg)
{
	int ret;
	int i, j, subi, subj;
	int scol, srow;
	double halfW, halfH;
	int upx, upy, downx, downy;
	int dstW, dstH;
	int dstHalfW, dstHalfH;
	double dx, dy;
	double rbx, rby;
	double dispower2, dispower4;
	double projectionX, projectionY; // 以中心为原点之后的原始图像素坐标
	Point p0, tmp, lastPoint;
	int srcySize, srcuSize, srcvSize;
	unsigned char *srcHeadPtr;
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

	scol = src->w;
	srow = src->h;
	halfW = src->w / 2;
	halfH = src->h / 2;
	srcySize = scol * srow;
	srcuSize = srcvSize = (scol / 2) * (srow / 2);
	p0.x = 0;
	p0.y = 0;
	
	printf("k1=%10.20f, k2=%10.20f\n", k, k2);

	// right bottom
	tmp.x = halfW;
	tmp.y = halfH;
	dispower2 = pointDisPower2(&p0, &tmp);
	dispower4 = dispower2 * dispower2;
	rbx = CORRECT_COOR(tmp.x, k, dispower2, k2, dispower4);
	rby = CORRECT_COOR(tmp.y, k, dispower2, k2, dispower4);

	dstW = 2 * ceil(rbx);
	dstH = 2 * ceil(rby);
	dstHalfW = dstW / 2;
	dstHalfH = dstH / 2;

	/* 后续步骤需要用到该数据，由deinit释放 */
	ret = constructImage(dstImg, NULL, NULL, 0, dstW, dstH, src->imgFmt, BUF_TYPE_NOBUF);
	if (PANORAMA_OK != ret)
	{
		ret = PANORAMA_ERROR;
		goto clean;
	}

	Dbg("[%d, %d] -> [%d, %d]\n", scol, srow, dstW, dstH);

	
	ret = constructMat(&ySrc, scol, srow, 1, sizeof(unsigned char), IMAGE_Y_PTR(src));
	if (PANORAMA_OK != ret)
	{
		ret = PANORAMA_ERROR;
		goto clean;
	}

	ret = constructMat(&uSrc, halfW, halfH, 1, sizeof(unsigned char), IMAGE_U_PTR(src));
	if (PANORAMA_OK != ret)
	{
		ret = PANORAMA_ERROR;
		goto clean;
	}

	ret = constructMat(&vSrc, halfW, halfH, 1, sizeof(unsigned char), IMAGE_V_PTR(src));
	if (PANORAMA_OK != ret)
	{
		ret = PANORAMA_ERROR;
		goto clean;
	}

	ret = constructMat(&yDst, dstW, dstH, 1, sizeof(unsigned char), IMAGE_Y_PTR(*dstImg));
	if (PANORAMA_OK != ret)
	{
		ret = PANORAMA_ERROR;
		goto clean;
	}

	ret = constructMat(&uDst, dstHalfW, dstHalfH, 1, sizeof(unsigned char), IMAGE_U_PTR(*dstImg));
	if (PANORAMA_OK != ret)
	{
		ret = PANORAMA_ERROR;
		goto clean;
	}

	ret = constructMat(&vDst, dstHalfW, dstHalfH, 1, sizeof(unsigned char), IMAGE_V_PTR(*dstImg));
	if (PANORAMA_OK != ret)
	{
		ret = PANORAMA_ERROR;
		goto clean;
	}

	srcHeadPtr = src->data[0];
	printf("srcHeadPtr=%p\n", srcHeadPtr);
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
			dx = CORRECT_COOR(projectionX, k, dispower2, k2, 0);
			dy = CORRECT_COOR(projectionY, k, dispower2, k2, 0);

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
	destructMat(&vDst);
	destructMat(&uDst);
	destructMat(&yDst);
	destructMat(&vSrc);
	destructMat(&uSrc);
	destructMat(&ySrc);

	return ret;
}

float pointDisPower2(Point *p1, Point *p2 )
{
	float dis;
	dis = ((p1->x - p2->x) * (p1->x - p2->x) + (p1->y - p2->y) * (p1->y - p2->y));
	return dis;
}

float pointOverlap(KeyPoint *kp1, KeyPoint *kp2 )
{
	float a = kp1->size * 0.5f;
	float b = kp2->size * 0.5f;
	float a_2 = a * a;
	float b_2 = b * b;

	float c = POINT_DISTANCE(kp1->pt, kp2->pt);

	float ovrl = 0.f;

	// one circle is completely encovered by the other => no intersection points!
	if( MIN( a, b ) + c <= MAX( a, b ) )
	{
		return MIN( a_2, b_2 ) / MAX( a_2, b_2 );
	}

	if( c < a + b ) // circles intersect
	{
		float c_2 = c * c;
		float cosAlpha = ( b_2 + c_2 - a_2 ) / ( kp2->size * c );
		float cosBeta  = ( a_2 + c_2 - b_2 ) / ( kp1->size * c );
		float alpha = acos( cosAlpha );
		float beta = acos( cosBeta );
		float sinAlpha = sin(alpha);
		float sinBeta  = sin(beta);

		float segmentAreaA = a_2 * beta;
		float segmentAreaB = b_2 * alpha;

		float triangleAreaA = a_2 * sinBeta * cosBeta;
		float triangleAreaB = b_2 * sinAlpha * cosAlpha;

		float intersectionArea = segmentAreaA + segmentAreaB - triangleAreaA - triangleAreaB;
		float unionArea = (a_2 + b_2) * (float)PANORAMA_PI - intersectionArea;

		ovrl = intersectionArea / unionArea;
	}

	return ovrl;
}

int keypointAssignment(KeyPoint *kp, float x, float y, float size,
		float angle, float response, int octave, int classId)
{
	if (!kp)
	{
		return PANORAMA_ERROR;
	}

	kp->pt.x = x;
	kp->pt.y = y;
	kp->size = size;
	kp->angle = angle;
	kp->response = response;
	kp->octave = octave;
	kp->classId = classId;

	return PANORAMA_OK;
}

int keypointVectorPush(Vector *vPtr, float x, float y, float size,
		float angle, float response, int octave, int classId)
{
	int ret = PANORAMA_OK;

	if (!vPtr)
	{
		return PANORAMA_ERROR;
	}

	if (vPtr->size == vPtr->capacity)
	{
		ret = vectorResize(vPtr, vPtr->capacity * 2);
		if (ret != PANORAMA_OK)
		{
			return PANORAMA_ERROR;
		}
	}

	KeyPoint *kp = ((KeyPoint *)vPtr->elemArray) + vPtr->size;
	kp->pt.x = x;
	kp->pt.y = y;
	kp->size = size;
	kp->angle = angle;
	kp->response = response;
	kp->octave = octave;
	kp->classId = classId;

	vPtr->size++;

	return ret;
}
