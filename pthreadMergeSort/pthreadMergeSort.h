#pragma once

#include <stdlib.h>
#include "threadPool.h"

typedef struct ThreadArg {

	Queue* queue;
	int (*compare) (int*, int*);
	
} ThreadArg;

typedef struct Environment {

	Queue* queue;
	pthread_t* threads;
	int numThreads;
	ThreadArg* threadArg;

} Environment;

typedef struct TaskData {

	int* array;
	size_t size;

} TaskData;


void parallelMergeSort (int* array, int size, int chunkSize, int (*compare) (), int numThreads);

Environment* threadsInit (int numThreads, int (*compare) ());

void qSortAction (int* left, int* right, size_t chunkSize, Queue* queue);

void threadsFinalize (Environment* env);

void mergeAction (int* left, int* right, size_t chunkSize, int (*compare) ());

int* merge (int* left1, int* left2, int* right1, int* right2, int (*compare) (const void*, const void*));

void* consumer (void* args);