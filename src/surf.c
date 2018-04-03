#include <stdio.h>
#include <math.h>
#include "panorama_inner.h"
#include "panorama.h"

static const int   SURF_ORI_SEARCH_INC = 5;
static const float SURF_ORI_SIGMA      = 2.5f;
static const float SURF_DESC_SIGMA     = 3.3f;

// Wavelet size at first layer of first octave.
static const int SURF_HAAR_SIZE0 = 9;

// Wavelet size increment between layers. This should be an even number,
// such that the wavelet sizes in an octave are either all even or all odd.
// This ensures that when looking for the neighbours of a sample, the layers
// above and below are aligned correctly.
static const int SURF_HAAR_SIZE_INC = 6;

inline int cvRound( double value )
{
    return (int)(value + (value >= 0 ? 0.5 : -0.5));
}

inline float calcHaarPattern( const int* origin, const SurfHF* f, int n )
{
	int k;
    double d = 0;
    for( k = 0; k < n; k++ )
        d += (origin[f[k].p0] + origin[f[k].p3] - origin[f[k].p1] - origin[f[k].p2])*f[k].w;
    return (float)d;
}


static void resizeHaarPattern( const int src[][5], SurfHF* dst, int n, int oldSize, int newSize, int widthStep )
{
	int k;
    float ratio = (float)newSize/oldSize;
    for( k = 0; k < n; k++ )
    {
        int dx1 = cvRound( ratio*src[k][0] );
        int dy1 = cvRound( ratio*src[k][1] );
        int dx2 = cvRound( ratio*src[k][2] );
        int dy2 = cvRound( ratio*src[k][3] );
        dst[k].p0 = dy1*widthStep + dx1;
        dst[k].p1 = dy2*widthStep + dx1;
        dst[k].p2 = dy1*widthStep + dx2;
        dst[k].p3 = dy2*widthStep + dx2;
        dst[k].w = src[k][4]/((float)(dx2-dx1)*(dy2-dy1));
    }
}


/*
 * Calculate the determinant and trace of the Hessian for a layer of the
 * scale-space pyramid
 */
static void calcLayerDetAndTrace( Mat* sum, int size, int sampleStep,
                                  Mat *det, Mat *trace )
{
#define CALC_DET_TRACE_NX 3
#define CALC_DET_TRACE_NY 3
#define CALC_DET_TRACE_NXY 4
	int i, j;
    const int dx_s[CALC_DET_TRACE_NX][5] = { {0, 2, 3, 7, 1}, {3, 2, 6, 7, -2}, {6, 2, 9, 7, 1} };
    const int dy_s[CALC_DET_TRACE_NY][5] = { {2, 0, 7, 3, 1}, {2, 3, 7, 6, -2}, {2, 6, 7, 9, 1} };
    const int dxy_s[CALC_DET_TRACE_NXY][5] = { {1, 1, 4, 4, 1}, {5, 1, 8, 4, -1}, {1, 5, 4, 8, -1}, {5, 5, 8, 8, 1} };

    SurfHF Dx[CALC_DET_TRACE_NX], Dy[CALC_DET_TRACE_NY], Dxy[CALC_DET_TRACE_NXY];

    if( size > sum->rows-1 || size > sum->cols-1 )
       return;

	memset(Dx, 0, CALC_DET_TRACE_NX * sizeof(SurfHF));
	memset(Dy, 0, CALC_DET_TRACE_NY * sizeof(SurfHF));
	memset(Dxy, 0, CALC_DET_TRACE_NXY * sizeof(SurfHF));

    resizeHaarPattern( dx_s , &Dx , CALC_DET_TRACE_NX , 9, size, sum->cols ); // TODO Dx ? &Dx
    resizeHaarPattern( dy_s , &Dy , CALC_DET_TRACE_NY , 9, size, sum->cols );
    resizeHaarPattern( dxy_s, &Dxy, CALC_DET_TRACE_NXY, 9, size, sum->cols );

    /* The integral image 'sum' is one pixel bigger than the source image */
    int samples_i = 1+(sum->rows-1-size)/sampleStep;
    int samples_j = 1+(sum->cols-1-size)/sampleStep;

    /* Ignore pixels where some of the kernel is outside the image */
    int margin = (size/2)/sampleStep;

    for( i = 0; i < samples_i; i++ )
    {
		const int* sum_ptr = (int *)MAT_ROW_PTR(sum, i*sampleStep);
        float* det_ptr = (float *)MAT_AT_COOR(det, i+margin, margin);

        float* trace_ptr = (float *)MAT_AT_COOR(trace, i+margin, margin);
        for( j = 0; j < samples_j; j++ )
        {
            float dx  = calcHaarPattern( sum_ptr, &Dx , 3 );
            float dy  = calcHaarPattern( sum_ptr, &Dy , 3 );
            float dxy = calcHaarPattern( sum_ptr, &Dxy, 4 );
            sum_ptr += sampleStep;

            det_ptr[j] = dx*dy - 0.81f*dxy*dxy;

            trace_ptr[j] = dx + dy;
        }
    }
}

