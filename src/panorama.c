#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "panorama_inner.h"
#include "panorama.h"
#include "stitch.h"

static int ctxCnt = 0;
int gLogMask;

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
	innerCtx->status = INIT;

	innerCtx->cfg.yAngleOffset = 0;
	innerCtx->cfg.viewingAngle = -1;
	innerCtx->cfg.rotateAngle = -1;
	innerCtx->cfg.focalLength = -1;
	innerCtx->cfg.overlapWidth = -1;
	innerCtx->cfg.stitchWidth = DEFAULT_STITCH_WIDTH;
	innerCtx->cfg.imgTotalNum = -1;
	innerCtx->cfg.outImageWidth = -1;
	innerCtx->cfg.outImageHeight = -1;
	innerCtx->cfg.outImageFmt = IMG_FMT_YUV420P_I420;

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

	if (cfg->stitchWidth > cfg->overlapWidth)
	{
		Log(LOG_ERROR, "stitch width(%d) too large, should smaller than overlap width(%d)\n",
			cfg->stitchWidth, cfg->overlapWidth);
		return PANORAMA_ERROR;
	}

	memcpy(&inCtx->cfg, cfg, sizeof(PANORAMA_CFG));

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
	int i;
	int ret;
	int panoW, panoH;
	Image *img = NULL;
	INNER_STATUS status;

	if (!ctx)
	{
		Log(LOG_ERROR, "ctx NULL\n");
		return PANORAMA_ERROR;
	}

	PANORAMA_INNER_CTX *inCtx = GET_INNER_CTX(ctx);

	status = inCtx->status;
	switch (status)
	{
		case INIT:
			panoW = inCtx->images[0].w * inCtx->cfg.imgTotalNum;
			panoW -= inCtx->cfg.overlapWidth * (inCtx->cfg.imgTotalNum - 1);
			panoH = inCtx->images[0].h;
			img = &inCtx->pano;
			ret = constructImage(&img, NULL, NULL, 0, panoW,
				panoH, inCtx->images[0].imgFmt, BUF_TYPE_NOBUF);
			inCtx->status = PROCESS;
			break;
		case PROCESS:
			break;
	}

#if 0
	for (i = inCtx->imgToBeHandle; i < inCtx->imgNum; i++)
	{
		ret = constructVector(&(inCtx->kpVecPtr[i]), sizeof(KeyPoint), -1);
		if (PANORAMA_OK != ret)
		{
			Log(LOG_ERROR, "image#%d: construct keypoint vector failed\n", i);
			ret = PANORAMA_ERROR;
			goto clean;
		}

		ret = inCtx->featureFinder.detectAndCompute(
			inCtx->featureFinder.cfg,
			&inCtx->images[i],
			inCtx->kpVecPtr[i],
			&(inCtx->kpdesVecPtr[i]));
		if (PANORAMA_OK != ret)
		{
			Log(LOG_ERROR, "image#%d: feature detect and compute failed\n", i);
			ret = PANORAMA_ERROR;
			goto clean;
		}
		Log(LOG_ERROR, "image#%d: feature detect and compute OK\n", i);
	}
#endif

#ifdef DEBUG_FUNC

/*	
	KeyPoint *ckp = NULL;
	int j;

	for (i = 0; i < inCtx->imgNum; i++)
	{
		printf("image#%d:+++++++++++++++++feature points cnt:%d+++++++++++++++++\n", i, inCtx->kpVecPtr[i]->size);
		for (j = 0; j < inCtx->kpVecPtr[i]->size; j++)
		{
			ckp = (KeyPoint *)VECTOR_AT(inCtx->kpVecPtr[i], j);
			printf("kp%d:[%f,%f], size=%f, angle=%f, response=%f, octave=%d, classId=%d\n",
				   j,
				   ckp->pt.x, ckp->pt.y,
				   ckp->size, ckp->angle,
				   ckp->response, ckp->octave,
				   ckp->classId);
		}
	}


	for (i = 0; i < inCtx->imgNum; i++)
	{
		Log(LOG_DEBUG, "image#%d: keypoints descriptor, cnt:%d, dimension:%d\n", i, inCtx->kpdesVecPtr[i]->rows, inCtx->kpdesVecPtr[i]->cols);
		//PRINT(Mat, inCtx->kpdesVecPtr[i]);
	}
	*/
#endif

	ret = knnMatcher();
	if (PANORAMA_OK != ret)
	{
		Log(LOG_ERROR, "feature match failed\n");
		ret = PANORAMA_ERROR;
		goto clean;
	}

	for (i = inCtx->imgToBeHandle; i < inCtx->imgNum; i++)
	{
		ret = stitch(inCtx, i);
		if (PANORAMA_OK != ret)
		{
			Log(LOG_ERROR, "image#%d stitch failed\n", i);
			ret = PANORAMA_ERROR;
			goto clean;
		}
	}

	/* 更新待处理图片下标 */
	inCtx->imgToBeHandle = inCtx->imgNum;

	ret = PANORAMA_PROCESS_FINISH;

clean:

	return ret;
}

int PanoramaProcessQuery (PANORAMA_CTX *ctx)
{
	if (!ctx)
	{
		return PANORAMA_ERROR;
	}

	return PANORAMA_PROCESS_FINISH;
}

int PanoramaFetch (PANORAMA_CTX *ctx, char **ptr, int *bufsize, int *imgWidth, int *imgHeight, IMG_FORMAT *format)
{
	if (!ctx)
	{
		return PANORAMA_ERROR;
	}

	PANORAMA_INNER_CTX *inCtx = NULL;
	inCtx = GET_INNER_CTX(ctx);

	if (INIT == inCtx->status)
	{
		Log(LOG_ERROR, "should at least run PanoramaProcess one time\n");
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

	if (INIT == inCtx->status)
	{
		Log(LOG_ERROR, "should at least run PanoramaProcess one time\n");
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
	innerCtx->status = INIT;

	innerCtx->cfg.yAngleOffset = 0;
	innerCtx->cfg.viewingAngle = -1;
	innerCtx->cfg.rotateAngle = -1;
	innerCtx->cfg.focalLength = -1;
	innerCtx->cfg.overlapWidth = -1;
	innerCtx->cfg.stitchWidth = DEFAULT_STITCH_WIDTH;
	innerCtx->cfg.imgTotalNum = -1;
	innerCtx->cfg.outImageWidth = -1;
	innerCtx->cfg.outImageHeight = -1;
	innerCtx->cfg.outImageFmt = IMG_FMT_YUV420P_I420;

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


