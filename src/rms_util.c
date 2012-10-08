#include "rms.h"
#include "rms_internal.h"

int alloc_strcpy(char **dest, char *src) {
	if(src) {
		*dest = (char*)malloc(strlen(src)+1);
		return sprintf((*dest),"%s",src);
	}
	return 0;
}


int init_dgram_sock(int port) {
	struct sockaddr_in local;
	int sock;
	int ret;

	//open dgram socket
	sock = socket ( AF_INET, SOCK_DGRAM, 0) ;
	if(sock < 0)
		return sock;

	local.sin_family = AF_INET ;
	local.sin_port = htons (mport) ;
	local.sin_addr.s_addr = htonl(INADDR_ANY);
	ret = bind(sock ,(struct sockaddr *)&local, sizeof(local));

	if(ret <0)
		return ret;

	return sock;
}

int set_sock_ttl(int socket, int ttl) {
	return setsockopt(rmsb->mcast_sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
}

int join_mcast_group(int socket, char *group) {
	struct ip_mreq 		mreq;
	//join group
	mreq.imr_multiaddr.s_addr = inet_addr(group);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	return setsockopt(socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq,sizeof(mreq));
}

int rms_meta_peer_init(rms_meta_peer **peer, char* hostname, unsigned long id) {
	(*peer) = (rms_meta_peer *)malloc(sizeof(rms_meta_peer));
	(*peer)->id = id;
	alloc_strcpy(&(*peer)->hostname, hostname);
	(*peer)->sequence_sent = 0;
	(*peer)->sequence_rcvd = 0;
	(*peer)->sequence_sync = 0;
	(*peer)->lost 		  = 0;
	(*peer)->last_hbeat.tv_sec = 0;
	(*peer)->last_hbeat.tv_usec = 0;
	htbl_init(&(*peer)->ht_unseq_msgs, 10, 0);
	return 0;
}


int rms_meta_peer_free(rms_meta_peer *peer) {
	htbl_close(peer->ht_unseq_msgs);
	free(peer->hostname);
	free(peer);
	return 0;
}

rms_meta_msg *new_meta_msg(rmscb *rmsb) {
	rms_meta_msg *msg = (rms_meta_msg*)malloc(sizeof(rms_meta_msg));
	msg->size = rmsb->bufsize;
	msg->buf = (char*)malloc(msg->size);
	msg->hdr = (rms_msg_hdr*)msg->buf;
	msg->hdr->version = rmsb->version;
	msg->data = msg->buf + sizeof(rms_msg_hdr);
	return msg;
}

void clear_meta_msg(rms_meta_msg *msg) {
	free(msg->buf);
	free(msg);
}

rms_meta_msg *alloc_meta_msg(rmscb *rmsb) {
	rms_meta_msg *msg = (rms_meta_msg*)mqueue_remove(rmsb->sq_meta_msg_pool);
	if(msg) {
		return msg;
	}
	else {
		return new_meta_msg();
	}
}

void release_meta_msg(rmscb *rmsb, rms_meta_msg *msg) {
	mqueue_add(rmsb->sq_meta_msg_pool, msg);
}

/*
int init_ipc_sockets(void) {
	int ret;
	struct sockaddr_in local;
	struct hostent *hp;

	if(rmsb->initialized==12345)
		return 0;

	rmsb->initialized==12345;

	//ipc server socket
	rmsb->ipc_svr_sock = socket( AF_INET, SOCK_STREAM, 0) ;
	if(rmsb->ipc_svr_sock < 0)
		return rmsb->ipc_svr_sock;
	local.sin_family = AF_INET ;
	local.sin_port = htons (0) ;
	local.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(rmsb->ipc_svr_sock,(struct sockaddr *)&local , sizeof(local))<0)
		return -1;
	rmsb->ipc_port = ntohs(local.sin_port);
	printf("\n\tRMC: ipc port = %d",rmsb->ipc_port);

	ret = listen(rmsb->ipc_svr_sock,1);
	if(ret < 0)
		return ret;

	// ipc client socket
	rmsb->ipc_cli_sock = socket( AF_INET, SOCK_STREAM, 0) ;
	if(rmsb->ipc_cli_sock < 0)
		return rmsb->ipc_cli_sock;
	rmsb->maxrcv = rmsb->ipc_cli_sock+1;
	local.sin_family = AF_INET ;
	local.sin_port = htons (0) ;
	local.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(rmsb->ipc_cli_sock ,(struct sockaddr *)&local , sizeof(local))<0)
		return -1;

	//connect ipc sockets
	local.sin_port = htons (rmsb->ipc_port) ;
	ret = connect( rmsb->ipc_cli_sock, (struct sockaddr *)&local, sizeof(local));
	if(ret < 0)
		return ret;
	rmsb->ipc_sock = accept(rmsb->ipc_svr_sock,(struct sockaddr *)&local, &ret);
	if(rmsb->ipc_sock < 0)
		return rmsb->ipc_sock;
	rmsb->maxsock = rmsb->ipc_sock + 1;
	return 0;
}
*/