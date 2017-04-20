// Comprehensive stress test for socket-like API

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <arpa/inet.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "ZeroTierSDK.h"

#define PASSED  0
#define FAILED -1

std::string str = "welcome to the machine";

// [] random
// [OK] simple client ipv4
// [OK] simple server ipv4
// [?] simple client ipv6
// [?] simple server ipv6
// [OK] sustained client ipv4
// [OK] sustained server ipv4
// [?] sustained client ipv6
// [?] sustained server ipv6
// [] comprehensive client ipv4
// [] comprehensive server ipv6

// --- 

// comprehensive client addr port
// comprehensive server port
// simple [4|6] client addr port
// simple [4|6] server port

/****************************************************************************/
/* SIMPLE CLIENT                                                            */
/****************************************************************************/

// 
int ipv4_tcp_client_test(char *path, char *nwid, struct sockaddr_in *addr, int port)
{
	int sockfd, err;
	if((sockfd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("error creating ZeroTier socket");
		exit(0);
	}
	if((err = zts_connect(sockfd, (const struct sockaddr *)addr, sizeof(addr))) < 0) {
		printf("error connecting to remote host (%d)\n", err);
		exit(0);
	}
	int wrote = zts_write(sockfd, str.c_str(), str.length());
	err = zts_close(sockfd);
	return (wrote == str.length() && !err) ? PASSED : FAILED; // if wrote correct number of bytes
}

// 
int ipv6_tcp_client_test(char *path, char *nwid, struct sockaddr_in6 *addr, int port)
{
	int sockfd, err;
	if((sockfd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("error creating ZeroTier socket");
		exit(0);
	}
	if((err = zts_connect(sockfd, (const struct sockaddr *)addr, sizeof(addr))) < 0) {
		printf("error connecting to remote host (%d)\n", err);
		exit(0);
	}
	int wrote = zts_write(sockfd, str.c_str(), str.length());
	err = zts_close(sockfd);
	return (wrote == str.length() && !err) ? PASSED : FAILED; // if wrote correct number of bytes
}




/****************************************************************************/
/* SIMPLE SERVER                                                            */
/****************************************************************************/

//
int ipv4_tcp_server_test(char *path, char *nwid, struct sockaddr_in *addr, int port)
{
	int sockfd, err;
	if((sockfd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("error creating ZeroTier socket");
		exit(0);
	}
	if((err = zts_bind(sockfd, (struct sockaddr *)addr, sizeof(struct sockaddr_in)) < 0)) {
		printf("error binding to interface (%d)\n", err);
		exit(0);
	}
	if((err = zts_listen(sockfd, 1)) < 0) {
		printf("error placing socket in LISTENING state (%d)\n", err);
		exit(0);
	}
	// TODO: handle new address
	if((err = zts_accept(sockfd, (struct sockaddr *)&addr, (socklen_t *)sizeof(addr)) < 0)) {
		printf("error accepting connection (%d)\n", err);
		exit(0);
	}
	int wrote = zts_write(sockfd, str.c_str(), str.length());
	err = zts_close(sockfd);
	return (wrote == str.length() && !err) ? PASSED : FAILED; // if wrote correct number of bytes
}

//
int ipv6_tcp_server_test(char *path, char *nwid, struct sockaddr_in6 *addr, int port)
{
	int sockfd, err;
	if((sockfd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("error creating ZeroTier socket");
		exit(0);
	}
	if((err = zts_bind(sockfd, (struct sockaddr *)addr, sizeof(struct sockaddr_in6)) < 0)) {
		printf("error binding to interface (%d)\n", err);
		exit(0);
	}
	if((err = zts_listen(sockfd, 1)) < 0) {
		printf("error placing socket in LISTENING state (%d)\n", err);
		exit(0);
	}
	// TODO: handle new address
	if((err = zts_accept(sockfd, (struct sockaddr *)&addr, (socklen_t *)sizeof(addr)) < 0)) {
		printf("error accepting connection (%d)\n", err);
		exit(0);
	}
	int wrote = zts_write(sockfd, str.c_str(), str.length());
	err = zts_close(sockfd);
	return (wrote == str.length() && !err) ? PASSED : FAILED; // if wrote correct number of bytes
}





/****************************************************************************/
/* SUSTAINED CLIENT                                                         */
/****************************************************************************/

// Maintain transfer for n_seconds OR n_times
int ipv4_tcp_client_sustained_test(char *path, char *nwid, struct sockaddr_in *addr, int port, int n_seconds, int n_times)
{
	int sockfd, err;
	int msg_len = str.length();
	int wrote = 0;
	if((sockfd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("error creating ZeroTier socket");
		exit(0);
	}
	if((err = zts_connect(sockfd, (const struct sockaddr *)addr, sizeof(addr))) < 0) {
		printf("error connecting to remote host (%d)\n", err);
		exit(0);
	}
	if(n_seconds) {
		/*
		printf("testing for (%d) seconds\n", n_seconds);
		int wrote;
		for(int i=0; i<n_seconds; i++) {
			wrote = zts_write(sockfd, str.c_str(), str.length());
			printf("wrote = %d\n", wrote);
		}
		*/
	}
	if(n_times) {
		printf("testing (%d) times\n", n_seconds);
		for(int i=0; i<n_times; i++) {
			wrote += zts_write(sockfd, str.c_str(), str.length());
			printf("[%d] wrote = %d\n", i, wrote);
		}
	}
	err = zts_close(sockfd);
	return (wrote == (str.length()*n_times) && !err) ? PASSED : FAILED; // if wrote correct number of bytes
}

// Maintain transfer for n_seconds OR n_times
int ipv6_tcp_client_sustained_test(char *path, char *nwid, struct sockaddr_in6 *addr, int port, int n_seconds, int n_times)
{
	int sockfd, err;
	int msg_len = str.length();
	int wrote = 0;
	if((sockfd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("error creating ZeroTier socket");
		exit(0);
	}
	if((err = zts_connect(sockfd, (const struct sockaddr *)addr, sizeof(addr))) < 0) {
		printf("error connecting to remote host (%d)\n", err);
		exit(0);
	}
	if(n_seconds) {
		/*
		printf("testing for (%d) seconds\n", n_seconds);
		int wrote;
		for(int i=0; i<n_seconds; i++) {
			wrote = zts_write(sockfd, str.c_str(), str.length());
			printf("wrote = %d\n", wrote);
		}
		*/
	}
	if(n_times) {
		printf("testing (%d) times\n", n_seconds);
		for(int i=0; i<n_times; i++) {
			wrote += zts_write(sockfd, str.c_str(), str.length());
			printf("[%d] wrote = %d\n", i, wrote);
		}
	}
	err = zts_close(sockfd);
	return (wrote == (str.length()*n_times) && !err) ? PASSED : FAILED; // if wrote correct number of bytes
}





/****************************************************************************/
/* SUSTAINED SERVER                                                         */
/****************************************************************************/

// Maintain transfer for n_seconds OR n_times
int ipv4_tcp_server_sustained_test(char *path, char *nwid, struct sockaddr_in *addr, int port, int n_seconds, int n_times)
{
	int sockfd, err;
	int msg_len = str.length();
	int wrote = 0;
	if((sockfd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("error creating ZeroTier socket");
		exit(0);
	}
	if((err = zts_bind(sockfd, (struct sockaddr *)addr, (socklen_t)sizeof(struct sockaddr_in)) < 0)) {
		printf("error binding to interface (%d)\n", err);
		exit(0);
	}
	if((err = zts_listen(sockfd, 1)) < 0) {
		printf("error placing socket in LISTENING state (%d)\n", err);
		exit(0);
	}
	// TODO: handle new address
	if((err = zts_accept(sockfd, (struct sockaddr *)&addr, (socklen_t *)sizeof(addr)) < 0)) {
		printf("error accepting connection (%d)\n", err);
		exit(0);
	}
	if(n_seconds) {
		/*
		printf("testing for (%d) seconds\n", n_seconds);
		int wrote;
		for(int i=0; i<n_seconds; i++) {
			wrote = zts_write(sockfd, str.c_str(), str.length());
			printf("wrote = %d\n", wrote);
		}
		*/
	}
	if(n_times) {
		printf("testing (%d) times\n", n_seconds);
		for(int i=0; i<n_times; i++) {
			wrote += zts_write(sockfd, str.c_str(), str.length());
			printf("[%d] wrote = %d\n", i, wrote);
		}
	}
	err = zts_close(sockfd);
	return (wrote == (str.length()*n_times) && !err) ? PASSED : FAILED; // if wrote correct number of bytes
}

// Maintain transfer for n_seconds OR n_times
int ipv6_tcp_server_sustained_test(char *path, char *nwid, struct sockaddr_in6 *addr, int port, int n_seconds, int n_times)
{
	int sockfd, err;
	int msg_len = str.length();
	int wrote = 0;
	if((sockfd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("error creating ZeroTier socket");
		exit(0);
	}
	if((err = zts_bind(sockfd, (struct sockaddr *)addr, (socklen_t)sizeof(struct sockaddr_in6)) < 0)) {
		printf("error binding to interface (%d)\n", err);
		exit(0);
	}
	if((err = zts_listen(sockfd, 1)) < 0) {
		printf("error placing socket in LISTENING state (%d)\n", err);
		exit(0);
	}
	// TODO: handle new address
	if((err = zts_accept(sockfd, (struct sockaddr *)&addr, (socklen_t *)sizeof(addr)) < 0)) {
		printf("error accepting connection (%d)\n", err);
		exit(0);
	}
	if(n_seconds) {
		/*
		printf("testing for (%d) seconds\n", n_seconds);
		int wrote;
		for(int i=0; i<n_seconds; i++) {
			wrote = zts_write(sockfd, str.c_str(), str.length());
			printf("wrote = %d\n", wrote);
		}
		*/
	}
	if(n_times) {
		printf("testing (%d) times\n", n_seconds);
		for(int i=0; i<n_times; i++) {
			wrote += zts_write(sockfd, str.c_str(), str.length());
			printf("[%d] wrote = %d\n", i, wrote);
		}
	}
	err = zts_close(sockfd);
	return (wrote == (str.length()*n_times) && !err) ? PASSED : FAILED; // if wrote correct number of bytes
}






/****************************************************************************/
/* main                                                                     */
/****************************************************************************/

int main(int argc , char *argv[])
{
    if(argc < 3) {
        printf("invalid arguments\n");     
        return 1;
    }
    int port;
    struct hostent *server;
    std::string protocol;
	std::string mode;
    struct sockaddr_in6 addr6;
	struct sockaddr_in addr;
	char *path  = (char*)"./zt";

	int n_seconds = 10;
	int n_times = 10000;

	std::string nwid = argv[1]; // "ebcd7a7e120f4492"
	std::string type = argv[2]; // simple, sustained, comprehensive

	// If we're performing a non-random test, join the network we want to test on
	// and wait until the service initializes the SocketTap and provides an address
	if(type == "simple" || type == "sustained" || type == "comprehensive") {
		zts_start(path);
		while(!zts_service_running())
			sleep(1);
		zts_join_network(nwid.c_str());
		while(!zts_has_address(nwid.c_str()))
			sleep(1);
	}

	/****************************************************************************/
	/* SIMPLE                                                                   */
	/****************************************************************************/

	// SIMPLE
	// performs a one-off test of a particular subset of the API
	// For instance (ipv4 client, ipv6 server, etc)
	if(type == "simple")
	{
		protocol = argv[3]; // 4, 6
		mode = argv[4];     // client, server
		
		// SIMPLE CLIENT
		if(mode == "client")
		{
			port = atoi(argv[6]);
			printf("connecting to %s on port %s\n", argv[5], argv[6]);

			// IPv4
			if(protocol == "4") {
				addr.sin_addr.s_addr = inet_addr(argv[5]);
				addr.sin_family = AF_INET;
				addr.sin_port = htons(port);
				printf(" running (%s) test as ipv=%s\n", mode.c_str(), protocol.c_str());
				if(ipv4_tcp_client_test(path, (char*)nwid.c_str(), &addr, port) == PASSED)
					printf("PASSED\n");
				else
					printf("FAILED\n");
				return 0;
			}

			// IPv6
			if(protocol == "6") {
				server = gethostbyname2(argv[1],AF_INET6);
    			memset((char *) &addr6, 0, sizeof(addr6));
    			addr6.sin6_flowinfo = 0;
    			addr6.sin6_family = AF_INET6;
    			memmove((char *) &addr6.sin6_addr.s6_addr, (char *) server->h_addr, server->h_length);
    			addr6.sin6_port = htons(port);
				printf(" running (%s) test as ipv=%s\n", mode.c_str(), protocol.c_str());
				if(ipv6_tcp_client_test(path, (char*)nwid.c_str(), &addr6, port) == PASSED)
					printf("PASSED\n");
				else
					printf("FAILED\n");
				return 0;
			}
		}

		// SIMPLE SERVER
		if(mode == "server")
		{
			port = atoi(argv[5]);
			printf("serving on port %s\n", argv[6]);

			// IPv4
			if(protocol == "4") {
				addr.sin_port = htons(port);
				addr.sin_addr.s_addr = inet_addr("10.9.9.40");
				// addr.sin_addr.s_addr = htons(INADDR_ANY);
				addr.sin_family = AF_INET;
				printf(" running (%s) test as ipv=%s\n", mode.c_str(), protocol.c_str());
				if(ipv4_tcp_server_test(path, (char*)nwid.c_str(), &addr, port) == PASSED)
					printf("PASSED\n");
				else
					printf("FAILED\n");
				return 0;
			}

			// IPv6
			if(protocol == "6") {
				server = gethostbyname2(argv[1],AF_INET6);
    			memset((char *) &addr6, 0, sizeof(addr6));
    			addr6.sin6_flowinfo = 0;
    			addr6.sin6_family = AF_INET6;
    			addr6.sin6_port = htons(port);

				addr6.sin6_addr = in6addr_any;
    			//memmove((char *) &addr6.sin6_addr.s6_addr, (char *) server->h_addr, server->h_length);

				printf(" running (%s) test as ipv=%s\n", mode.c_str(), protocol.c_str());
				if(ipv6_tcp_server_test(path, (char*)nwid.c_str(), &addr6, port) == PASSED)
					printf("PASSED\n");
				else
					printf("FAILED\n");
				return 0;
			}
		}
	}

	/****************************************************************************/
	/* SUSTAINED                                                                */
	/****************************************************************************/

	// SUSTAINED
	// Performs a stress test for benchmarking performance
	if(type == "sustained")
	{
		protocol = argv[3]; // 4, 6
		mode = argv[4];     // client, server

		// SUSTAINED CLIENT
		if(mode == "client")
		{
			port = atoi(argv[6]);
			printf("connecting to %s on port %s\n", argv[5], argv[6]);

			// IPv4
			if(protocol == "4") {
				addr.sin_port = htons(port);
				addr.sin_addr.s_addr = inet_addr(argv[5]);
				addr.sin_family = AF_INET;
				printf(" running (%s) test as ipv=%s\n", mode.c_str(), protocol.c_str());
				if(ipv4_tcp_client_sustained_test(path, (char*)nwid.c_str(), &addr, port, n_seconds, n_times) == PASSED)
					printf("PASSED\n");
				else
					printf("FAILED\n");
				return 0;
			}

			// IPv6
			if(protocol == "6") {
				server = gethostbyname2(argv[1],AF_INET6);
    			memset((char *) &addr6, 0, sizeof(addr6));
    			addr6.sin6_flowinfo = 0;
    			addr6.sin6_family = AF_INET6;
    			memmove((char *) &addr6.sin6_addr.s6_addr, (char *) server->h_addr, server->h_length);
    			addr6.sin6_port = htons(port);
				printf(" running (%s) test as ipv=%s\n", mode.c_str(), protocol.c_str());
				if(ipv6_tcp_client_sustained_test(path, (char*)nwid.c_str(), &addr6, port, n_seconds, n_times) == PASSED)
					printf("PASSED\n");
				else
					printf("FAILED\n");
				return 0;
			}
		}

		// SUSTAINED SERVER
		if(mode == "server")
		{
			port = atoi(argv[5]);
			printf("serving on port %s\n", argv[6]);

			// IPv4
			if(protocol == "4") {
				addr.sin_port = htons(port);
				addr.sin_addr.s_addr = inet_addr("10.9.9.0");
				// addr.sin_addr.s_addr = htons(INADDR_ANY);
				addr.sin_family = AF_INET;
				printf(" running (%s) test as ipv=%s\n", mode.c_str(), protocol.c_str());
				if(ipv4_tcp_server_sustained_test(path, (char*)nwid.c_str(), &addr, port, n_seconds, n_times) == PASSED)
					printf("PASSED\n");
				else
					printf("FAILED\n");
				return 0;
			}

			// IPv6
			if(protocol == "6") {
				server = gethostbyname2(argv[1],AF_INET6);
    			memset((char *) &addr6, 0, sizeof(addr6));
    			addr6.sin6_flowinfo = 0;
    			addr6.sin6_family = AF_INET6;
    			addr6.sin6_port = htons(port);

				addr6.sin6_addr = in6addr_any;
    			//memmove((char *) &addr6.sin6_addr.s6_addr, (char *) server->h_addr, server->h_length);

				printf(" running (%s) test as ipv=%s\n", mode.c_str(), protocol.c_str());
				if(ipv6_tcp_server_sustained_test(path, (char*)nwid.c_str(), &addr6, port, n_seconds, n_times) == PASSED)
					printf("PASSED\n");
				else
					printf("FAILED\n");
				return 0;
			}
		}
	}


	/****************************************************************************/
	/* COMPREHENSIVE                                                            */
	/****************************************************************************/

	// COMPREHENSIVE
	// Tests ALL API calls
	if(type == "comprehensive")
	{

	}


	/****************************************************************************/
	/* RANDOM                                                                   */
	/****************************************************************************/

	// RANDOM
	// performs random API calls with plausible (and random) arguments/data
	if(type == "random")
	{

	}

	printf("invalid configuration. exiting.\n");
	return 0;
}