#include <stdlib.h>
#include <stdio.h> 

#include "panorama.h"

#define PANO_W 600
#define PANO_H 240
#define IMG_NUM 12

int main(int argc, char **argv)
{
	int i, panoBufSize, panoW, panoH, process;
	int ret = PANORAMA_OK;
	IMG_FORMAT panoFmt;
	char *pano = NULL; 
	char *imgName[IMG_NUM] = {
		"/home/pg/w/0.yuv",
		"/home/pg/w/1.yuv",
		"/home/pg/w/2.yuv",
		"/home/pg/w/3.yuv",
		"/home/pg/w/4.yuv",
		"/home/pg/w/5.yuv",
		"/home/pg/w/6.yuv",
		"/home/pg/w/7.yuv",
		"/home/pg/w/8.yuv",
		"/home/pg/w/9.yuv",
		"/home/pg/w/10.yuv",
		"/home/pg/w/11.yuv",
	};
	PANORAMA_CTX *ctx = NULL;
	PANORAMA_CFG cfg;

	printf("%s, argc=%d\n", argv[0], argc);

	ctx = PanoramaInit();
	if (!ctx)
	{
		PMD();	
		goto out;
	}


	ret = PanoramaGetCfg(ctx, &cfg);
	if (ret != PANORAMA_OK)
	{
		PMD();	
		goto out;
	}

	cfg.outImageFmt = IMG_FMT_YUV420P_I420;
	cfg.outImageWidth = PANO_W;
	cfg.outImageHeight = PANO_H;

	ret = PanoramaSetCfg(ctx, &cfg);
	if (ret != PANORAMA_OK)
	{
		PMD();	
		goto out;
	}

	for (i = 0; i < IMG_NUM; i++)
	{
		if (PANORAMA_OK != PanoramaLoadSrcImgFile(ctx,
					imgName[i], 320, 240, IMG_FMT_YUV420P_I420))
		{
			PMD();
			goto out;
		}
	}
	PMD();

	while (1)
	{
		process = PanoramaProcess(ctx);
		if (PANORAMA_PROCESS_ERROR == process)
		{
			PMD();
			goto out;
		}

		if (PANORAMA_PROCESS_FINISH == process)
		{
			// 完成
			break;
		}
		else
		{
			// 更新进度
			PMD("process %d/100\n", process);
		}
	}

	pano = (char *)malloc(sizeof(char) * PANO_W * PANO_H);
	if (!pano)
	{
		PMD();
		goto out;
	}

	ret = PanoramaFetch(ctx, pano, &panoBufSize, &panoW, &panoH, &panoFmt);
	if (ret != PANORAMA_OK)
	{
		PMD();
		goto out;
	}

out:
	if (pano) free(pano);

	ret = PanoramaDeInit(ctx);

	return ret;
}
