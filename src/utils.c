#include "utils.h"
#include "panorama.h"

int constructVector(Vector *vPtr, int elemSize, int capacity)
{
	int totalSize = 0;

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

	if (-1 == capacity)
	{
		capacity = DEF_VECTOR_INIT_CAPACITY;
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

unsigned char *vectorGetAndReserveTail(Vector *vPtr)
{
	unsigned char *res = NULL;
	unsigned char *tmp = NULL;
	int oldSize = 0;
	int newSize = 0;

	if (!vPtr)
	{
		return NULL;
	}

	if (vPtr->size >= vPtr->capacity)
	{
		if (vPtr->capacity <= MAX_VECTOR_CAPACITY/2)
		{
			oldSize = vPtr->capacity * vPtr->elemSize;
			newSize = oldSize<<1;
			tmp = (unsigned char *)realloc(vPtr->elemArray, newSize); 
			if (!tmp)
			{
				return NULL;
			}
			memset((unsigned char *)vPtr->elemArray + oldSize,
					0, newSize - oldSize);
			vPtr->elemArray = tmp;
			(vPtr->capacity)<<1;
		}
		else
		{
			return NULL;
		}
	}

	res = (unsigned char *)vPtr->elemArray + vPtr->size * vPtr->elemSize;
	vPtr->size++;

	return res;
}

unsigned char *vectorPop(Vector *vPtr)
{
	unsigned char *res = NULL;

	if (!vPtr || vPtr->size < 1)
	{
		return NULL;
	}

	res = (unsigned char *)vPtr->elemArray + (vPtr->size - 1) * vPtr->elemSize;	
	vPtr->size--;

	return res;
}

int vectorResize(Vector *vPtr, int newCapa)
{
	int oldSize, newSize;
	int oldCapa;
	unsigned char *tmp = NULL;

	if (!vPtr)
	{
		return PANORAMA_ERROR;
	}

	oldSize = vPtr->capacity * vPtr->elemSize;
	newSize = newCapa * vPtr->elemSize;

	if (newSize > oldSize)
	{
		tmp = (unsigned char *)realloc(vPtr->elemArray, newSize); 
		if (!tmp)
		{
			return PANORAMA_ERROR;
		}
		memset((unsigned char *)vPtr->elemArray + oldSize,
				0, newSize - oldSize);
		vPtr->elemArray = tmp;
		vPtr->capacity = newCapa;
	}
	else
	{
		// TODO decrease vector size
		vPtr->capacity = newCapa;
	}

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

