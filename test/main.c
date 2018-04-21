#include <stdlib.h>
#include <stdio.h> 

#include "panorama.h"

#define PANO_W 600
#define PANO_H 240
#define IMG_NUM 8

int main(int argc, char **argv)
{
	int i, panoBufSize, panoW, panoH, curImgPercent, totalPercent;
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
			"/home/pg/w/IPC40_srcImg_y-10_1280_720/IPC_060_1_x-150y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_y-10_1280_720/IPC_060_2_x-120y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_y-10_1280_720/IPC_060_3_x-90y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_y-10_1280_720/IPC_060_4_x-60y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_y-10_1280_720/IPC_060_5_x-30y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_y-10_1280_720/IPC_060_6_x0y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_y-10_1280_720/IPC_060_7_x30y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_y-10_1280_720/IPC_060_8_x60y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_y-10_1280_720/IPC_060_9_x90y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_y-10_1280_720/IPC_060_10_x120y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_y-10_1280_720/IPC_060_11_x150y-10.jpg.yuv",
		};
#endif
#if 0
#define WW 1280
#define HH 720
#define OVERLAP 360
#define YANGLE 0
#define VIEWANGLE 56.7
#define ROTATEANGLE 30
	char *imgName[IMG_NUM] = {
		"/home/pg/w/IPC40-tc#1/1.jpg.yuv",
		"/home/pg/w/IPC40-tc#1/2.jpg.yuv",
		"/home/pg/w/IPC40-tc#1/3.jpg.yuv",
		"/home/pg/w/IPC40-tc#1/4.jpg.yuv",
		"/home/pg/w/IPC40-tc#1/5.jpg.yuv",
		"/home/pg/w/IPC40-tc#1/6.jpg.yuv",
		"/home/pg/w/IPC40-tc#1/7.jpg.yuv",
		"/home/pg/w/IPC40-tc#1/8.jpg.yuv",
	};
#endif

#if 0
#define WW 320
#define HH 240
#define OVERLAP 360
#define YANGLE 0
#define VIEWANGLE 56.7
#define ROTATEANGLE 30
		char *imgName[IMG_NUM] = {
			"/home/pg/w/IPC40_srcImg_y-10_320_240/IPC_060_1_x-150y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_y-10_320_240/IPC_060_2_x-120y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_y-10_320_240/IPC_060_3_x-90y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_y-10_320_240/IPC_060_4_x-60y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_y-10_320_240/IPC_060_5_x-30y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_y-10_320_240/IPC_060_6_x0y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_y-10_320_240/IPC_060_7_x30y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_y-10_320_240/IPC_060_8_x60y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_y-10_320_240/IPC_060_9_x90y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_y-10_320_240/IPC_060_10_x120y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_y-10_320_240/IPC_060_11_x150y-10.jpg.yuv",
		};
#endif
		
#if 0
#define WW 640
#define HH 480
#define OVERLAP 360
#define YANGLE 0
#define VIEWANGLE 56.7
#define ROTATEANGLE 30
		char *imgName[IMG_NUM] = {
			"/home/pg/w/IPC40_srcImg_y-10_640_480/IPC_060_1_x-150y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_y-10_640_480/IPC_060_2_x-120y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_y-10_640_480/IPC_060_3_x-90y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_y-10_640_480/IPC_060_4_x-60y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_y-10_640_480/IPC_060_5_x-30y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_y-10_640_480/IPC_060_6_x0y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_y-10_640_480/IPC_060_7_x30y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_y-10_640_480/IPC_060_8_x60y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_y-10_640_480/IPC_060_9_x90y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_y-10_640_480/IPC_060_10_x120y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_y-10_640_480/IPC_060_11_x150y-10.jpg.yuv",
		};
#endif

