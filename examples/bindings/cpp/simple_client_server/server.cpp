#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <string>

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

int main(int argc, char **argv)
{
	if (argc != 4) {
		printf("\nlibzt example server\n");
		printf("server [config_file_path] [nwid] [bind_port]\n");
		exit(0);
	}
	std::string path      = argv[1];
	std::string nwidstr   = argv[2];
	int bind_port         = atoi(argv[3]);
	int w=0, r=0, err=0, sockfd, accfd;
	char rbuf[32];
	memset(rbuf, 0, sizeof rbuf);

	struct sockaddr_in in4, acc_in4;
	in4.sin_port = htons(bind_port);
	in4.sin_addr.s_addr = INADDR_ANY;
	in4.sin_family = AF_INET;

	// --- BEGIN


	DEBUG_TEST("Waiting for libzt to come online...\n");
	uint64_t nwid = strtoll(nwidstr.c_str(),NULL,16);
	zts_startjoin(path.c_str(), nwid);
	uint64_t nodeId = zts_get_node_id();
	DEBUG_TEST("I am %x", nodeId);
	sleep(2);

	// socket()
	if ((sockfd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0)
		DEBUG_ERROR("error creating ZeroTier socket");
	
	// bind()
	if ((err = zts_bind(sockfd, (struct sockaddr *)&in4, sizeof(struct sockaddr_in)) < 0))
		DEBUG_ERROR("error binding to interface (%d)", err);
	
	// listen()
	if ((err = zts_listen(sockfd, 100)) < 0)
		DEBUG_ERROR("error placing socket in LISTENING state (%d)", err);
	
	// accept()
	if ((accfd = zts_accept(sockfd, (struct sockaddr *)&acc_in4, (socklen_t *)sizeof(struct sockaddr_in))) < 0)
		DEBUG_ERROR("error accepting connection (%d)", err);

	// getpeername()
	//socklen_t peer_addrlen;
	//zts_getpeername(accfd, (struct sockaddr*)&acc_, &peer_addrlen);
	//DEBUG_INFO("getpeername() => %s : %d", inet_ntoa(acc_in4.sin_addr), ntohs(acc_in4.sin_port));

	// rx
	r = zts_read(accfd, rbuf, sizeof rbuf);

	// tx
	w = zts_write(accfd, rbuf, strlen(rbuf));

	DEBUG_TEST("Received : %s", rbuf);

	sleep(2);
	err = zts_close(sockfd);
	err = zts_close(accfd);

	return err;
}