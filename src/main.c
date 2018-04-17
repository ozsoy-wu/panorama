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
	

#if 0
#define WW 1280
#define HH 720
#define OVERLAP 360
#define YANGLE 0
#define VIEWANGLE 56.7
#define ROTATEANGLE 30
	char *imgName[IMG_NUM] = {
		"/home/pg/w/IPC40_srcImg_y0/IPC_060_1_x-150y0.jpg.yuv",
		"/home/pg/w/IPC40_srcImg_y0/IPC_060_2_x-120y0.jpg.yuv",
		"/home/pg/w/IPC40_srcImg_y0/IPC_060_3_x-90y0.jpg.yuv",
		"/home/pg/w/IPC40_srcImg_y0/IPC_060_4_x-60y0.jpg.yuv",
		"/home/pg/w/IPC40_srcImg_y0/IPC_060_5_x-30y0.jpg.yuv",
		"/home/pg/w/IPC40_srcImg_y0/IPC_060_6_x0y0.jpg.yuv",
		"/home/pg/w/IPC40_srcImg_y0/IPC_060_7_x30y0.jpg.yuv",
		"/home/pg/w/IPC40_srcImg_y0/IPC_060_8_x60y0.jpg.yuv",
		"/home/pg/w/IPC40_srcImg_y0/IPC_060_9_x90y0.jpg.yuv",
		"/home/pg/w/IPC40_srcImg_y0/IPC_060_10_x120y0.jpg.yuv",
		"/home/pg/w/IPC40_srcImg_y0/IPC_060_11_x150y0.jpg.yuv",

	};
#endif

#if 0
#define WW 1280
#define HH 720
#define OVERLAP 360
	char *imgName[IMG_NUM] = {
		"/home/pg/w/IPC40_srcImg_y20/IPC_060_1_x-150y20.jpg.yuv",
		"/home/pg/w/IPC40_srcImg_y20/IPC_060_2_x-120y20.jpg.yuv",
		"/home/pg/w/IPC40_srcImg_y20/IPC_060_3_x-90y20.jpg.yuv",
		"/home/pg/w/IPC40_srcImg_y20/IPC_060_4_x-60y20.jpg.yuv",
		"/home/pg/w/IPC40_srcImg_y20/IPC_060_5_x-30y20.jpg.yuv",
		"/home/pg/w/IPC40_srcImg_y20/IPC_060_6_x0y20.jpg.yuv",
		"/home/pg/w/IPC40_srcImg_y20/IPC_060_7_x30y20.jpg.yuv",
		"/home/pg/w/IPC40_srcImg_y20/IPC_060_8_x60y20.jpg.yuv",
		"/home/pg/w/IPC40_srcImg_y20/IPC_060_9_x90y20.jpg.yuv",
		"/home/pg/w/IPC40_srcImg_y20/IPC_060_10_x120y20.jpg.yuv",
		"/home/pg/w/IPC40_srcImg_y20/IPC_060_11_x150y20.jpg.yuv",
	};
#endif

#if 0
#define WW 1280
#define HH 720
	char *imgName[IMG_NUM] = {
		"/home/pg/w/IPC40_srcImg/IPC_060_1_x-150y60.jpg.yuv",
		"/home/pg/w/IPC40_srcImg/IPC_060_2_x-120y60.jpg.yuv",
		"/home/pg/w/IPC40_srcImg/IPC_060_3_x-90y60.jpg.yuv",
		"/home/pg/w/IPC40_srcImg/IPC_060_4_x-60y60.jpg.yuv",
		"/home/pg/w/IPC40_srcImg/IPC_060_5_x-30y60.jpg.yuv",
		"/home/pg/w/IPC40_srcImg/IPC_060_6_x0y60.jpg.yuv",
		"/home/pg/w/IPC40_srcImg/IPC_060_7_x30y60.jpg.yuv",
		"/home/pg/w/IPC40_srcImg/IPC_060_8_x60y60.jpg.yuv",
		"/home/pg/w/IPC40_srcImg/IPC_060_9_x90y60.jpg.yuv",
		"/home/pg/w/IPC40_srcImg/IPC_060_10_x120y60.jpg.yuv",
		"/home/pg/w/IPC40_srcImg/IPC_060_11_x150y60.jpg.yuv",
	};
