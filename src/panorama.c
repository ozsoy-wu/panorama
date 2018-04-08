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

	gLogMask = LOG_DEBUG | LOG_INFO | LOG_WARN | LOG_ERROR |LOG_FATAL;

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


	surfCfg = tMalloc(SURF_CFG);
	if (!surfCfg)
	{
		goto err;
	}
	memset((void *)surfCfg, 0, sizeof(SURF_CFG));

	surfCfg->hessianThreshold = 500;
	surfCfg->nOctaves = 4;
	surfCfg->nOctaveLayers = 3;
	surfCfg->extended = 0;
	surfCfg->upright = 0;

	innerCtx->featureFinder.cfg = (void *)surfCfg;
	innerCtx->featureFinder.detect = surfFeatureDetect;
	innerCtx->featureFinder.compute= surfFeatureCompute;
	innerCtx->featureFinder.detectAndCompute = surfFeatureDetectAndCompute;

	TAILQ_INIT(&innerCtx->imageQueue); // TODO

	ctx->innerCtx = (void *)innerCtx;

	Log(LOG_DEBUG, "init ok\n");

	return ctx;

err:

	FREE(surfCfg);
	FREE(innerCtx);
	FREE(ctx);

	return NULL;
}

int PanoramaDeInit (PANORAMA_CTX *ctx)
{
	if (!ctx)
	{
		return PANORAMA_ERROR;
	}


	// TODO
	// free ctx
	// free innerCtx
	// free image

	return PANORAMA_OK;
}

int PanoramaGetCfg (PANORAMA_CTX *ctx, PANORAMA_CFG *cfg)
{
	if (!ctx || !cfg)
	{
		return PANORAMA_ERROR;
	}

	return PANORAMA_OK;
}

int PanoramaSetCfg (PANORAMA_CTX *ctx, PANORAMA_CFG *cfg)
{
	if (!ctx || !cfg)
	{
		return PANORAMA_ERROR;
	}

	return PANORAMA_OK;
}

int PanoramaLoadSrcImgFile (PANORAMA_CTX *ctx, char *filename, int imgWidth, int imgHeight, IMG_FORMAT format)
{
	int ret = PANORAMA_ERROR;
	int imgTotalSize = 0;
	int nread = 0;
	int i;
	int idx;
	unsigned char *imgBuf = NULL;
	FILE *fp;
	Image *img = NULL;
	PANORAMA_INNER_CTX *inCtx = NULL;

	if (!ctx || !filename)
	{
		return PANORAMA_ERROR;
	}

	inCtx = GET_INNER_CTX(ctx);

	/*
	img = (Image *)malloc(sizeof(Image));
	if (!img)
	{
		goto error;
	}
	*/

	if (IMG_FMT_YUV420P_I420 == format)
	{
		imgTotalSize = (imgWidth * imgHeight * 3)>>1;
		imgBuf = (unsigned char *)malloc(sizeof(unsigned char) * imgTotalSize);
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

		idx = inCtx->imgNum;
		inCtx->images[idx].w = imgWidth;
		inCtx->images[idx].h = imgHeight;
		inCtx->images[idx].imgFmt = format;
		inCtx->images[idx].needFree = 1;
		inCtx->images[idx].dataBlocks = 1;
		inCtx->images[idx].dataSize[0] = imgTotalSize;
		inCtx->images[idx].data[0] = (dataType *)imgBuf;

		inCtx->imgNum++;

		/*
		img->w = imgWidth;
		img->h = imgHeight;
		img->imgFmt = format;
		img->needFree = 1;
		img->dataBlocks = 1;
		img->dataSize[0] = imgTotalSize;
		img->data[0] = (dataType *)imgBuf;
		TAILQ_INSERT_TAIL(&(GET_INNER_CTX(ctx)->imageQueue), img, entries);
		*/
	}
	else
	{
		// TODO
	}

	return PANORAMA_OK;
error:
	if (imgBuf)
	{
		free(imgBuf);
		imgBuf = NULL;
	}

	if (img)
	{
		free(img);
		img = NULL;
	}

	return ret;
}

int PanoramaLoadSrcImgBuffer (PANORAMA_CTX *ctx, char *imgBuf, int bufSize, int imgWidth, int imgHeight, IMG_FORMAT format)
{
	if (!ctx || !imgBuf || bufSize < 0)
	{
		return PANORAMA_ERROR;
	}

	return PANORAMA_OK;
}

int PanoramaProcess (PANORAMA_CTX *ctx)
{
	int i;
	int ret;

	if (!ctx)
	{
		Log(LOG_ERROR, "ctx NULL\n");
		return PANORAMA_ERROR;
	}

	PANORAMA_INNER_CTX *inCtx = GET_INNER_CTX(ctx);
	//Vector *kpVecPtr[MAX_IMAGE_NUM] = {NULL};
	//Mat *kpdesVecPtr[MAX_IMAGE_NUM] = {NULL};

	// int surfFeatureDetectAndCompute(SURF_CFG *cfg, Image *img, KeyPoint *kp, KeyPointDescriptor* kpdes)
	for (i = 0; i < inCtx->imgNum; i++)
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
	}

#ifdef DEBUG_FUNC
	for (i = 0; i < inCtx->imgNum; i++)
	{
		int j;
		KeyPoint *curkp = NULL;
		Log(LOG_DEBUG, "image#%d: keypointsCnt:%d\n", i, inCtx->kpVecPtr[i]->size);
		for (j = 0; j < inCtx->kpVecPtr[i]->size; j++)
		{
			curkp = (KeyPoint *)VECTOR_AT(inCtx->kpVecPtr[i], j);
			//printf("[%f, %f]\n", curkp->pt.x, curkp->pt.y);
		}
	}
	for (i = 0; i < inCtx->imgNum; i++)
	{
		Log(LOG_DEBUG, "image#%d: keypoints descriptor, cnt:%d, dimension:%d\n", i, inCtx->kpdesVecPtr[i]->rows, inCtx->kpdesVecPtr[i]->cols);
		//PRINT(Mat, inCtx->kpdesVecPtr[i]);
	}
#endif

	ret = knnMatcher();
	if (PANORAMA_OK != ret)
	{
		Log(LOG_ERROR, "feature match failed\n");
		ret = PANORAMA_ERROR;
		goto clean;
	}

	ret = stitch(inCtx);
	if (PANORAMA_OK != ret)
	{
		Log(LOG_ERROR, "image stitch failed\n");
		ret = PANORAMA_ERROR;
		goto clean;
	}

	ret = PANORAMA_PROCESS_FINISH;

clean:
	for (i = 0; i < inCtx->imgNum; i++)
	{
		destructVector(&(inCtx->kpVecPtr[i]));
		destructMat(&(inCtx->kpdesVecPtr[i]));
	}

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

int PanoramaFetch (PANORAMA_CTX *ctx, char *ptr, int *bufsize, int *imgWidth, int *imgHeight, IMG_FORMAT *format)
{
	if (!ctx || !ptr || !bufsize || !imgWidth || !imgHeight || !format)
	{
		return PANORAMA_ERROR;
	}

	return PANORAMA_OK;
}

int PanoramaSaveToFile (PANORAMA_CTX *ctx, char *filename, int *imgWidth, int *imgHeight, IMG_FORMAT format)
{
	if (!ctx || !filename)
	{
		return PANORAMA_ERROR;
	}

	return PANORAMA_OK;
}
