#include "rms.h"
#include "rms_internal.h"

int rms_channel_open(rms_channel *channel) {
	rms_meta_channel *meta_channel;

	if( (int)channel->init == 321) {
		rmscb->error = RMS_ERR_CHANNEL_OPENED;
		return -1;
	}

	channel->meta_channel = malloc(sizeof(rms_meta_channel));

	meta_channel = (rms_meta_channel*)channel->meta_channel;

	meta_channel->id 			= rmscb->n_channels++;
	meta_channel->port 			= 0;
	meta_channel->sock			= 0;

	meta_channel->name 			= NULL;

	meta_channel->ip_str 		= NULL;

	meta_channel->isMcast 		= 0;
	meta_channel->ttl			= 255;
	meta_channel->mcastReTX 	= 0;

	if(mqueue_init(&meta_channel->mq_out_buf) < 0) {
		rmscb->error = RMS_ERR_DSTRUCT_INIT;
		free(channel->meta_channel);
		channel->meta_channel = NULL;
		return -1;
	}

	if(mqueue_init(&meta_channel->mq_in_buf) < 0) {
		rmscb->error = RMS_ERR_DSTRUCT_INIT;
		free(channel->meta_channel);
		channel->meta_channel = NULL;
		return -1;
	}

	if(htbl_init(&meta_channel->ht_out_buf) < 0) {
		rmscb->error = RMS_ERR_DSTRUCT_INIT;
		free(channel->meta_channel);
		channel->meta_channel = NULL;
		return -1;
	}

	meta_channel->state			= 0;
	meta_channel->hbeat_ack 	= 0;
	meta_channel->msg_per_sec 	= 0;
	meta_channel->msg_this_sec	= 0;
	meta_channel->hbeat_rate 	= 5;
	meta_channel->buf_size		= 1500 + sizeof(rms_msg_hdr);;

	if(syncqueue_init(&(meta_channel->sq_in)) < 0) {
		rmscb->error = RMS_ERR_DSTRUCT_INIT;
		free(channel->meta_channel);
		channel->meta_channel = NULL;
		return -1;
	}

	if(syncqueue_init(&(meta_channel->sq_out)) < 0) {
		rmscb->error = RMS_ERR_DSTRUCT_INIT;
		free(channel->meta_channel);
		channel->meta_channel = NULL;
		return -1;
	}

	if(syncqueue_init(&(meta_channel->sq_hbeat)) < 0) {
		rmscb->error = RMS_ERR_DSTRUCT_INIT;
		free(channel->meta_channel);
		channel->meta_channel = NULL;
		return -1;
	}

	if(htbl_init(&(meta_channel->ht_peers)) < 0) {
		rmscb->error = RMS_ERR_DSTRUCT_INIT;
		free(channel->meta_channel);
		channel->meta_channel = NULL;
		return -1;
	}

	if(mqueue_init(&(meta_channel->mq_peers)) < 0) {
		rmscb->error = RMS_ERR_DSTRUCT_INIT;
		free(channel->meta_channel);
		channel->meta_channel = NULL;
		return -1;
	}

	meta_channel->oldest_seq	= 0;
	meta_channel->hb_seq_p1		= 0;
	meta_channel->hb_seq_p2		= 0;
	meta_channel->hb_seq_p3		= 0;

	meta_channel->connected		= 0;

	//register channel with service
	mqueue_add(rmscb->mq_channels, channel);

	channel->init == 321;
	return 0;
}

int rms_channel_config(rms_channel *channel, int port, int max_msg_size, int hb_rate_sec, int hb_ack) {
	rms_meta_channel *meta_channel = (rms_meta_channel*)channel->meta_channel;
	if((max_msg_size > 100) && (max_msg_size < 1500)) {
		meta_channel->buf_size	= max_msg_size + sizeof(rms_msg_hdr);
	}
	meta_channel->hbeat_rate	= hb_rate_sec;
	meta_channel->hbeat_ack		= hb_ack;
	if((port) && (meta_channel->port==0)) {
		meta_channel->port = port;
	}
	return 0;
}

int rms_channel_connect(rms_channel *channel, char *peer, int port) {
	rms_meta_channel *meta_channel = (rms_meta_channel*)channel->meta_channel;

	//check address

	//check port
	if(meta_channel->port <= 0) {
		meta_channel->port = port;
	}

	//init sock
	meta_channel->sock		= init_dgram_sock(meta_channel->port);
	meta_channel->isMCast 	= 0;

	if(meta_channel->sock < 0) {
		rmscb->error = RMS_ERR_SOCK_INIT;
		free(channel->meta_channel);
		channel->meta_channel = NULL;
		return -1;
	}

	if (channel->connected) {
		rmscb->error = RMS_ERR_CHANNEL_CONNECTED;
		return -1;
	}

	uthread_mutex_lock(&rmscb->mutx_rmscb);
	mqueue_add(rmscb->mq_channels_tmp, channel);
	uthread_mutex_unlock(&rmscb->mutx_rmscb);

	meta_channel->connected		= 1;
	return 0;
}

