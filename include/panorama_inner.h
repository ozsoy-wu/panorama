#ifndef __PANORAMA_PANORAMA_INNER_H__
#define __PANORAMA_PANORAMA_INNER_H__

#include <sys/queue.h>

#include "log.h"
#include "utils.h"
#include "features2d.h"
#include "matrix.h"
#include "surf.h"


#define MAX_IMAGE_NUM 12

typedef struct FeaturesModule_S
{
	void *cfg;
	int (*detect)(void *cfg, Image *img, KeyPoint *kp);
	int (*compute)(void *cfg, Image *img, KeyPoint *kp, KeyPointDescriptor* kpdes);
	int (*detectAndCompute)(void *cfg, Image *img, KeyPoint *kp, KeyPointDescriptor* kpdes);
} FeaturesModule;

typedef struct PANORAMA_INNER_CTX_S
{
	int imgNum;

	FeaturesModule featureMod;

	Image images[MAX_IMAGE_NUM];

	TAILQ_HEAD(, Image_S) imageQueue;
} PANORAMA_INNER_CTX;


/* 获取内部contex指针 */
#define GET_INNER_CTX(ctx) ((PANORAMA_INNER_CTX *)((ctx)->innerCtx))

#endif // __PANORAMA_PANORAMA_INNER_H__
