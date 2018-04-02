#ifndef __PANORAMA_FEATURES_H__
#define __PANORAMA_FEATURES_H__

#include <math.h>
#include "utils.h"
#include "matrix.h"

#define MAX_KEYPOINTS_NUM 1000
#define MAX_DESCRIPTOR_NUM 1000


typedef struct Point_S
{
	float x;
	float y;
} Point;

typedef struct KeyPoint_S
{
	Point pt;
	float size;
	float angle;
	float response;
	int octave;
	int classId;
} KeyPoint;

typedef Mat KeyPointDescriptor;

#define POINT_DISTANCE_P(p1, p2) \
	({ \
		float xd = p1->x - p2->x;\
		float yd = p1->y - p2->y;\
		float res = (float)sqrt(xd * xd + yd * yd);\
		res; \
	})

#define POINT_DISTANCE(p1, p2) \
	({ \
		float xd = p1.x - p2.x;\
		float yd = p1.y - p2.y;\
		float res = (float)sqrt(xd * xd + yd * yd);\
		res; \
	})

#define KeyPointAssignment(kp, x, y, size, angle, response, octave, classId) do {\
	kp->x = (x);\
	kp->y = (y);\
	kp->size = (size);\
	kp->angle = (angle);\
	kp->response = (response);\
	kp->octave = (octave);\
	kp->classId = (classId);\
} while(0)

int keypointAssignment(KeyPoint *kp, float x, float y, float size,
		float angle, float response, int octave, int classId);
int keypointVectorPush(Vector *vPtr, float x, float y, float size,
		float angle, float response, int octave, int classId);
#endif // __PANORAMA_FEATURES_H__
