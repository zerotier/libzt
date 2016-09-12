#include <stdio.h>
#include <pthread.h>
#include <sys/socket.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_FD_SZ 1024
int nfds = 0;
pthread_mutex_t fd_mutex;
char fds[MAX_FD_SZ];

int fd_valid(int fd)
{
     if (fd < 3 || fd >= FD_SETSIZE)
          return 0;
     if (fcntl(fd, F_GETFL) == -1 && errno == EBADF)
          return 0;
     return 1;
}

void watch_fd(int fd)
{
	pthread_mutex_lock(&fd_mutex);
//	printf("watching fd=%d\n", fd);
	nfds = fd > nfds ? fd : nfds;
	fds[fd] = fds[fd] == 0 ? 1 : fds[fd];
	pthread_mutex_unlock(&fd_mutex);
}

void *monitor_events()
{
	while(1)
	{   
		usleep(10000);
		pthread_mutex_lock(&fd_mutex);
	
		struct sockaddr_in addr;
		socklen_t addrlen;
		int i;
		for(i=0; i<nfds; i++)
		{
			int err = getsockname(i, &addr, &addrlen);
			if(err == 0) {
				if(fds[i] == 1)
				{
					fds[i] = 2;
					printf("getsockname(%d, ...) = %d\n", i, err);
					int newfd = MAX_FD_SZ-i;
					dup2(i, newfd);
					fds[newfd] = 2;
					printf("duplicated %d\n", newfd);
				}
			}
		}
		pthread_mutex_unlock(&fd_mutex);
	}
}

void monitor_new_fds()
{
#if defined(__linux__)
    char *procfd_dir = "/proc/self/fd";
#elif defined(__APPLE__)
    char *procfd_dir = "/dev/fd";
#endif

printf("procfd_dir = %s\n", procfd_dir);
    while(1)
    {
    	usleep(1000000);
	    struct dirent *dp;
	    DIR *dir = opendir(procfd_dir);
	    if(!dir) {
	    	printf("error loading fd dir\n");
	    }
	    while((dp = readdir(dir)) != NULL)
		{
			//printf("\n\ndp->d_ino = %d\n", dp->d_ino);
			//printf("dp->d_off = %d\n", dp->d_off);
			//printf("dp->d_reclen = %d\n", dp->d_reclen);
			//printf("dp->d_type = %d\n", dp->d_type);
/*
			printf("\n\ndp->d_name = %s\n", dp->d_name);
			if(dp->d_type == DT_BLK) printf("\tDT_BLK\n");
			if(dp->d_type == DT_CHR) printf("\tDT_CHR\n");
			if(dp->d_type == DT_DIR) printf("\tDT_DIR\n");
			if(dp->d_type == DT_FIFO) printf("\tDT_FIFO\n");
			if(dp->d_type == DT_LNK) printf("\tDT_LNK\n");
			if(dp->d_type == DT_REG) printf("\tDT_REG\n");
			if(dp->d_type == DT_SOCK) printf("\tDT_SOCK\n");
			if(dp->d_type == DT_UNKNOWN) printf("\tDT_UNKNOWN\n");
*/
			if(dp->d_type == DT_LNK || dp->d_type == DT_UNKNOWN) {
				int fd = atoi(dp->d_name);
				if(fd_valid(fd)) {
					watch_fd(fd);
				}
			}
		}
		closedir(dir);
	}
}



int main()
{
	// Create test fd
	int sz;
	char input[32];

	int i=0;
	for(i=0;i<MAX_FD_SZ;i++) { fds[i]=0; }


	// Monitor known relevent fds for key events
	pthread_t monitor_events_thread;
	if(pthread_create(&monitor_events_thread, NULL, &monitor_events, NULL)) {
		printf("error creating monitor_events_thread\n");
	}

	// Monitor /proc/PID/fd DIR for candidate fds to put in the event watch list
	pthread_t monitor_new_fds_thread;
	if(pthread_create(&monitor_new_fds_thread, NULL, &monitor_new_fds, NULL)) {
		printf("error creating monitor_new_fds_thread\n");
	}


	sleep(5);
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	printf("sock fd = %d\n", sock);

		struct sockaddr_in localhost;
		localhost.sin_family = AF_INET;
		localhost.sin_addr.s_addr = INADDR_ANY;
		localhost.sin_port = htons(9999);

	bind(sock, &localhost, sizeof(localhost));

	pthread_join(monitor_events_thread, NULL);
	pthread_join(monitor_new_fds_thread, NULL);



}
