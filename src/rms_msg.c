#include "rms.h"
#include "rms_internal.h"

int	rms_msg_init(rms_msg* msg) {
	//*msg = (rms_msg)malloc(sizeof(struct rms_msg));
	msg->size  = 0;
	msg->mem  = 0;
	msg->data  = NULL;
	return 0;
}

void rms_msg_free(rms_msg* msg) {
	if(msg->mem) {
		free(msg->data);
		msg->mem = 0;
	}
	msg->data  = NULL;
	//free(msg);
	//*msg = NULL;
}

void rms_msg_reset(rms_msg* msg) {
	if(msg->mem) {
		free(msg->data);
		msg->mem = 0;
	}
	msg->data  = NULL;
	msg->size  = 0;
}

int	rms_msg_set_data(rms_msg* msg, void *data, int len) {
	if(msg->mem) {
		free(msg->data);
		msg->mem = 0;
	}
	msg->data = data;
	msg->size = len;
	msg->src  = &rmsb->ip_addr->s_addr;
	msg->slen = 4;
	return 0;
}

int	rms_msg_get_size(rms_msg* msg) {
	return msg->size;
}

int	rms_msg_get_source(rms_msg* msg, char *dest) {
	memcpy(msg->src, dest, msg->slen);
	return msg->slen;
}

int	rms_msg_get_data(rms_msg* msg, void *data, int len) {
	len = (len<msg->size ? len : msg->size);
	memcpy(data, msg->data, len);
	return len;
}