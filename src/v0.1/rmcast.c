#include "rmcast.h"

int alloc_strcpy(char **dest, char *src) {
	if(src) {
		*dest = (char*)malloc(strlen(src)+1);
		return sprintf((*dest),"%s",src);
	}
	return 0;
}

struct ipc_metadata {
	int nbytes;
	struct sockaddr_in from;
};

int init_ipc_sockets(void) {
	int ret;
	struct sockaddr_in local;
	struct hostent *hp;

	if(rmcb->initialized==12345)
		return 0;

	rmcb->initialized==12345;
    srand(time(NULL));

	//ipc server socket
	rmcb->ipc_svr_sock = socket( AF_INET, SOCK_STREAM, 0) ;
	if(rmcb->ipc_svr_sock < 0) {
        perror("socket");
		return rmcb->ipc_svr_sock;
	}
	int value = 1;
	if(setsockopt(rmcb->ipc_svr_sock, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) < 0) {
        perror("setsockopt/resue-addr");
        close(rmcb->ipc_svr_sock);
		return -1;
	}
	local.sin_family = AF_INET ;
	local.sin_port = 10000 + (rand() % 30000);    //htons (0) ;
	local.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(rmcb->ipc_svr_sock,(struct sockaddr *)&local , sizeof(local)) < 0)  {
        perror("bind");
        close(rmcb->ipc_svr_sock);
		return -1;
	}
	rmcb->ipc_port = ntohs(local.sin_port);
	printf("\n\tRMC: ipc port = %d",rmcb->ipc_port);

	ret = listen(rmcb->ipc_svr_sock,1);
	if(ret < 0) {
	    printf("\n\tRMC: Error in listen %d",ret);
        close(rmcb->ipc_svr_sock);
		return ret;
	}

	// ipc client socket
	rmcb->ipc_cli_sock = socket( AF_INET, SOCK_STREAM, 0) ;
	if(rmcb->ipc_cli_sock < 0) {
	    perror("socket");
        close(rmcb->ipc_svr_sock);
		return rmcb->ipc_cli_sock;
	}
	if(setsockopt(rmcb->ipc_cli_sock, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) < 0) {
        perror("setsockopt/reuse-addr");
        close(rmcb->ipc_svr_sock);
        close(rmcb->ipc_cli_sock);
		return -1;
	}
	rmcb->maxrcv = rmcb->ipc_cli_sock+1;
	local.sin_family = AF_INET ;
	local.sin_port = 10000 + (rand() % 30000) ;
	local.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(rmcb->ipc_cli_sock ,(struct sockaddr *)&local , sizeof(local)) < 0) {
        perror("bind");
        close(rmcb->ipc_svr_sock);
        close(rmcb->ipc_cli_sock);
		return -1;
	}

	//connect ipc sockets - need better IPC mechanism
	local.sin_port = htons (rmcb->ipc_port) ;
	ret = connect( rmcb->ipc_cli_sock, (struct sockaddr *)&local, sizeof(local));
	if(ret < 0) {
	    perror("connect");
        close(rmcb->ipc_svr_sock);
        close(rmcb->ipc_cli_sock);
		return ret;
	}
	rmcb->ipc_sock = accept(rmcb->ipc_svr_sock,(struct sockaddr *)&local, &ret);
	if(rmcb->ipc_sock < 0) {
	    perror("accept");
        close(rmcb->ipc_svr_sock);
        close(rmcb->ipc_cli_sock);
		return rmcb->ipc_sock;
	}
	rmcb->maxsock = rmcb->ipc_sock + 1;
	return 0;
}

