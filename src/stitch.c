#include <stdio.h>

#include "stitch.h"

int stitch(PANORAMA_INNER_CTX *innerCtx, int idx)
{
	int j, k;
	int tw = 0;
	int th = 0;
	int overlap = 0;
	int stitchWitdh = 0;
	int stitchBegin = 0;
	int stitchEnd = 0;
	int ySize, uSize, vSize;
	int curw;
	int curys;
	int curuvs;
	int uvidx = 0;
	int curImageStart = 0;
	Image *curImage = NULL;
	unsigned char *srcBuf = NULL;
	unsigned char *dstBuf = NULL;
	unsigned char *syp = NULL;
	unsigned char *sup = NULL;
	unsigned char *svp = NULL;
	unsigned char *dyp = NULL;
	unsigned char *dup = NULL;
	unsigned char *dvp = NULL;
	unsigned char *srcUStart = NULL;
	unsigned char *srcVStart = NULL;
	unsigned char *dstUStart = NULL;
	unsigned char *dstVStart = NULL;

	if (!innerCtx || idx < 0 || idx >= innerCtx->cfg.commonImgTotalNum)
	{
		Log(LOG_ERROR, "arguments invalid\n");
		return PANORAMA_ERROR;
	}

	overlap = innerCtx->cfg.stitchOverlapWidth;
	stitchWitdh = innerCtx->cfg.stitchInterpolationWidth;
	curImage = &innerCtx->images[idx];
	tw = innerCtx->pano.w;
	th = innerCtx->pano.h;
	curImageStart = idx * (curImage->w - overlap);

	ySize = tw * th;
	uSize = vSize = ySize / 4;

	srcBuf = (unsigned char*)curImage->data[0];
	dstBuf = innerCtx->pano.data[0];

	curw = curImage->w;
	curys = curImage->w * curImage->h;
	curuvs = curys / 4;

	stitchBegin = (overlap - stitchWitdh)>>1;
	stitchEnd = (overlap + stitchWitdh)>>1;

	// Y
	for (j = 0; j < curImage->h; j++)
	{
		/* y分量内存可以保证连续 */
		syp = srcBuf + j * curImage->w;

		/* pano的buf可保证内存连续 */
		dyp = dstBuf + j * tw + curImageStart;

		if (idx == 0)
		{
			memcpy(dyp, syp, curw);
		}
		else
		{
			for (k = 0; k < curImage->w; k++)
			{
				if (k < stitchBegin)
				{
					//dyp[k] = dyp[k];
				}
				else if (k < stitchEnd)
				{
					dyp[k] = syp[k] * k/overlap + dyp[k] * (overlap - k) / overlap;
				}
				else
				{
					dyp[k] = syp[k];
				}
			}
		}
	}

	// UV
	if (IMG_FMT_YUV420P_I420 == curImage->imgFmt)
	{
		if (1 == curImage->dataBlocks)
		{
			srcUStart = (unsigned char*)curImage->data[0] + curys;
			srcVStart = srcUStart + curuvs;
		}
		else if (2 == curImage->dataBlocks)
		{
			srcUStart = (unsigned char*)curImage->data[1];
			srcVStart = srcUStart + curuvs;
		}
		else
		{
			srcUStart = (unsigned char*)curImage->data[1];
			srcVStart = (unsigned char*)curImage->data[2];
		}

		dstUStart = dstBuf + ySize;
		dstVStart = dstBuf + ySize + uSize;

		for (j = 0; j < curImage->h; j++)
		{
			if (j % 2 == 1)
			{
				continue;
			}
			sup = srcUStart + (j/2) * (curImage->w / 2);
			svp = srcVStart + (j/2) * (curImage->w / 2);

			dup = dstUStart + (j/2) * (tw/2) + (curImageStart / 2);
			dvp = dstVStart + (j/2) * (tw/2) + (curImageStart / 2);

			if (idx == 0)
			{
				for (k = 0; k < curImage->w; k++)
				{
					if (k%2 == 1)
					{
						continue;
					}

					uvidx = k/2;
					dup[uvidx] = sup[uvidx];
					dvp[uvidx] = svp[uvidx];
				}
			}
			else
			{
				for (k = 0; k < curImage->w; k++)
				{
					if (k%2 == 1)
					{
						continue;
					}

					uvidx = k/2;

					if (k < stitchBegin)
					{
						// do nothing
					}
					else if (k < stitchEnd)
					{
						dup[uvidx] = sup[uvidx] * k/overlap +dup[uvidx] *(overlap - k) / overlap;
						dvp[uvidx] = svp[uvidx] * k/overlap +dvp[uvidx] *(overlap - k) / overlap;
					}
					else
					{
						dup[uvidx] = sup[uvidx];
						dvp[uvidx] = svp[uvidx];
					}
				}
			}
		}
	}

	return PANORAMA_OK;
}

