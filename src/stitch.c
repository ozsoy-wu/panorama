#include <stdio.h>

#include "stitch.h"

#define onlytwo

int stitch(PANORAMA_INNER_CTX *innerCtx)
{
	int i, j, k;
	int tw = 0;
	int th = 0;
	int overlap = 192;

	int ySize, uSize, vSize, totalSize;

	for (i = 0; i <innerCtx->imgNum; i++)
	{
		tw += innerCtx->images[i].w;
		th = MAX(innerCtx->images[i].h, th);

		if (i > 0)
			tw -= overlap;
	}

	Dbg("total width:%d, total height:%d\n", tw, th);

	ySize = tw * th;
	uSize = ySize / 4;
	vSize = ySize / 4;
	totalSize = ySize + uSize + vSize;

	unsigned char *buf = lMalloc(unsigned char, totalSize * sizeof(unsigned char));
	memset(buf, 0, totalSize * sizeof(unsigned char));

	unsigned char *syp = NULL;
	unsigned char *sup = NULL;
	unsigned char *svp = NULL;
	unsigned char *dyp = NULL;
	unsigned char *dup = NULL;
	unsigned char *dvp = NULL;
	int curTotalW = 0;
	int lastOverlapEnd = 0;
	for (i = 0; i <innerCtx->imgNum; i++)
	{
		int curw = innerCtx->images[i].w;
		int curys = innerCtx->images[i].w * innerCtx->images[i].h;
		int curuvs = curys / 4;
		int overlapB = 0;
		
		dyp = 0;

		for (j = 0; j < innerCtx->images[i].h; j++)
		{
			syp = (unsigned char*)innerCtx->images[i].data[0] + j * innerCtx->images[i].w;
			sup = (unsigned char*)innerCtx->images[i].data[0] + curys + j * (innerCtx->images[i].w / 4);
			svp = (unsigned char*)innerCtx->images[i].data[0] + curys + curuvs + j * (innerCtx->images[i].w / 4);

			dyp = buf + j * tw + curTotalW;
			dup = buf + ySize + j * tw / 4 + curTotalW / 4;
			dvp = buf + ySize + uSize + j * tw / 4 +curTotalW / 4;

			if (i == 0)
			{
				memcpy(dyp, syp, curw);
			}
			else
			{
				for (k = 0; k < innerCtx->images[i].w; k++)
				{
#ifdef onlytwo
					if (i >= 2 && k < (2 * overlap - innerCtx->images[i].w))
					{
						// do nothing
					}
					else 
#endif
					if (k < overlap)
					{
						dyp[k] = syp[k] * k/overlap + dyp[k] * (overlap - k) / overlap;
					}
					else
					{
						dyp[k] = syp[k];
					}

					if (j%2==0 && k%4 == 0)
					{
						int uvidx = k/4;

#ifdef onlytwo
						if (i >= 2 && k < (2 * overlap - innerCtx->images[i].w))
						{
							// do nothing
						}
						else 
#endif
						if (k < overlap)
						{
							dup[uvidx] = sup[uvidx] * k/overlap +dup[uvidx] *(overlap - k) / overlap;
							dvp[uvidx] = svp[uvidx] * k/overlap +dvp[uvidx] *(overlap - k) / overlap;
						}
						else
						{
							dvp[uvidx] = svp[uvidx];
						}
					}
				}
			}
			/*
			memcpy(dyp, syp, curw);
			*/
			memcpy(dup, sup , curw / 4);
			memcpy(dvp, svp, curw / 4);
		}

		curTotalW += innerCtx->images[i].w;
		curTotalW -= overlap;
		lastOverlapEnd = innerCtx->images[i].w * (i - 1) + overlap * (i - 2);
		lastOverlapEnd = lastOverlapEnd < 0 ? 0 : lastOverlapEnd;
	}

	system("rm /home/pg/w/opencv-result/combine.yuv");
	FILE *fp = fopen("/home/pg/w/opencv-result/combine.yuv", "w+");
	if (!fp)
	{
		Dbg("open file failed\n");
	}
	else {
		fwrite(buf, totalSize * sizeof(unsigned char), 1, fp);
		fclose(fp);
		Dbg("write finished\n");
	}
	

	return PANORAMA_OK;
}