static int interpolateKeypoint( float N9[3][9], int dx, int dy, int ds, KeyPoint *kpt )
{
	float b[3] = {-(N9[1][5]-N9[1][3])/2,  // Negative 1st deriv with respect to x
		-(N9[1][7]-N9[1][1])/2,  // Negative 1st deriv with respect to y
		-(N9[2][4]-N9[0][4])/2};

	float a[3][3] = {N9[1][3]-2*N9[1][4]+N9[1][5],            // 2nd deriv x, x
		(N9[1][8]-N9[1][6]-N9[1][2]+N9[1][0])/4, // 2nd deriv x, y
		(N9[2][5]-N9[2][3]-N9[0][5]+N9[0][3])/4, // 2nd deriv x, s
		(N9[1][8]-N9[1][6]-N9[1][2]+N9[1][0])/4, // 2nd deriv x, y
		N9[1][1]-2*N9[1][4]+N9[1][7],            // 2nd deriv y, y
		(N9[2][7]-N9[2][1]-N9[0][7]+N9[0][1])/4, // 2nd deriv y, s
		(N9[2][5]-N9[2][3]-N9[0][5]+N9[0][3])/4, // 2nd deriv x, s
		(N9[2][7]-N9[2][1]-N9[0][7]+N9[0][1])/4, // 2nd deriv y, s
		N9[0][4]-2*N9[1][4]+N9[2][4]           // 2nd deriv s, s
	};

	float x[3] = {0};

	float d = a[0][0]*(a[1][1]*a[2][2] - a[2][1]*a[1][2]) -
		a[0][1]*(a[1][0]*a[2][2] - a[2][0]*a[1][2]) +
		a[0][2]*(a[1][0]*a[2][1] - a[2][0]*a[1][1]);

	if( d == 0 )
		return PANORAMA_ERROR;

	d = 1/d;
	x[0] = d*(b[0]*(a[1][1]*a[2][2] - a[1][2]*a[2][1]) -
		a[0][1]*(b[1]*a[2][2] - a[1][2]*b[2]) +
		a[0][2]*(b[1]*a[2][1] - a[1][1]*b[2]));

	x[1] = d*(a[0][0]*(b[1]*a[2][2] - a[1][2]*b[2]) -
		b[0]*(a[1][0]*a[2][2] - a[1][2]*a[2][0]) +
		a[0][2]*(a[1][0]*b[2] - b[1]*a[2][0]));

	x[2] = d*(a[0][0]*(a[1][1]*b[2] - b[1]*a[2][1]) -
		a[0][1]*(a[1][0]*b[2] - b[1]*a[2][0]) +
		b[0]*(a[1][0]*a[2][1] - a[1][1]*a[2][0]));

    if( (x[0] != 0 || x[1] != 0 || x[2] != 0) &&
        fabs(x[0]) <= 1 && fabs(x[1]) <= 1 && fabs(x[2]) <= 1 )
    {
        kpt->pt.x += x[0]*dx;
        kpt->pt.y += x[1]*dy;
        kpt->size = (float)cvRound( kpt->size + x[2]*ds );
		return PANORAMA_OK;
    }
	else
	{
		return PANORAMA_ERROR;
	}
}

/*
 * Find the maxima in the determinant of the Hessian in a layer of the
 * scale-space pyramid
 */
