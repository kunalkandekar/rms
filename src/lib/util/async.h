#ifndef ASYNC_H__
#define ASYNC_H__

#include <stddef.h>
#include <stdlib.h>

#include "mqueue.h"
#include "uthread.h"

/*
#define PTHREAD_EVT_SUCCESS		0
#define PTHREAD_EVT_TIMEDOUT	21

//EVENT
typedef struct pthread_evt {
	int 			status;
	struct timespec cond_var_timeout;
	struct timeval 	now;
	pthread_cond_t 	cond_var;
	pthread_mutex_t cond_var_mutex;// = PTHREAD_MUTEX_INITIALIZER;
} pthread_evt, *pthread_evt_t;

int pthread_evt_init(pthread_evt_t *evt);
int pthread_evt_destroy(pthread_evt_t evt);

int pthread_evt_waitcount(pthread_evt_t evt);

int pthread_evt_signal(pthread_evt_t evt);
int pthread_evt_broadcast(pthread_evt_t evt);

int pthread_evt_wait(pthread_evt_t evt);
int pthread_evt_timedwait(pthread_evt_t evt, long timeoutms);
*/

//QUEUE
typedef struct syncqueue {
	int				waiting;
	mqueue_t	  	queue;
	uthread_evt_t 	evt;
	uthread_mutex_t mutx;
} syncqueue, *syncqueue_t;

void  syncqueue_init(syncqueue_t *queue, int init);
void  syncqueue_destroy(syncqueue_t queue);
int   syncqueue_size(syncqueue_t queue);
void  syncqueue_signal(syncqueue_t queue);

void  syncqueue_enq(syncqueue_t queue, void *data);
void* syncqueue_deq(syncqueue_t queue);

int   syncqueue_wait_count(syncqueue_t queue);

void  syncqueue_signal_data(syncqueue_t queue, void *data);
void* syncqueue_wait_data(syncqueue_t queue);
void* syncqueue_timedwait_data(syncqueue_t queue, long timeoutms);

#endif
