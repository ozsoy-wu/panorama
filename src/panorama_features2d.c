/******************************************************************************
 * Copyright (c) 2015-2018 TP-Link Technologies CO.,LTD.
 *
 * 文件名称:		panorama_features2d.c
 * 版           本:	1.0
 * 摘           要:	像素点相关操作
 * 作           者:	wupimin<wupimin@tp-link.com.cn>
 * 创建时间:		2018-04-28
 ******************************************************************************/

#include <math.h>

#include "panorama.h"
#include "panorama_features2d.h"
#include "panorama_utils.h"
#include "panorama_log.h"

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
