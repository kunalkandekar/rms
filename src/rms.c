#include "rms.h"
#include "rms_internal.h"

int rms_init(void) {
	int ret;
	struct hostent *hp;

	rmsb->version = 0;
	mqueue_init( &(rmsb->mq_channels), 3);
	syncqueue_init( &(rmsb->mq_channels_tmp), 3);
	syncqueue_init( &(rmsb->mq_meta_msg_pool), 10);

	uthread_mutex_init(&mutx_rmscb);

	rmsb->state = 0;

	rmsb->hostname = (char*)malloc(64);

	if(gethostname(rmsb->hostname, 64) < 0)
		return -1;

	hp = gethostbyname(rmsb->hostname);
	rmsb->id = ((struct in_addr*)hp->h_addr)->s_addr;

	memcpy(rmsb->ip_addr,hp->h_addr, sizeof(ip_addr));

	rms_peer_init(&rmsb->self, rmsb->hostname, rmsb->id);
	rmsb->initialized = RMS_INITIALIZED;

	rmsb->maxsock = 0;
	rmsb->n_channels = 0;

	rmsb->sock = 0;
	rmsb->listening = 0;
	rmsb->error = 0;
	rmsb->active = 1;

	//init threads
	if(uthread_init(& (rmsb->rms_mgr_thr), rms_manager, 1) < 0) {
		rmsb->error = RMS_ERR_THREADING;
		return -1;
	}
	if(uthread_init(& (rmsb->rms_snd_thr), rms_sender, 1) < 0) {
		rmsb->error = RMS_ERR_THREADING;
		return -1;
	}
	if(uthread_init(& (rmsb->rms_rcv_thr), rms_receiver, 1) < 0) {
		rmsb->error = RMS_ERR_THREADING;
		return -1;
	}

	//start threads
	if(uthread_start(&(rmsb->rms_mgr_thr), rmsb) < 0) {
		rmsb->error = RMS_ERR_THREADING;
		return -1;
	}
	if(uthread_start(&(rmsb->rms_snd_thr), rmsb) < 0) {
		rmsb->error = RMS_ERR_THREADING;
		return -1;
	}
	if(uthread_start(&(rmsb->rms_rcv_thr), rmsb) < 0) {
		rmsb->error = RMS_ERR_THREADING;
		return -1;
	}
	return 0;
}

/*
int rms_listen(char *ip_addr, int port) {
	rmsb->sock = init_dgram_sock(port);
	if(rmsb->sock < 0)
		return -1;
	if(join_mcast_group(rmsb->sock, ip_addr) <0)
		return -1;

	rmsb->listening = 1;
	return 0;
}
*/

int rms_shutdown(void) {
	rms_channel *channel;

	rmsb->active = 0;

	uthread_destroy(&(rmsb->rms_rcv_thr));
	uthread_destroy(&(rmsb->rms_snd_thr));
	uthread_destroy(&(rmsb->rms_mgr_thr));

	if(rmsb->listening) {
		shutdown(rmsb->sock);
		close(rmsb->sock);
	}

	while( (channel = (rms_channel*)mqueue_remove(rmsb->mq_channels))) {
		rms_channel_close(channel);
	}
	mqueue_destroy( rmsb->mq_channels);

	uthread_mutex_lock(&rmsb->mutx_rmscb);
	while( (channel = (rms_channel*)mqueue_deq(rmsb->mq_channel_tmp))) {
		rms_channel_close(channel);
	}
	mqueue_destroy( rmsb->mq_channels_tmp);
	uthread_mutex_unlock(&rmsb->mutx_rmscb);
	uthread_mutex_destroy(&rmsb->mutx_rmscb);

	rmsb->state = 0;
	free(rmsb->hostname);

	rmsb->initialized = -1;
	return 0;
}

int rms_get_error(void) {
	return rmsb->error;
}

const char* rms_error_desc(int err) {
	return "RMS error";
}
