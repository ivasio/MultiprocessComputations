#pragma once

#include <pthread.h>

typedef void QueueItemData;


typedef struct QueueItem QueueItem;

struct QueueItem {

	QueueItem* next;
	QueueItemData* data;

};


typedef struct Queue {
	
	QueueItem* head;
	QueueItem* tail;

	pthread_mutex_t* lock;
	pthread_cond_t* cond;

} Queue;


Queue* queueInit ();

void enqueue (Queue* queue, QueueItemData* itemData);

QueueItemData* dequeue (Queue* queue);

void queueDestroy (Queue* queue);