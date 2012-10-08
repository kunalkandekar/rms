#include "rms.h"
#include "rms_internal.h"

//int  	rms_reg_callback(rms_channel *channel, int (*rms_on_msg)(rms_msg* msg));

int   	rms_sys_set_ttl(rms_channel *channel, int ttl) {
	return 0;
}

int   	rms_sys_get_ttl(rms_channel *channel) {
	return 0;
}

int   	rms_sys_hbeat_get_rate(rms_channel *channel) {
	return 0;
}

int   	rms_sys_hbeat_set_rate(rms_channel *channel, int rate) {
	return 0;
}

int   	rms_sys_peer_count(rms_channel *channel) {
	return 0;
}

int   	rms_sys_peer_enum(rms_channel *channel, char **members, int num) {
	return 0;
}

int   	rms_sys_peer_join_callback(rms_channel *channel, int (*rms_on_peer_join)(void)) {
	return 0;
}

int   	rms_sys_peer_leave_callback(rms_channel *channel, int (*rms_on_peer_leave)(void)) {
	return 0;
}
