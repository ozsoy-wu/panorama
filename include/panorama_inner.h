#ifndef __PANORAMA_PANORAMA_INNER_H__
#define __PANORAMA_PANORAMA_INNER_H__

#include <sys/queue.h>

#include "panorama_log.h"
#include "panorama_utils.h"
#include "panorama_vector.h"
#include "panorama_features2d.h"
#include "panorama_matrix.h"
#include "panorama_surf.h"
#include "panorama_features_match.h"

#define MAX_IMAGE_NUM 12

typedef enum INNER_STATUS_E {
	STATUS_PREPARE = 0,
	STATUS_INIT,
	STATUS_NEW_IMAGE,
#ifdef UNDISTORT_SUPPORT
	STATUS_UNDISTORT_IMAGE,
#endif
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
	int imgNum;	/* 当前已装载的图片数量 */
	int imgToBeHandle; /* 待处理的图片编号，以0开始 */
	INNER_STATUS status;
	int totalProcessPercent;	/* 总的处理进度 */

	/* 以下参数由cfg的配置计算得到 */
	int undistortImgW;				/* 矫正图中心水平线宽度 */
	int undistortImgH;				/* 矫正图中心竖直线宽度 */
	int stitchOverlapWidth;			/* 线性插值算法参数，相邻两张图片的重合宽度，像素单位 */
	int stitchInterpolationWidth;	/* 线性插值算法参数，对重合区域的n%进行插值，n即为本参数 */

	PANORAMA_CFG cfg;
	Image *images[MAX_IMAGE_NUM];	/* 图片数据 */
	Image pano;	/* 全景图 */

	Vector *kpVecPtr[MAX_IMAGE_NUM];	/* 每张图片的特征点 */
	Mat *kpdesVecPtr[MAX_IMAGE_NUM];	/* 每张图片的特征点描述向量 */

	FeaturesFinder featureFinder;
	FeaturesMatcher matcher;
} PANORAMA_INNER_CTX;

/* 获取内部contex指针 */
#define GET_INNER_CTX(ctx) ((PANORAMA_INNER_CTX *)((ctx)->innerCtx))

#define GET_SINGLE_PERCENT(status) ((int)(100 * ((status) + 1) / STATUS_LAST))

#endif // __PANORAMA_PANORAMA_INNER_H__
