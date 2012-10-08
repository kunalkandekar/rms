#include "rms.h"
#include "rms_internal.h"

void* rms_receiver(void* arg) {
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
		//FD_SET(rmsb->ipc_sock, &readset);
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
					if(!htbl_get(rmsb->peers, msg->hdr->id, (void*)&peer)) {
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
							send_nack(peer, msg);
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
					if(!htbl_get(rmsb->peers, msg->hdr->id, (void*)&peer)) {
						//peer not registered, register it
						if(hbeat->namelen) {
							rms_meta_peer_init(&peer, msg->data, msg->hdr->id);
						}
						else {
							rms_meta_peer_init(&peer, "noname", msg->hdr->id);
						}

						htbl_put(rmsb->peers, peer->id, peer);
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
							send_nack(peer, msg);
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
					if(htbl_remove(rmsb->peers, msg->hdr->id, (void*)&peer)) {
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
		}
	}

	return 0;
}