int init_mcast_socket(int mport) {
	struct sockaddr_in local;
	//open mcast socket
	rmcb->mcast_sock = socket ( AF_INET, SOCK_DGRAM, 0) ;
	if(rmcb->mcast_sock < 0)
		return rmcb->mcast_sock;
	if(rmcb->mcast_sock >= rmcb->maxsock)
		rmcb->maxsock = rmcb->mcast_sock + 1;
    
    int value = 1;
	if(setsockopt(rmcb->mcast_sock, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) < 0) {
        perror("setsockopt/reuse-addr");
        close(rmcb->mcast_sock);
        close(rmcb->ipc_svr_sock);
        close(rmcb->ipc_cli_sock);
		return -1;
	}

    char loop = 0;
	if(setsockopt(rmcb->mcast_sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(char)) < 0) {
        perror("setsockopt/loopback");
        close(rmcb->mcast_sock);
        close(rmcb->ipc_svr_sock);
        close(rmcb->ipc_cli_sock);
		return -1;
	}

	local.sin_family = AF_INET ;
	local.sin_port = htons (mport) ;
	local.sin_addr.s_addr = htonl(INADDR_ANY);
	return bind(rmcb->mcast_sock ,(struct sockaddr *)&local, sizeof(local));


	/*
	//open ucast socket
	rmcb->ucast_sock = socket ( AF_INET, SOCK_DGRAM, 0) ;
	if(rmcb->ucast_sock < 0)
		return sd;
	if(rmcb->ucast_sock >= rmcb->maxsock)
		rmcb->maxsock = rmcb->ucast_sock + 1;
	local.sin_family = AF_INET ;
	local.sin_port = htons (mport+1) ;
	local.sin_addr.s_addr = htonl(INADDR_ANY);
	ret = bind(rmcb->ucast_sock ,(struct sockaddr *)&local, sizeof(client));
	return ret;
	*/
}

int rmcast_member_init(rmc_member **mem, char* hostname, unsigned long id) {
	(*mem) = (rmc_member *)malloc(sizeof(rmc_member));
	(*mem)->id = id;
	alloc_strcpy(&(*mem)->hostname, hostname);
	(*mem)->sequence      = 0;
	(*mem)->sequence_sent = 0;
	(*mem)->sequence_rcvd = 0;
	(*mem)->sequence_sync = 0;
	(*mem)->lost 		  = 0;
	(*mem)->last_hbeat.tv_sec = 0;
	(*mem)->last_hbeat.tv_usec = 0;
	htbl_init(&(*mem)->unseq_msgs, 10, 0);
	return 0;
}


int rmcast_member_free(rmc_member *mem) {
	free(mem->hostname);
	free(mem);
	return 0;
}

rmc_msg *new_msg(void) {
	rmc_msg *msg = (rmc_msg*)malloc(sizeof(rmc_msg));
	msg->size = rmcb->bufsize;
	msg->buf = (char*)malloc(msg->size);
	msg->hdr = (rmc_msg_hdr*)msg->buf;
	msg->hdr->version = rmcb->version;
	msg->data = msg->buf + sizeof(rmc_msg_hdr);
	return msg;
}

void clear_msg(rmc_msg *msg) {
	free(msg->buf);
	free(msg);
}

rmc_msg *alloc_msg(void) {
//	rmc_msg *msg = (rmc_msg*)mqueue_remove(rmcb->bufmsgq);
//	if(msg) {
//		return msg;
//	}
//	else
		return new_msg();
}

void release_msg(rmc_msg *msg) {
	mqueue_add(rmcb->bufmsgq, msg);
}

int  send_nack(rmc_member *mem, rmc_msg* msg) {
	unsigned long *seq;
	void *data;
	int count;
	int itr;
	unsigned long seq2 = mem->sequence - 1;
	msg->hdr->type = RMCAST_MSG_NACK;
	msg->hdr->sequence = htonl(mem->sequence);
	msg->hdr->id  = htonl(mem->id);

	count = 1;
	seq = (unsigned long*)msg->data;
	for(itr = mem->sequence + 1; itr!=seq2; itr++) {
		if(!htbl_get(mem->unseq_msgs, itr, &data)) {
			seq[count] = htonl(itr);
			count++;
			mem->lost++;
			rmcb->self->lost++;
		}
	}
	seq[0] = htonl(count);
	count = count*sizeof(unsigned long);
	msg->hdr->length = htons(count);
	rmcb->ucast_addr.sin_addr.s_addr = htonl(mem->id);
	return sendto(rmcb->mcast_sock,
			msg->buf,
			sizeof(rmc_msg_hdr) + count,
			0,
			(struct sockaddr*) &rmcb->ucast_addr,
			sizeof(rmcb->mcast_addr));
}

