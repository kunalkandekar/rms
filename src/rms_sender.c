#include "rms.h"
#include "rms_internal.h"


void* rms_sender(void* arg) {
	//init
	int 	nfound;
	int 	ret;
	int		err;
	int		itr;
	int		ipc_read;
	int		count;
	int 	gottime;
	char 	*data;
	float   t1;
	float   t2;
	fd_set	readset;
	struct 	timeval timeout;
	struct 	timeval ratechk;
	unsigned long 	*keys;
	unsigned long 	*seq;

	rms_meta_msg *msg;
	int		addr_len;
	rms_msg_hbeat *hbeat;

	rmscb *rmsb = (rmscb*)arg;

	gottime = 0;

	while(rmsb->active) {
		FD_ZERO(&readset);
		FD_SET(rmsb->ipc_sock, &readset);
		FD_SET(rmsb->mcast_sock, &readset);

		timeout.tv_sec	= 1;
		timeout.tv_usec = 0;

		nfound= select(rmsb->maxsock, &readset, (fd_set*)0, (fd_set*)0, &timeout);

		if(nfound > 0) {
			//available to read on ipc port
			if((nfound = FD_ISSET(rmsb->ipc_sock, &readset)) > 0) {
				err = 1;
				ret = recv(rmsb->ipc_sock,
						&ipc_read,
						sizeof(int),
						0);
				if(ret > 0) {
					//alloc buffer
					msg = alloc_meta_msg();
					if(ipc_read > rmsb->bufsize) {
						rmsb->bufsize =  ipc_read * 1.5;
						msg->size = rmsb->bufsize;
						msg->buf = realloc(msg->buf,msg->size);
						msg->hdr = (rms_msg_hdr*)msg->buf;
						msg->data = msg->buf + sizeof(rms_msg_hdr);
					}
					ret = recv(rmsb->ipc_sock,
							&msg->data,
							ipc_read,
							0);
					if(ret == ipc_read) {
						msg->length =  ipc_read;
						msg->hdr->type = RMS_DATA;
						msg->hdr->length = htons(msg->length);
						msg->hdr->sequence = htonl(rmsb->self->sequence_sent);
						msg->hdr->id  = htonl(rmsb->self->id);
						ret = sendto(rmsb->mcast_sock,
								msg->buf,
								ret + sizeof(rms_msg_hdr),
								0,
								(struct sockaddr*) &rmsb->mcast_addr,
								sizeof(rmsb->mcast_addr));
						if (ret > 0) {
							htbl_put(rmsb->outmsgbuf, rmsb->self->sequence_sent, msg);
							rmsb->self->sequence_sent++;
							rmsb->msg_this_sec++;
							err = 0;
						}
					}
				}
				if(err) {
					release_meta_msg(msg);
				}
			}
		}

	}
	return 0;
}
