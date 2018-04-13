#ifndef __PANORAMA_PANORAMA_INNER_H__
#define __PANORAMA_PANORAMA_INNER_H__

#include <sys/queue.h>

#include "log.h"
#include "utils.h"
#include "vector.h"
#include "features2d.h"
#include "matrix.h"
#include "surf.h"
#include "features_match.h"


#define MAX_IMAGE_NUM 12

typedef struct FeaturesFinder_S
{
	void *cfg;
	int (*detect)(void *cfg, Image *img, Vector *kp);
	int (*compute)(void *cfg, Image *img, Vector *kp, Mat **kpdes);
	int (*detectAndCompute)(void *cfg, Image *img, Vector *kp, Mat **kpdes);
} FeaturesFinder;

typedef struct FeaturesMatcher_S
{
	int (*match)();
} FeaturesMatcher;


typedef struct PANORAMA_INNER_CTX_S
{
	int imgNum;

	Vector *kpVecPtr[MAX_IMAGE_NUM];
	Mat *kpdesVecPtr[MAX_IMAGE_NUM];

	FeaturesFinder featureFinder;

	FeaturesMatcher matcher;

	float yAngleOffset;	/* 镜头y轴偏移水平线角度，向上偏移取正值，向下偏移取负值 */
	float viewingAngle;	/* 镜头视场角 */
	float rotateAngle;	/* 相邻两张图片之间的转动角度 */
	float focalLength;	/* 镜头焦距 */
	int overlapWidth;	/* 镜头y轴偏移为0的情况下，相邻两张图片的重合宽度，像素单位 */

	Image images[MAX_IMAGE_NUM];

	TAILQ_HEAD(, Image_S) imageQueue;
} PANORAMA_INNER_CTX;


/* 获取内部contex指针 */
#define GET_INNER_CTX(ctx) ((PANORAMA_INNER_CTX *)((ctx)->innerCtx))

#endif // __PANORAMA_PANORAMA_INNER_H__
