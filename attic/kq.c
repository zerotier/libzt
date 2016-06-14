#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/event.h>
#include <sys/time.h>
#include <err.h>
#include <errno.h>

#include <pthread.h>
#include <time.h>


#include "netcon.h"
#include "RPC.h"

void monitor_fds();
void test_poll_loop();

#define MIN_FD 3
#define SET_SZ 10

void die(const char *str) {
	perror(str);
	exit(EXIT_FAILURE);
}

int make_nonblocking(int fd) {
	int flags;
	if (-1 == (flags = fcntl(fd, F_GETFL)))
		return -1;
	flags |= O_NONBLOCK;
	if (-1 == fcntl(fd, F_SETFL, flags))
		return -1;
	return 0;
}

void changeling()
{
	set_netpath("/root/dev/ztest/nc_e5cd7a9e1c7d408c");
	pthread_t monitor_thread;
	pthread_t poll_thread;
	int i = 7;
	// Socket monitor and swap thread
	if(pthread_create(&monitor_thread, NULL, monitor_fds, (void *)i)) {
		die("unable to start changeling thread\n");
	}
	// Test poll loop thread
	if(pthread_create(&poll_thread, NULL, test_poll_loop, (void *)i)) {
		die("unable to start test poll loop thread\n");
	}
}



void test_poll_loop()
{
	printf("[POLL test thread]\n");
	fd_set in_set, rfds, wfds, efds;
	struct timeval tmout;
	//tmout.tv_usec = 8000000; 
	int ev;

	for(;;)
	{
		//printf("polling...\n");
		FD_ZERO(&in_set);
		FD_SET(4, &in_set);
		ev = select(4+1, &rfds, &wfds, &efds, NULL);
		if(ev == -1)
		{
		//	perror("select");
		}
		if(ev > 0) {
			printf("ev = %d\n", ev);
			if(FD_ISSET(4, &rfds)) {
				printf("[Read] event detected!, ev = %d\n", ev);
			}
			if(FD_ISSET(4, &wfds)) {
				printf("[Write] event detected!, ev = %d\n", ev);
			}
			if(FD_ISSET(4, &efds)) {
				printf("[Exception] event detected!, ev = %d\n", ev);
			}
			sleep(1);
		}
	}
}

unsigned int vnode_events = NOTE_DELETE | NOTE_WRITE | NOTE_EXTEND | NOTE_ATTRIB | NOTE_LINK | NOTE_RENAME | NOTE_REVOKE;

char *flagstring(int flags)
{
    static char ret[512];
    char *or = "";
 
    ret[0]='\0'; // clear the string.
    if (flags & NOTE_DELETE) {strcat(ret,or);strcat(ret,"NOTE_DELETE");or="|";}
    if (flags & NOTE_WRITE) {strcat(ret,or);strcat(ret,"NOTE_WRITE");or="|";}
    if (flags & NOTE_EXTEND) {strcat(ret,or);strcat(ret,"NOTE_EXTEND");or="|";}
    if (flags & NOTE_ATTRIB) {strcat(ret,or);strcat(ret,"NOTE_ATTRIB");or="|";}
    if (flags & NOTE_LINK) {strcat(ret,or);strcat(ret,"NOTE_LINK");or="|";}
    if (flags & NOTE_RENAME) {strcat(ret,or);strcat(ret,"NOTE_RENAME");or="|";}
    if (flags & NOTE_REVOKE) {strcat(ret,or);strcat(ret,"NOTE_REVOKE");or="|";}
 
    return ret;
}


