#include <math.h>

#include "panorama.h"
#include "features2d.h"
#include "utils.h"

#if 0
float calcK1()
{
	// Ô­µã
	Point p0;
	p0.x = 640;
	p0.y = 360;

	int hpcnt = 13;
	float hpx[100] = {
		43, 65, 87, 113, 145, 177, 205, 270, 319, 361, 404, 462, 499};
	float hpy[100] = {
		10, 11, 12, 13, 15, 17, 20, 26, 30, 34, 38, 44, 48};

	int vpcnt = 11;
	float vpx[100] = {
		41, 38, 35, 28, 26, 25, 22, 17, 14, 13, 12};
	float vpy[100] = {
		13, 34, 36, 115, 144, 155, 187, 259, 297, 322, 359};

	Point[100] hp;
	Point[100] vp;

	// ¸³Öµ
	int i;
	for (i = 0; i < hpcnt; i++)
	{
		hp.x = hpx[i];
		hp.y = hpy[i];
	}
	for (i = 0; i < vpcnt; i++)
	{
		vp.x = vpx[i];
		vp.y = vpy[i];
	}

	float vk1, vk2;
	
}


int undistort(float k, float k2, Image *src, Image *dst)
{
	int i, j;
	int scol, srow;
	int upx, upy, downx, downy;
	int dstMaxX, dstMaxY;
	float dx, dy;
	float rbx, rby;
	float dispower2;
	Point p0, tmp, lastPoint;
	unsigned char *srcHeadPtr;
	unsigned char *srcPtr;

	scol = src->w;
	srow = src->h;
	p0.x = src->w / 2;
	p0.y = src->h / 2;

	// right bottom
	tmp.x = src->w;
	tmp.y = src->h;
	dispower2 = pointDisPower2(&p0, &tmp);
	rbx = tmp.x + k * dispower2;
	rby = tmp.y + k * dispower2;

	dstMaxX = ceil(rbx);
	dstMaxY = ceil(rby);

	unsigned char *ptr0;
	unsigned char *ptr1;
	unsigned char *ptr2;
	unsigned char *ptr3;
	Mat *dst = NULL;
	constructMat(&dst, dstMaxX, dstMaxY, 1, sizeof(unsigned char), NULL);

	srcHeadPtr = src->data[0];

	lastPoint.x = p0.x;
	lastPoint.y = p0.y;
	for (j = p0.y; j >= 0; j--)
	{
		for (i = p0.x; i <= scol; i++)
		{
			srcPtr = srcHeadPtr + src->w * j + i;
		
			tmp.x = i;
			tmp.y = j;
			dispower2 = pointDisPower2(&p0, &tmp);
			dx = i + k * dispower2;
			dy = j + k * dispower2;

			upx = ceil(dx);
			upy = ceil(dy);
			downx = floor(dx);
			downy = floor(dy);

			ptr0 = (unsigned char *)MAT_AT_COOR(dst, downx, downy);
			ptr1 = (unsigned char *)MAT_AT_COOR(dst, upx, downy);
			ptr2 = (unsigned char *)MAT_AT_COOR(dst, downx, upy);
			ptr3 = (unsigned char *)MAT_AT_COOR(dst, upx, upy);

			*ptr0 += (((float)upx - dx) * (*srcPtr));
			*ptr1 += ((dx - (float)downx) * (*srcPtr));
			*ptr2 += (((float)upx - dx) * (*srcPtr));
			*ptr3 += (((float)upx - dx) * (*srcPtr));
			

			lastPoint.x = dx;
			lastPoint.y = dy;
		}
	}
}
#endif

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
