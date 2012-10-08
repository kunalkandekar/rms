#ifndef __RMS_INTERNAL_H
#define __RMS_INTERNAL_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>

//sockets
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

//#include <stropts.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

//threads
#include <sys/time.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <errno.h>

#include "uthread.h"
#include "async.h"
#include "hashtable.h"


#define  RMS_DISCOVERY	50
#define  RMS_PEER_QUERY	51
#define  RMS_PEER_REPLY	52
#define  RMS_RTT_PROBE	53

#define  RMS_DATA		60
#define  RMS_DATA_END	61
#define  RMS_DATA_UNCRT	62
#define  RMS_DATA_CERT	63

#define  RMS_HEARTBEAT	71
#define  RMS_NACK		72
#define  RMS_ACK		73
#define  RMS_JOIN		74
#define  RMS_LEAVE		75


#define  RMS_TYPE_REL	80
#define  RMS_TYPE_CERT	81

#define  RMS_RATE_FACTOR 500

#define  RMS_INITIALIZED 12345

/*************************** MSG FORMATS *******************************/
#pragma pack(1)
typedef struct rms_msg_hdr {
	unsigned char   version;
	unsigned char	type;
	unsigned short 	id;
	unsigned long 	sequence;
} rms_msg_hdr;

typedef struct rms_msg_hbeat {
	unsigned long	lost;
	unsigned long 	namelen;
	unsigned short	acks;
	unsigned short 	pgback;
} rms_msg_hbeat;

#pragma pack(4)

typedef struct rms_meta_peer {
	long			id;
	char 			*hostname;
	unsigned long	sequence_sent;
	unsigned long	sequence_rcvd;
	unsigned long	sequence_sync;
	unsigned long	total_rcvd;
	unsigned long	lost;
	struct timeval	last_hbeat;
	unsigned int	msg_len;
	mqueue_t		mq_msg_in;
	htbl_t			ht_unseq_msgs;
} rms_meta_peer;

typedef struct rms_meta_msg {
	int				how;
	char			*src;
	int				slen;
	unsigned long	sequence;
	char 			*buf;
	int 			size;
	int				length;
	rms_msg_hdr		*hdr;
	char			*data;
} rms_meta_msg;


/************************* INTERNAL STRUCTS *****************************/
typedef struct rms_meta_channel {
	int					id;
	int 				sock;
	char 				*name;
	char				*ip_str;
	struct sockaddr_in 	to_addr;		//ucast or mcast
	int					port;

	int					connected;
	int					isMcast;
	unsigned char		ttl;
	int					mcastReTX;

	mqueue_t			mq_out_buf;
	mqueue_t			mq_in_buf;
	htbl_t				ht_out_buf;

	int 				maxsock;
	int					state;
	int					hbeat_ack;
	float 				msg_per_sec;
	float				msg_this_sec;
	int					hbeat_rate;

	syncqueue_t			sq_in;
	syncqueue_t			sq_out;
	syncqueue_t			sq_hbeat;

	htbl_t				ht_peers;
	mqueue_t			mq_peers;

	unsigned long 		oldest_seq;
	unsigned long 		hb_seq_p1;
	unsigned long 		hb_seq_p2;
	unsigned long 		hb_seq_p3;
} rms_meta_channel;

typedef struct rms_ctrl_block {
	int					listening;
	int 				initialized;
	char				version;
	int					error;

	unsigned long 		id;
	char				*hostname;
	struct sockaddr_in 	ip_addr;
	int					sock;
	int					port;
	rms_meta_peer		*self;

	uthread_mutex_t		mutx_rmscb;
	mqueue_t			mq_channels_reg;
	mqueue_t			mq_channels_unreg;
	mqueue_t			mq_channels;
	mqueue_t			mq_channels_tmp;
	int					n_channels;
	syncqueue_t 		sq_meta_msg_pool;

	int					bufsize;
	fd_set				rcvset;
	int					maxrcv;

	struct timeval 		timeout;
	struct sockaddr_in 	recv_addr;

	uthread_t			rms_snd_thr;		//sender
	uthread_t			rms_rcv_thr;		//receiver
	uthread_t			rms_mgr_thr;		//manager

} rms_ctrl_block;

static rms_ctrl_block _rmsb;
static rms_ctrl_block *rmsb = &_rmscb;

int alloc_strcpy(char **dest, char *src);

int init_dgram_socket(int port);

int rms_meta_peer_init(rms_meta_peer **peer, char* hostname, unsigned long id);
int rms_meta_peer_free(rms_meta_peer *peer);

rms_meta_msg 	*new_meta_msg(void);
void 			clear_meta_msg(rms_meta_msg *msg);
rms_meta_msg 	*alloc_meta_msg(void);
void 			release_meta_msg(rms_meta_msg *msg);

void *rms_manager(void *arg);
void *rms_sender(void *arg);
void *rms_receiver(void *arg);
void *rms_callback_handler(void* arg);

#endif