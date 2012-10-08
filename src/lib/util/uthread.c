#include "uthread.h"

/*void uthread_param_init(uthread_paramt_t *param, void *arg);
{
	(*param) = (uthread_param_t) malloc(sizeof(uthread_param));
	(*param)->arg = arg;
};
*/
/************************ THREAD IMPLEMENTATION ****************************************/

/******** CTOR *********/

int uthread_init(	uthread_t *thread,
					void* (*thread_proc)(void*),
					int id,
					int priority)
{
	(*thread) = (uthread_t) malloc(sizeof(uthread));
	(*thread)->exitCode=0;
	//(*thread)->id = id;
	(*thread)->stop=0;
	(*thread)->priority = priority;
	(*thread)->status = UTHREAD_STATUS_INIT;
	//(*thread)->param=param;
	(*thread)->run = thread_proc;

	uthread_evt_init(&(*thread)->event);

	if(priority>0)
		uthread_set_priority(thread, priority);
#ifdef WIN32
	//
#else
	pthread_attr_init(&(*thread)->attr);
	pthread_mutex_init(&(*thread)->key_mutex, NULL);
	pthread_key_create(&(*thread)->key, NULL);
#endif
	return 0;
}

/******** DTOR *********/

int uthread_destroy(uthread_t *thread)
{
	int ret =0;
	int status=0;
#ifdef WIN32
	//nothing to be done
#else
	pthread_attr_destroy(&(*thread)->attr);
	pthread_mutex_destroy(&(*thread)->key_mutex);
	pthread_key_delete((*thread)->key);

	//pthread_exit(&status);
#endif
	uthread_evt_destroy(&(*thread)->event);
	free((*thread));
	(*thread) = NULL;
	return 0;
}

/******** START *********/

int 	uthread_start(uthread_t *thread, void *arg)
{
#ifdef WIN32
	DWORD ret =0;
	if((*thread)->status!=UTHREAD_STATUS_INIT)
	{
		return UTHREAD_ERR_ALREADY_STARTED;
	}

	(*thread)->param = arg;

	(*thread)->hThread =  CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)(*thread)->run, (*thread)->param, 0, &(*thread)->thread_id);
	if((*thread)->hThread==NULL)
	{
		return -1;
	}
	else
	{
		//(*thread)->hThread = (HANDLE)ret;
		(*thread)->status = UTHREAD_STATUS_RUNNING;
	}
	ret =0;
#else
	int ret = 0;
	if((*thread)->status!=UTHREAD_STATUS_INIT)
		return UTHREAD_ERR_ALREADY_STARTED;

	(*thread)->param = arg;

	if(ret==0)
		ret = pthread_attr_setscope(&(*thread)->attr, PTHREAD_SCOPE_PROCESS);

	if(ret==0)
	{
		ret = pthread_create(&(*thread)->thread_id, &(*thread)->attr, (*thread)->run, (*thread)->param);
		(*thread)->status = UTHREAD_STATUS_RUNNING;
	}

#endif

	return ret;
}

int		uthread_start_detached(uthread_t *thread, void *arg)
{

#ifdef WIN32
	DWORD ret =0;

	if((*thread)->status!=UTHREAD_STATUS_INIT)
		return UTHREAD_ERR_ALREADY_STARTED;

	(*thread)->param = arg;

	(*thread)->hThread =  CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)(*thread)->run, (*thread)->param, 0, &(*thread)->thread_id);
	if((*thread)->hThread == NULL)
		return -1;
	ret =0;
#else
	int ret=0;

	if((*thread)->status!=UTHREAD_STATUS_INIT)
		return UTHREAD_ERR_ALREADY_STARTED;

	(*thread)->param = arg;

	ret = pthread_attr_setinheritsched(&(*thread)->attr,PTHREAD_INHERIT_SCHED);

	if(ret==0)
		ret = pthread_attr_setscope(&(*thread)->attr, PTHREAD_SCOPE_PROCESS);

	if(ret==0)
		ret = pthread_attr_setdetachstate(&(*thread)->attr,PTHREAD_CREATE_DETACHED);

	/* start thread */
	if(ret==0)
		ret = pthread_create(&(*thread)->thread_id, &(*thread)->attr, (*thread)->run, (*thread)->param);
