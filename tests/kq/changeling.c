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
#define MAX_FD_SCAN_SZ 10

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
	printf("changeling()\n");
	int i = 7;
	if(pthread_create(&thread, NULL, monitor_fds, (void *)i)) {
		die("unable to start changeling thread\n");
	}
}

struct timespec tmout = { 0,     /* block for 5 seconds at most */ 
                          500000 };   /* nanoseconds */

void monitor_fds(){
	//sleep(3);
	printf("monitor_fds()...\n");
	struct kevent change[MAX_FD_SCAN_SZ], event[MAX_FD_SCAN_SZ];
	int sockfd, nev, kq;
	ssize_t nbytes;
	int error;
	char buf[BUFSIZ];


	if (-1 == (kq = kqueue()))
		die("kqueue()");
 

	for(int i=MIN_FD;i<MAX_FD_SCAN_SZ; i++)
	{
		printf("registering (%d)\n", i);
		EV_SET(&change[i-MIN_FD], i, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, 0);
		//if (-1 == kevent(kq, change, 2, NULL, 0, NULL))
		//	printf(" unable to register (%d)\n", i);
	}

	//EV_SET(&change[0], STDIN_FILENO, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, 0);
	//EV_SET(&change[1], sockfd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, 0);
	 
	 if (-1 == kevent(kq, change, 2, NULL, 0, NULL))
	 	die("1kevent()");
 
	for (;;)
	{
		//usleep(50);
		//printf("blocking in kevent()\n");
		if (-1 == (nev = kevent(kq, NULL, 0, event, 2, NULL)))
			die("2kevent()");
		
		if(nev)
		{
			printf("kevent() returned %d\n", nev);
	 
			for (int i = 0; i < nev; i++)
			{
				printf("event[%d].ident = %d\n", i, event[i].ident);
				/*
				if (event[i].ident == STDIN_FILENO)
				{
					fgets(buf, sizeof(buf), stdin);

					nbytes = send(sockfd, buf, strlen(buf), 0);

					if (-1 == nbytes)
						if (EWOULDBLOCK != errno)
							die("send()");
	 
				}
				else
				{
					while ((nbytes = recv(sockfd, buf, sizeof(buf), 0)) > 0)
					{
						
						if (nbytes != write(STDOUT_FILENO, buf, nbytes))
						{
							die("write()");
						}
					}
				}
				*/
			}
		}
	}
}