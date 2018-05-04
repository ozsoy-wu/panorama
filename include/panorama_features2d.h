#ifndef __PANORAMA_FEATURES2D_H__
#define __PANORAMA_FEATURES2D_H__

#include <math.h>
#include "panorama_utils.h"
#include "panorama_image.h"
#include "panorama_matrix.h"
#include "panorama_vector.h"

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

#define CORRECT_COOR(v, k1, r2, k2, r4) ((v)*(1+(k1)*(r2)+(k2)*(r4)))

#define SAME_SIDE_WITH_P0(p0dis, pdis) (((p0dis) < 0 && (pdis) < 0) || ((p0dis) > 0 && (pdis) > 0))

float pointOverlap(KeyPoint *kp1, KeyPoint *kp2 );
int keypointAssignment(KeyPoint *kp, float x, float y, float size,
		float angle, float response, int octave, int classId);
int keypointVectorPush(Vector *vPtr, float x, float y, float size,
		float angle, float response, int octave, int classId);


float pointDisPower2(Point *p1, Point *p2 );

#endif // __PANORAMA_FEATURES2D_H__
