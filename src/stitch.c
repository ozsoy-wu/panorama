#include <stdio.h>

#include "stitch.h"

int stitch(PANORAMA_INNER_CTX *innerCtx)
{
	int i, j;
	int tw = 0;
	int th = 0;
	int overlap = 220;

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

	unsigned char *syp = NULL;
	unsigned char *sup = NULL;
	unsigned char *svp = NULL;
	unsigned char *dyp = NULL;
	unsigned char *dup = NULL;
	unsigned char *dvp = NULL;
	int curTotalW = 0;
	for (i = 0; i <innerCtx->imgNum; i++)
	{
		int curw = innerCtx->images[i].w;
		int curys = innerCtx->images[i].w * innerCtx->images[i].h;
		int curuvs = curys / 4;
		
		dyp = 0;

		Dbg("image#%d\n", i);

		for (j = 0; j < innerCtx->images[i].h; j++)
		{
			syp = (unsigned char*)innerCtx->images[i].data[0] + j * innerCtx->images[i].w;
			sup = (unsigned char*)innerCtx->images[i].data[0] + curys + j * (innerCtx->images[i].w / 4);
			svp = (unsigned char*)innerCtx->images[i].data[0] + curys + curuvs + j * (innerCtx->images[i].w / 4);

			dyp = buf + j * tw + curTotalW;
			dup = buf + ySize + j * tw / 4 + curTotalW / 4;
			dvp = buf + ySize + uSize + j * tw / 4 +curTotalW / 4;

			memcpy(dyp, syp, curw);
			memcpy(dup, sup , curw / 4);
			memcpy(dvp, svp, curw / 4);
		}

		curTotalW += innerCtx->images[i].w;
		//if (i > 0)
			curTotalW -= overlap;

		Dbg("current total width: %d\n", curTotalW);
	}

	FILE *fp = fopen("/home/pg/w/opencv-result/combine.yuv", "w+");
	if (!fp)
	{
		Dbg("open file failed\n");
	}
	else {
		fwrite(buf, totalSize * sizeof(unsigned char), 1, fp);
		fclose(fp);
	}
	

	return PANORAMA_OK;
}

