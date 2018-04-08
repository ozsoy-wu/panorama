#ifndef __PANORAMA_VECTOR_H__
#define __PANORAMA_VECTOR_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEF_VECTOR_INIT_CAPACITY 8
#define MAX_VECTOR_CAPACITY 2048

typedef struct Vector_S
{
	int selfNeedFree;
	int dataNeedFree;
	int size;	/* 当前有效数据大小 */
	int capacity;	/* vector容量，如果容量不足以容纳更多数据则二倍增长当前容量 */
	int elemSize;	/* 每个元素大小，字节单位 */
	void *elemArray;	/* 存储元素位置 */
} Vector;

#define VECTOR_AT(vecPtr, idx) ((unsigned char *)((vecPtr)->elemArray) + (vecPtr)->elemSize * (idx))

int constructVector(Vector **vPtrIn, int elemSize, int capacity);
unsigned char *vectorGetAndReserveTail(Vector *vPtr);
unsigned char *vectorPop(Vector *vPtr);
int vectorResize(Vector *vPtr, int newCapa);
int destructVector(Vector **vPtr);

#endif // __PANORAMA_VECTOR_H__

