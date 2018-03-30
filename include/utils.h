#ifndef __PANORAMA_UTILS_H__
#define __PANORAMA_UTILS_H__

/* fundamental constants */
#define PANORAMA_PI   3.1415926535897932384626433832795
#define PANORAMA_2PI  6.283185307179586476925286766559
#define PANORAMA_LOG2 0.69314718055994530941723212145818

#define SELF_NEED_FREE_MASK 0x1
#define DATA_NEED_FREE_MASK 0x2

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

#ifdef DEBUG_FUNC
#define PRINT(type, pEntity) ((type *)(pEntity))->print(pEntity)
#else
#define PRINT(type, pEntity)
#endif

typedef struct Vector_S
{
	int selfNeedFree;
	int dataNeedFree;
	int size;	/* 当前有效数据大小 */
	int capacity;	/* vector容量，如果容量不足以容纳更多数据则二倍增长当前容量 */
	int elemSize;	/* 每个元素大小，字节单位 */
	void *elemArray;	/* 存储元素位置 */
} Vector;

int constructVector(Vector *vPtr, int elemSize, int capacity);
int destructVector(Vector *vPtr);

#endif // __PANORAMA_UTILS_H__
