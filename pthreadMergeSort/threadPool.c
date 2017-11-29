#include <stdlib.h>
#include <pthread.h>

#include "threadPool.h"

Queue* queueInit () {

	Queue* queue = (Queue*)calloc (1, sizeof (Queue));
	
	queue->head = NULL;
	queue->tail = NULL;

	queue->lock = (pthread_mutex_t*) calloc (1, sizeof (pthread_mutex_t));
	pthread_mutex_init (queue->lock, NULL);
	queue->cond = (pthread_cond_t*) calloc (1, sizeof (pthread_cond_t));
	pthread_cond_init (queue->cond, NULL);

	return queue;	

}


void enqueue (Queue* queue, QueueItemData* newItemData) {
	
	QueueItem* newItem = (QueueItem*)calloc (1, sizeof (QueueItem));
	newItem->next = NULL;
	newItem->data = newItemData;

	pthread_mutex_lock (queue->lock);
	
	if (queue->head) // очередь непуста
		queue->head->next = newItem;
	else {
		queue->tail = newItem;
		pthread_cond_broadcast (queue->cond);
	}
	queue->head = newItem;

	pthread_mutex_unlock (queue->lock);

}


QueueItemData* dequeue (Queue* queue) {

	pthread_mutex_lock (queue->lock);

	while (!queue->tail)
		pthread_cond_wait (queue->cond, queue->lock);

	QueueItem* item = queue->tail;
	if (item) { // не конец работы
		queue->tail = queue->tail->next;
		if (!queue->tail) // удалили единтвенный элемент
			queue->head = NULL;
	}


	pthread_mutex_unlock (queue->lock);

	QueueItemData* result = item->data;	
	free (item);
	return result;

}

void queueDestroy (Queue* queue) {


}