int  send_hbeat(char *hname, void *pgback, int len) {
	int itr;
	int ret;
	int offset;
	rmc_msg_hbeat 	*hbeat;
	rmc_msg 		*msg;
	char 			*data;
	offset = 0;
	msg = alloc_msg();

	msg->hdr->type = RMCAST_MSG_HBEAT;
	msg->hdr->sequence = htonl(rmcb->self->sequence);
	msg->hdr->id  = htonl(rmcb->self->id);

	hbeat = (rmc_msg_hbeat*)msg->data;
	data = msg->data + sizeof(rmc_msg_hbeat);
	hbeat->lost = htonl(rmcb->self->lost);

	if(hname) {
		hbeat->namelen = htons(strlen(hname) + 1);
		data += sprintf(data,"%s",hname);
		data+=offset;
	}
	else
		hbeat->namelen = 0;

	if(rmcb->hb_ack) {
		rmc_member 		*mem;
		unsigned long *nums = (unsigned long*)data;
		hbeat->acks = mqueue_size(rmcb->membersq);
		for(itr=0; itr < hbeat->acks; itr++) {
			mem = mqueue_peek_at_index(rmcb->membersq, itr);
			nums[itr*2]   = htonl(mem->id);
			nums[itr*2+1] = htonl(mem->sequence);
			offset+=2*sizeof(long);
		}
		hbeat->acks = htons(hbeat->acks);
		data+=offset;
	}
	else
		hbeat->acks = 0;

	if(pgback) {
		if(len < (msg->size - (int)(data - msg->buf))) {
			memcpy(data,pgback,len);
			offset+=len;
		}
	}
	else
		hbeat->pgback = 0;

	msg->hdr->length = htons(sizeof(rmc_msg_hbeat) + offset);
	ret = sendto(rmcb->mcast_sock,
			msg->buf,
			sizeof(rmc_msg_hdr) + sizeof(rmc_msg_hbeat) + offset,
			0,
			(struct sockaddr*) &rmcb->mcast_addr,
			sizeof(rmcb->mcast_addr));
	release_msg(msg);

	//clear out out buffer
	rmcb->hb_seq_p3 = rmcb->hb_seq_p2;
	rmcb->hb_seq_p2 = rmcb->hb_seq_p1;
	rmcb->hb_seq_p1 = rmcb->self->sequence;

	while(rmcb->oldest_seq < rmcb->hb_seq_p3){
		if(htbl_remove(rmcb->outmsgbuf,rmcb->oldest_seq,(void*)&msg)) {
			release_msg(msg);
		}
		rmcb->oldest_seq++;
	}
	return ret;
}

