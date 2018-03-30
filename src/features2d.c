#include <math.h>

#include "features2d.h"
#include "utils.h"

float overlap(KeyPoint *kp1, KeyPoint *kp2 )
{
	float a = kp1->size * 0.5f;
	float b = kp2->size * 0.5f;
	float a_2 = a * a;
	float b_2 = b * b;

	float c = POINT_DISTANCE(kp1->pt, kp2->pt);

	float ovrl = 0.f;

	// one circle is completely encovered by the other => no intersection points!
	if( MIN( a, b ) + c <= MAX( a, b ) )
	{
		return MIN( a_2, b_2 ) / MAX( a_2, b_2 );
	}

	if( c < a + b ) // circles intersect
	{
		float c_2 = c * c;
		float cosAlpha = ( b_2 + c_2 - a_2 ) / ( kp2->size * c );
		float cosBeta  = ( a_2 + c_2 - b_2 ) / ( kp1->size * c );
		float alpha = acos( cosAlpha );
		float beta = acos( cosBeta );
		float sinAlpha = sin(alpha);
		float sinBeta  = sin(beta);

		float segmentAreaA = a_2 * beta;
		float segmentAreaB = b_2 * alpha;

		float triangleAreaA = a_2 * sinBeta * cosBeta;
		float triangleAreaB = b_2 * sinAlpha * cosAlpha;

		float intersectionArea = segmentAreaA + segmentAreaB - triangleAreaA - triangleAreaB;
		float unionArea = (a_2 + b_2) * (float)PANORAMA_PI - intersectionArea;

		ovrl = intersectionArea / unionArea;
	}

	return ovrl;
}
