#include <stdlib.h>
#include <stdio.h> 

#include "panorama.h"
#include "features2d.h"

#define STRINGEQ(s1, s2) (!strcmp((s1), (s2)))

#define PANO_W 600
#define PANO_H 240

int userSetK1 = 0;
int userSetOverlap = 0;
int gImgNum = 0;
int gSrcW = 1280;
int gSrcH = 720;
int gOverlap = -1;
int gStitchWidthPer = 10;
double gCamFocolLen = 4;
double gCamVA = 55.5;//56.7; // 55.5
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
			userSetOverlap = 1;
			i++;
		}
		else if (STRINGEQ(argv[i], "--stitchw"))
		{
			gStitchWidthPer = atoi(argv[i+1]);
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

static int calcK1(double *k1)
{
	int j;
	int imgW = 1280;
	int imgH = 720;
	double halfImgW = imgW / 2;
	double halfImgH = imgH / 2;

	// 原点
	Point p0;
	p0.x = 0;
	p0.y = 0;

	int vpcnt = 11;
	double vpx[100] = {41, 38, 35, 28, 26, 25, 22, 17, 14, 13, 12};
	double vpy[100] = {13, 34, 36, 115, 144, 155, 187, 259, 297, 322, 359};

	int hpcnt = 15;
	double hpx[100] = {
		30, 27, 25, 23, 20, 17, 17, 16, 16, 16, 17, 18, 22, 25, 30};
	double hpy[100] = {
		34, 70, 105, 142, 179, 216, 254, 292, 329, 367, 406, 444, 518, 592, 663};

	Point hp[100];
	Point vp[100];

	// 赋值
	int i;
	for (i = 0; i < hpcnt; i++)
	{
		hp[i].x = hpx[i] - halfImgW;
		hp[i].y = hpy[i] - halfImgH;
	}
	for (i = 0; i < vpcnt; i++)
	{
		vp[i].x = vpx[i] - halfImgW;
		vp[i].y = vpy[i] - halfImgH;
	}

	int calcCnt = 0;
	double vk1, vk2;
	double finalhk1, finalvk1, finalk1;
	double vk1Left, vk1Right;

	Point endp1, endp2;
	double p1R2, p1R4;
	double p2R2, p2R4;
	double mpR2, mpR4;
	double lineDisNumerator, p0DisNumerator;
	double lineDisDenominator;
	double xb, yb, xe, ye, xm, ym;
	double totalD, curD;
	double lastDis = -1;
	double thresh;

	// 端点赋值
	endp1.x = hp[0].x;
	endp1.y = hp[0].y;
	endp2.x = hp[hpcnt-1].x;
	endp2.y = hp[hpcnt-1].y;


	// 计算距离原点距离平方
	p1R2 = pointDisPower2(&p0, &endp1);
	p2R2 = pointDisPower2(&p0, &endp2);
	p1R4 = p1R2 * p1R2;
	p2R4 = p2R2 * p2R2;

	calcCnt = 0;
	thresh = 10;
	vk1Left = 0;
	vk1Right = 10;
	vk2 = 0; // 不计算k2
	while (vk1Left < vk1Right)
	{
		calcCnt++;
		totalD = 0.;

		vk1 = (vk1Left + vk1Right) / 2;

		// 校正后直线端点
		xb = CORRECT_COOR(endp1.x, vk1, p1R2, vk2, p1R4);
		yb = CORRECT_COOR(endp1.y, vk1, p1R2, vk2, p1R4);
		xe = CORRECT_COOR(endp2.x, vk1, p2R2, vk2, p2R4);
		ye = CORRECT_COOR(endp2.y, vk1, p2R2, vk2, p2R4);

		// 计算原点与直线的关系
		p0DisNumerator = ((ye - yb) * p0.x -
				(xe - xb) * p0.y +
				((xe - xb) * yb - (ye - yb) * xb));

		// 计算直线距离的分母
		lineDisDenominator = sqrt((ye - yb) * (ye - yb) + (xe - xb) * (xe - xb));
		for (i = 1; i < hpcnt - 1; i++)
		{

			mpR2 = pointDisPower2(&p0, &hp[i]);
			mpR4 = mpR2 * mpR2;
			xm = CORRECT_COOR(hp[i].x, vk1, mpR2, vk2, mpR4);
			ym = CORRECT_COOR(hp[i].y, vk1, mpR2, vk2, mpR4);
			//xm = hp[i].x;
			//ym = hp[i].y;
			lineDisNumerator = ((ye - yb) * xm -
				(xe - xb) * ym +
				((xe - xb) * yb - (ye - yb) * xb));
			/*
			curD = (lineDisNumerator) / lineDisDenominator;
			totalD += curD;
			*/
			curD = fabs(lineDisNumerator) / lineDisDenominator;

			if (SAME_SIDE_WITH_P0(p0DisNumerator, lineDisNumerator))
			{
				totalD -= curD;
			}
			else
			{
				totalD += curD;
			}
		}
	
		finalhk1 = vk1;
		if (fabs(totalD) <= 1e-20)
		{
			break;
		}

		if (calcCnt >= 5000)
		{
			break;
		}

		// 默认为桶形畸变
		// k1太大，导致矫正点与原点在同一侧
		if (totalD < 0)
		{
			vk1Right = vk1;
		}
		else
		{
			vk1Left = vk1;
		}
	}

	*k1 = finalhk1;

	return PANORAMA_OK;
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

	if (!userSetK1)
	{
		calcK1(&gCamK1);
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
		cfg.stitchOverlapWidth = gOverlap;
	}
	cfg.stitchInterpolationPercent = gStitchWidthPer;
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
			printf("PanoramaLoadSrcImgFile failed\n");
			goto out;
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
			//printf("total process percent: %d/100\n", totalPercent);

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

		sprintf(fn, "/home/pg/w/pano-result/pano%d_W%d_H%d_overlap%d_%d_%d.yuv", i, panoW, panoH, cfg.stitchOverlapWidth, gSrcW, gSrcH);
		fp = fopen(fn, "w+");
		if (fp)
		{
			fwrite(pano, panoBufSize, 1, fp);
			fclose(fp);
			fp = NULL;
		}
	}

	printf("finish\n");

out:

	ret = PanoramaDeInit(ctx);

	return ret;
}