int rms_channel_join(rms_channel *channel, char *group, int port, int ittl) {
	unsigned char 		ttl;
	u_char 				loop;
	struct ip_mreq 		mreq;
	rms_meta_channel 	*meta_channel = (rms_meta_channel*)channel->meta_channel;

	ttl = (unsigned char) ittl;

	//init sock
	meta_channel->port 		= port;
	meta_channel->sock		= init_dgram_sock(meta_channel->port);
	meta_channel->isMCast 	= 1;

	//set TTL
	if(setsockopt(meta_channel->sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0)
		return -1;

	//disable loopback
	loop 	= 0;
	if (setsockopt(meta_channel->sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop))) {
		rmscb->error = RMS_ERR_SOCK_OPT;
		return -1;
	}


	//set group
	alloc_strcpy(&meta_channel->group, group);
	memset(&meta_channel->to_addr, 0, sizeof(rmscb->ip_addr));
	meta_channel->to_addr.sin_family 		= AF_INET;
	meta_channel->to_addr.sin_addr.s_addr 	= inet_addr(meta_channel->group);
	meta_channel->to_addr.sin_port=htons(meta_channel->port);

	//join group
	mreq.imr_multiaddr.s_addr	= inet_addr(meta_channel->group);
	mreq.imr_interface.s_addr	= htonl(INADDR_ANY);
	if (setsockopt(meta_channel->sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
		rmscb->error = RMS_ERR_SOCK_OPT;
		return -1;
	}

	meta_channel->connected		= 1;
	meta_channel->active = 1;

	//register channel
	uthread_mutex_lock(&rmscb->mutx_rmscb);
	mqueue_add(rmscb->mq_channels_tmp, channel);
	uthread_mutex_unlock(&rmscb->mutx_rmscb);

	return 0;
}

int rms_channel_leave(rms_channel *channel) {
	rms_meta_channel 	*meta_channel = (rms_meta_channel*)channel->meta_channel;

	if(meta_channel->isMCast) {
		struct ip_mreq 		mreq;
		mreq.imr_multiaddr.s_addr	= inet_addr(meta_channel->group);
		mreq.imr_interface.s_addr	= htonl(INADDR_ANY);
		if (setsockopt(meta_channel->sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
			return -1;
		}
	}
	return 0;
}

int rms_channel_close(rms_channel *channel) {
	rms_meta_channel *meta_channel;
	rms_meta_msg	 *msg;

	if( (int)channel->init == 321) {
		rmscb->error = RMS_ERR_CHANNEL_OPENED;
		return -1;
	}
	meta_channel = (rms_meta_channel*)channel->meta_channel;

	meta_channel->id 			= rmscb->n_channels++;
	meta_channel->port 			= 0;
	meta_channel->sock			= 0;

	meta_channel->name 			= NULL;

	meta_channel->ip_str 		= NULL;

	meta_channel->isMcast 		= 0;
	meta_channel->ttl			= 255;
	meta_channel->mcastReTX 	= 0;


	//empty q loop
	while(msg = (rms_meta_msg*)mqueue_remove(meta_channel->mq_out_buf) {
		//release msg

		msg = mqueue_remove(meta_channel->mq_out_buf);
	}
	mqueue_destroy(&meta_channel->mq_out_buf);

	while(msg = (rms_meta_msg*)mqueue_remove(meta_channel->mq_in_buf) {
		//release msg

		msg = mqueue_remove(meta_channel->mq_in_buf);
	}
	mqueue_destroy(&meta_channel->mq_in_buf);

	//empty htbl
	htbl_destroy(meta_channel->ht_out_buf);


	while(msg = (rms_meta_msg*)syncqueue_deq(&(meta_channel->sq_in))) {
		//release msg
		rms_msg_free(msg);

		//msg = syncqueue_remove(meta_channel->sq_in);
	}
	syncqueue_destroy(&meta_channel->sq_in);


	syncqueue_destroy(&meta_channel->sq_out);
	syncqueue_destroy(&meta_channel->sq_hbeat);

	//empty htbl
	htbl_destroy(&(meta_channel->ht_peers));

	mqueue_destroy(&(meta_channel->mq_peers));

	//unregister channel with service
	mqueue_remove(rmscb->mq_channels, channel);
	channel->init == -1;
	free(channel);
	return 0;
}

//Group or Peer-to-Peer Communication
int  rms_send(rms_channel *channel, rms_msg* msg) {
	rms_meta_msg		*meta_msg;
	rms_meta_channel 	*meta_channel = (rms_meta_channel*)channel->meta_channel;

	//alloc meta_msg
	meta_msg = alloc_meta_msg();

    meta_msg->
	//alloc or realloc mem if necessary

	//enq meta_msg in channel out queue
	syncqueue_enq(meta_channel->sq_out, meta_msg);
	return 0;
}

int  rms_recv(rms_channel *channel, rms_msg* msg) {
	return 0;
}

int  rms_timed_recv(rms_channel *channel, rms_msg* msg, long timeout) {
	return 0;
}

int  rms_send_cert(rms_channel *channel, rms_msg* msg) {
	return 0;
}
