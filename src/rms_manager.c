#include "rms.h"
#include "rms_internal.h"

int  send_nack(rms_meta_channel *channel, rms_meta_peer *peer, rms_meta_msg* msg) {
	unsigned long *seq;
	void *data;
	int count;
	int itr;
	unsigned long seq2 = peer->sequence_sent-1;
	msg->hdr->type = RMS_NACK;
	msg->hdr->sequence = htonl(peer->sequence_rcvd);
	msg->hdr->id  = htonl(peer->id);

	count = 1;
	seq = (unsigned long*)msg->data;
	for(itr = peer->sequence_rcvd + 1; itr!=seq2; itr++) {
		if(!htbl_get(peer->ht_unseq_msgs, itr, &data)) {
			seq[count] = htonl(itr);
			count++;
			peer->lost++;
			rmsb->self->lost++;
		}
	}
	seq[0] = htonl(count);
	count = count*sizeof(unsigned long);
	msg->hdr->length = htons(count);
	rmsb->ucast_addr.sin_addr.s_addr = htonl(peer->id);
	return sendto(rmsb->mcast_sock,
			msg->buf,
			sizeof(rms_msg_hdr) + count,
			0,
			(struct sockaddr*) &rmsb->ucast_addr,
			sizeof(rmsb->mcast_addr));
}