static void findMaximaInLayer( Mat *sum, Mat *mask_sum,
	Mat *dets, Mat *traces,
	int *sizes, Vector *kpVecPtr,
	int octave, int layer, float hessianThreshold, int sampleStep )
{
	int i, j;
	// Wavelet Data
#define NM 1
	const int dm[NM][5] = { {0, 0, 9, 9, 1} };
	SurfHF Dm;
	KeyPoint kp;

	int size = sizes[layer];

	// The integral image 'sum' is one pixel bigger than the source image
	int layer_rows = (sum->rows-1)/sampleStep;
	int layer_cols = (sum->cols-1)/sampleStep;

	// Ignore pixels without a 3x3x3 neighbourhood in the layer above
	int margin = (sizes[layer+1]/2)/sampleStep+1;

	if( mask_sum )
		resizeHaarPattern( dm, &Dm, NM, 9, size, mask_sum->cols );

	int step = (int)(dets[layer].step/dets[layer].elemSize);

	for( i = margin; i < layer_rows - margin; i++ )
	{
		/*
		const float* det_ptr = dets[layer].ptr<float>(i);
		const float* trace_ptr = traces[layer].ptr<float>(i);
		*/
		const float* det_ptr = (const float *)MAT_ROW_PTR(&dets[layer], i);
		const float* trace_ptr = (const float *)MAT_ROW_PTR(&traces[layer], i);
		for( j = margin; j < layer_cols-margin; j++ )
		{
			float val0 = det_ptr[j];
			if( val0 > hessianThreshold )
			{
				/* Coordinates for the start of the wavelet in the sum image. There
				is some integer division involved, so don't try to simplify this
				(cancel out sampleStep) without checking the result is the same */
				int sum_i = sampleStep*(i-(size/2)/sampleStep);
				int sum_j = sampleStep*(j-(size/2)/sampleStep);

				/* The 3x3x3 neighbouring samples around the maxima.
				The maxima is included at N9[1][4] */

				/*
				const float *det1 = &dets[layer-1].at<float>(i, j);
				const float *det2 = &dets[layer].at<float>(i, j);
				const float *det3 = &dets[layer+1].at<float>(i, j);
				*/
				const float *det1 = (const float *)MAT_AT_COOR(&dets[layer - 1], i, j);
				const float *det2 = (const float *)MAT_AT_COOR(&dets[layer], i, j);
				const float *det3 = (const float *)MAT_AT_COOR(&dets[layer + 1], i, j);

				float N9[3][9] = { { det1[-step-1], det1[-step], det1[-step+1],
				     det1[-1]  , det1[0] , det1[1],
				     det1[step-1] , det1[step] , det1[step+1]  },
				   { det2[-step-1], det2[-step], det2[-step+1],
				     det2[-1]  , det2[0] , det2[1],
				     det2[step-1] , det2[step] , det2[step+1]  },
				   { det3[-step-1], det3[-step], det3[-step+1],
				     det3[-1]  , det3[0] , det3[1],
				     det3[step-1] , det3[step] , det3[step+1]  } };

				/* Check the mask - why not just check the mask at the center of the wavelet? */
				if( mask_sum )
				{
					//const int* mask_ptr = &mask_sum.at<int>(sum_i, sum_j);
					const int* mask_ptr = MAT_AT_COOR(mask_sum, sum_i, sum_j);
					float mval = calcHaarPattern( mask_ptr, &Dm, 1 );
					if( mval < 0.5 )
						continue;
				}

				/* Non-maxima suppression. val0 is at N9[1][4]*/
				if( val0 > N9[0][0] && val0 > N9[0][1] && val0 > N9[0][2] &&
					val0 > N9[0][3] && val0 > N9[0][4] && val0 > N9[0][5] &&
					val0 > N9[0][6] && val0 > N9[0][7] && val0 > N9[0][8] &&
					val0 > N9[1][0] && val0 > N9[1][1] && val0 > N9[1][2] &&
					val0 > N9[1][3]                    && val0 > N9[1][5] &&
					val0 > N9[1][6] && val0 > N9[1][7] && val0 > N9[1][8] &&
					val0 > N9[2][0] && val0 > N9[2][1] && val0 > N9[2][2] &&
					val0 > N9[2][3] && val0 > N9[2][4] && val0 > N9[2][5] &&
					val0 > N9[2][6] && val0 > N9[2][7] && val0 > N9[2][8] )
				{
					/* Calculate the wavelet center coordinates for the maxima */
					float center_i = sum_i + (size-1)*0.5f;
					float center_j = sum_j + (size-1)*0.5f;
					
					keypointAssignment(&kp, center_j, center_i, (float)sizes[layer],
						-1, val0, octave, (trace_ptr[j] > 0) - (trace_ptr[j] < 0) );

					/* Interpolate maxima location within the 3x3x3 neighbourhood  */
					int ds = size - sizes[layer-1];
					int interp_ok = interpolateKeypoint( N9, sampleStep, sampleStep, ds, &kp );

					/* Sometimes the interpolation step gives a negative size etc. */
					if(PANORAMA_OK == interp_ok)
					{
						keypointVectorPush(kpVecPtr, kp.pt.x, kp.pt.y, kp.size, kp.angle, kp.response,
							kp.octave, kp.classId);
					}
				}
			}
		}
	}
}


