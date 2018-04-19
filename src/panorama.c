#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "panorama_inner.h"
#include "panorama.h"
#include "stitch.h"

static int ctxCnt = 0;
int gLogMask;

int srcImgWidth;				/* 原始图属性，宽度 */
int srcImgHeight;				/* 原始图属性，高度 */
IMG_FORMAT srcImageFmt; 		/* 原始图属性，格式 */

static int _PanoramaCfgInit(PANORAMA_CFG *cfg)
{
	if (!cfg)
	{
		return PANORAMA_ERROR;
	}

	cfg->commonImgTotalNum = -1;
	cfg->commonLogMask = LOG_DEBUG | LOG_INFO | LOG_WARN | LOG_ERROR | LOG_FATAL;
	cfg->camYOffset = 0;
	cfg->camViewingAngle = 0;
	cfg->camRotateAngle = 0;
	cfg->camFocalLength = 0;
	cfg->stitchOverlapWidth = -1;
	cfg->stitchInterpolationWidth = DEFAULT_STITCH_WIDTH;
	cfg->srcImgWidth = -1;
	cfg->srcImgHeight = -1;
	cfg->srcImageFmt = IMG_FMT_YUV420P_I420;
	cfg->panoImageWidth = -1;
	cfg->panoImageHeight = -1;
	cfg->panoImageFmt = IMG_FMT_YUV420P_I420;

	gLogMask = cfg->commonLogMask;

	return PANORAMA_OK;
}

static int _PanoramaCfgCheck(PANORAMA_CFG *cfg)
{
	float overlap = 0.;

	if (!cfg)
	{
		return PANORAMA_ERROR;
	}

	/* 原始图像数量必须大于0 */
	if (cfg->commonImgTotalNum <= 0)
	{
		Log(LOG_ERROR, "Invalid image numbers %d\n", cfg->commonImgTotalNum);
		return PANORAMA_ERROR;
	}

	/* 如果cfg里提供的重合区域宽度参数非法，那么将从镜头参数及原始图宽度计算重合区域宽度 */
	if (cfg->stitchOverlapWidth < 0)
	{
		if (FLOAT_EQUAL(cfg->camViewingAngle, 0) ||
			FLOAT_EQUAL(cfg->camRotateAngle, 0) ||
			cfg->srcImgWidth <= 0)
		{
			Log(LOG_ERROR, "Invalid camera arguments\n");
			return PANORAMA_ERROR;
		}

		overlap = (cfg->srcImgWidth * (cfg->camViewingAngle - cfg->camRotateAngle)) / cfg->camViewingAngle;
		cfg->stitchOverlapWidth = (int)overlap;

		Dbg("va(%f), ra(%f), w(%d), overlap(%f)\n",
			cfg->camViewingAngle,
			cfg->camRotateAngle,
			cfg->srcImgWidth,
			overlap);

		Log(LOG_DEBUG, "Calculate overlap width = %d\n", cfg->stitchOverlapWidth);
	}

	/* 线性插值区域宽度须小于重合区域宽度 */
	if (cfg->stitchInterpolationWidth > cfg->stitchOverlapWidth)
	{
		Log(LOG_ERROR, "Interpolation width(%d) too large, should smaller than overlap width(%d)\n",
			cfg->stitchInterpolationWidth, cfg->stitchOverlapWidth);
		return PANORAMA_ERROR;
	}

	return PANORAMA_OK;
}