#endif
	return ret;
}

int		uthread_join(uthread_t *thread, int* status)
{
	int ret=-1;
#ifdef WIN32
	return WaitForSingleObject((*thread)->hThread, INFINITE);
#else
	if(!pthread_equal(pthread_self(), (*thread)->thread_id))
		ret = pthread_join((*thread)->thread_id, (void**)status);
	return ret;
#endif
}

int		uthread_detach(uthread_t *thread)
{
	int ret=-1;
#ifdef WIN32
	return 0;
#else
	if(!pthread_equal(pthread_self(), (*thread)->thread_id))
		ret = pthread_detach((*thread)->thread_id);
	return ret;
#endif
}

/******** SIGNALLING *********/

int 	uthread_signal(uthread_t *thread)
{
	return uthread_evt_signal(&(*thread)->event);
}

int	uthread_wait(uthread_t *thread)
{
	return uthread_evt_wait(&(*thread)->event);
}

int	uthread_timedwait(uthread_t *thread, long msec)
{
	return uthread_evt_timedwait(&(*thread)->event, msec);
}

int		uthread_kill(uthread_t *thread, int sig)
{
	//return pthread_kill(thread_id, sig);
	return 0;
}

int		uthread_stop(uthread_t *thread)
{
	(*thread)->stop = 1;
	return 0;
}


/******** SETTING AND GETTING *********/

int		uthread_set_local(uthread_t *thread, void* value)
{
	int ret=0;
#ifdef WIN32
	//
/*
#include <stdio.h>
#include <windows.h>

#define THREADCOUNT 4
DWORD dwTlsIndex;

VOID ErrorExit(LPTSTR);

VOID CommonFunc(VOID)
{
   LPVOID lpvData;

// Retrieve a data pointer for the current thread.

   lpvData = TlsGetValue(dwTlsIndex);
   if ((lpvData == 0) && (GetLastError() != 0))
      ErrorExit("TlsGetValue error");

// Use the data stored for the current thread.

   printf("common: thread %d: lpvData=%lx\n",
      GetCurrentThreadId(), lpvData);

   Sleep(5000);
}

DWORD WINAPI ThreadFunc(VOID)
{
   LPVOID lpvData;

// Initialize the TLS index for this thread.

   lpvData = (LPVOID) LocalAlloc(LPTR, 256);
   if (! TlsSetValue(dwTlsIndex, lpvData))
      ErrorExit("TlsSetValue error");

   printf("thread %d: lpvData=%lx\n", GetCurrentThreadId(), lpvData);

   CommonFunc();

// Release the dynamic memory before the thread returns.

   lpvData = TlsGetValue(dwTlsIndex);
   if (lpvData != 0)
      LocalFree((HLOCAL) lpvData);

   return 0;
}

DWORD main(VOID)
{
   DWORD IDThread;
   HANDLE hThread[THREADCOUNT];
   int i;

// Allocate a TLS index.

   if ((dwTlsIndex = TlsAlloc()) == -1)
      ErrorExit("TlsAlloc failed");

// Create multiple threads.

   for (i = 0; i < THREADCOUNT; i++)
   {
      hThread[i] = CreateThread(NULL, // no security attributes
         0,                           // use default stack size
         (LPTHREAD_START_ROUTINE) ThreadFunc, // thread function
         NULL,                    // no thread function argument
         0,                       // use default creation flags
         &IDThread);              // returns thread identifier

   // Check the return value for success.
      if (hThread[i] == NULL)
         ErrorExit("CreateThread error\n");
   }

   for (i = 0; i < THREADCOUNT; i++)
      WaitForSingleObject(hThread[i], INFINITE);

   TlsFree(dwTlsIndex);

   return 0;
}

VOID ErrorExit (LPTSTR lpszMessage)
{
   fprintf(stderr, "%s\n", lpszMessage);
   ExitProcess(0);
}
	*/
#else
	pthread_mutex_lock(&(*thread)->key_mutex);
	ret = pthread_setspecific((*thread)->key, value);
	pthread_mutex_unlock(&(*thread)->key_mutex);
#endif
	return ret;
}

