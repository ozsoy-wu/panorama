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
	IMG_FMT_YUV420P_I420,			/* YUV420P I420��ʽyyyyyyyy uu vv */
	IMG_FMT_YUV420P_YV12,			/* YUV420P YV12��ʽyyyyyyyy vv uu */
	IMG_FMT_YUV420SP_NV12,		/* YUV420SP_NV12��ʽyyyyyyyy uvuv */
	IMG_FMT_YUV420SP_NV21,		/* YUV420SP_NV21��ʽyyyyyyyy vuvu */
} IMG_FORMAT;

typedef struct PANORAMA_CTX_S
{
	int id;
	void *innerCtx;
	/* ���� */
} PANORAMA_CTX;

typedef struct PANORAMA_CFG_S
{
	float yAngleOffset;	/* ��ͷy��ƫ��ˮƽ�߽Ƕȣ�����ƫ��ȡ��ֵ������ƫ��ȡ��ֵ */
	float viewingAngle;	/* ��ͷ�ӳ��� */
	float rotateAngle;	/* ��������ͼƬ֮���ת���Ƕ� */
	float focalLength;	/* ��ͷ���� */
	int overlapWidth;	/* ��ͷy��ƫ��Ϊ0������£���������ͼƬ���غϿ�ȣ����ص�λ */
	int stitchWidth;	/* ��ϴ����Բ�ֵ�Ŀ�ȣ����ص�λ */
	int imgTotalNum;	/* ԭʼͼƬ������ */
	int outImageWidth;			/* ���ȫ��ͼ�ֱ��ʿ�� */
	int outImageHeight;		/* ���ȫ��ͼ�ֱ��ʸ߶� */
	IMG_FORMAT outImageFmt;	/* ���ȫ��ͼͼƬ��ʽ */
	int logMask;	/* log���룬�����λ��ʼ��ÿһλ����LOG_DEBUG��LOG_INFO��LOG_WARN��LOG_ERROR��LOG_FATAL */
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
