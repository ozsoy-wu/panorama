/******************************************************************************
 * Copyright (c) 2015-2018 TP-Link Technologies CO.,LTD.
 *
 * 文件名称:		panorama.c
 * 版           本:	1.0
 * 摘           要:	全景图库接口实现
 * 作           者:	wupimin<wupimin@tp-link.com.cn>
 * 创建时间:		2018-04-28
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "panorama.h"
#include "panorama_inner.h"
#include "panorama_stitch.h"

static int ctxCnt = 0;
int gLogMask;

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
	cfg->camDistortionK1 = 0;
	cfg->camDistortionK2 = 0;
	cfg->stitchOverlapPercent = DEFAULT_STITCH_WIDTH_PERCENT;
	cfg->stitchInterpolationPercent = DEFAULT_INTERPOLATION_WIDTH_PERCENT;
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

	/*
	if (FLOAT_EQUAL(cfg->camViewingAngle, 0) || cfg->camViewingAngle < 0)
	{
		Log(LOG_ERROR, "Invalid camViewingAngle %10.10f\n", cfg->camViewingAngle);
		return PANORAMA_ERROR;
	}

	if (FLOAT_EQUAL(cfg->camRotateAngle, 0) || cfg->camRotateAngle < 0)
	{
		Log(LOG_ERROR, "Invalid camViewingAngle %10.10f\n", cfg->camRotateAngle);
		return PANORAMA_ERROR;
	}
	*/

	if (cfg->srcImgWidth <= 0 ||cfg->srcImgHeight <= 0)
	{
		Log(LOG_ERROR, "Invalid source image size [%d * %d]\n", cfg->srcImgWidth, cfg->srcImgHeight);
		return PANORAMA_ERROR;
	}

	if (cfg->stitchInterpolationPercent < 0 || cfg->stitchInterpolationPercent > 1)
	{
		Log(LOG_ERROR, "Invalid Interpolation Percent %.10f, should in range [0, 1]\n",
			cfg->stitchInterpolationPercent);
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
	int ret;
	int midW, midH, topW, topH;

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

#ifdef SUPPORT_UNDISTORT
	double k1, k1FromLevel, k2FromLevel;
	double p1R2, p1R4;
	double p2R2, p2R4;
	Point p0, p1, p2;

	if (FLOAT_EQUAL(cfg->camDistortionK1, -1))
	{
		ret = distortCalcK1K2(-0.00857, cfg->srcImgWidth,
			cfg->srcImgHeight, &k1FromLevel, &k2FromLevel);
		if (PANORAMA_OK != ret)
		{
			Log(LOG_ERROR, "distortCalcK1K2 failed\n");
			return PANORAMA_ERROR;
		}

		Log(LOG_DEBUG, "k1 from coor = %10.20f, k1from level = %10.20f\n", k1, k1FromLevel);
	}

	/* 计算原始图像矫正后的分辨率 */
	p0.x = 0;
	p0.y = 0;
	p1.x = cfg->srcImgWidth / 2;
	p1.y = cfg->srcImgHeight / 2;
	p2.x = cfg->srcImgWidth / 2;
	p2.y = 0;
	p1R2 = pointDisPower2(&p0, &p1);
	p1R4 = p1R2 * p1R2;
	p2R2 = pointDisPower2(&p1, &p2);
	p2R4 = p2R2 * p2R2;

	/* 矫正后的图像分辨率 */
	topW = 2 * ceil(CORRECT_COOR(p1.x, cfg->camDistortionK1, p1R2, cfg->camDistortionK2, p1R4));
	topH = 2 * ceil(CORRECT_COOR(p1.y, cfg->camDistortionK1, p1R2, cfg->camDistortionK2, p1R4));
	inCtx->undistortImgW = topW;
	inCtx->undistortImgH = topH;

	midW = 2 * ceil(CORRECT_COOR(p2.x, cfg->camDistortionK1, p2R2, cfg->camDistortionK2, p2R4));
	midH = 2 * ceil(CORRECT_COOR(p2.y, cfg->camDistortionK1, p2R2, cfg->camDistortionK2, p2R4));
#else
	topW = midW = cfg->srcImgWidth;
	topH = midH = cfg->srcImgHeight;
#endif

	/* 计算重合区域宽度 */
	inCtx->stitchOverlapWidth = ceil(midW * cfg->stitchOverlapPercent);
	inCtx->stitchOverlapWidth += (topW - midW);
	if (inCtx->stitchOverlapWidth & 0x1)
	{
		inCtx->stitchOverlapWidth--;
	}

	/* 计算插值宽度 */
	inCtx->stitchInterpolationWidth = ceil(inCtx->stitchOverlapWidth * cfg->stitchInterpolationPercent);
	if (inCtx->stitchInterpolationWidth & 0x1)
	{
		inCtx->stitchInterpolationWidth--;
	}

	/* pano */
	inCtx->pano.w = cfg->commonImgTotalNum * topW - (cfg->commonImgTotalNum - 1) * inCtx->stitchOverlapWidth;
	inCtx->pano.h = topH;

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
	PANORAMA_INNER_CTX *inCtx = NULL;

	if (!ctx || !filename)
	{
		return PANORAMA_ERROR;
	}

	inCtx = GET_INNER_CTX(ctx);

	idx = inCtx->imgNum;

	if (IMG_FMT_YUV420P_I420 == format)
	{
		imgTotalSize = (imgWidth * imgHeight * 3)>>1;

		/* 该buf应由Image释放 */
		imgBuf = lMalloc(unsigned char, sizeof(unsigned char) * imgTotalSize);
		if (!imgBuf)
		{
			Log(LOG_ERROR, "malloc image buf failed\n");
			goto clean;
		}

		fp = fopen(filename, "r");
		if (!fp)
		{
			Log(LOG_ERROR, "open file %s failed\n", filename);
			goto clean;
		}

		nread = fread(imgBuf, 1, imgTotalSize, fp);
		if (nread < imgTotalSize)
		{
			Log(LOG_ERROR, "read %d bytes while %d bytes expected\n");
			goto clean;
		}

		/* 原始图数据 */
		ret = imageConstruct(&inCtx->images[idx], (unsigned char *)&imgBuf, &imgTotalSize, 1, imgWidth,
			imgHeight, format, BUF_TYPE_NOCOPY_DELETE);
		if (PANORAMA_OK != ret)
		{
			goto clean;
		}

		inCtx->imgNum++;
	}
	else
	{
		// TODO
	}

	ret = PANORAMA_OK;

clean:
	if (fp)
	{
		fclose(fp);
		fp = NULL;
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
	PANORAMA_INNER_CTX *inCtx = NULL;

	inCtx = GET_INNER_CTX(ctx);
	idx = inCtx->imgNum;

	if (copy)
	{
		ret = imageConstruct(&inCtx->images[idx], buf, bufSize, bufCnt, imgWidth,
			imgHeight, format, BUF_TYPE_COPY_NODELETE);
	}
	else
	{
		ret = imageConstruct(&inCtx->images[idx], buf, bufSize, bufCnt, imgWidth,
			imgHeight, format, BUF_TYPE_NOCOPY_NODELETE);
	}

	if (PANORAMA_OK != ret)
	{
		ret = PANORAMA_ERROR;
		goto clean;
	}

	inCtx->imgNum++;
	ret = PANORAMA_OK;

clean:

	return ret;

}

int PanoramaProcess (PANORAMA_CTX *ctx)
{
	int ret;
	int panoW, panoH;
	int percent;
	int currentStep = 0;
	int totalStep = 0;
	Image *img = NULL;
	Image *srcImg = NULL;
	Image *undistortImg = NULL;
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
			img = &inCtx->pano;
			panoW = img->w;
			panoH = img->h;

			ret = imageConstruct(&img, NULL, NULL, 0, panoW,
				panoH, inCtx->cfg.panoImageFmt, BUF_TYPE_NOBUF);
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
			inCtx->status++;
			break;

#ifdef UNDISTORT_SUPPORT
		case STATUS_UNDISTORT_IMAGE:
			/* 校正 */
			srcImg = inCtx->images[inCtx->imgToBeHandle];
			if (PANORAMA_OK != imageUndistort(inCtx->cfg.camDistortionK1, inCtx->cfg.camDistortionK2,
				srcImg, &undistortImg))
			{
				Log(LOG_ERROR, "image#%d: undistort failed\n", inCtx->imgToBeHandle);
				ret = PANORAMA_PROCESS_ERROR;
				goto out;
			}

			/* 释放原始图 */
			imageDestruct(&srcImg);

			/* 将矫正后的图像指针更新到inner context中 */
			inCtx->images[inCtx->imgToBeHandle] = undistortImg;

			inCtx->status++;
			break;
#endif
#ifdef FEATURE_BASE
		case STATUS_FEATURE_DETECT:
 			ret = vectorConstruct(&(inCtx->kpVecPtr[inCtx->imgToBeHandle]), sizeof(KeyPoint), -1);
			if (PANORAMA_OK != ret)
			{
				Log(LOG_ERROR, "image#%d: construct keypoint vector failed\n", inCtx->imgToBeHandle);
				ret = PANORAMA_PROCESS_ERROR;
				goto out;
			}

			ret = inCtx->featureFinder.detect(
				(SURF_CFG *)inCtx->featureFinder.cfg,
				inCtx->images[inCtx->imgToBeHandle],
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
				inCtx->images[inCtx->imgToBeHandle],
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
			ret = matchKnn();
			if (PANORAMA_OK != ret)
			{
				Log(LOG_ERROR, "feature match failed\n");
				ret = PANORAMA_PROCESS_ERROR;
				goto out;
			}
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
		imageDestruct(&innerCtx->images[i]);
		vectorDestruct(&innerCtx->kpVecPtr[i]);
		matDestruct(&innerCtx->kpdesVecPtr[i]);
	}

	imgPtr = &innerCtx->pano;
	imageDestruct(&imgPtr);

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


