#ifndef __PANORAMA_UTILS_H__
#define __PANORAMA_UTILS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* fundamental constants */
#define PANORAMA_PI   3.1415926535897932384626433832795
#define PANORAMA_2PI  6.283185307179586476925286766559
#define PANORAMA_LOG2 0.69314718055994530941723212145818

//#define GET_BIT(v, bitmask) ((v)&(bitmask))
//#define SET_BIT(v, bitmask) ((v)|(bitmask))

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define MALLOC(cnt, type) malloc((cnt) * sizeof(type))

/* malloc memmory with length, and convert to specif type. */
#define lMalloc(type, len) ( (type *)malloc(len))

/* malloc memmory with 'sizeof(type)', and convert to specif type. */
#define tMalloc(type) ( (type *)malloc(sizeof(type)) )

#define FREE(p) do {\
	if (p) { free(p); (p) = NULL; } \
} while (0)

#define FLOAT_EQUAL(a, b) (fabs((a) - (b)) <= 1e-5)
#define ANGLE2RADIAN(a) (PANORAMA_2PI * (a) / 360)

int cvRound( double value );

#endif // __PANORAMA_UTILS_H__
