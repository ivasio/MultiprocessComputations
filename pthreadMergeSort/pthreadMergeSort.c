#include <stdlib.h>
#include <string.h>

#include "threadPool.h"
#include "pthreadMergeSort.h"


void parallelMergeSort (int* array, int size, int chunkSize, int (*compare) (), int numThreads) {

	Environment* env = threadsInit (numThreads, compare);

	qSortAction (array, array + size - 1, chunkSize, env->queue);
	threadsFinalize (env);

	mergeAction (array, array + size - 1, chunkSize, compare);

}



Environment* threadsInit (int numThreads, int (*compare) ()) {

	pthread_t* threads = (pthread_t*)calloc (numThreads, sizeof (pthread_t));
	
	Queue* queue = queueInit ();
	ThreadArg* threadArg = (ThreadArg*)calloc (1, sizeof (ThreadArg));

	threadArg->queue = queue;
	threadArg->compare = compare;

	for (int i = 0; i < numThreads; i++) {
		pthread_create (threads + i, NULL, consumer, (void*)threadArg);
	}

	Environment* env = (Environment*)calloc (1, sizeof(Environment));
	env->queue = queue;
	env->threads = threads;
	env->numThreads = numThreads;
	env->threadArg = threadArg;

	return env;
}


void qSortAction (int* left, int* right, size_t chunkSize, Queue* queue) {

	if (right - left > chunkSize) {
		

		int* mid = left + (right - left) / 2; 

		qSortAction (left, mid, chunkSize, queue);
		qSortAction (mid + 1, right, chunkSize, queue);



	} else {

		TaskData* task = calloc (1, sizeof (TaskData));
		task->array = left;
		task->size = right - left + 1;

		

		enqueue (queue, (QueueItemData*)task);
	}

}


void threadsFinalize (Environment* env) {
	
	for (int i = 0; i < env->numThreads; i++)
		enqueue (env->queue, NULL);

	void* status;
	for (int i = 0; i < env->numThreads; i++)
		pthread_join (env->threads[i], &status);

	pthread_mutex_destroy (env->queue->lock);
	free (env->queue->lock);
	pthread_cond_destroy (env->queue->cond);
	free (env->queue->cond);
	free (env->queue);
	free (env->threadArg);
	free (env->threads);
	free (env);
}


void mergeAction (int* left, int* right, size_t chunkSize, int (*compare) ()) {

	int partSize = right - left + 1;
	if (partSize > chunkSize) {

		int* left1 = left;
		int* right1 = left + (right - left) / 2; 
		
		int* left2 = right1 + 1;
		int* right2 = right; 
		
		mergeAction (left1, right1, chunkSize, compare);
		mergeAction (left2, right2, chunkSize, compare);


		int* buffer = (int*)calloc (partSize, sizeof (int));
		int* newArray = buffer;

		while (left1 <= right1 && left2 <= right2) {
			if (compare (left1, left2) < 0) {
				*newArray = *left1;
				left1++;
			} else {
				*newArray = *left2;
				left2++;
			}
			newArray++;
		}	


		if (left1 > right1) 
			while (left2 <= right2) {
				*newArray = *left2;
				left2++;
				newArray++;
			}
		else
			while (left1 <= right1) {
				*newArray = *left1;
				left1++;
				newArray++;
			}



		memcpy (left, buffer, partSize * sizeof (int));
		free (buffer);
	}

}



void* consumer (void* args) { // вызывается ptread_create'ом

	Queue* queue = ((ThreadArg*)args)->queue;
	int (*compare) () = ((ThreadArg*)args)->compare;

	TaskData* data;
	while (data = dequeue (queue)) { // получил следующие данные
		qsort (data->array, data->size, sizeof (int), compare);
		free (data);
	}

}