void monitor_fds(){
	printf("[MONITOR thread]\n");
	struct timespec tmout = { 0, /* s */ 500000 /* ns */ };
	struct kevent evSet[SET_SZ];
    struct kevent evList[32];
    int fd, kq, nev, i;
    struct sockaddr_storage addr;
    socklen_t socklen = sizeof(addr);

	int s = 3;

	// Get new kernel event queue
	kq = kqueue();

	// For tracking changes in open fds 
	int watch_list_sz = SET_SZ;
	int registered_sz = 0;
	int last_registered_sz = 0;

	int swap = 0;

	for (;;)
	{
		registered_sz=0;
		/* Register range of idents */
		for(int i=MIN_FD; i<SET_SZ; i++)
		{
			//printf("EV_SET fd = %d\n", i);
			//EV_SET(&evSet[i-MIN_FD], i, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
			EV_SET(&evSet[i-MIN_FD], i, EVFILT_VNODE, EV_ADD | EV_CLEAR, vnode_events, 0, NULL);
			//if(kevent(kq, &evSet[i-MIN_FD], 1, NULL, 0, NULL) == -1) {
				//printf("\tunable to register (fd = %d)\n", i);
			//}
			//else {
			//	registered_sz++;
			//}
		}

		/* Check for events */
		if (-1 == (nev = kevent(kq, evSet, SET_SZ-MIN_FD, evList, 32, &tmout))) {
			perror("kevent()");
			//die("kevent()");	
		}

		int fd_delta = registered_sz > last_registered_sz;
		if(fd_delta) {
			printf("NEW fd registered!\n");
		}
		last_registered_sz = registered_sz;



		int s=4;

		// Check on newly created sockets
		if(!swap && fd_delta && true==false)
		{
			swap = 1;
			printf("new socket detected zt_socket = %p\n", (void*)&zt_socket);
			
			int opt;
			socklen_t opt_len;
			int err;

			int newsock = zt_socket(AF_INET, SOCK_STREAM, 0);
			printf("newsock = %d\n", newsock);			
			err = dup2(newsock, s);
			//printf("dup2() = %d\n", err);
			sleep(5);

			/*
			if((err = getsockopt(s, SOL_SOCKET, SO_TYPE, (void*)&opt, &opt_len)) < 0) {
				printf("getsockopt(): err = %d\n", err);
			}
			if(opt && SOCK_STREAM) {
				printf("SOCK_STREAM socket detected!\n");
				sleep(1);
			}
			else
			{
				printf("opt = %d\n", opt);
			}*/
		}
		//printf("Complete\n");

		// Check on incoming connections
		if(fd_delta && true == false)
		{
			// Peer name check 
			socklen_t len;
			struct sockaddr_storage addr;
			char ipstr[INET6_ADDRSTRLEN];
			int port;

			len = sizeof(addr);
			getpeername(s, (struct sockaddr *)&addr, &len);

			if (addr.ss_family == AF_INET) {
			    struct sockaddr_in *s = (struct sockaddr_in *)&addr;
			    port = ntohs(s->sin_port);
			    inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
			}
			/* else { // AF_INET6
			    struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
			    port = ntohs(s->sin6_port);
			    inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
			}
			*/

			printf("Peer IP address: %s\n", ipstr);
			printf("Peer port      : %d\n", port);

			sleep(10);

			printf("calling zt_socket()...\n");
			int intercepted_fd = zt_socket(AF_INET, SOCK_STREAM, 0);
			if(-1 == dup2(evList[i].ident, intercepted_fd)) {
				perror("dup2():");
			}
		}


		/* Process events */
		if(nev /*&& fd_delta*/)
		{
			printf("kevent() = %d\n", nev);
			for (int i = 0; i < nev; i++) {

				/*
				if(evList[i].ident == 4)
				{
					printf("calling zt_socket()...\n");
					int intercepted_fd = zt_socket(AF_INET, SOCK_STREAM, 0);
					if(-1 == dup2(evList[i].ident, intercepted_fd)) {
						perror("dup2():");
					}
				}
				*/

				printf("\tEVENT on (%d)\n", evList[i].ident);
				printf("\t\tevent[%d].ident = %d\n", i, evList[i].ident);
				printf("\t\tevent[%d].filter = %d\n", i, evList[i].filter);
				printf("\t\tevent[%d].flags = %d\n", i, evList[i].flags);
					if(evList[i].flags & EVFILT_READ) { printf("\t\t\tEVFILT_READ\n"); }
					if(evList[i].flags & EVFILT_WRITE) { printf("\t\t\tEVFILT_WRITE\n"); }
					if(evList[i].flags & EVFILT_AIO) { printf("\t\t\tEVFILT_AIO\n"); }
					if(evList[i].flags & EVFILT_VNODE) { printf("\t\t\tEVFILT_VNODE\n"); }
					if(evList[i].flags & EVFILT_PROC) { printf("\t\t\tEVFILT_PROC\n"); }
				printf("\t\tevent[%d].fflags = %d\n", i, evList[i].fflags);
					if(evList[i].fflags > 0) 
						printf("\t\t\tfflags = %s\n", flagstring(evList[i].fflags));
				printf("\t\tevent[%d].data = %d\n", i, evList[i].data);
			}
		}
	}
}