void* rmcast_daemon(void* arg) {
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

    struct ipc_metadata rcvd;
	int		addr_len = sizeof(rcvd.from);
	rmc_msg_hbeat *hbeat;

	gottime = 0;
	while(rmcb->active) {
		FD_ZERO(&readset);
		FD_SET(rmcb->ipc_sock, &readset);
		FD_SET(rmcb->mcast_sock, &readset);

		timeout.tv_sec	= 1;
		timeout.tv_usec = 0;

		nfound= select(rmcb->maxsock, &readset, (fd_set*)0, (fd_set*)0, &timeout);

		if(nfound > 0) {
			//available to read on mcast port
			if((nfound = FD_ISSET(rmcb->mcast_sock, &readset)) > 0) {
				rmc_msg *msg = alloc_msg();
				ret = recvfrom(rmcb->mcast_sock,
						msg->buf,
						msg->size,
						0,
						(struct sockaddr*)&(rcvd.from), //(rmcb->recv_addr),
						&addr_len);
				/*if((long int) rmcb->recv_addr.sin_addr.s_addr == 0) {
				    release_msg(msg);
				    //continue;   //it's our own
				}*/				

				if(ret < sizeof(rmc_msg_hdr)) {
					//not enough data, drop it
					release_msg(msg);
					continue;
				}

				msg->hdr->length = ntohs(msg->hdr->length);
				msg->hdr->sequence = ntohl(msg->hdr->sequence);
				msg->hdr->id  = ntohl(msg->hdr->id);

				/*printf("\n\tRMC: recvd %d/%d bytes (type=%d) from %s:%d = [%s]",
					ret, msg->hdr->length, msg->hdr->type, 
					inet_ntoa(rcvd.from.sin_addr), ntohs(rcvd.from.sin_port), msg->data);
					//(long int) rmcb->recv_addr.sin_addr.s_addr);
				fflush(stdout);*/

				if(ret < msg->hdr->length + sizeof(rmc_msg_hdr)) {
					//not enough data
					continue;
				}
				switch(msg->hdr->type) {
				case RMCAST_MSG_DATA: {
                	rmc_member *mem;				
				    if(!htbl_get(rmcb->members, msg->hdr->id, (void*)&mem)) {
						//member not registered, drop packet
						release_msg(msg);
					}
					else {
					//mem->sequence = msg->hdr->sequence;
					if(mem->sequence + 1 != msg->hdr->sequence) {
						//gap in sequence num
						if(mem->sequence < msg->hdr->sequence) {
		                  	printf("\nsending nack to %lu seq got=%lu, exp=", mem->id, msg->hdr->sequence);
							//cache msg
							htbl_put(mem->unseq_msgs, msg->hdr->sequence, msg);

							//send NACK
							msg = alloc_msg();
							send_nack(mem, msg);
						}
						//else retransmitted data, drop packet
						release_msg(msg);
					}
					else {
						//in sequence
						//syncqueue_signal_data(rmcb->inmsgq, msg);
						nfound = msg->hdr->length;
                        rcvd.nbytes = nfound;
						ret = send(rmcb->ipc_sock, &rcvd, sizeof(struct ipc_metadata), 0);
						ret = 0;
						while(ret < nfound) {
							ret+=send(rmcb->ipc_sock, msg->data+ret, nfound-ret, 0);
						}
						mem->sequence++;

						//release out of sequence data if any
						while(htbl_remove(mem->unseq_msgs, mem->sequence, (void*)&msg)) {
							//syncqueue_signal_data(rmcb->inmsgq, msg);
							nfound = msg->hdr->length;
							ret = send(rmcb->ipc_sock, &nfound, sizeof(int),0);
							ret = 0;
							while(ret < nfound) {
								ret+=send(rmcb->ipc_sock, msg->data+ret, nfound-ret, 0);
							}
							mem->sequence++;
						}
					}}
					break;
				}

				case RMCAST_MSG_HBEAT: {
					hbeat = (rmc_msg_hbeat*) msg->data;
					hbeat->namelen = ntohs(hbeat->namelen);
					data = msg->data + sizeof(rmc_msg_hbeat);
                	rmc_member *mem = NULL;
					if(!htbl_get(rmcb->members, msg->hdr->id, (void*)&mem)) {
						//member not registered, register it
						if(hbeat->namelen) {
							rmcast_member_init(&mem, msg->data, msg->hdr->id);
						}
						else {
							rmcast_member_init(&mem, "noname", msg->hdr->id);
						}
						mem->sequence = msg->hdr->sequence - 1;
						htbl_put(rmcb->members, mem->id, mem);
						mqueue_add(rmcb->membersq, mem);
					}

					mem->lost = ntohs(hbeat->lost);
					//check for namelen
					if(hbeat->namelen) {
						data+=hbeat->namelen;
					}

					//check for acks
					if(hbeat->acks) {
						seq = (unsigned long*)data;
						hbeat->acks = ntohs(hbeat->acks);
						for(itr=0;itr < hbeat->acks; itr++) {
							if(ntohl(seq[itr*2])== rmcb->self->id) {
								mem->sequence_sync = ntohl(seq[itr*2 + 1]);
								break;
							}
						}
						data+=hbeat->acks*2*sizeof(long);
					}

					//check for piggybacked data
					if(hbeat->pgback) {
						hbeat->pgback = ntohs(hbeat->pgback);
					}

					gettimeofday(&timeout, NULL);
					gottime=1;
					mem->last_hbeat.tv_sec = timeout.tv_sec;
					mem->last_hbeat.tv_usec = timeout.tv_usec;
					if(mem->sequence + 1 != msg->hdr->sequence) {
						//gap in sequence num
						if(mem->sequence < msg->hdr->sequence) {
							//send NACK
							send_nack(mem, msg);
						}
					}
					release_msg(msg);
					break;
				}

				case RMCAST_MSG_NACK:
					seq = (unsigned long*)msg->data;
					count = seq[0];
					rmcb->ucast_addr.sin_addr.s_addr = htonl(msg->hdr->id);
					for(itr=1; itr <= count; itr++) {
						seq[itr]=ntohl(seq[itr]);
						if(htbl_get(rmcb->outmsgbuf, seq[itr], (void*)&msg)) {
							ret = sendto(rmcb->mcast_sock,
									msg->buf,
									msg->length + sizeof(rmc_msg_hdr),
									0,
									(struct sockaddr*) &rmcb->ucast_addr,
									sizeof(rmcb->ucast_addr));
						}
					}
					break;

				case RMCAST_MSG_LEAVE: {
                	rmc_member *mem;
					if(htbl_remove(rmcb->members, msg->hdr->id, (void*)&mem)) {
						//clean up unseq msgs buffer
						count = htbl_count(mem->unseq_msgs);
						keys = (unsigned long*)malloc(count*sizeof(unsigned long));
						memset(keys,0,count*sizeof(unsigned long));
						count = htbl_enum_int_keys(mem->unseq_msgs, (void*)keys, count);
						for(itr = 0; itr < count; itr++) {
							if(htbl_remove(mem->unseq_msgs, keys[itr], (void*)&msg)) {
								release_msg(msg);
							}
						}
						free(keys);
						mqueue_remove_item(rmcb->membersq, mem);
						rmcast_member_free(mem);
					}
					break;
				}

				case RMCAST_MSG_ACK:
				case RMCAST_MSG_JOIN:
				case RMCAST_MSG_CERT:
				default:
					//unimplemented options, drop packet
					release_msg(msg);
					break;
				}
			}

			//available to read on ipc port
			if((nfound = FD_ISSET(rmcb->ipc_sock, &readset)) > 0) {
				err = 1;
				ret = recv(rmcb->ipc_sock,
						&ipc_read,
						sizeof(int),
						0);
				if(ret > 0) {
					//alloc buffer
					rmc_msg *msg = alloc_msg();
					if(ipc_read > rmcb->bufsize) {
						rmcb->bufsize =  ipc_read * 1.5;
						msg->size = rmcb->bufsize;
						msg->buf = realloc(msg->buf,msg->size);
						msg->hdr = (rmc_msg_hdr*)msg->buf;
						msg->data = msg->buf + sizeof(rmc_msg_hdr);
					}
					ret = recv(rmcb->ipc_sock,
							msg->data,
							ipc_read,
							0);
					if(ret == ipc_read) {
						msg->length =  ipc_read;
						msg->hdr->type = RMCAST_MSG_DATA;
						msg->hdr->length = htons(msg->length);
						msg->hdr->sequence = htonl(rmcb->self->sequence);
						msg->hdr->id  = htonl(rmcb->self->id);
						ret = sendto(rmcb->mcast_sock,
								msg->buf,
								ret + sizeof(rmc_msg_hdr),
								0,
								(struct sockaddr*) &rmcb->mcast_addr,
								sizeof(rmcb->mcast_addr));
						if (ret > 0) {
							htbl_put(rmcb->outmsgbuf, rmcb->self->sequence, msg);
							rmcb->self->sequence++;
							rmcb->msg_this_sec++;
							err = 0;
						}
					}
				}
				if(err) {
					//release_msg(msg);
				}
			}
		}

		//do periodic things
		if(!gottime)
			gettimeofday(&timeout, NULL);
		gottime=0;

		//heartbeat
		if(timeout.tv_sec - rmcb->self->last_hbeat.tv_sec >= rmcb->hbeat_rate) {
			//time for hbeat
			send_hbeat(rmcb->hostname,NULL,0);

			//check msg send rate
			t1 = (float)timeout.tv_sec + ((float)timeout.tv_usec)/1000000;
			t2 = (float)rmcb->self->last_hbeat.tv_sec
				+ ((float)rmcb->self->last_hbeat.tv_usec)/1000000;

			rmcb->msg_per_sec = rmcb->msg_this_sec/(t1-t2);

			//calculate heartbeat rate
			rmcb->hbeat_rate  = RMCAST_RATE_FACTOR / rmcb->msg_per_sec;

			//clamp
			if(rmcb->hbeat_rate > 5)
				rmcb->hbeat_rate = 5;
			else if(rmcb->hbeat_rate < 1)
				rmcb->hbeat_rate = 1;

			rmcb->msg_this_sec = 0;

			rmcb->self->last_hbeat.tv_sec = timeout.tv_sec;
			rmcb->self->last_hbeat.tv_usec = timeout.tv_usec;
		}

		//if acks are used
		if(rmcb->hb_ack) {
			//clear buffers
			count = mqueue_size(rmcb->membersq);
			for(itr=0; itr < count; itr++) {
            	rmc_member *mem = mqueue_peek_at_index(rmcb->membersq, itr);
				if(mem->sequence_sync < rmcb->self->sequence_sync);
					rmcb->self->sequence_sync = mem->sequence_sync;
			}
			while(rmcb->oldest_seq < rmcb->self->sequence_sync){
			    rmc_msg *msg = NULL;
				if(htbl_remove(rmcb->outmsgbuf, rmcb->oldest_seq, (void*)&msg)) {
					release_msg(msg);
				}
				rmcb->oldest_seq++;
			}
		}
	}
	//send leave message
	rmc_msg *msg = alloc_msg();
	msg->hdr->type = RMCAST_MSG_LEAVE;
	msg->hdr->length = 0;
	msg->hdr->sequence = htonl(rmcb->self->sequence);
	msg->hdr->id  = htonl(rmcb->self->id);
	ret = sendto(rmcb->mcast_sock,
			msg->buf,
			sizeof(rmc_msg_hdr),
			0,
			(struct sockaddr*) &rmcb->mcast_addr,
			sizeof(rmcb->mcast_addr));
	release_msg(msg);
	return 0;
}