static int fastHessianDetector (Mat *sum, Mat *mask_sum, Vector *kpVecPtr,
                                 int nOctaves, int nOctaveLayers, float hessianThreshold)
{
	int i;
	int octave, layer;
	int col, row;
	int ret = PANORAMA_OK;

	/* Sampling step along image x and y axes at first octave. This is doubled
	for each additional octave. WARNING: Increasing this improves speed,
	however keypoint extraction becomes unreliable. */
	const int SAMPLE_STEP0 = 1;

	// Allocate space and calculate properties of each layer
	int index = 0, middleIndex = 0, step = SAMPLE_STEP0;

	int nTotalLayers = (nOctaveLayers+2)*nOctaves;
	int nMiddleLayers = nOctaveLayers*nOctaves;

	Mat dets[nTotalLayers];
	Mat traces[nTotalLayers];
	int sizes[nTotalLayers];
	int sampleSteps[nTotalLayers];
	int middleIndices[nMiddleLayers];


	for (i = 0; i < nTotalLayers; i++)
	{
		dets[i].data = NULL;
		dets[i].dataNeedFreeByMat = 0;
		traces[i].data = NULL;
		traces[i].dataNeedFreeByMat = 0;
	}

	for(octave = 0; octave < nOctaves; octave++ )
	{
		for(layer = 0; layer < nOctaveLayers+2; layer++ )
		{
			/* The integral image sum is one pixel bigger than the source image*/
			col = (sum->cols-1)/step;
			row = (sum->rows-1)/step;

			// TODO first parameter OK? 
			ret = constructMat(&dets[index], col, row, 1, sizeof(float), NULL);
			if (PANORAMA_OK != ret)
			{
				ret = PANORAMA_ERROR;
				goto exit;
			}

			ret = constructMat(&traces[index], col, row, 1, sizeof(float), NULL);
			if (PANORAMA_OK != ret)
			{
				ret = PANORAMA_ERROR;
				goto exit;
			}

			sizes[index] = (SURF_HAAR_SIZE0 + SURF_HAAR_SIZE_INC*layer) << octave;
			sampleSteps[index] = step;

			if( 0 < layer && layer <= nOctaveLayers )
				middleIndices[middleIndex++] = index;
			index++;
		}
		step *= 2;
	}

	// Calculate hessian determinant and trace samples in each layer
	/*parallel_for_( Range(0, nTotalLayers),
	SURFBuildInvoker(sum, sizes, sampleSteps, dets, traces) );
	*/
	for (i = 0; i < nTotalLayers; i++)
	{
		calcLayerDetAndTrace(sum, sizes[i], sampleSteps[i], &dets[i], &traces[i]);

		//PRINT(Mat, &dets[i]);
	}

	// Find maxima in the determinant of the hessian
	/*parallel_for_( Range(0, nMiddleLayers),
	SURFFindInvoker(sum, mask_sum, dets, traces, sizes,
	sampleSteps, middleIndices, keypoints,
	nOctaveLayers, hessianThreshold) );
	*/

	for (i = 0; i < nMiddleLayers; i++)
	{
		/*SURFFindInvoker(sum, mask_sum, dets, traces, sizes,
			sampleSteps, middleIndices, keypoints,
			nOctaveLayers, hessianThreshold)
			*/
		int layer = middleIndices[i];
		int octave = i / nOctaveLayers;
		findMaximaInLayer( sum, mask_sum, dets, traces, &sizes,
		                   kpVecPtr, octave, layer, hessianThreshold,
		                   sampleSteps[layer] );
	}

	//std::sort(keypoints.begin(), keypoints.end(), KeypointGreater());

exit:
	for (i = 0; i < nTotalLayers; i++)
	{
		destructMat(&dets[i]);
		destructMat(&traces[i]);
	}

	return ret;
}
//-------------------------- keypoint detect auxiliary end -----------------



//-------------------------- keypoint descriptor auxiliary begin -----------------
int getGaussianKernel(Mat *kernel, int n, double sigma)
{
#define SMALL_GAUSSIAN_SIZE 7
	static const float small_gaussian_tab[][SMALL_GAUSSIAN_SIZE] =
	{
		{1.f},
		{0.25f, 0.5f, 0.25f},
		{0.0625f, 0.25f, 0.375f, 0.25f, 0.0625f},
		{0.03125f, 0.109375f, 0.21875f, 0.28125f, 0.21875f, 0.109375f, 0.03125f}
	};
	const float* fixed_kernel = 0;
	int i;
	float* cf = NULL;
	double sigmaX;
	double scale2X;
	double sum;
	double x, t;

	if (n % 2 == 1 && n <= SMALL_GAUSSIAN_SIZE && sigma <= 0)
	{
		fixed_kernel = &small_gaussian_tab[n>>1];
	}

	if (!kernel)
	{
		return PANORAMA_ERROR;
	}

	cf = (float *)MAT_ROW_PTR(kernel, 0);

	sigmaX = sigma > 0 ? sigma : ((n-1)*0.5 - 1)*0.3 + 0.8;
	scale2X = -0.5/(sigmaX*sigmaX);
	sum = 0;

	for( i = 0; i < n; i++ )
	{
		x = i - (n-1)*0.5;
		t = fixed_kernel ? (double)fixed_kernel[i] : exp(scale2X*x*x);

		cf[i] = (float)t;
		sum += cf[i];
	}

	sum = 1./sum;
	for( i = 0; i < n; i++ )
	{
		cf[i] = (float)(cf[i]*sum);
	}

	return PANORAMA_OK;
}


