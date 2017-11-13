#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <inttypes.h>

#if defined(__linux__) || defined(__APPLE__)
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

#if defined(__MINGW32__) || defined(__MINGW64__)
#include <WinSock2.h>
#include <stdint.h>
#endif

#include "libzt.h"

char *msg = (char*)"welcome to the machine";

int main(int argc, char **argv)
{
	if (argc != 5) {
		printf("\nlibzt example client\n");
		printf("client [config_file_path] [nwid] [remote_addr] [remote_port]\n");
		exit(0);
	}
	std::string path        = argv[1];
	std::string nwidstr     = argv[2];
	std::string remote_addr = argv[3];
	int remote_port         = atoi(argv[4]);
	int r=0, w=0, err=0, sockfd;
	char rbuf[32];
	memset(rbuf, 0, sizeof rbuf);

	struct sockaddr_in in4;
	in4.sin_port = htons(remote_port);
	in4.sin_addr.s_addr = inet_addr(remote_addr.c_str());
	in4.sin_family = AF_INET;


	// --- BEGIN

	DEBUG_TEST("Waiting for libzt to come online...\n");
	uint64_t nwid = strtoull(nwidstr.c_str(),NULL,16);
	printf("nwid=%llx\n", nwid);
	zts_startjoin(path.c_str(), nwid);
	uint64_t nodeId = zts_get_node_id();
	DEBUG_TEST("I am %llx", nodeId);
	sleep(2);

	// socket()
	if ((sockfd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0)
		DEBUG_ERROR("error creating ZeroTier socket");

	// connect()
	if ((err = zts_connect(sockfd, (const struct sockaddr *)&in4, sizeof(in4))) < 0)
		DEBUG_ERROR("error connecting to remote host (%d)", err);

	// tx
	w = zts_write(sockfd, msg, strlen(msg));
	
	// rx
	r = zts_read(sockfd, rbuf, strlen(msg));

	DEBUG_TEST("Sent     : %s", msg);
	DEBUG_TEST("Received : %s", rbuf);

	sleep(2);
	err = zts_close(sockfd);

	return err;
}