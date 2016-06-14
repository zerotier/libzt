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

void monitor_fds();

#define MIN_FD 0
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
	pthread_t thread;
	int i = 7;
	if(pthread_create(&thread, NULL, monitor_fds, (void *)i)) {
		die("unable to start changeling thread\n");
	}
}

struct timespec tmout = { 0, /* s */ 500000 /* ns */ };

void monitor_fds(){
	sleep(5);
	printf("monitor_fds()...\n");
	/*
	struct kevent changeList[SET_SZ];
	struct kevent eventList[SET_SZ];
	int sockfd, nev, kq;
	ssize_t nbytes;
	int error;
	char buf[BUFSIZ];

	if (-1 == (kq = kqueue()))
		die("kqueue()");
	for(int i=MIN_FD;i<SET_SZ; i++)
	{
		printf("registering (%d)\n", i);
		EV_SET(&changeList[i-MIN_FD], i, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, 0);
		//if (-1 == kevent(kq, change, 2, NULL, 0, NULL))
		//	printf(" unable to register (%d)\n", i);
	}
	*/



	struct kevent evSet;
    struct kevent evList[32];
    int nev, i;
    struct sockaddr_storage addr;
    socklen_t socklen = sizeof(addr);
    int fd;

	int local_s = 3;
	int kq;

	kq = kqueue();

	EV_SET(&evSet, local_s, EVFILT_READ, EV_ADD, 0, 0, NULL);
	if (kevent(kq, &evSet, 1, NULL, 0, NULL) == -1)
	    err(1, "1kevent");


 	printf("watching...\n");
	for (;;)
	{
		if (-1 == (nev = kevent(kq, NULL, 0, evList, 32, NULL)))
			die("2kevent()");	
		if(nev)
		{
			printf("kevent() = %d\n", nev);
			for (int i = 0; i < nev; i++) {
				printf("\tevent[%d].ident = %d\n", i, evList[i].ident);
			}
		}
	}
}