static const float atan2_p1 = 0.9997878412794807f*(float)(180/PANORAMA_PI);
static const float atan2_p3 = -0.3258083974640975f*(float)(180/PANORAMA_PI);
static const float atan2_p5 = 0.1555786518463281f*(float)(180/PANORAMA_PI);
static const float atan2_p7 = -0.04432655554792128f*(float)(180/PANORAMA_PI);
static const float DBL_EPSILON = 0;

float fastAtan2( float y, float x )
{
	float ax = abs(x), ay = abs(y);//首先不分象限，求得一个锐角角度
	float a, c, c2;
	if( ax >= ay )
	{
		c = ay/(ax + (float)DBL_EPSILON);
		c2 = c*c;
		a = (((atan2_p7*c2 + atan2_p5)*c2 + atan2_p3)*c2 + atan2_p1)*c;
	}
	else
	{
		c = ax/(ay + (float)DBL_EPSILON);
		c2 = c*c;
		a = 90.f - (((atan2_p7*c2 + atan2_p5)*c2 + atan2_p3)*c2 + atan2_p1)*c;
	}
	if( x < 0 )//锐角求出后，根据x和y的正负性确定向量的方向，即角度
		a = 180.f - a;
	if( y < 0 )
		a = 360.f - a;
	return a;
}


int fastAtan(float *x, float *y, float *angle, int n, int angleInDegrees)
{
	int i;
	for (i = 0; i < n; i++)
	{
		angle[i] = fastAtan2(y[i], x[i]);
	}
}


//-------------------------- keypoint descriptor auxiliary end -----------------



int surfFeatureDetect(SURF_CFG *cfg, Image *img, Vector *kp)
{
	int ret = PANORAMA_OK;
	Mat srcImg;
	Mat sumImg;

	if (!cfg || !img)
	{
		Log(LOG_DEBUG, "parameter invalid!\n");
		return PANORAMA_ERROR;
	}

	if (cfg->hessianThreshold < 0 ||
		cfg->nOctaves <= 0 ||
		cfg->nOctaveLayers <= 0)
	{
		Log(LOG_DEBUG, "surf configuration invalid!\n");
		return PANORAMA_ERROR;
	}

	ret = constructMat(&srcImg, img->w, img->h, 1, sizeof(unsigned char), img->data[0]);
	if (ret != PANORAMA_OK)
	{
		goto out;
	}

	ret = constructMat(&sumImg, img->w + 1, img->h + 1, 1, sizeof(int), NULL);
	if (ret != PANORAMA_OK)
	{
		goto out;
	}

	integral (&srcImg, &sumImg);
	fastHessianDetector(&sumImg, &sumImg, kp, cfg->nOctaves, cfg->nOctaveLayers, (float)cfg->hessianThreshold);

out:
	destructMat(&srcImg);
	destructMat(&sumImg);

	return ret;
}

