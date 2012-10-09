#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "rmcast.h"
int stop  =0;

void* reader(void* arg)
{
	char inbuf[1024];
	int ret =0;
	printf("Reader started...\n");
	struct sockaddr_in from;
	while(!stop) {
		ret = rmcast_timed_recvfrom(inbuf, sizeof(inbuf), 0, &from);
		inbuf[ret] = '\0'; //null-term just in case
		printf("\nBuddy@%s:%d sez: %s\n\tYou sez: ", inet_ntoa(from.sin_addr), ntohs(from.sin_port), inbuf);
		fflush(stdout);
	}
	return NULL;
}

void usage(void) {
	fprintf(stderr,"\n USAGE: rmctest <port> <group_addr> [local_interface]\n");
	exit(1);
}

int main(int argc,char *argv[]) {
	int ret;
	pthread_t thread_id;
	char str[1000];

	if((argc != 1) && (argc != 3) && (argc != 4)) {
		usage();
	}

	printf("\n Starting...%d",getpid());
	if(rmcast_init(1024, 16, 5, 0) < 0) {
	   printf("Error in init.\n");
	   return 0;
	}
	printf("\n Initialized...");
	
	char * group = "234.5.6.7";
	char * local = NULL;
	int port=34567;
    if(argc > 2) {
    	port = atoi(argv[1]);
    	group = argv[2];
    	if(argc > 3) {
    	   local = argv[3];
    	}
    }
	int ttl = 8;
	printf("\n Joining %s:%d",group, port);
	
	if(rmcast_join_on(group, port, ttl, local) < 0) {
		perror( "\n server: can't open a socket");
		return -1;
	}
	
	printf("\n Channel opened...");
	
	/* start thread */
	printf("\n Starting reader thread...");
	ret = pthread_create(&thread_id, NULL, reader, NULL);

	while(!stop) {
		printf("\tYou sez: ");
		gets(str);
		int l = strlen(str) + 1;
		if(l <= 1) {
            continue;
		}
		str[l] = '\0';
		rmcast_send(str, l);
		if(!strcmp(str,"quit")) {
			stop=1;
		}
	}
    rmcast_leave();
    rmcast_close();
	printf("Done!!! \n\n");
}
