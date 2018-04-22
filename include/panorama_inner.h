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
	STATUS_PREPARE = 0,
	STATUS_INIT,
	STATUS_NEW_IMAGE,
	STATUS_UNDISTORT_IMAGE,
#ifdef FEATURE_BASE
	STATUS_FEATURE_DETECT,
	STATUS_FEATURE_COMPUTE,
	STATUS_FEATURE_MATCH,
#endif
	STATUS_STITCH,
	STATUS_LAST
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
	int imgNum;	/* ��ǰ��װ�ص�ͼƬ���� */
	int imgToBeHandle; /* �������ͼƬ��ţ���0��ʼ */
	INNER_STATUS status;
	int totalProcessPercent;	/* �ܵĴ������ */

	PANORAMA_CFG cfg;
	Image images[MAX_IMAGE_NUM];	/* ԭʼͼƬ���� */
	Image pano;	/* ȫ��ͼ */

	Vector *kpVecPtr[MAX_IMAGE_NUM];	/* ÿ��ͼƬ�������� */
	Mat *kpdesVecPtr[MAX_IMAGE_NUM];	/* ÿ��ͼƬ���������������� */

	FeaturesFinder featureFinder;
	FeaturesMatcher matcher;
} PANORAMA_INNER_CTX;

/* ��ȡ�ڲ�contexָ�� */
#define GET_INNER_CTX(ctx) ((PANORAMA_INNER_CTX *)((ctx)->innerCtx))

#define GET_SINGLE_PERCENT(status) ((int)(100 * ((status) + 1) / STATUS_LAST))

#endif // __PANORAMA_PANORAMA_INNER_H__
