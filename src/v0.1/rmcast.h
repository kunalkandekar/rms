#ifndef __RMCAST_H
#define __RMCAST_H

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

#include "async.h"
#include "mqueue.h"#include "hashtable.h"


#define  RMCAST_MSG_DATA	60
#define  RMCAST_MSG_HBEAT	61
#define  RMCAST_MSG_ACK		62
#define  RMCAST_MSG_NACK	63
#define  RMCAST_MSG_JOIN	64
#define  RMCAST_MSG_LEAVE	65
#define  RMCAST_MSG_CERT	66

#define  RMCAST_TYPE_REL	70
#define  RMCAST_TYPE_CERT	71

#define  RMCAST_RATE_FACTOR 500

typedef struct rmc_msg_hdr {
	unsigned char   version;
	unsigned char	type;
	unsigned short	length;
	unsigned long 	sequence;
	unsigned long 	id;
} rmc_msg_hdr;

typedef struct rmc_msg_hbeat {
	unsigned long	lost;
	unsigned long 	namelen;
	unsigned short	acks;
	unsigned short 	pgback;
} rmc_msg_hbeat;

typedef struct rmc_member {
	long			id;
	char 			*hostname;
	unsigned long	sequence_sent;
	unsigned long	sequence_rcvd;
	unsigned long	sequence_sync;
	long			lost;
	struct timeval	last_hbeat;
	htbl_t		unseq_msgs;
} rmc_member;

typedef struct rmc_msg {
	int				how;
	unsigned long	sequence;
	char 			*buf;
	int 			size;
	int				length;
	rmc_msg_hdr		*hdr;
	char			*data;
} rmc_msg;

typedef struct rmc_ctrl_block {
	int			active;
	char		version;

	unsigned long id;
	char		*hostname;
	char		*group;
	int			port;

	rmc_member	*self;

	mqueue_t	bufmsgq;
	syncqueue_t	inmsgq;
	htbl_t	members;
	mqueue_t	membersq;
	htbl_t	outmsgbuf;

	unsigned long oldest_seq;
	unsigned long hb_seq_p1;
	unsigned long hb_seq_p2;
	unsigned long hb_seq_p3;

	int 		maxsock;
	int			state;
	int			hb_ack;
	float 		msg_per_sec;
	float		msg_this_sec;
	int			hbeat_rate;

	int 		initialized;
	int 		ipc_svr_sock;
	int 		ipc_cli_sock;
	int 		ipc_sock;
	int			ipc_port;
	int 		mcast_sock;
	int			mcast_port;

	int			bufsize;

	char 		*discard;
	int			discard_size;
	fd_set		rcvset;
	int			maxrcv;
	struct	timeval timeout;

	struct sockaddr_in 	recv_addr;
	struct sockaddr_in 	mcast_addr;
	struct sockaddr_in 	ucast_addr;
	struct ip_mreq 		mreq;
	pthread_t			rmcastd;

	//int 		ucast_sock;
	//int			ucast_port;
} rmcastcb;

static rmcastcb _rmcb;
static rmcastcb *rmcb = &_rmcb;

int   rmcast_init(int max_msg_size, int nbufs, int hb_rate_sec, int hb_ack);
int   rmcast_join(char *group, int port, int ttl);
int   rmcast_leave(void);
int   rmcast_send(void *data, int len);
//int   rmcast_send_cert(void *data, int len);
int   rmcast_recv(void *data, int len);
int   rmcast_close(void);

#endif