int  send_hbeat(rmscb *rmsb, void *pgback, int len) {
	int itr;
	int ret;
	int offset;
	rms_msg_hbeat 	*hbeat;
	rms_meta_msg 		*msg;
	char 			*data;
	offset = 0;
	msg = alloc_meta_msg();

	msg->hdr->type = RMS_HEARTBEAT;
	msg->hdr->sequence = htonl(rmsb->self->sequence_sent);
	msg->hdr->id  = htonl(rmsb->self->id);

	hbeat = (rms_msg_hbeat*)msg->data;
	data = msg->data + sizeof(rms_msg_hbeat);
	hbeat->lost = htonl(rmsb->self->lost);

	if(rmsb->hostname) {
		hbeat->namelen = htons(strlen(rmsb->hostname));
		data += sprintf(data,"%s",rmsb->hostname);
		data+=offset;
	}
	else
		hbeat->namelen = 0;

	if(rmsb->hb_ack) {
		rms_meta_peer 		*peer;
		unsigned long *nums = (unsigned long*)data;
		hbeat->acks = mqueue_size(rmsb->mq_peers);
		for(itr=0; itr < hbeat->acks; itr++) {
			peer = (rms_meta_peer*) mqueue_peek_at_index(rmsb->mq_peers, itr);
			nums[itr*2]   = htonl(peer->id);
			nums[itr*2+1] = htonl(peer->sequence_rcvd);
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

	msg->hdr->length = htons(sizeof(rms_msg_hbeat) + offset);
	ret = sendto(rmsb->mcast_sock,
			msg->buf,
			sizeof(rms_msg_hdr) + sizeof(rms_msg_hbeat) + offset,
			0,
			(struct sockaddr*) &rmsb->mcast_addr,
			sizeof(rmsb->mcast_addr));
	release_meta_msg(msg);

	//clear out out buffer
	rmsb->hb_seq_p3 = rmsb->hb_seq_p2;
	rmsb->hb_seq_p2 = rmsb->hb_seq_p1;
	rmsb->hb_seq_p1 = rmsb->self->sequence_sent;

	while(rmsb->oldest_seq < rmsb->hb_seq_p3){
		if(htbl_remove(rmsb->outmsgbuf,rmsb->oldest_seq,(void*)&msg)) {
			release_meta_msg(msg);
		}
		rmsb->oldest_seq++;
	}
	return ret;
}

void* rms_callback_handler(void* arg) {

}

void* rms_manager(void* arg) {
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
	rms_meta_peer *peer;
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
			//available to read on mcast port
			if((nfound = FD_ISSET(rmsb->mcast_sock, &readset)) > 0) {
				msg = alloc_meta_msg();
				ret = recvfrom(rmsb->mcast_sock,
						&msg->buf,
						msg->size,
						0,
						(struct sockaddr*)&rmsb->recv_addr,
						&addr_len);
				printf("\n\tRMC: recvd %d bytes from %ld",
					ret,
					rmsb->recv_addr.sin_addr.s_addr);
				if(ret < sizeof(rms_msg_hdr)) {
					//not enough data
					continue;
				}
				msg->hdr->length = ntohs(msg->hdr->length);
				msg->hdr->sequence = ntohl(msg->hdr->sequence);
				msg->hdr->id  = ntohl(msg->hdr->id);
				if(ret < msg->hdr->length + sizeof(rms_msg_hdr)) {
					//not enough data
					continue;
				}
				switch(msg->hdr->type) {
				case RMS_DATA:
					if(!htbl_get(rmsb->ht_peers, msg->hdr->id, (void*)&peer)) {
						//peer not registered, drop packet
						release_meta_msg(msg);
					}
					else {
					peer->sequence_sent = msg->hdr->sequence;
					if(peer->sequence_rcvd + 1 != msg->hdr->sequence) {
						//gap in sequence num
						if(peer->sequence_rcvd < msg->hdr->sequence) {
							//cache msg
							htbl_put(peer->ht_unseq_msgs, msg->hdr->sequence, msg);

							//send NACK
							msg = alloc_meta_msg();
							send_nack(rmsb, peer, msg);
						}
						//else retransmitted data, drop packet
						release_meta_msg(msg);
					}
					else {
						//in sequence
						//syncqueue_signal_data(rmsb->inmsgq, msg);
						nfound = msg->hdr->length;
						ret = send(rmsb->ipc_sock, &nfound, sizeof(int),0);
						ret = 0;
						while(ret < nfound) {
							ret+=send(rmsb->ipc_sock, msg->data+ret, nfound-ret, 0);
						}
						peer->sequence_rcvd++;

						//release out of sequence data if any
						while(htbl_remove(peer->ht_unseq_msgs, peer->sequence_sent, (void*)&msg)) {
							//syncqueue_signal_data(rmsb->inmsgq, msg);
							nfound = msg->hdr->length;
							ret = send(rmsb->ipc_sock, &nfound, sizeof(int),0);
							ret = 0;
							while(ret < nfound) {
								ret+=send(rmsb->ipc_sock, msg->data+ret, nfound-ret, 0);
							}
							peer->sequence_rcvd++;
						}
					}}
					break;

				case RMS_HEARTBEAT:
					hbeat = (rms_msg_hbeat*) msg->data;
					data = msg->data + sizeof(rms_msg_hbeat);
					if(!htbl_get(rmsb->ht_peers, msg->hdr->id, (void*)&peer)) {
						//peer not registered, register it
						if(hbeat->namelen) {
							rms_meta_peer_init(&peer, msg->data, msg->hdr->id);
						}
						else {
							rms_meta_peer_init(&peer, "noname", msg->hdr->id);
						}

						htbl_put(rmsb->ht_peers, peer->id, peer);
						mqueue_add(rmsb->mq_peers, peer);
					}

					peer->lost = ntohs(hbeat->lost);
					//check for namelen
					if(hbeat->namelen) {
						hbeat->namelen = htonl(hbeat->namelen);
						data+=hbeat->namelen;
					}

					//check for acks
					if(hbeat->acks) {
						seq = (unsigned long*)data;
						hbeat->acks = ntohs(hbeat->acks);
						for(itr=0;itr < hbeat->acks; itr++) {
							if(ntohl(seq[itr*2])== rmsb->self->id) {
								peer->sequence_sync = ntohl(seq[itr*2 + 1]);
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
					peer->last_hbeat.tv_sec = timeout.tv_sec;
					peer->last_hbeat.tv_usec = timeout.tv_usec;
					if(peer->sequence_sent + 1 != msg->hdr->sequence) {
						//gap in sequence num
						if(peer->sequence_sent < msg->hdr->sequence) {
							//send NACK
							send_nack(rmsb, peer, msg);
						}
					}
					release_meta_msg(msg);
					break;

				case RMS_NACK:
					seq = (unsigned long*)msg->data;
					count = seq[0];
					rmsb->ucast_addr.sin_addr.s_addr = htonl(msg->hdr->id);
					for(itr=1; itr <= count; itr++) {
						seq[itr]=ntohl(seq[itr]);
						if(htbl_get(rmsb->outmsgbuf, seq[itr], (void*)&msg)) {
							ret = sendto(rmsb->mcast_sock,
									msg->buf,
									msg->length + sizeof(rms_msg_hdr),
									0,
									(struct sockaddr*) &rmsb->ucast_addr,
									sizeof(rmsb->ucast_addr));
						}
					}
					break;

				case RMS_LEAVE:
					if(htbl_remove(rmsb->ht_peers, msg->hdr->id, (void*)&peer)) {
						//clean up unseq msgs buffer
						count = htbl_count(peer->ht_unseq_msgs);
						keys = (unsigned long*)malloc(count*sizeof(unsigned long));
						memset(keys,0,count*sizeof(unsigned long));
						count = htbl_enum_keys(peer->ht_unseq_msgs, (void*)keys, count);
						for(itr = 0; itr < count; itr++) {
							if(htbl_remove(peer->ht_unseq_msgs, keys[itr], (void*)&msg)) {
								release_meta_msg(msg);
							}
						}
						free(keys);
						mqueue_remove_item(rmsb->mq_peers, peer);
						rms_meta_peer_free(peer);
					}
					break;

				case RMS_ACK:
				case RMS_JOIN:
				case RMS_CERT:
				default:
					//unimplemented options, drop packet
					release_meta_msg(msg);
					break;
				}
			}

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

		//do periodic things
		if(!gottime)
			gettimeofday(&timeout, NULL);
		gottime=0;

		//heartbeat
		if(timeout.tv_sec - rmsb->self->last_hbeat.tv_sec >= rmsb->hbeat_rate) {
			//time for hbeat
			send_hbeat(rmsb, NULL,0);

			//check msg send rate
			t1 = (float)timeout.tv_sec + ((float)timeout.tv_usec)/1000000;
			t2 = (float)rmsb->self->last_hbeat.tv_sec
				+ ((float)rmsb->self->last_hbeat.tv_usec)/1000000;

			rmsb->msg_per_sec = rmsb->msg_this_sec/(t1-t2);

			//calculate heartbeat rate
			rmsb->hbeat_rate  = RMS_RATE_FACTOR / rmsb->msg_per_sec;

			//clamp
			if(rmsb->hbeat_rate > 5)
				rmsb->hbeat_rate = 5;
			else if(rmsb->hbeat_rate < 1)
				rmsb->hbeat_rate = 1;

			rmsb->msg_this_sec = 0;

			rmsb->self->last_hbeat.tv_sec = timeout.tv_sec;
			rmsb->self->last_hbeat.tv_usec = timeout.tv_usec;
		}

		//if acks are used
		if(rmsb->hb_ack) {
			//clear buffers
			count = mqueue_size(rmsb->mq_peers);
			for(itr=0; itr < count; itr++) {
				peer = (rms_meta_peer*) mqueue_peek_at_index(rmsb->mq_peers, itr);
				if(peer->sequence_sync < rmsb->self->sequence_sync);
					rmsb->self->sequence_sync = peer->sequence_sync;
			}
			while(rmsb->oldest_seq < rmsb->self->sequence_sync){
				if(htbl_remove(rmsb->outmsgbuf, rmsb->oldest_seq, (void*)&msg)) {
					release_meta_msg(msg);
				}
				rmsb->oldest_seq++;
			}
		}
	}
	//send leave message
	msg = alloc_meta_msg();
	msg->hdr->type = RMS_LEAVE;
	msg->hdr->length = 0;
	msg->hdr->sequence = htonl(rmsb->self->sequence_sent);
	msg->hdr->id  = htonl(rmsb->self->id);
	ret = sendto(rmsb->mcast_sock,
			msg->buf,
			sizeof(rms_msg_hdr),
			0,
			(struct sockaddr*) &rmsb->mcast_addr,
			sizeof(rmsb->mcast_addr));
	release_meta_msg(msg);
	return 0;
}
