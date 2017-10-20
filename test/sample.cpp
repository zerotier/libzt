#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "libzt.h"

int main() 
{
	char *str = "welcome to the machine";
	char *nwid = "c7cd7c9e1b0f52a2";      // network
	char *path = "config_path";           // where this instance's keys and configs are stored
	char *ip = "10.8.8.42";               // remote address
	int port = 8080;                      // remote port

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip);
	addr.sin_port = htons(port);	

	zts_startjoin(path, nwid);

	int fd, err = 0;
	if ((fd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("error creating socket\n");
	}
	if ((err = zts_connect(fd, (const struct sockaddr *)&addr, sizeof(addr))) < 0) {
		printf("error connecting to remote host\n");
	}
	if ((err = zts_write(fd, str, strlen(str))) < 0) {
		printf("error writing to socket\n");
	}
	if ((err = zts_close(fd)) < 0) {
		printf("error closing socket\n");
	}
	zts_stop();
	return 0;
}