PANORAMA_CTX * PanoramaInit()
{
	PANORAMA_CTX *ctx = NULL;
	PANORAMA_INNER_CTX *innerCtx = NULL;
	SURF_CFG *surfCfg = NULL;

	gLogMask = LOG_DEBUG | LOG_INFO | LOG_WARN | LOG_ERROR | LOG_FATAL;

	ctx = tMalloc(PANORAMA_CTX);
	if (!ctx)
	{
		goto err;
	}
	memset((void *)ctx, 0, sizeof(PANORAMA_CTX));

	ctx->id = ++ctxCnt;

	innerCtx = tMalloc(PANORAMA_INNER_CTX);
	if (!innerCtx)
	{
		goto err;
	}
	memset((void *)innerCtx, 0, sizeof(PANORAMA_INNER_CTX));

	innerCtx->imgNum = 0;
	innerCtx->imgToBeHandle = 0;
	innerCtx->status = STATUS_PREPARE;
	innerCtx->totalProcessPercent = 0;

	_PanoramaCfgInit(&innerCtx->cfg);

	surfCfg = tMalloc(SURF_CFG);
	if (!surfCfg)
	{
		goto err;
	}
	memset((void *)surfCfg, 0, sizeof(SURF_CFG));

	surfCfg->hessianThreshold = 300;
	surfCfg->nOctaves = 3;
	surfCfg->nOctaveLayers = 4;
	surfCfg->extended = 0;
	surfCfg->upright = 0;

	innerCtx->featureFinder.cfg = (void *)surfCfg;
	innerCtx->featureFinder.detect = surfFeatureDetect;
	innerCtx->featureFinder.compute= surfFeatureCompute;
	innerCtx->featureFinder.detectAndCompute = surfFeatureDetectAndCompute;

	ctx->innerCtx = (void *)innerCtx;

	Log(LOG_DEBUG, "init ok\n");

	return ctx;

err:

	FREE(surfCfg);
	FREE(innerCtx);
	FREE(ctx);

	return NULL;
}

int PanoramaGetCfg (PANORAMA_CTX *ctx, PANORAMA_CFG *cfg)
{
	if (!ctx || !cfg)
	{
		return PANORAMA_ERROR;
	}
	PANORAMA_INNER_CTX *inCtx = NULL;
	inCtx = GET_INNER_CTX(ctx);

	memcpy(cfg, &inCtx->cfg, sizeof(PANORAMA_CFG));

	return PANORAMA_OK;
}

int PanoramaSetCfg (PANORAMA_CTX *ctx, PANORAMA_CFG *cfg)
{
	if (!ctx || !cfg)
	{
		return PANORAMA_ERROR;
	}

	PANORAMA_INNER_CTX *inCtx = NULL;
	inCtx = GET_INNER_CTX(ctx);

	if (PANORAMA_OK != _PanoramaCfgCheck(cfg))
	{
		Log(LOG_ERROR, "Cfg check failed\n");
		return PANORAMA_ERROR;
	}

	memcpy(&inCtx->cfg, cfg, sizeof(PANORAMA_CFG));
	gLogMask = cfg->commonLogMask;

	return PANORAMA_OK;
}

int PanoramaLoadSrcImgFile (PANORAMA_CTX *ctx, char *filename, int imgWidth, int imgHeight, IMG_FORMAT format)
{
	int ret = PANORAMA_ERROR;
	int imgTotalSize = 0;
	int nread = 0;
	int idx;
	unsigned char *imgBuf = NULL;
	FILE *fp;
	Image *curImage = NULL;
	PANORAMA_INNER_CTX *inCtx = NULL;

	if (!ctx || !filename)
	{
		return PANORAMA_ERROR;
	}

	inCtx = GET_INNER_CTX(ctx);

	idx = inCtx->imgNum;
	curImage = &(inCtx->images[idx]);

	if (IMG_FMT_YUV420P_I420 == format)
	{
		imgTotalSize = (imgWidth * imgHeight * 3)>>1;

		/* 该buf应由Image释放 */
		imgBuf = lMalloc(unsigned char, sizeof(unsigned char) * imgTotalSize);
		if (!imgBuf)
		{
			goto error;
		}

		fp = fopen(filename, "r");
		if (!fp)
		{
			goto error;
		}

		nread = fread(imgBuf, 1, imgTotalSize, fp);
		if (nread < imgTotalSize)
		{
			goto error;
		}

		ret = constructImage(&curImage, (unsigned char *)&imgBuf, &imgTotalSize, 1, imgWidth,
			imgHeight, format, BUF_TYPE_NOCOPY_DELETE);
		if (PANORAMA_OK != ret)
		{
			goto error;
		}

		inCtx->imgNum++;
	}
	else
	{
		// TODO
	}

	return PANORAMA_OK;
error:
	if (imgBuf)
	{
		FREE(imgBuf);
	}

	return ret;
}

