#ifndef __PANORAMA_PANORAMA_H__
#define __PANORAMA_PANORAMA_H__

#define PANORAMA_OK					0
#define PANORAMA_ERROR					-1
#define PANORAMA_INVALID_IMG_FMT		-2
#define PANORAMA_IMG_FMT_NOT_MATCH	-3
#define PANORAMA_PROCESS_FINISH		100
#define PANORAMA_PROCESS_ERROR		PANORAMA_ERROR

typedef enum IMG_FORMAT_E
{
	IMG_FMT_YUV420P_I420,			/* YUV420P I420格式yyyyyyyy uu vv */
	IMG_FMT_YUV420P_YV12,			/* YUV420P YV12格式yyyyyyyy vv uu */
	IMG_FMT_YUV420SP_NV12,		/* YUV420SP_NV12格式yyyyyyyy uvuv */
	IMG_FMT_YUV420SP_NV21,		/* YUV420SP_NV21格式yyyyyyyy vuvu */
} IMG_FORMAT;

typedef struct PANORAMA_CTX_S
{
	int id;
	void *innerCtx;
	/* 待定 */
} PANORAMA_CTX;

typedef struct PANORAMA_CFG_S
{
	int commonImgTotalNum;	/* 原始图片总数量 */
	int commonLogMask;		/* log掩码，从最低位开始，每一位代表LOG_DEBUG、LOG_INFO、LOG_WARN、LOG_ERROR、LOG_FATAL */
	float camViewingAngle;			/* 镜头参数，视场角 */
	float camYOffset;				/* 镜头参数，镜头偏移水平线角度 */
	float camRotateAngle;			/* 镜头参数，每次转动角度 */
	float camFocalLength;			/* 镜头参数，焦距 */
	int stitchOverlapWidth;			/* 线性插值算法参数，相邻两张图片的重合宽度，像素单位 */
	int stitchInterpolationWidth;	/* 线性插值算法参数，缝合处线性插值的宽度，像素单位 */
	int srcImgWidth;				/* 原始图属性，宽度 */
	int srcImgHeight;				/* 原始图属性，高度 */
	IMG_FORMAT srcImageFmt;			/* 原始图属性，格式 */
	int panoImageWidth;				/* 全景图属性，宽度 */
	int panoImageHeight;			/* 全景图属性，高度 */
	IMG_FORMAT panoImageFmt;		/* 全景图属性，格式 */
} PANORAMA_CFG;

PANORAMA_CTX *PanoramaInit();
int PanoramaDeInit (PANORAMA_CTX *ctx);
int PanoramaGetCfg (PANORAMA_CTX *ctx, PANORAMA_CFG *cfg);
int PanoramaSetCfg (PANORAMA_CTX *ctx, PANORAMA_CFG *cfg);
int PanoramaLoadSrcImgFile (PANORAMA_CTX *ctx, char *filename,	int imgWidth, int imgHeight, IMG_FORMAT format);
int PanoramaLoadSrcImgBuffer (PANORAMA_CTX *ctx, char **buf,
	int *bufSize, int bufCnt, int imgWidth, int imgHeight, IMG_FORMAT format, int copy);
int PanoramaProcess (PANORAMA_CTX *ctx);
int PanoramaProcessQuery (PANORAMA_CTX *ctx);
int PanoramaFetch (PANORAMA_CTX *ctx, char **ptr, int *bufsize,	int *imgWidth, int *imgHeight, IMG_FORMAT *format);
int PanoramaResetCtx (PANORAMA_CTX *ctx);
int PanoramaSaveToFile (PANORAMA_CTX *ctx, char *filename,	int *imgWidth, int *imgHeight, IMG_FORMAT format);

#define PMD(fmt...) printf("%s, %d\n", __func__, __LINE__); 

#endif // __PANORAMA_PANORAMA_H__