int surfFeatureCompute(SURF_CFG *cfg, Image *img, Vector *kp, Mat **kpdes)
{
	//enum { ORI_RADIUS = 6, ORI_WIN = 60, PATCH_SZ = 20 };
#define ORI_RADIUS 6
#define ORI_WIN 60
#define PATCH_SZ 20
#define nOriSampleBound ((2*ORI_RADIUS+1)*(2*ORI_RADIUS+1))

	if (!cfg || !img || !kp)
	{
		Log(LOG_DEBUG, "parameter invalid!\n");
		return PANORAMA_ERROR;
	}

	int i, j;
	int ret = PANORAMA_OK;
	int N = kp->size;
	int desCols = cfg->extended ? 128 : 64;

	if (N <= 0)
	{
		return PANORAMA_ERROR;
	}

	if (NULL == *kpdes)
	{
		if (PANORAMA_ERROR == constructMat(*kpdes, desCols, N, 1, sizeof(double), NULL))
		{
			return PANORAMA_ERROR;
		}
	}

	// init begin
	//const int nOriSampleBound = (2*ORI_RADIUS+1)*(2*ORI_RADIUS+1);
	int nOriSamples = 0;
	Vector apt;
	if (PANORAMA_OK != constructVector(&apt, sizeof(Point), nOriSampleBound))
	{
		ret = PANORAMA_ERROR;
		goto cleanup;
	}

	float aptw[nOriSampleBound];
	float DW[PATCH_SZ*PATCH_SZ];

	Point *curPt = NULL;
	Mat G_ori;
	Mat G_desc;

	constructMat(&G_ori, 1, 2*ORI_RADIUS+1, 1, sizeof(float), NULL);

	/* Coordinates and weights of samples used to calculate orientation */
	getGaussianKernel(&G_ori, 2*ORI_RADIUS+1, SURF_ORI_SIGMA);
	nOriSamples = 0;
	for( i = -ORI_RADIUS; i <= ORI_RADIUS; i++ )
	{
		for( j = -ORI_RADIUS; j <= ORI_RADIUS; j++ )
		{
			if( i*i + j*j <= ORI_RADIUS*ORI_RADIUS )
			{
				curPt = (Point *)VECTOR_AT(&apt, nOriSamples);
				curPt->x = i;
				curPt->y = j;

				aptw[nOriSamples] = *(float *)MAT_AT_COOR(&G_ori, i+ORI_RADIUS, 0) * *(float *)MAT_AT_COOR(&G_ori, j+ORI_RADIUS, 0);

				nOriSamples++;
			}
		}
	}

	if ( nOriSamples > nOriSampleBound )
	{
		ret = PANORAMA_ERROR;
		goto cleanup;
	}

	float *dwP = NULL;
	float *gdesP0 = NULL;
	float *gdesP1 = NULL;
	constructMat(&G_desc, 1, PATCH_SZ, 1, sizeof(float), NULL);
	for(i = 0; i < PATCH_SZ; i++ )
	{
		for(j = 0; j < PATCH_SZ; j++)
		{
			gdesP0 = (float *)MAT_AT_COOR(&G_desc, i, 0);
			gdesP1 = (float *)MAT_AT_COOR(&G_desc, j, 0);

			DW[i*PATCH_SZ+j] = *gdesP0 * *gdesP1;
		}
	}
	// init end


	/* X and Y gradient wavelet data */
#define FEATURE_COMPUTE_NX 2
#define FEATURE_COMPUTE_NY 2
	const int dx_s[FEATURE_COMPUTE_NX][5] = {{0, 0, 2, 4, -1}, {2, 0, 4, 4, 1}};
	const int dy_s[FEATURE_COMPUTE_NY][5] = {{0, 0, 4, 2, 1}, {0, 2, 4, 4, -1}};
	KeyPoint *curkp = NULL;
	float maxSize = 0;
	int imaxSize = 0;
    float X[nOriSampleBound], Y[nOriSampleBound], angle[nOriSampleBound];
	//unsigned char PATCH[PATCH_SZ+1][PATCH_SZ+1];
	float DX[PATCH_SZ][PATCH_SZ], DY[PATCH_SZ][PATCH_SZ];
	Mat _patch;
	constructMat(&_patch, PATCH_SZ+1, PATCH_SZ+1, 1, sizeof(unsigned char), NULL);
	for (i = 0; i < N; i++)
	{
		curkp = (KeyPoint *)VECTOR_AT(kp, i);
		maxSize = MAX(maxSize, curkp->size);
	}

	imaxSize = MAX(ceil((PATCH_SZ+1)*maxSize*1.2f/9.0f), 1);

	int k, kk, nangle, x, y;
	float vx, vy;
	Point *curPoint = NULL;
	Mat srcImg;
	Mat sumImg;

	ret = constructMat(&srcImg, img->w, img->h, 1, sizeof(unsigned char), img->data[0]);
	if (ret != PANORAMA_OK)
	{
		goto cleanup;
	}

	ret = constructMat(&sumImg, img->w + 1, img->h + 1, 1, sizeof(int), NULL);
	if (ret != PANORAMA_OK)
	{
		goto cleanup;
	}

	integral(&srcImg, &sumImg); // TODO, 这里可以复用detect函数的结果
	for (k = 0; k < N; k++)
	{
		float *vec;
		SurfHF dx_t[FEATURE_COMPUTE_NX], dy_t[FEATURE_COMPUTE_NY];
		curkp = (KeyPoint *)VECTOR_AT(kp, k);
		float size = kp->size;
		float s = size*1.2f/9.0f;
		/* To find the dominant orientation, the gradients in x and y are
		 * sampled in a circle of radius 6s using wavelets of size 4s.
		 * We ensure the gradient wavelet size is even to ensure the
		 * wavelet pattern is balanced and symmetric around its center 
		 */
		int grad_wav_size = 2*cvRound( 2*s );

		if (sumImg.rows < grad_wav_size || sumImg.cols < grad_wav_size)
		{
			/* when grad_wav_size is too big,
			 * the sampling of gradient will be meaningless
			 * mark keypoint for deletion.
			 */
			curkp->size = -1;
			continue;
		}

		float descriptor_dir = 360.f - 90.f;

		if ( 0 == cfg->upright )
		{
			resizeHaarPattern( dx_s, &dx_t, FEATURE_COMPUTE_NX, 4, grad_wav_size, sumImg.cols );
			resizeHaarPattern( dy_s, &dy_t, FEATURE_COMPUTE_NY, 4, grad_wav_size, sumImg.cols );

			for (kk = 0, nangle = 0; kk <nOriSamples; kk++)
			{
				curPoint = (Point *)VECTOR_AT(&apt, kk);
				x = cvRound( curkp->pt.x + curPoint->x*s - (float)(grad_wav_size-1)/2 );
				y = cvRound( curkp->pt.y + curPoint->y*s - (float)(grad_wav_size-1)/2 );
				if( y < 0 || y >= sumImg.rows - grad_wav_size ||
					x < 0 || x >= sumImg.cols - grad_wav_size )
				{
					continue;
				}

				const int* ptr = (const int *)MAT_AT_COOR(&sumImg, y, x);
				vx = calcHaarPattern( ptr, &dx_t, 2 );
				vy = calcHaarPattern( ptr, &dy_t, 2 );
				X[nangle] = vx*aptw[kk];
				Y[nangle] = vy*aptw[kk];
				nangle++;
			}

			if (nangle == 0)
			{
				/* No gradient could be sampled because the keypoint is too
				 * near too one or more of the sides of the image. As we
				 * therefore cannot find a dominant direction, we skip this
				 * keypoint and mark it for later deletion from the sequence.
				 */
				curkp->size = -1;
				continue;
			}

			// TODO 计算角度
			// phase( Mat(1, nangle, CV_32F, X), Mat(1, nangle, CV_32F, Y), Mat(1, nangle, CV_32F, angle), true );
			fastAtan(X, Y, angle, nangle, 1);
		
			float bestx = 0, besty = 0, descriptor_mod = 0;
			for( i = 0; i < 360; i += SURF_ORI_SEARCH_INC )
			{
				float sumx = 0, sumy = 0, temp_mod;
				for( j = 0; j < nangle; j++ )
				{
					int d = abs(cvRound(angle[j]) - i);
					if( d < ORI_WIN/2 || d > 360-ORI_WIN/2 )
					{
						sumx += X[j];
						sumy += Y[j];
					}
				}
				temp_mod = sumx*sumx + sumy*sumy;
				if( temp_mod > descriptor_mod )
				{
					descriptor_mod = temp_mod;
					bestx = sumx;
					besty = sumy;
				}
			}
			descriptor_dir = fastAtan2( -besty, bestx );
		}

		curkp->angle = descriptor_dir;

		/* Extract a window of pixels around the keypoint of size 20s */
		int win_size = (int)((PATCH_SZ+1)*s);
		if (imaxSize < win_size)
		{
			continue;
		}

		Mat win;
		constructMat(&win, win_size, win_size, 1, sizeof(float), NULL);

		if( 0 == cfg->upright )
		{
			descriptor_dir *= (float)(PANORAMA_PI/180);
			float sin_dir = -sin(descriptor_dir);
			float cos_dir =  cos(descriptor_dir);

			/* Subpixel interpolation version (slower). Subpixel not required since
			the pixels will all get averaged when we scale down to 20 pixels */
			/*
			float w[] = { cos_dir, sin_dir, center.x,
			-sin_dir, cos_dir , center.y };
			CvMat W = cvMat(2, 3, CV_32F, w);
			cvGetQuadrangleSubPix( img, &win, &W );
			*/

			float win_offset = -(float)(win_size-1)/2;
			float start_x = curkp->pt.x + win_offset*cos_dir + win_offset*sin_dir;
			float start_y = curkp->pt.y - win_offset*sin_dir + win_offset*cos_dir;
			unsigned char* WIN = win.data;

			int ncols1 = img->cols-1, nrows1 = img->rows-1;
			size_t imgstep = img->step;
			for( i = 0; i < win_size; i++, start_x += sin_dir, start_y += cos_dir )
			{
				double pixel_x = start_x;
				double pixel_y = start_y;
				for( j = 0; j < win_size; j++, pixel_x += cos_dir, pixel_y -= sin_dir )
				{
					int ix = floorf(pixel_x), iy = floorf(pixel_y);
					if( (unsigned)ix < (unsigned)ncols1 &&
					(unsigned)iy < (unsigned)nrows1 )
					{
						float a = (float)(pixel_x - ix), b = (float)(pixel_y - iy);
						unsigned char* imgptr = (unsigned char*)MAT_AT_COOR(srcImg, iy, ix);
						WIN[i*win_size + j] = (unsigned char)cvRound(imgptr[0]*(1.f - a)*(1.f - b) +
							imgptr[1]*a*(1.f - b) +
							imgptr[imgstep]*(1.f - a)*b +
							imgptr[imgstep+1]*a*b);
					}
					else
					{
						int x = MIN(MAX(cvRound(pixel_x), 0), ncols1);
						int y = MIN(MAX(cvRound(pixel_y), 0), nrows1);
						WIN[i*win_size + j] = *(unsigned char*)MAT_AT_COOR(srcImg, y, x);
					}
				}
			}

		}
		else
		{
			// extract rect - slightly optimized version of the code above
			// TODO: find faster code, as this is simply an extract rect operation,
			//       e.g. by using cvGetSubRect, problem is the border processing
			// descriptor_dir == 90 grad
			// sin_dir == 1
			// cos_dir == 0

			float win_offset = -(float)(win_size-1)/2;
			int start_x = cvRound(curkp->pt.x + win_offset);
			int start_y = cvRound(curkp->pt.y - win_offset);
			unsigned char* WIN = win.data;
			for( i = 0; i < win_size; i++, start_x++ )
			{
				int pixel_x = start_x;
				int pixel_y = start_y;
				for( j = 0; j < win_size; j++, pixel_y-- )
				{
					int x = MAX( pixel_x, 0 );
					int y = MAX( pixel_y, 0 );
					x = MIN( x, srcImg->cols-1 );
					y = MIN( y, srcImg->rows-1 );
					WIN[i*win_size + j] = *(unsigned char*)MAT_AT_COOR(srcImg, y, x);
				}
			}
		}
		// Scale the window to size PATCH_SZ so each pixel's size is s. This
		// makes calculating the gradients with wavelets of size 2s easy
		resize(win, _patch, _patch.size(), 0, 0, INTER_AREA);
	}


cleanup:
	destructMat(&sumImg);
	destructMat(&srcImg);
	destructMat(&_patch);
	destructMat(&G_desc);
	destructMat(&G_ori);
	destructVector(&apt);

	return ret;


	
#if 0
	int i, j, N = (int)keypoints.size();
	if( N > 0 )
	{
		Mat descriptors;
		bool _1d = false;
		int dcols = extended ? 128 : 64;
		size_t dsize = dcols*sizeof(float);

		if( doDescriptors )
		{
			_1d = _descriptors.kind() == _InputArray::STD_VECTOR && _descriptors.type() == CV_32F;
			if( _1d )
			{
				_descriptors.create(N*dcols, 1, CV_32F);
				descriptors = _descriptors.getMat().reshape(1, N);
			}
			else
			{
				_descriptors.create(N, dcols, CV_32F);
				descriptors = _descriptors.getMat();
			}
		}

		// we call SURFInvoker in any case, even if we do not need descriptors,
		// since it computes orientation of each feature.
		parallel_for_(Range(0, N), SURFInvoker(img, sum, keypoints, descriptors, extended, upright) );

		// remove keypoints that were marked for deletion
		for( i = j = 0; i < N; i++ )
		{
			if( keypoints[i].size > 0 )
			{
				if( i > j )
				{
					keypoints[j] = keypoints[i];
					if( doDescriptors )
						memcpy( descriptors.ptr(j), descriptors.ptr(i), dsize);
				}
				j++;
			}
		}
		if( N > j )
		{
			N = j;
			keypoints.resize(N);
			if( doDescriptors )
			{
				Mat d = descriptors.rowRange(0, N);
				if( _1d )
					d = d.reshape(1, N*dcols);
				d.copyTo(_descriptors);
			}
		}
	}
#endif
}