int PanoramaLoadSrcImgBuffer (PANORAMA_CTX *ctx, char **buf,
	int *bufSize, int bufCnt, int imgWidth, int imgHeight, IMG_FORMAT format, int copy)
{
	if (!ctx || bufCnt <= 0 || !bufSize || !buf)
	{
		return PANORAMA_ERROR;
	}

	int ret;
	int idx;
	Image *curImage = NULL;
	PANORAMA_INNER_CTX *inCtx = NULL;

	inCtx = GET_INNER_CTX(ctx);

	idx = inCtx->imgNum;
	curImage = &(inCtx->images[idx]);

	if (copy)
	{
		ret = constructImage(&curImage, buf, bufSize, bufCnt, imgWidth,
			imgHeight, format, BUF_TYPE_COPY_NODELETE);
	}
	else
	{
		ret = constructImage(&curImage, buf, bufSize, bufCnt, imgWidth,
			imgHeight, format, BUF_TYPE_NOCOPY_NODELETE);
	}

	if (PANORAMA_OK != ret)
	{
		return PANORAMA_ERROR;
	}

	inCtx->imgNum++;
	return PANORAMA_OK;
}

int PanoramaProcess (PANORAMA_CTX *ctx)
{
	int ret;
	int panoW, panoH;
	int percent;
	int currentStep = 0;
	int totalStep = 0;
	Image *img = NULL;
	INNER_STATUS status;

	if (!ctx)
	{
		Log(LOG_ERROR, "ctx NULL\n");
		return PANORAMA_PROCESS_ERROR;
	}

	PANORAMA_INNER_CTX *inCtx = GET_INNER_CTX(ctx);

	status = inCtx->status;

	/* 获取当前图片处理进度 */
	percent = GET_SINGLE_PERCENT(status);

	/* 获取总进度 */
	if (inCtx->imgToBeHandle > 0)
	{
		currentStep = (inCtx->imgToBeHandle) * STATUS_LAST;
	}
	currentStep += status;
	totalStep = inCtx->cfg.commonImgTotalNum * STATUS_LAST;
	inCtx->totalProcessPercent = 100 * currentStep / totalStep;

	switch (status)
	{
		case STATUS_PREPARE:
			if (PANORAMA_OK != _PanoramaCfgCheck(&inCtx->cfg))
			{
				Log(LOG_ERROR, "Cfg check failed\n");
				ret = PANORAMA_PROCESS_ERROR;
				goto out;
			}
			inCtx->status = STATUS_INIT;
			break;
		case STATUS_INIT:
			panoW = inCtx->images[0].w * inCtx->cfg.commonImgTotalNum;
			panoW -= inCtx->cfg.stitchOverlapWidth * (inCtx->cfg.commonImgTotalNum - 1);
			panoH = inCtx->images[0].h;
			img = &inCtx->pano;
			ret = constructImage(&img, NULL, NULL, 0, panoW,
				panoH, inCtx->images[0].imgFmt, BUF_TYPE_NOBUF);
			if (PANORAMA_OK != ret)
			{
				Log(LOG_ERROR, "Construct panorama image failed\n");
				ret = PANORAMA_PROCESS_ERROR;
				goto out;
			}
			
			inCtx->status = STATUS_NEW_IMAGE;
			break;
		case STATUS_NEW_IMAGE:
			if (inCtx->imgToBeHandle >= inCtx->imgNum &&
				inCtx->imgNum >= inCtx->cfg.commonImgTotalNum)
			{
				Log(LOG_INFO, "All %d images have been processed\n");
				ret = PANORAMA_PROCESS_FINISH;
				goto out;
			}

			if (inCtx->imgToBeHandle >= inCtx->imgNum)
			{
				Log(LOG_INFO, "Need more image\n");
				ret = PANORAMA_PROCESS_FINISH;
				goto out;
			}
#ifdef FEATURE_BASE
			inCtx->status = STATUS_FEATURE_DETECT;
			break;
		case STATUS_FEATURE_DETECT:
 			ret = constructVector(&(inCtx->kpVecPtr[inCtx->imgToBeHandle]), sizeof(KeyPoint), -1);
			if (PANORAMA_OK != ret)
			{
				Log(LOG_ERROR, "image#%d: construct keypoint vector failed\n", inCtx->imgToBeHandle);
				ret = PANORAMA_PROCESS_ERROR;
				goto out;
			}

			ret = inCtx->featureFinder.detect(
				(SURF_CFG *)inCtx->featureFinder.cfg,
				&inCtx->images[inCtx->imgToBeHandle],
				inCtx->kpVecPtr[inCtx->imgToBeHandle]);
			if (PANORAMA_OK != ret)
			{
				Log(LOG_ERROR, "image#%d: feature detect failed\n",
					inCtx->imgToBeHandle);
				ret = PANORAMA_PROCESS_ERROR;
				goto out;
			}
			inCtx->status = STATUS_FEATURE_COMPUTE;
			break;
		case STATUS_FEATURE_COMPUTE:
			ret = inCtx->featureFinder.compute(
				(SURF_CFG *)inCtx->featureFinder.cfg,
				&inCtx->images[inCtx->imgToBeHandle],
				inCtx->kpVecPtr[inCtx->imgToBeHandle],
				&inCtx->kpdesVecPtr[inCtx->imgToBeHandle]);
			if (PANORAMA_OK != ret)
			{
				Log(LOG_ERROR, "image#%d: feature compute failed\n",
					inCtx->imgToBeHandle);
				ret = PANORAMA_PROCESS_ERROR;
				goto out;
			}
			inCtx->status = STATUS_FEATURE_MATCH;
			break;
		case STATUS_FEATURE_MATCH:
			ret = knnMatcher();
			if (PANORAMA_OK != ret)
			{
				Log(LOG_ERROR, "feature match failed\n");
				ret = PANORAMA_PROCESS_ERROR;
				goto out;
			}
			inCtx->status = STATUS_STITCH;
			break;
#else
			inCtx->status = STATUS_STITCH;
			break;
#endif
		case STATUS_STITCH:
			ret = stitch(inCtx, inCtx->imgToBeHandle);
			if (PANORAMA_OK != ret)
			{
				Log(LOG_ERROR, "image#%d stitch failed\n", inCtx->imgToBeHandle);
				ret = PANORAMA_PROCESS_ERROR;
				goto out;
			}

			/* 更新待处理图片下标 */
			inCtx->imgToBeHandle++;
			percent = PANORAMA_PROCESS_FINISH;

			if (inCtx->imgToBeHandle >= inCtx->cfg.commonImgTotalNum)
			{
				inCtx->totalProcessPercent = PANORAMA_PROCESS_FINISH;
				inCtx->status = STATUS_LAST;
			}
			else
			{
				inCtx->status = STATUS_NEW_IMAGE;
			}

			break;
		case STATUS_LAST:
			Log(LOG_INFO, "All %d images have been processed\n", inCtx->cfg.commonImgTotalNum);
			inCtx->totalProcessPercent = PANORAMA_PROCESS_FINISH;
			percent = PANORAMA_PROCESS_FINISH;
			break;
	}

	ret = percent;
out:

	return ret;
}

