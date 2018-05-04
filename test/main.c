/******************************************************************************
 * Copyright (c) 2015-2018 TP-Link Technologies CO.,LTD.
 *
 * 文件名称:		main.c
 * 版           本:	1.0
 * 摘           要:	全景图库接口测试函数
 * 作           者:	wupimin<wupimin@tp-link.com.cn>
 * 创建时间:		2018-04-28
 ******************************************************************************/

#include <stdlib.h>
#include <stdio.h>

#include "panorama.h"

#define STRINGEQ(s1, s2) (!strcmp((s1), (s2)))

#define PANO_W 600
#define PANO_H 240

int userSetK1 = 0;
int userSetOverlap = 0;
int gImgNum = 0;
int gSrcW = 1280;
int gSrcH = 720;
int gOverlap = -1;
double gStitchOverlapPer = 0.5;
double gInterpolationWidthPer = 0.1;
double gCamFocolLen = 4;
double gCamVA = 55.5;//56.7; // 55.5
double gCamRA = 30;
double gCamK1 = 0.00000030752116827957;
double gCamK2 = 0;
int gPanoW = 2000;
int gPanoH = 720;

char fns[20][150] = {0};

#define PMD printf("%s, %d\n", __func__, __LINE__);

static void printUsage()
{
    printf(
        "Panorama.\n"
        "./pn img1 img2 [...imgN] [flags]\n\n"
        "Src images info:\n"
        "  --width <int>\n"
        "      src image width\n"
        "  --height <int>\n"
        "      src image height\n"
        "\nStitch flags:\n"
        "  --overlap <int>\n"
        "      Adjacent image overlap width, -1 for self calc\n"
        "  --overlapPer <double>\n"
        "      Adjacent image overlap width percent\n"
        "  --interPer <double>\n"
        "      Adjacent image linear stitch width.\n"
        "\nCamera Info:\n"
        "  --focalLength <double>\n"
        "      focal length.\n"
        "  --viewAngle <double>\n"
        "      viewing angle\n"
        "  --rotateAngle <double>\n"
        "      rotate angle.\n"
        "  --k1 <float>\n"
        "      k1\n"
        "  --k2 <float>\n"
        "      k2.\n"
        "\Panorama Info:\n"
        "  --panoW <int>\n"
        "      output panorama width.\n"
        "  --panoH <int>\n"
        "      output panorama height.\n"
     );
}

static int parseCmdArgs(int argc, char** argv)
{
	int i;
	for (i = 1; i < argc; ++i)
	{
		if (STRINGEQ(argv[i], "--help") || STRINGEQ(argv[i], "-h"))
		{
			printUsage();
			return -1;
		}
		else if (STRINGEQ(argv[i], "--width"))
		{
			gSrcW = atoi(argv[i+1]);
			i++;
		}
		else if (STRINGEQ(argv[i], "--height"))
		{
			gSrcH = atoi(argv[i+1]);
			i++;
		}
		else if (STRINGEQ(argv[i], "--overlap"))
		{
			gOverlap = atoi(argv[i+1]);
			userSetOverlap = 1;
			i++;
		}
		else if (STRINGEQ(argv[i], "--overlapPer"))
		{
			gStitchOverlapPer = atof(argv[i+1]);
			i++;
		}
		else if (STRINGEQ(argv[i], "--interPer"))
		{
			gInterpolationWidthPer = atof(argv[i+1]);
			i++;
		}
		else if (STRINGEQ(argv[i], "--focalLength"))
		{
			gCamFocolLen = atof(argv[i+1]);
			i++;
		}
		else if (STRINGEQ(argv[i], "--viewAngle"))
		{
			gCamVA = atof(argv[i+1]);
			i++;
		}
		else if (STRINGEQ(argv[i], "--rotateAngle"))
		{
			gCamRA = atof(argv[i+1]);
			i++;
		}
		else if (STRINGEQ(argv[i], "--k1"))
		{
			userSetK1 = 1;
			gCamK1 = atof(argv[i+1]);
			i++;
		}
		else if (STRINGEQ(argv[i], "--k2"))
		{
			gCamK2 = atof(argv[i+1]);
			i++;
		}
		else if (STRINGEQ(argv[i], "--panoW"))
		{
			gPanoW = atoi(argv[i+1]);
			i++;
		}
		else if (STRINGEQ(argv[i], "--panoH"))
		{
			gPanoH = atoi(argv[i+1]);
			i++;
		}
		else
		{
			strcpy(fns[gImgNum], argv[i]);
			gImgNum++;
		}
	}
	return 0;
}