int surfFeatureDetectAndCompute(SURF_CFG *cfg, Image *img, Vector *kp, Mat **kpdes)
{
	int ret = PANORAMA_OK;
	Mat srcImg;
	Mat sumImg;

	if (!cfg || !img)
	{
		Log(LOG_DEBUG, "parameter invalid!\n");
		return PANORAMA_ERROR;
	}

	if (cfg->hessianThreshold < 0 ||
		cfg->nOctaves <= 0 ||
		cfg->nOctaveLayers <= 0)
	{
		Log(LOG_DEBUG, "surf configuration invalid!\n");
		return PANORAMA_ERROR;
	}

	ret = constructMat(&srcImg, img->w, img->h, 1, sizeof(unsigned char), img->data[0]);
	if (ret != PANORAMA_OK)
	{
		goto out;
	}

	ret = constructMat(&sumImg, img->w + 1, img->h + 1, 1, sizeof(int), NULL);
	if (ret != PANORAMA_OK)
	{
		goto out;
	}

	integral (&srcImg, &sumImg);
	fastHessianDetector(&sumImg, &sumImg, kp, cfg->nOctaves, cfg->nOctaveLayers, (float)cfg->hessianThreshold);

out:
	destructMat(&srcImg);
	destructMat(&sumImg);

	return ret;
}