#if 1
#define WW 1280
#define HH 720
#define OVERLAP 360
#define YANGLE 0
#define VIEWANGLE 56.7
#define ROTATEANGLE 30
		char *imgName[IMG_NUM] = {
			"/home/pg/w/IPC40A_adjust_src/1_26.2cm.jpg.yuv",
			"/home/pg/w/IPC40A_adjust_src/2_26.5cm.jpg.yuv",
			"/home/pg/w/IPC40A_adjust_src/3_26.2cm.jpg.yuv",
			"/home/pg/w/IPC40A_adjust_src/6_26.0cm.jpg.yuv",
			"/home/pg/w/IPC40A_adjust_src/6_29.5cm.jpg.yuv",
		};
#endif
	
#if 0
#define WW 1280
#define HH 720
#define OVERLAP 360
#define YANGLE 0
#define VIEWANGLE 56.7
#define ROTATEANGLE 30
		char *imgName[IMG_NUM] = {
			"/home/pg/w/IPC40_srcImg_ipc2_y-10_1280_720/IPC_062_1_x-150y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_ipc2_y-10_1280_720/IPC_062_2_x-120y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_ipc2_y-10_1280_720/IPC_062_3_x-90y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_ipc2_y-10_1280_720/IPC_062_4_x-60y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_ipc2_y-10_1280_720/IPC_062_5_x-30y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_ipc2_y-10_1280_720/IPC_062_6_x0y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_ipc2_y-10_1280_720/IPC_062_7_x30y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_ipc2_y-10_1280_720/IPC_062_8_x60y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_ipc2_y-10_1280_720/IPC_062_9_x90y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_ipc2_y-10_1280_720/IPC_062_10_x120y-10.jpg.yuv",
			"/home/pg/w/IPC40_srcImg_ipc2_y-10_1280_720/IPC_062_11_x150y-10.jpg.yuv",
		};
#endif

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
#if 0
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


	// TODO delete
	double k1;
	//calcK1(&k1);

	int userOverlap = -1;
	if (argc > 1)
	{
		userOverlap = atoi(argv[1]);
		printf("userOverlap=%d\n", userOverlap);
	}

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

	cfg.commonImgTotalNum = IMG_NUM;
	cfg.camViewingAngle = VIEWANGLE;
	cfg.camRotateAngle = ROTATEANGLE;
	cfg.camFocalLength = 4;
	cfg.camDistortionK1 = k1 * 6;
	printf("cfg.camDistortionK1=%10.10f\n", cfg.camDistortionK1);
	cfg.camDistortionK2 = 0;
	cfg.srcImgWidth = WW;
	cfg.srcImgHeight = HH;
	if (userOverlap != -1)
		cfg.stitchOverlapWidth = userOverlap;
	cfg.stitchInterpolationWidth = 100;
	cfg.panoImageFmt = IMG_FMT_YUV420P_I420;
	cfg.panoImageWidth = PANO_W;
	cfg.panoImageHeight = PANO_H;

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

		printf("image#%d\n", i);

		// 处理图片
		while (1)
		{
			curImgPercent = PanoramaProcess(ctx);
			if (PANORAMA_PROCESS_ERROR == curImgPercent)
			{
				goto out;
			}

			// 更新当前原始图处理进度
			// printf("current image process percent: %d/100\n", curImgPercent);

			// 获取总的时间进度
			totalPercent = PanoramaProcessQuery(ctx);
			printf("total process percent: %d/100\n", totalPercent);

			if (PANORAMA_PROCESS_FINISH == curImgPercent)
			{
				// 完成
				break;
			}
		}

		// 获取全景图数据
		ret = PanoramaFetch(ctx, &pano, &panoBufSize, &panoW, &panoH, &panoFmt);
		if (ret != PANORAMA_OK)
		{
			printf("PanoramaFetch failed\n");
			goto out;
		}
		
		ret = PanoramaGetCfg(ctx, &cfg);

		sprintf(fn, "/home/pg/w/pano-result/fetchCombine%d_totalw%d_overW%d_interW%d_%d_%d.yuv", i, panoW, cfg.stitchOverlapWidth,cfg.stitchInterpolationWidth, WW, HH);
		fp = fopen(fn, "w+");
		if (fp)
		{
			fwrite(pano, panoBufSize, 1, fp);
			fclose(fp);
			fp = NULL;
		}
	}

out:

	ret = PanoramaDeInit(ctx);

	return ret;
}