void* 	uthread_get_local(uthread_t *thread)
{
	void *value =NULL;
#ifdef WIN32
	return NULL;
#else
	pthread_mutex_lock(&(*thread)->key_mutex);
	value = pthread_getspecific((*thread)->key);
	pthread_mutex_unlock(&(*thread)->key_mutex);
	#endif
	return value;
}


void 	uthread_set_param(uthread_t *thread, void* obj)
{
	(*thread)->param = obj;
}

void* 	uthread_get_param(uthread_t *thread)
{
	return (*thread)->param;
}

int		uthread_set_priority(uthread_t *thread, int priority)
{
#ifdef WIN32
	return -1;
#else
	/* sched_priority will be the priority of the thread */
	(*thread)->sched_param.sched_priority = priority;

	/* only supported policy, others will result in ENOTSUP */
	(*thread)->policy = SCHED_OTHER;

	/* scheduling parameters of target thread */
	return pthread_setschedparam((*thread)->thread_id, (*thread)->policy, &(*thread)->sched_param);
#endif
}

int		uthread_get_priority(uthread_t *thread)
{
#ifdef WIN32
	return 0;
#else
	int ret = pthread_getschedparam ((*thread)->thread_id, &(*thread)->policy, &(*thread)->sched_param);
	if(ret==0)
		return (*thread)->sched_param.sched_priority;
#endif
}

/************************ MUTEX IMPLEMENTATION ******************************/
int uthread_mutex_init(uthread_mutex_t *mutex) {
	int ret = 0;
	(*mutex) = (uthread_mutex_t)malloc(sizeof(uthread_mutex));
#ifdef WIN32
	(*mutex)->mutex = CreateMutex(NULL, FALSE, NULL);
#else
	ret = pthread_mutex_init(&(*mutex)->cond_var_mutex, NULL);
	(*mutex)->status = 0;
#endif
	return ret;
}

int uthread_mutex_destroy(uthread_mutex_t *mutex) {
	int ret = 0;
#ifdef WIN32
	ret = CloseHandle((*mutex)->mutex);
#else
	ret = pthread_mutex_destroy(&(*mutex)->cond_var_mutex);
	(*mutex)->status = 0;
#endif
	free(*mutex);
	return ret;
}

int 	uthread_mutex_lock(uthread_mutex_t *mutex) {
	int ret = 0;
#ifdef WIN32
	ret = WaitForSingleObject((*mutex)->mutex, INFINITE);
#else
	ret = pthread_mutex_lock(&(*mutex)->cond_var_mutex);
	(*mutex)->status = 0;
#endif
	return ret;
}

int 	uthread_mutex_lock_timed(uthread_mutex_t *mutex, int timeoutms) {
	int ret = UTHREAD_MUTEX_SUCCESS;
	int err = 0;
	if(timeoutms<0)
		return uthread_mutex_lock(mutex);
#ifdef WIN32
	err=WaitForSingleObject((*mutex)->mutex, timeoutms);
	if(err!=WAIT_OBJECT_0)
		ret=UTHREAD_MUTEX_TIMEDOUT;
#else
	pthread_mutex_lock(&(*mutex)->cond_var_mutex);
	ret=UTHREAD_MUTEX_SUCCESS;
#endif
	return ret;
}

int 	uthread_mutex_unlock(uthread_mutex_t *mutex) {
	int ret = 0;
#ifdef WIN32
	ret = ReleaseMutex((*mutex)->mutex);
#else
	ret = pthread_mutex_unlock(&(*mutex)->cond_var_mutex);
#endif
	return ret;
}


/************************ EVENT IMPLEMENTATION ******************************/