int   rmcast_init(int max_msg_size, int nbufs, int hb_rate_sec, int hb_ack) {
	int ret;
	struct hostent *hp;

	rmcb->version = 0;

	if(max_msg_size < 128)
		max_msg_size = 128;

	rmcb->bufsize = max_msg_size + sizeof(rmc_msg_hdr);

	mqueue_init(&rmcb->bufmsgq, 100);

	htbl_init(&rmcb->outmsgbuf, 50, 20);

	syncqueue_init(&rmcb->inmsgq, 20);

	htbl_init(&rmcb->members, 20, 0);
	mqueue_init(&rmcb->membersq, 0);

	if(nbufs < 20) {
		nbufs = 20;
	}

	for(ret=0; ret < nbufs; ret++) {
		release_msg(new_msg());
	}

	rmcb->state = 0;
	if((hb_rate_sec > 5)||(hb_rate_sec < 0)) {
		hb_rate_sec = 5;
	}
	rmcb->hbeat_rate = hb_rate_sec;
	rmcb->msg_per_sec = 0;
	rmcb->msg_this_sec = 0;

	rmcb->hb_ack = hb_ack;
	rmcb->ipc_svr_sock = 0;
	rmcb->ipc_cli_sock = 0;
	rmcb->ipc_port   = 0;
	rmcb->mcast_sock = 0;
	rmcb->mcast_port = 0;

	rmcb->hb_seq_p1	 = 0;
	rmcb->hb_seq_p2  = 0;
	rmcb->hb_seq_p3  = 0;
	rmcb->discard	 = NULL;
	rmcb->discard_size = 0;

	rmcb->hostname = (char*)malloc(64);
	if(gethostname(rmcb->hostname, 64) < 0)
		return -1;
	hp = gethostbyname(rmcb->hostname);
	rmcb->id = ((struct in_addr*)hp->h_addr)->s_addr;
	rmcast_member_init(&rmcb->self, rmcb->hostname, rmcb->id);

	ret = init_ipc_sockets();
	if(ret < 0) {
		perror("IPC");
	}
	return ret;
}