int PanoramaProcessQuery (PANORAMA_CTX *ctx)
{
	if (!ctx)
	{
		return PANORAMA_ERROR;
	}

	PANORAMA_INNER_CTX *inCtx = NULL;
	inCtx = GET_INNER_CTX(ctx);

	return inCtx->totalProcessPercent;
}

int PanoramaFetch (PANORAMA_CTX *ctx, char **ptr, int *bufsize, int *imgWidth, int *imgHeight, IMG_FORMAT *format)
{
	if (!ctx)
	{
		return PANORAMA_ERROR;
	}

	PANORAMA_INNER_CTX *inCtx = NULL;
	inCtx = GET_INNER_CTX(ctx);

	if (STATUS_PREPARE == inCtx->status ||
		STATUS_INIT == inCtx->status)
	{
		Log(LOG_ERROR, "Panorama not available\n");
		return PANORAMA_ERROR;
	}

	*ptr = (char *)inCtx->pano.data[0];

	if (bufsize)
	{
		*bufsize = inCtx->pano.dataSize[0];
	}

	if (imgWidth)
	{
		*imgWidth = inCtx->pano.w;
	}

	if (imgHeight)
	{
		*imgHeight = inCtx->pano.h;
	}

	if (format)
	{
		*format = inCtx->pano.imgFmt;
	}

	return PANORAMA_OK;
}

