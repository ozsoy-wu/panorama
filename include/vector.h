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
	int size;	/* ��ǰ��Ч���ݴ�С */
	int capacity;	/* vector����������������������ɸ������������������ǰ���� */
	int elemSize;	/* ÿ��Ԫ�ش�С���ֽڵ�λ */
	void *elemArray;	/* �洢Ԫ��λ�� */
} Vector;

#define VECTOR_AT(vecPtr, idx) ((unsigned char *)((vecPtr)->elemArray) + (vecPtr)->elemSize * (idx))

int constructVector(Vector **vPtrIn, int elemSize, int capacity);
unsigned char *vectorGetAndReserveTail(Vector *vPtr);
unsigned char *vectorPop(Vector *vPtr);
int vectorResize(Vector *vPtr, int newCapa);
int destructVector(Vector **vPtr);

#endif // __PANORAMA_VECTOR_H__

