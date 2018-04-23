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

	// ԭ��
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
	double totalD, curD;

	// �˵㸳ֵ
	endp1.x = -halfW;
	endp2.x = halfW;
	endp1.y = endp2.y = halfH * (1 + distortLevel);
	midp.x = 0;
	midp.y = halfH;

	// �������ԭ�����ƽ��
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

		// У����ֱ�߶˵�
		xb = CORRECT_COOR(endp1.x, vk1, p1R2, vk2, p1R4);
		yb = CORRECT_COOR(endp1.y, vk1, p1R2, vk2, p1R4);
		xe = CORRECT_COOR(endp2.x, vk1, p2R2, vk2, p2R4);
		ye = CORRECT_COOR(endp2.y, vk1, p2R2, vk2, p2R4);
		xm = CORRECT_COOR(midp.x, vk1, mpR2, vk2, mpR4);
		ym = CORRECT_COOR(midp.y, vk1, mpR2, vk2, mpR4);

		// ����ԭ����ֱ�ߵĹ�ϵ
		p0DisNumerator = ((ye - yb) * p0.x -
				(xe - xb) * p0.y +
				((xe - xb) * yb - (ye - yb) * xb));

		// ����ֱ�߾���ķ�ĸ
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

		// Ͱ�λ���
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
		// ���λ���
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

/* ���� */
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
	double projectionX, projectionY; // ������Ϊԭ��֮���ԭʼͼ��������
	Point p0, p1, tmp, lastPoint;
	double p1R2, p1R4;
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

	/* ����ԭʼͼ�������ķֱ��� */
	p0.x = 0;
	p0.y = 0;
	p1.x = halfW;
	p1.y = halfH;
	p1R2 = pointDisPower2(&p0, &p1);
	p1R4 = p1R2 * p1R2;

	/* �������ͼ������ˮƽ�߿�� */
	dstW = 2 * ceil(CORRECT_COOR(p1.x, k, p1R2, k2, p1R4));
	dstH = 2 * ceil(CORRECT_COOR(p1.y, k, p1R2, k2, p1R4));
	dstHalfW = dstW / 2;
	dstHalfH = dstH / 2;

	/* ����������Ҫ�õ������ݣ���deinit�ͷ� */
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
