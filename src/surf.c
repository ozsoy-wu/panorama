#include <stdio.h>
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
	int i, j;
#define NX 3
#define NY 3
#define NXY 4
    //const int NX=3, NY=3, NXY=4;
    const int dx_s[NX][5] = { {0, 2, 3, 7, 1}, {3, 2, 6, 7, -2}, {6, 2, 9, 7, 1} };
    const int dy_s[NY][5] = { {2, 0, 7, 3, 1}, {2, 3, 7, 6, -2}, {2, 6, 7, 9, 1} };
    const int dxy_s[NXY][5] = { {1, 1, 4, 4, 1}, {5, 1, 8, 4, -1}, {1, 5, 4, 8, -1}, {5, 5, 8, 8, 1} };

    SurfHF Dx[NX], Dy[NY], Dxy[NXY];

    if( size > sum->rows-1 || size > sum->cols-1 )
       return;

	memset(Dx, 0, NX * sizeof(SurfHF));
	memset(Dy, 0, NY * sizeof(SurfHF));
	memset(Dxy, 0, NXY * sizeof(SurfHF));

    resizeHaarPattern( dx_s , &Dx , NX , 9, size, sum->cols ); // TODO Dx ? &Dx
    resizeHaarPattern( dy_s , &Dy , NY , 9, size, sum->cols );
    resizeHaarPattern( dxy_s, &Dxy, NXY, 9, size, sum->cols );

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

int surfFeatureCompute(SURF_CFG *cfg, Image *img, Vector *kp, Vector* kpdes)
{
	PMD();
	return 0;
}

int surfFeatureDetectAndCompute(SURF_CFG *cfg, Image *img, Vector *kp, Vector *kpdes)
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

