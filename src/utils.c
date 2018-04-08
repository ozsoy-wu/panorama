#include "log.h"
#include "utils.h"
#include "panorama.h"

int cvRound( double value )
{
    return (int)(value + (value >= 0 ? 0.5 : -0.5));
}

