#ifndef __RMS_H
#define __RMS_H

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

#include <string.h>
#include <strings.h>
#include <sys/stat.h>

//threads
#include <sys/time.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <errno.h>

#define  RMS_OK							0
#define  RMS_ERR						-1

#define  RMS_ERR_THREADING				-20011
#define  RMS_ERR_DSTRUCT_INIT			-20012
//#define  RMS_ERR_TRANSPORT_ALLOCATED	-20010

#define  RMS_ERR_CHANNEL_OPENED			-20021
#define  RMS_ERR_CHANNEL_CONNECTED		-20022

#define  RMS_ERR_SOCK_INIT				-20031

typedef struct rms_msg {
	int 	size;
	int 	mem;
	void 	*data;
	char	*src;
	int		slen;
} rms_msg;

typedef struct rms_channel {
	void 	*meta_channel;
	void	*init;
} rms_channel;


int   	rms_init(void);
//int  	rms_listen(char *ip_addr, int port);
int   	rms_shutdown(void);
int   	rms_get_error(void);

int		rms_msg_init(rms_msg *msg);
void	rms_msg_free(rms_msg *msg);
void	rms_msg_reset(rms_msg *msg);
int		rms_msg_set_data(rms_msg *msg, void *data, int len);
int		rms_msg_get_size(rms_msg *msg);
int		rms_msg_get_data(rms_msg *msg, void *data, int len);
int  	rms_msg_get_source(rms_msg* msg, char *addr, int len);

int   	rms_channel_open(rms_channel *channel);
int   	rms_channel_close(rms_channel *channel);
int   	rms_channel_config(rms_channel *channel, int max_msg_size, int hb_rate_sec, int hb_ack);
int   	rms_channel_connect(rms_channel *channel, char *peer, int port);
int   	rms_channel_join(rms_channel *channel, char *group, int port, int ttl);
int   	rms_channel_leave(rms_channel *channel);

//Group or Peer-to-Peer Communication
int   	rms_send(rms_channel *channel, rms_msg* msg);
int   	rms_recv(rms_channel *channel, rms_msg* msg);
int   	rms_timed_recv(rms_channel *channel, rms_msg* msg, long timeout);
int   	rms_send_cert(rms_channel *channel, rms_msg* msg);


//int  	rms_reg_callback(rms_channel *channel, int (*rms_on_msg)(rms_msg* msg));

int   	rms_sys_set_ttl(rms_channel *channel, int ttl);
int   	rms_sys_get_ttl(rms_channel *channel);
int   	rms_sys_hbeat_get_rate(rms_channel *channel);
int   	rms_sys_hbeat_set_rate(rms_channel *channel, int rate);
int   	rms_sys_peer_count(rms_channel *channel);
int   	rms_sys_peer_enum(rms_channel *channel, char **members, int num);
int   	rms_sys_peer_join_callback(rms_channel *channel, int (*rms_on_peer_join)(void));
int   	rms_sys_peer_leave_callback(rms_channel *channel, int (*rms_on_peer_leave)(void));


#endif