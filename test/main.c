#include <stdlib.h>
#include <stdio.h> 

#include "panorama.h"

#define STRINGEQ(s1, s2) (!strcmp((s1), (s2)))

#define PANO_W 600
#define PANO_H 240

int gImgNum = 0;
int gSrcW = 1280;
int gSrcH = 720;
int gOverlap = -1;
int gStitchWidth = 50;
double gCamFocolLen = 4;
double gCamVA = 56.7;
double gCamRA = 30;
double gCamK1 = -1;
double gCamK2 = 0;
int gPanoW = 2000;
int gPanoH = 720;

char fns[20][150] = {0};

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
        "  --stitchw <int>\n"
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
			i++;
		}
		else if (STRINGEQ(argv[i], "--stitchw"))
		{
			gStitchWidth = atoi(argv[i+1]);
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

	cfg.commonImgTotalNum = gImgNum;
	cfg.camViewingAngle = gCamVA;
	cfg.camRotateAngle = gCamRA;
	cfg.camFocalLength = gCamFocolLen;
	cfg.camDistortionK1 = gCamK1;
	cfg.camDistortionK2 = gCamK2;
	cfg.srcImgWidth = gSrcW;
	cfg.srcImgHeight = gSrcH;
	cfg.stitchOverlapWidth = gOverlap;
	cfg.stitchInterpolationWidth = gStitchWidth;
	cfg.panoImageFmt = IMG_FMT_YUV420P_I420;
	cfg.panoImageWidth = gPanoW;
	cfg.panoImageHeight = gPanoH;

	ret = PanoramaSetCfg(ctx, &cfg);
	if (ret != PANORAMA_OK)
	{
		printf("PanoramaSetCfg failed\n");
		goto out;
	}

	for (i = 0; i < gImgNum; i++)
	{
		// 加载图片
		if (PANORAMA_OK != PanoramaLoadSrcImgFile(ctx,
					fns[i], gSrcW, gSrcH, IMG_FMT_YUV420P_I420))
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

		sprintf(fn, "/home/pg/w/pano-result/pano%d_W%d_H%d_overW%d_interW%d_%d_%d.yuv", i, panoW, panoH, cfg.stitchOverlapWidth,cfg.stitchInterpolationWidth, gSrcW, gSrcH);
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