int   rmcast_join(char *group, int port, int inttl) {
	int ret;
	unsigned char ttl;
	ttl = (unsigned char)inttl;
	if(rmcb->active)
		return -1;	//already joined one group
	ret = init_mcast_socket(port);
	/*
	//init ucast addr
	rmcb->ucast_addr.sin_family = AF_INET;
	rmcb->ucast_addr.sin_addr.s_addr = 0;
	rmcb->ucast_addr.sin_port = htons(rmcb->ucast_port);
	*/

	//set TTL
	if(setsockopt(rmcb->mcast_sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0)
		return -1;
	//set group
	alloc_strcpy(&rmcb->group, group);
	rmcb->port = port;
	memset(&rmcb->mcast_addr, 0, sizeof(rmcb->mcast_addr));
	rmcb->mcast_addr.sin_family = AF_INET;
	rmcb->mcast_addr.sin_addr.s_addr = inet_addr(rmcb->group);
	rmcb->mcast_addr.sin_port=htons(rmcb->port);

	//join group
	/* use setsockopt() to request that the kernel join a multicast group */
	rmcb->mreq.imr_multiaddr.s_addr=inet_addr(rmcb->group);
	rmcb->mreq.imr_interface.s_addr=htonl(INADDR_ANY);
	if (setsockopt(rmcb->mcast_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &rmcb->mreq,sizeof(rmcb->mreq)) < 0) {
		return -1;
	}

	rmcb->active = 1;

	//start thread
	ret = pthread_create(&rmcb->rmcastd, NULL, rmcast_daemon, NULL);
	return ret;
}

int rmcast_leave(void) {
	unsigned long *keys;
	unsigned long *keys1;
	int cnt;
	int cnt1;
	int nkeys;
	int itr;
	int itr1;
	rmc_member *mem;
	rmc_msg *msg = NULL;
	mem = NULL;

	//send leave msg
	rmcb->active = 0;

	//clear out all buffers
	nkeys = htbl_count(rmcb->outmsgbuf);
	keys = (unsigned long*)malloc(nkeys*sizeof(unsigned long));
	memset(keys,0,nkeys*sizeof(unsigned long));
	cnt = htbl_enum_int_keys(rmcb->outmsgbuf, (void*)keys, nkeys);
	for(itr = 0; itr < cnt; itr++) {
		if(htbl_remove(rmcb->outmsgbuf,keys[itr], (void*)&msg)) {
			release_msg(msg);
		}
	}

	//clear out in buffer
	while((msg=syncqueue_deq(rmcb->inmsgq))!=NULL) {
		release_msg(msg);
	}

	//clear out all members
	cnt = htbl_count(rmcb->members);
	if(cnt > nkeys) {
		free(keys);
		keys = (unsigned long*)malloc(nkeys*sizeof(unsigned long));
	}
	cnt = htbl_enum_int_keys(rmcb->members, (void**)keys, nkeys);
	for(itr = 0; itr < cnt; itr++) {
		if(htbl_remove(rmcb->members,keys[itr], (void*)&mem)) {
			//clean up unseq msgs buffer
			nkeys = htbl_count(mem->unseq_msgs);
			keys1 = (unsigned long*)malloc(nkeys*sizeof(unsigned long));
			memset(keys1,0,nkeys*sizeof(unsigned long));
			cnt1 = htbl_enum_int_keys(mem->unseq_msgs, (void*)keys1, nkeys);
			for(itr1 = 0; itr1 < cnt1; itr1++) {
				if(htbl_remove(mem->unseq_msgs, keys1[itr1], (void*)&msg)) {
					release_msg(msg);
				}
			}
			free(keys1);
			rmcast_member_free(mem);
		}
	}
	free(keys);

	//reset all values
	rmcb->hb_seq_p1	 = 0;
	rmcb->hb_seq_p2  = 0;
	rmcb->hb_seq_p3  = 0;
	rmcb->oldest_seq = 0;
	rmcb->self->sequence = 0;
	rmcb->self->sequence_sent = 0;
	rmcb->self->sequence_rcvd = 0;
	rmcb->self->sequence_sync = 99999999;
	rmcb->self->lost = 0;
}

int   rmcast_close(void) {
	rmc_msg *msg;
	//send leave message
	if(rmcb->active) {
		rmcast_leave();
	}
	while((msg = mqueue_remove(rmcb->bufmsgq))!=NULL) {
		//clear_msg(msg);
	}
	free(rmcb->hostname);

	rmcast_member_free(rmcb->self);
	mqueue_free(rmcb->bufmsgq);
	htbl_free(rmcb->outmsgbuf);
	syncqueue_destroy(rmcb->inmsgq);
	htbl_free(rmcb->members);

	return 0;
}

int   rmcast_send(void *data, int len) {
	int ret=0;
	if(!rmcb->active)
		return -1;
    
    //send lenght first, then data -- OPTIMIZE THIS!
	ret = send(rmcb->ipc_cli_sock,&len, sizeof(int),0);
	if(ret < 0)
		return -1;
	return send(rmcb->ipc_cli_sock, data, len, 0);
}

//int   rmcast_send_cert(void *data, int len);
int   rmcast_recv(void *data, int len) {
	return rmcast_timed_recv(data, len, 0);
}

int   rmcast_timed_recv(void *data, int len, long timeoutms) {
    return rmcast_timed_recvfrom(data, len, timeoutms, NULL);
}

int   rmcast_timed_recvfrom(void *data, int len, long timeoutms, struct sockaddr_in *from) {
	int ret=0;
	struct ipc_metadata rcvd;

	if(!rmcb->active)
		return -1;
	FD_ZERO(&rmcb->rcvset);
	FD_SET(rmcb->ipc_cli_sock, &rmcb->rcvset);

	if(timeoutms > 0) {
		rmcb->timeout.tv_sec  = timeoutms/1000;
		rmcb->timeout.tv_usec = (timeoutms%1000)*1000;
		ret = select(rmcb->maxrcv, &rmcb->rcvset, (fd_set*)0, (fd_set*)0, &rmcb->timeout);
	}
	else {
		ret = select(rmcb->maxrcv, &rmcb->rcvset, (fd_set*)0, (fd_set*)0, NULL);
	}

	if(ret>0){
		ret = recv(rmcb->ipc_cli_sock, &rcvd, sizeof(struct ipc_metadata),0);
		if(ret < 0)
			return -1;
		if(from) {
            memcpy(from, &(rcvd.from), sizeof(struct sockaddr_in));
		}
		if(rcvd.nbytes <= len) {
			return recv(rmcb->ipc_cli_sock, data, rcvd.nbytes, 0);
		}
		else {
			ret = recv(rmcb->ipc_cli_sock, data, len, 0);
			if(ret < 0)
				return ret;
			rcvd.nbytes-=len;
			if(rmcb->discard_size < rcvd.nbytes) {
				rmcb->discard = realloc(rmcb->discard, rcvd.nbytes);
				rmcb->discard_size = rcvd.nbytes;
			}
			recv(rmcb->ipc_cli_sock, rmcb->discard, rcvd.nbytes, 0);
			return ret;
		}
	}
	else
		return 0;
}