int uthread_evt_init(uthread_evt_t *evt) {
	int ret=0;
	*evt = (uthread_evt_t) malloc(sizeof(uthread_evt));
#ifdef WIN32
	//(*evt)->mutex = CreateMutex(NULL, FALSE, NULL);
	(*evt)->event = CreateEvent(0, FALSE, FALSE, 0);
#else
	(*evt)->status = 0;
	ret = pthread_mutex_init(&(*evt)->cond_var_mutex, NULL);
	ret = pthread_cond_init(&(*evt)->cond_var, NULL);
#endif
	return ret;
}

int uthread_evt_destroy(uthread_evt_t *evt) {
#ifdef WIN32
	CloseHandle((*evt)->event);
	//CloseHandle((*evt)->mutex);
#else
	pthread_cond_destroy(&(*evt)->cond_var);
	pthread_mutex_destroy(&(*evt)->cond_var_mutex);
	free ((*evt));
	(*evt) = NULL;
#endif
	return 0;
}

int uthread_evt_signal(uthread_evt_t *evt) {
#ifdef WIN32
	SetEvent((*evt)->event);
/*
	WaitForSingleObject((*evt)->mutex);
	if ((*evt)->status)
		SetEvent((*evt)->event);
	ReleaseMutex((*evt)->mutex);
*/
#else
	pthread_mutex_lock(&(*evt)->cond_var_mutex);
	if ((*evt)->status)
		pthread_cond_signal(&(*evt)->cond_var);
	pthread_mutex_unlock(&(*evt)->cond_var_mutex);
#endif
	return 0;
}

int uthread_evt_broadcast(uthread_evt_t *evt) {
#ifdef WIN32
	PulseEvent((*evt)->event);
/*
	WaitForSingleObject((*evt)->mutex, INFINITE);
	if ((*evt)->status)
		PulseEvent((*evt)->event);
	ReleaseMutex((*evt)->mutex);
*/
#else
	pthread_mutex_lock(&(*evt)->cond_var_mutex);
	if ((*evt)->status)
		pthread_cond_broadcast(&(*evt)->cond_var);
	pthread_mutex_unlock(&(*evt)->cond_var_mutex);
#endif
	return 0;
}

int uthread_evt_timedwait(uthread_evt_t *evt, long timeoutms) {
	int ret=0;
	if(timeoutms<0)
		return uthread_evt_wait(evt);
#ifdef WIN32
	(*evt)->status++;
	ret = WaitForSingleObject((*evt)->event, timeoutms);
	(*evt)->status--;
	if(ret == WAIT_OBJECT_0) {
		ret = 0;
	}
	else {
		ret = -1;
	}
/*
	ret = WaitForSingleObject((*evt)->mutex, timeoutms);
	if(ret == WAIT_TIMEOUT) {
		return -1;
	}
	(*evt)->status++;
	ret = WaitForSingleObject((*evt)->event, timeoutms);
	if(ret == WAIT_OBJECT_0) {
		ret = 0;
	}
	else {
		ret = -1;
	}
	(*evt)->status--;
	ReleaseMutex((*evt)->mutex);
*/
#else
	pthread_mutex_lock(&(*evt)->cond_var_mutex);
	(*evt)->status++;
	gettimeofday(&(*evt)->now,NULL);
	(*evt)->cond_var_timeout.tv_sec  = (*evt)->now.tv_sec + timeoutms/1000;
	(*evt)->cond_var_timeout.tv_nsec = (*evt)->now.tv_usec*1000 + ((timeoutms%1000)*1000000);

	ret = pthread_cond_timedwait(&(*evt)->cond_var,
			&(*evt)->cond_var_mutex,
			&(*evt)->cond_var_timeout);
	(*evt)->status--;
	pthread_mutex_unlock(&(*evt)->cond_var_mutex);
#endif
	return ret;
}

int uthread_evt_wait(uthread_evt_t *evt) {
	int ret = 0;
#ifdef WIN32
	(*evt)->status++;
	ret=WaitForSingleObject((*evt)->event, INFINITE);
	(*evt)->status--;
	if(ret == WAIT_OBJECT_0) {
		ret = 0;
	}
	else {
		ret = -1;
	}
#else
	pthread_mutex_lock(&(*evt)->cond_var_mutex);
	(*evt)->status++;
	while ((*evt)->status) {
		ret = pthread_cond_wait(&(*evt)->cond_var, &(*evt)->cond_var_mutex);
	}
	(*evt)->status--;
	pthread_mutex_unlock(&(*evt)->cond_var_mutex);
#endif
	return ret;
}

