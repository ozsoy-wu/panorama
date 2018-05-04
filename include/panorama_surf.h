#ifndef __PANORAMA_SURF_H__
#define __PANORAMA_SURF_H__

#include "panorama_features2d.h"
#include "panorama_matrix.h"

typedef struct SURF_CFG_S
{
	double hessianThreshold;
	int nOctaves;
	int nOctaveLayers;
	int extended;
	int upright;
} SURF_CFG;


typedef struct SurfHF_S
{
    int p0, p1, p2, p3;
    float w;
} SurfHF;


int surfFeatureDetect(SURF_CFG *cfg, Image *img, Vector *kp);
int surfFeatureCompute(SURF_CFG *cfg, Image *img, Vector *kp, Mat **kpdes);
int surfFeatureDetectAndCompute(SURF_CFG *cfg, Image *img, Vector *kp, Mat **kpdes);


#endif // __PANORAMA_SURF_H__