#endif
#if 1
#define WW 320
#define HH 240
#define OVERLAP 192
	char *imgName[IMG_NUM] = {
		"/home/pg/w/yuv_from_bmp/11.bmp.yuv",
		"/home/pg/w/yuv_from_bmp/10.bmp.yuv",
		"/home/pg/w/yuv_from_bmp/9.bmp.yuv",
		"/home/pg/w/yuv_from_bmp/8.bmp.yuv",
		"/home/pg/w/yuv_from_bmp/7.bmp.yuv",
		"/home/pg/w/yuv_from_bmp/6.bmp.yuv",
		"/home/pg/w/yuv_from_bmp/5.bmp.yuv",
		"/home/pg/w/yuv_from_bmp/4.bmp.yuv",
		"/home/pg/w/yuv_from_bmp/3.bmp.yuv",
		"/home/pg/w/yuv_from_bmp/2.bmp.yuv",
		"/home/pg/w/yuv_from_bmp/1.bmp.yuv",
		"/home/pg/w/yuv_from_bmp/0.bmp.yuv",
		};
#endif
#if 0
	char *imgName[IMG_NUM] = {
		"/home/pg/w/11.yuv",
		"/home/pg/w/10.yuv",
		"/home/pg/w/9.yuv",
		"/home/pg/w/8.yuv",
		"/home/pg/w/7.yuv",
		"/home/pg/w/6.yuv",
		"/home/pg/w/5.yuv",
		"/home/pg/w/4.yuv",
		"/home/pg/w/3.yuv",
		"/home/pg/w/2.yuv",
		"/home/pg/w/1.yuv",
		"/home/pg/w/0.yuv",
		};
#endif
#if 0
	char *imgName[IMG_NUM] = {
		"/home/pg/w/1.yuv",
		"/home/pg/w/0.yuv",
		};/*
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
	};*/
#endif
	FILE *fp = NULL;
	char fn[100] = {0};
	PANORAMA_CTX *ctx = NULL;
	PANORAMA_CFG cfg;

	ctx = PanoramaInit();
	if (!ctx)
	{
		printf("init failed\n");
		goto out;
	}


	ret = PanoramaGetCfg(ctx, &cfg);
	if (ret != PANORAMA_OK)
	{
		printf("PanoramaGetCfg failed\n");
		goto out;
	}

	cfg.imgTotalNum = IMG_NUM;
	cfg.overlapWidth = OVERLAP;
	cfg.stitchWidth = 50;
	cfg.outImageFmt = IMG_FMT_YUV420P_I420;
	cfg.outImageWidth = PANO_W;
	cfg.outImageHeight = PANO_H;

	ret = PanoramaSetCfg(ctx, &cfg);
	if (ret != PANORAMA_OK)
	{
		printf("PanoramaSetCfg failed\n");
		goto out;
	}

	for (i = 0; i < IMG_NUM; i++)
	{
		// 加载图片
		if (PANORAMA_OK != PanoramaLoadSrcImgFile(ctx,
					imgName[i], WW, HH, IMG_FMT_YUV420P_I420))
		{
			goto out;
		}

		// 处理图片
		while (1)
		{
			process = PanoramaProcess(ctx);
			if (PANORAMA_PROCESS_ERROR == process)
			{
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
				printf("process %d/100\n", process);
			}
		}

		// 获取全景图数据
		ret = PanoramaFetch(ctx, &pano, &panoBufSize, &panoW, &panoH, &panoFmt);
		if (ret != PANORAMA_OK)
		{
			printf("PanoramaFetch failed\n");
			goto out;
		}

		printf("pano.width(%d), .height(%d), panoBufSize=%d, pano=%p\n", panoW, panoH, panoBufSize, pano);

		sprintf(fn, "/home/pg/w/pano-result/fetchCombine%d.yuv", i);
		fp = fopen(fn, "w+");
		if (fp)
		{
			fwrite(pano, panoBufSize, 1, fp);
			fclose(fp);
			fp = NULL;
		}

		sprintf(fn, "/home/pg/w/pano-result/writeCombine%d.yuv", i);
		ret = PanoramaSaveToFile(ctx, fn, &panoW, &panoH, panoFmt);
		if (ret != PANORAMA_OK)
		{
			printf("PanoramaSaveToFile failed\n");
			goto out;
		}
	}

out:

	ret = PanoramaDeInit(ctx);

	return ret;
}