int uthread_evt_waitcount(uthread_evt_t *evt) {
	int ret = 0;
#ifdef WIN32
	ret = (*evt)->status;
#else
	pthread_mutex_lock(&(*evt)->cond_var_mutex);
	ret = (*evt)->status;
	pthread_mutex_unlock(&(*evt)->cond_var_mutex);
#endif
	return ret;
}

int 	uthread_sleep(long timeoutms) {
#ifdef WIN32
	Sleep(timeoutms);
#else
	usleep(timeoutms*1000);
#endif
}

/*
int		uthread_evt_create(uthread_evt_t *evt)
{
	int ret=0;
	(*evt) = (uthread_evt_t)malloc(sizeof(uthread_evt));
#ifdef WIN32

	(*evt)->event = CreateEvent(0, FALSE, FALSE, 0);
#else

	status = 0;
	ret = pthread_mutex_init(&(*evt)->cond_var_mutex, NULL);
	ret = pthread_cond_init(&(*evt)->cond_var, NULL);
#endif
	return ret;
}

int 	uthread_evt_destroy(uthread_evt_t *evt)
{
#ifdef WIN32
	CloseHandle((*evt)->event);
#else
	pthread_cond_destroy(&(*evt)->cond_var);
	pthread_mutex_destroy(&(*evt)->cond_var_mutex);
	//ret = pthread_condattr_destroy(&cond_var_attr);
#endif
	free((*evt));
	(*evt) = NULL;
	return 0;
}

int 	uthread_evt_signal(uthread_evt_t *evt)
{
#ifdef WIN32
	SetEvent((*evt)->event);
#else
	pthread_mutex_lock(&(*evt)->cond_var_mutex);
	if ((*evt)->status == 0)
		pthread_cond_signal(&(*evt)->cond_var);
	(*evt)->status = 1;
	pthread_mutex_unlock(&(*evt)->cond_var_mutex);
#endif
	return 0;
}

int uthread_timedwait(uthread_evt_t *evt, long timeoutInMilliSec)
{
	int err=0;
	int ret=0;

	if(timeoutInMilliSec<0)
		return uthread_wait(evt);

#ifdef WIN32
	err=WaitForSingleObject((*evt)->event, timeoutInMilliSec);
	if(err==WAIT_ABANDONED)
	{
		err=WaitForSingleObject((*evt)->event, timeoutInMilliSec);
	}
	if(err!=WAIT_OBJECT_0)
		ret=EVT_TIMEDOUT;
#else

	long sec = (timeoutInMilliSec/1000);
	long nsec = (timeoutInMilliSec - (sec*1000))*1000000;

	pthread_mutex_lock(&(*evt)->cond_var_mutex);
	gettimeofday(&(*evt)->now,NULL);
	(*evt)->cond_var_timeout.tv_sec = (*evt)->now.tv_sec + sec;
	(*evt)->cond_var_timeout.tv_nsec = (*evt)->now.tv_usec*1000+ nsec;

	err = pthread_cond_timedwait(&(*evt)->cond_var, &(*evt)->cond_var_mutex,&(*evt)->cond_var_timeout);

	if(err==ETIMEDOUT)
	{
		ret=EVT_TIMEDOUT;
	}
	else
		status = 0;

	pthread_mutex_unlock(&(*evt)->cond_var_mutex);
#endif
	return ret;
}

int uthread_evt_wait(uthread_evt_t *evt)
{
	int err=0;
#ifdef WIN32
	err=WaitForSingleObject((*evt)->event, INFINITE);
#else
	pthread_mutex_lock(&(*evt)->cond_var_mutex);

	while ((*evt)->status == 0)
	{
		err = pthread_cond_wait(&(*evt)->cond_var, &(*evt)->cond_var_mutex);
	}
	status = 0;
	pthread_mutex_unlock(&(*evt)->cond_var_mutex);
#endif
	return err;
}
*/