int main(int argc, char **argv)
{
	int i, panoBufSize, panoW, panoH, curImgPercent, totalPercent;
	int ret = PANORAMA_OK;
	IMG_FORMAT panoFmt;
	char *pano = NULL;

	int retval = parseCmdArgs(argc, argv);
	if (retval != 0)
	{
		return retval;
	}

	for (i = 0; i <gImgNum; i++)
	{
		printf("file#%d: %s\n", i, fns[i]);
	}

	int ysize, usize, vsize;
	FILE *fp = NULL;
	char fn[100] = {0};
	char * fbuf[3] = {0};
	char * bufWaitDel[150];
	int bufsize[3];
	int waitDelCnt = 0;
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

	cfg.commonImgTotalNum = gImgNum;
	cfg.camViewingAngle = gCamVA;
	cfg.camRotateAngle = gCamRA;
	cfg.camFocalLength = gCamFocolLen;
	cfg.camDistortionK1 = gCamK1;
	cfg.camDistortionK2 = gCamK2;
	cfg.srcImgWidth = gSrcW;
	cfg.srcImgHeight = gSrcH;
	if (userSetOverlap)
	{
		cfg.stitchOverlapPercent = (double)gOverlap / gSrcW;
	}
	else
	{
		cfg.stitchOverlapPercent = gStitchOverlapPer;
		gOverlap = gStitchOverlapPer * gSrcW;
	}
	cfg.stitchInterpolationPercent = gInterpolationWidthPer;
	cfg.panoImageFmt = IMG_FMT_YUV420P_I420;
	cfg.panoImageWidth = gPanoW;
	cfg.panoImageHeight = gPanoH;

	ret = PanoramaSetCfg(ctx, &cfg);
	if (ret != PANORAMA_OK)
	{
		printf("PanoramaSetCfg failed\n");
		goto out;
	}

	ysize = gSrcH * gSrcW;
	usize = vsize = (gSrcH / 2) * (gSrcW / 2);

	waitDelCnt = 0;
	for (i = 0; i < gImgNum; i++)
	{
		// 加载图片
		// 测试PanoramaLoadSrcImgFile接口
		if (i < gImgNum / 3)
		{
			if (PANORAMA_OK != PanoramaLoadSrcImgFile(ctx,
						fns[i], gSrcW, gSrcH, IMG_FMT_YUV420P_I420))
			{
				printf("PanoramaLoadSrcImgFile failed\n");
				goto out;
			}
		}
		// 测试PanoramaLoadSrcImgFile接口，copy数据到内部
		else if (i < 2 * gImgNum / 3)
		{
			int idx = 0;
			bufsize[idx] = ysize;
			fbuf[idx] = (char *)malloc(bufsize[idx] * sizeof(char));
			if (fbuf[idx] == NULL)
			{
				PMD
				goto out;
			}
			idx++;
			bufsize[idx] = usize;
			fbuf[idx] = (char *)malloc(bufsize[idx] * sizeof(char));
			if (fbuf[idx] == NULL)
			{
				PMD
				goto out;
			}
			idx++;
			bufsize[idx] = vsize;
			fbuf[idx] = (char *)malloc(bufsize[idx] * sizeof(char));
			if (fbuf[idx] == NULL)
			{
				PMD
				goto out;
			}

			fp = fopen(fns[i], "r");
			if (!fp)
			{
				PMD
				goto out;
			}
			fread(fbuf[0], bufsize[0], 1, fp);
			fread(fbuf[1], bufsize[1], 1, fp);
			fread(fbuf[2], bufsize[2], 1, fp);
			fclose(fp);
			fp = NULL;

			if (PANORAMA_OK != PanoramaLoadSrcImgBuffer(ctx, &fbuf[0],
				&bufsize, 3, gSrcW, gSrcH, IMG_FMT_YUV420P_I420, 1))
			{
				printf("PanoramaLoadSrcImgBuffer failed\n");
				goto out;
			}

			free(fbuf[0]); fbuf[0] = NULL;
			free(fbuf[1]); fbuf[1] = NULL;
			free(fbuf[2]); fbuf[2] = NULL;
		}
		// 测试PanoramaLoadSrcImgFile接口，不copy数据到内部
		else
		{
			bufsize[0] = ysize;
			bufWaitDel[waitDelCnt] = (char *)malloc(bufsize[0] * sizeof(char));
			if (bufWaitDel[waitDelCnt] == NULL)
			{
				PMD
				goto out;
			}
			waitDelCnt++;

			bufsize[1] = usize;
			bufWaitDel[waitDelCnt] = (char *)malloc(bufsize[1] * sizeof(char));
			if (bufWaitDel[waitDelCnt] == NULL)
			{
				PMD
				goto out;
			}
			waitDelCnt++;

			bufsize[2] = vsize;
			bufWaitDel[waitDelCnt] = (char *)malloc(bufsize[2] * sizeof(char));
			if (bufWaitDel[waitDelCnt] == NULL)
			{
				PMD
				goto out;
			}
			waitDelCnt++;

			fp = fopen(fns[i], "r");
			if (!fp)
			{
				PMD
				goto out;
			}
			fread(bufWaitDel[waitDelCnt - 3], bufsize[0], 1, fp);
			fread(bufWaitDel[waitDelCnt - 2], bufsize[1], 1, fp);
			fread(bufWaitDel[waitDelCnt - 1], bufsize[2], 1, fp);
			fclose(fp);
			fp = NULL;

			if (PANORAMA_OK != PanoramaLoadSrcImgBuffer(ctx, &bufWaitDel[waitDelCnt-3],
				&bufsize, 3, gSrcW, gSrcH, IMG_FMT_YUV420P_I420, 0))
			{
				printf("PanoramaLoadSrcImgBuffer failed\n");
				goto out;
			}
		}

		// 处理图片
		while (1)
		{
			curImgPercent = PanoramaProcess(ctx);
			if (PANORAMA_PROCESS_ERROR == curImgPercent)
			{
				printf("PanoramaProcess failed\n");
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

		if (i == gImgNum - 1)
		{
			ret = PanoramaGetCfg(ctx, &cfg);

			sprintf(fn, "/home/pg/w/pano-result/pano%d_W%d_H%d_overlap%d_%d_%d.yuv", i, panoW, panoH, gOverlap, gSrcW, gSrcH);
			fp = fopen(fn, "w+");
			if (fp)
			{
				fwrite(pano, panoBufSize, 1, fp);
				fclose(fp);
				fp = NULL;
			}
		}
	}

	int j = 0;
	for (j = 0; j < waitDelCnt; j++)
	{
		free(bufWaitDel[j]);
		bufWaitDel[j] = NULL;
	}

	ret = PanoramaResetCtx(ctx);

	printf("finish\n");

out:

	ret = PanoramaDeInit(ctx);

	return ret;
}
