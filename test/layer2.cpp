// This file is built with libzt.a via `make tests`

#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#include "libzt.h"

int main(int argc , char *argv[])
{
	if(argc < 3) {
        fprintf(stderr, "usage: layer2 <zt_home_dir> <nwid>\n");
        return 1;
    }

	printf("Starting libzt...\n");
	zts_simple_start(argv[1], argv[2]);
	char device_id[11];
	zts_get_device_id(device_id);
	printf("Complete. I am %s\n", device_id);

	// layer2 example code
	int rawsock;
	if((rawsock = zts_socket(AF_INET, SOCK_RAW, IPPROTO_UDP)) < 0)
	{
		printf("There was a problem creating the raw socket\n");
		return -1;
	}
	printf("Created raw socket (%d)\n", rawsock);
	return 0;
}