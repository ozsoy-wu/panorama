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

typedef enum INNER_STATUS_E {
	INIT = 0,
	PROCESS,
} INNER_STATUS;

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
	int imgNum;	/* 当前已装载的图片数量 */
	int imgToBeHandle; /* 待处理的图片编号，以0开始 */
	INNER_STATUS status;

	PANORAMA_CFG cfg;
	Image images[MAX_IMAGE_NUM];	/* 原始图片数据 */
	Image pano;	/* 全景图 */

	Vector *kpVecPtr[MAX_IMAGE_NUM];	/* 每张图片的特征点 */
	Mat *kpdesVecPtr[MAX_IMAGE_NUM];	/* 每张图片的特征点描述向量 */

	FeaturesFinder featureFinder;
	FeaturesMatcher matcher;
} PANORAMA_INNER_CTX;

/* 获取内部contex指针 */
#define GET_INNER_CTX(ctx) ((PANORAMA_INNER_CTX *)((ctx)->innerCtx))

#endif // __PANORAMA_PANORAMA_INNER_H__
