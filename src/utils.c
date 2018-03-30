#include "utils.h"

int constructVector(Vector *vPtr, int elemSize, int capacity)
{
	if (!vPtr)
	{
		vPtr = tMalloc(Vector);
		if (!vPtr)
		{
			return PANORAMA_ERROR;
		}
		memset(vPtr, 0, sizeof(Vector));

		vPtr->selfNeedFree = 1;
	}
	else
	{
		vPtr->selfNeedFree = 0;
	}

	vPtr->elemSize = elemSize;
	vPtr->capacity = capacity;
	vPtr->size = 0;
	vPtr->elemArray = lMalloc(void, elemSize * capacity);
	if (NULL == vPtr->elemArray)
	{
		if (vPtr->selfNeedFree)
		{
			FREE(vPtr);
		}
		return PANORAMA_ERROR;
	}
	memset(vPtr->elemArray, 0, elemSize * capacity);
	vPtr->dataNeedFree = 1;

	return PANORAMA_OK;
}

int destructVector(Vector *vPtr)
{
	if (vPtr)
	{
		if (vPtr->dataNeedFree)
		{
			FREE(vPtr->elemArray);
		}
		if (vPtr->selfNeedFree)
		{
				FREE(vPtr);
		}
	}

	return PANORAMA_OK;
}

