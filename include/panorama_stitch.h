#ifndef __PANORAMA_STITCH_H__
#define __PANORAMA_STITCH_H__

#include "panorama.h"
#include "panorama_inner.h"

#define DEFAULT_STITCH_WIDTH_PERCENT 0
#define DEFAULT_INTERPOLATION_WIDTH_PERCENT 0.1

int stitch(PANORAMA_INNER_CTX *innerCtx, int idx);

#endif //__PANORAMA_STITCH_H__

