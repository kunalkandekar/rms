#include "async.h"
//QUEUE
void  syncqueue_init(syncqueue_t *pqueue, int init) {
	syncqueue_t queue;
	*pqueue = (syncqueue_t)malloc(sizeof(syncqueue));
	queue = *pqueue;
	queue->waiting=0;
	mqueue_init(&queue->queue, init);
	uthread_evt_init(&queue->evt);
	uthread_mutex_init(&queue->mutx);
}

void  syncqueue_destroy(syncqueue_t queue) {
	mqueue_free(queue->queue);
	uthread_evt_destroy(&queue->evt);
	uthread_mutex_destroy(&queue->mutx);
	free(queue);
}

int   syncqueue_size(syncqueue_t queue) {
	int ret;
	uthread_mutex_lock(&queue->mutx);
	ret = mqueue_size(queue->queue);
	uthread_mutex_unlock(&queue->mutx);
	return ret;
}

void  syncqueue_signal(syncqueue_t queue) {
	uthread_evt_signal(&queue->evt);
}

void  syncqueue_enq(syncqueue_t queue, void *data) {
	uthread_mutex_lock(&queue->mutx);
	mqueue_append(queue->queue, data);
	uthread_mutex_unlock(&queue->mutx);
}

void* syncqueue_deq(syncqueue_t queue) {
	void *data;
	uthread_mutex_lock(&queue->mutx);
	data = mqueue_remove(queue->queue);
	uthread_mutex_unlock(&queue->mutx);
	return data;
}

void  syncqueue_signal_data(syncqueue_t queue, void *data) {
	syncqueue_enq(queue, data);
	uthread_evt_signal(&queue->evt);
}

int   syncqueue_wait_count(syncqueue_t queue) {
	return uthread_evt_waitcount(&queue->evt);
}

void* syncqueue_wait_data(syncqueue_t queue) {
	if(syncqueue_size(queue) < 1) {
		uthread_evt_wait(&queue->evt);
	}
	return syncqueue_deq(queue);
}

void* syncqueue_timedwait_data(syncqueue_t queue, long timeoutms) {
	if(syncqueue_size(queue) < 1) {
		uthread_evt_timedwait(&queue->evt, timeoutms);
	}
	return syncqueue_deq(queue);
}
