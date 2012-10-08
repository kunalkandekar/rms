#include "rms.h"
#include "rms_internal.h"


int   	rms_init(void) {
	if(rmscb->initialized = RMS_INITIALIZED) {
		return 1;
	}

	rmscb->listening 	= 0;
	rmscb->initialized 	= RMS_INITIALIZED;
	rmscb->version 		= 0;
	rmscb->error		= 0;

	rmscb->id			= 0;
	rmscb->hostname		= NULL;
	rmscb->sock			= 0;
	rmscb->port			= 0;

	//init
	rms_meta_peer		*self;

	uthread_mutex_init(&rmscb->mutx_rmscb);
	mqueue_init(rmscb->mq_channels_reg);
	mqueue_init(rmscb->mq_channels_unreg);
	mqueue_init(rmscb->mq_channels);

	rmscb->n_channels	= 0;
	syncqueue_init(rmscb->sq_meta_msg_pool);

	rmscb->bufsize 		= 1500;
	rmscb->maxrcv 		= -1;

	uthread_t			rms_snd_thr;		//sender
	uthread_t			rms_rcv_thr;		//receiver
	uthread_t			rms_mgr_thr;		//manager
	return 0;

}


//int  	rms_listen(char *ip_addr, int port);

int   	rms_shutdown(void) {
	rmscb->initialized = -1;
	//cleanup
	return 0;
}

int   	rms_get_error(void) {
	return rmscb->error;
}
