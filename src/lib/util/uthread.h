#ifndef UTHREAD_H__
#define UTHREAD_H__

#include <stddef.h>
#include <stdlib.h>

#ifdef WIN32
#include <conio.h>
#include <process.h>
#include <windows.h>
#include <winbase.h>
#else

#include <sys/time.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <errno.h>
#endif

#define UTHREAD_STATUS_INIT 		0
#define UTHREAD_STATUS_STARTED 		1
#define UTHREAD_STATUS_RUNNING 		2
#define UTHREAD_STATUS_WAITING 		3
#define UTHREAD_STATUS_STOPPED 		4
#define UTHREAD_STATUS_CRASHED 		-1

#define UTHREAD_ERR_NUL_PTR		 	-10
#define UTHREAD_ERR_ALREADY_STARTED	-11
#define UTHREAD_ERR_INVALID_JOIN 	-12
#define UTHREAD_ERR_INVALID_DETACH 	-13

#ifndef WIN32
#define INFINITE -1;
#endif

#define UTHREAD_MUTEX_SUCCESS		 0
#define UTHREAD_MUTEX_TIMEDOUT		-1

#define UTHREAD_EVT_SUCCESS			 0
#define UTHREAD_EVT_TIMEDOUT		-1

/*********************** UTHREAD EVENT *********************************/

typedef struct uthread_evt
{
	int status;
#ifdef WIN32
	HANDLE mutex;
	HANDLE event;
#else
	//pthread_timestruct_t
	struct timespec 	cond_var_timeout;
	struct timeval 		now;
	pthread_cond_t 		cond_var;
	pthread_condattr_t 	cond_var_attr;
	pthread_mutex_t 	cond_var_mutex;// = PTHREAD_MUTEX_INITIALIZER;
#endif
} uthread_evt, *uthread_evt_t;

int uthread_evt_init(uthread_evt_t *evt);
int uthread_evt_destroy(uthread_evt_t *evt);

int uthread_evt_waitcount(uthread_evt_t *evt);

int uthread_evt_signal(uthread_evt_t *evt);
int uthread_evt_broadcast(uthread_evt_t *evt);

int uthread_evt_wait(uthread_evt_t *evt);
int uthread_evt_timedwait(uthread_evt_t *evt, long timeoutms);

/*********************** UTHREAD MUTEX *********************************/

typedef struct uthread_mutex
{
#ifdef WIN32
	HANDLE mutex;
#else
	int 			status;
	struct timespec cond_var_timeout;
	struct timeval 	now;
	pthread_cond_t 	cond_var;
	pthread_mutex_t cond_var_mutex;// = PTHREAD_MUTEX_INITIALIZER;

#endif
} uthread_mutex, *uthread_mutex_t;

int 	uthread_mutex_init(uthread_mutex_t *mutex);
int 	uthread_mutex_destroy(uthread_mutex_t *mutex);
int 	uthread_mutex_lock(uthread_mutex_t *mutex);
int 	uthread_mutex_lock_timed(uthread_mutex_t *mutex, int timeoutms);
int 	uthread_mutex_unlock(uthread_mutex_t *mutex);

/*********************** UTHREAD THREADS ******************************/

typedef struct uthread_param
{
	void* thread;
	void* arg;
} u_thread_param, *uthread_param_t;

typedef struct uthread
{
	//Prototype:
	//int pthread_cond_init(pthread_cond_t *cv,	const pthread_condattr_t *cattr);
	//int condition;
	uthread_evt_t 	event;
	int 			exitCode;
	void			*(*run)(void*);
#ifdef WIN32
	unsigned int	thread_id;
	HANDLE 			hThread;
#else
	pthread_attr_t 	attr;
	pthread_t		thread_id;
	pthread_key_t 	key;
	pthread_mutex_t	key_mutex;

	struct sched_param sched_param;
	int 	policy;
#endif
	//pthread_once_t once_control = PTHREAD_ONCE_INIT;
	//pthread_once(&once_control, init);

	/* initialize a condition variable to its default value */
	//ret = pthread_cond_init(&cv, NULL);
	/* initialize a condition variable */
	//ret = pthread_cond_init(&cv, &cattr);

	int 	id;

	int 	stop;

	//static pthread_mutex_t	runMutex = PTHREAD_MUTEX_INITIALIZER;;


	int 	status;
	long 	timeout;

	int		priority;


	void	*param;

	//uthread_param_t thrParam;
} uthread, *uthread_t;

int 	uthread_init(uthread_t *thread, void *(*thread_proc)(void*), int id, int priority);
int 	uthread_destroy(uthread_t *thread);
int 	uthread_start(uthread_t *thread, void *arg);
int 	uthread_start_detached(uthread_t *thread, void *arg);
int 	uthread_signal(uthread_t *thread);
int 	uthread_wait(uthread_t *thread);
int 	uthread_timedwait(uthread_t *thread, long timeoutms);
int 	uthread_join(uthread_t *thread, int *status);
int 	uthread_detach(uthread_t *thread);
int 	uthread_set_local(uthread_t *thread, void *data);
void*	uthread_get_local(uthread_t *thread);
int		uthread_set_priority(uthread_t *thread, int priority);
int 	uthread_get_priority(uthread_t *thread);

int 	uthread_sleep(long timeoutms);

#ifdef WIN32
	static unsigned int __stdcall starter(void*);
#else
	static void* starter(void*);
#endif

#endif