int PanoramaSaveToFile (PANORAMA_CTX *ctx, char *filename, int *imgWidth, int *imgHeight, IMG_FORMAT format)
{
	if (!ctx || !filename)
	{
		return PANORAMA_ERROR;
	}

	PANORAMA_INNER_CTX *inCtx = NULL;
	inCtx = GET_INNER_CTX(ctx);

	if (STATUS_PREPARE == inCtx->status ||
		STATUS_INIT == inCtx->status)
	{
		Log(LOG_ERROR, "Panorama not available\n");
		return PANORAMA_ERROR;
	}

	if (format != inCtx->pano.imgFmt)
	{
		Log(LOG_ERROR, "Not yet support image format translation\n");
		return PANORAMA_ERROR;
	}

	FILE *fp = NULL;
	fp = fopen(filename, "w+");
	if (!fp)
	{
		Log(LOG_ERROR, "open file %s failed\n", filename);
		return PANORAMA_ERROR;
	}

	fwrite(inCtx->pano.data[0], inCtx->pano.dataSize[0], 1, fp);

	if (imgWidth)
	{
		*imgWidth = inCtx->pano.w;
	}

	if (imgHeight)
	{
		*imgHeight = inCtx->pano.h;
	}

	fclose(fp);
	fp = NULL;

	return PANORAMA_OK;
}

int PanoramaResetCtx (PANORAMA_CTX *ctx)
{
	if (!ctx)
	{
		return PANORAMA_ERROR;
	}

	int i;
	PANORAMA_INNER_CTX *innerCtx = NULL;
	Image *imgPtr = NULL;

	innerCtx = GET_INNER_CTX(ctx);
	if (!innerCtx)
	{
		return PANORAMA_ERROR;
	}

	innerCtx->imgNum = 0;
	innerCtx->imgToBeHandle = 0;
	innerCtx->status = STATUS_PREPARE;
	innerCtx->totalProcessPercent = 0;
	
	_PanoramaCfgInit(&innerCtx->cfg);

	for (i = 0; i < MAX_IMAGE_NUM; i++)
	{
		imgPtr = &innerCtx->images[i];
		destructImage(&imgPtr);
		destructVector(&innerCtx->kpVecPtr[i]);
		destructMat(&innerCtx->kpdesVecPtr[i]);
	}

	imgPtr = &innerCtx->pano;
	destructImage(&imgPtr);

	return PANORAMA_OK;
}

int PanoramaDeInit (PANORAMA_CTX *ctx)
{
	if (!ctx)
	{
		return PANORAMA_ERROR;
	}

	PANORAMA_INNER_CTX *inCtx = NULL;
	SURF_CFG *surfCfg = NULL;

	inCtx = GET_INNER_CTX(ctx);

	if (inCtx)
	{
		surfCfg = (SURF_CFG *)(inCtx->featureFinder.cfg);
	}

	PanoramaResetCtx(ctx);

	FREE(surfCfg);
	FREE(inCtx);
	FREE(ctx);

	return PANORAMA_OK;
}


