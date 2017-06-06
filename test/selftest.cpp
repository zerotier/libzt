/*
 * ZeroTier SDK - Network Virtualization Everywhere
 * Copyright (C) 2011-2017  ZeroTier, Inc.  https://www.zerotier.com/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * --
 *
 * You can be released from the requirements of the license by purchasing
 * a commercial license. Buying such a license is mandatory as soon as you
 * develop commercial closed-source software that incorporates or links
 * directly against ZeroTier software without disclosing the source code
 * of your own application.
 */

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
#include <fcntl.h>
#include <errno.h>

#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <map>

#include "ZeroTierSDK.h"

#define PASSED                0
#define FAILED               -1

#define ECHO_INTERVAL        100000 // us
#define SLAM_INTERVAL        500000
#define STR_SIZE             32

#define TEST_OP_N_BYTES      10
#define TEST_OP_N_SECONDS    11
#define TEST_OP_N_TIMES      12

#define TEST_MODE_CLIENT     20
#define TEST_MODE_SERVER     21

#define TEST_TYPE_SIMPLE     30
#define TEST_TYPE_SUSTAINED  31

#define MIN_PORT             5000
#define MAX_PORT             50000

char str[STR_SIZE];

std::map<std::string, std::string> testConf;

/* Tests in this file:

Basic RX/TX connect()/accept() Functionality:

[ ?]                      slam - perform thousands of the same call per second
[  ]                    random - act like a monkey, press all the buttons
[OK]        simple client ipv4 - connect, send one message and wait for an echo
[OK]        simple server ipv4 - accept, read one message and echo it back
[OK]        simple client ipv6 - connect, send one message and wait for an echo
[OK]        simple server ipv6 - accept, read one message and echo it back
[OK]     sustained client ipv4 - connect and rx/tx many messages
[OK]     sustained server ipv4 - accept and echo messages
[ ?]     sustained client ipv6 - connect and rx/tx many messages
[ ?]     sustained server ipv6 - accept and echo messages
[  ] comprehensive client ipv4 - test all ipv4/6 client simple/sustained modes
[  ] comprehensive server ipv6 - test all ipv4/6 server simple/sustained modes

Performance:

[  ]                Throughput - Test maximum RX/TX speeds
[  ]              Memory Usage - Test memory consumption profile
[  ]                 CPU Usage - Test processor usage
[  ]  Multithreaded Throughput - 
[  ]   Multithreaded CPU Usage - 

Correctness:

[  ]           Block/Non-block - Test that blocking and non-blocking behaviour is consistent
[  ]      Release of resources - Test that all destructor methods/blocks function properly
[  ]    Multi-network handling - Test internal Tap multiplexing works for multiple networks
[  ]          Address handling - Test that addresses are copied/parsed/returned properly

*/

void displayResults(int *results, int size)
{
	int success = 0, failure = 0;
	for(int i=0; i<size; i++) {
		if(results[i] == 0)
			success++;
		else
			failure++;
	}
	std::cout << "tials: " << size << std::endl;
	std::cout << " - success = " << (float)success / (float)size << std::endl;
	std::cout << " - failure = " << (float)failure / (float)size << std::endl;
}

void loadTestConfigFile(std::string filepath)
{
	std::string key;
	std::string value;
	std::ifstream testFile;
	testFile.open(filepath.c_str());
	while (testFile >> key >> value)
	    testConf[key] = value;
	testFile.close();
}


/****************************************************************************/
/* SIMPLE CLIENT                                                            */
/****************************************************************************/

// 
int ipv4_tcp_client_test(struct sockaddr_in *addr, int port)
{
	int r, w, sockfd, err, len = strlen(str);
	char rbuf[STR_SIZE];
	if((sockfd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("error creating ZeroTier socket");
	}
	if((err = zts_connect(sockfd, (const struct sockaddr *)addr, sizeof(addr))) < 0) {
		printf("error connecting to remote host (%d)\n", err);
	}
		//printf("WRITE!\n");

	w = zts_write(sockfd, str, len);
		//printf("READ!\n");

	r = zts_read(sockfd, rbuf, len);
		//printf("CLOSE!\n");

	err = zts_close(sockfd);
	return (w == len && r == len && !err) && !strcmp(rbuf, str) ? PASSED : FAILED;
}

// 
int ipv6_tcp_client_test(struct sockaddr_in6 *addr, int port)
{
	int r, w, sockfd, err, len = strlen(str);
	char rbuf[STR_SIZE];
	if((sockfd = zts_socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
		printf("error creating ZeroTier socket");
	}
	if((err = zts_connect(sockfd, (const struct sockaddr *)addr, sizeof(addr))) < 0) {
		printf("error connecting to remote host (%d)\n", err);
	}
	w = zts_write(sockfd, str, len);
	r = zts_read(sockfd, rbuf, len);
	err = zts_close(sockfd);
	return (w == len && r == len && !err) && !strcmp(rbuf, str) ? PASSED : FAILED;
}




/****************************************************************************/
/* SIMPLE SERVER                                                            */
/****************************************************************************/

//
int ipv4_tcp_server_test(struct sockaddr_in *addr, int port)
{
	printf("ipv4_tcp_server_test\n");
	int w=0, r=0, sockfd, accfd, err, len = strlen(str);
	char rbuf[STR_SIZE];
	if((sockfd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("error creating ZeroTier socket");
	}
	if((err = zts_bind(sockfd, (struct sockaddr *)addr, sizeof(struct sockaddr_in)) < 0)) {
		printf("error binding to interface (%d)\n", err);
	}
	if((err = zts_listen(sockfd, 100)) < 0) {
		printf("error placing socket in LISTENING state (%d)\n", err);
	}
	// TODO: handle new address
	if((accfd = zts_accept(sockfd, (struct sockaddr *)&addr, (socklen_t *)sizeof(addr))) < 0) {
		printf("error accepting connection (%d)\n", err);
	}
	//printf("READ!\n");
	r = zts_read(accfd, rbuf, sizeof rbuf);
	//printf("WRITE!\n");
	w = zts_write(accfd, rbuf, len);
	//printf("CLOSE sockfd!\n");
	zts_close(sockfd);
	//printf("CLOSE accfd!\n");

	zts_close(accfd);
	return (w == len && r == len && !err) && !strcmp(rbuf, str) ? PASSED : FAILED;
}

//
int ipv6_tcp_server_test(struct sockaddr_in6 *addr, int port)
{
	int w=0, r=0, sockfd, accfd, err, len = strlen(str);
	char rbuf[STR_SIZE];
	if((sockfd = zts_socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
		printf("error creating ZeroTier socket");
	}
	if((err = zts_bind(sockfd, (struct sockaddr *)addr, sizeof(struct sockaddr_in)) < 0)) {
		printf("error binding to interface (%d)\n", err);
	}
	if((err = zts_listen(sockfd, 100)) < 0) {
		printf("error placing socket in LISTENING state (%d)\n", err);
	}
	// TODO: handle new address
	if((accfd = zts_accept(sockfd, (struct sockaddr *)&addr, (socklen_t *)sizeof(addr))) < 0) {
		printf("error accepting connection (%d)\n", err);
	}
	r = zts_read(accfd, rbuf, sizeof rbuf);
	w = zts_write(accfd, rbuf, len);
	zts_close(sockfd);
	zts_close(accfd);
	return (w == len && r == len && !err) && !strcmp(rbuf, str) ? PASSED : FAILED;
}





/****************************************************************************/
/* SUSTAINED CLIENT                                                         */
/****************************************************************************/

// Maintain transfer for n_count OR n_count
int ipv4_tcp_client_sustained_test(struct sockaddr_in *addr, int port, int operation, int n_count, int delay)
{
	int w=0, r=0, sockfd, accfd, err, len = strlen(str);
	int tot, n=0;
	char rbuf[STR_SIZE];
	if((sockfd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("error creating ZeroTier socket");
	}
	if((err = zts_connect(sockfd, (const struct sockaddr *)addr, sizeof(addr))) < 0) {
		printf("error connecting to remote host (%d)\n", err);
	}
	//zts_fcntl(sockfd, F_SETFL, O_NONBLOCK);
	if(operation == TEST_OP_N_TIMES) {
		tot = len*n_count;
		for(int i=0; i<n_count; i++) {
			usleep(delay * 1000);
			n = zts_write(sockfd, str, len);
			if (n > 0)
				w += n;
			n = zts_read(sockfd, rbuf, len);
			if (n > 0)
				r += n;
		}
		err = zts_close(sockfd);
		return (r == tot && w == tot && !err) && !strcmp(rbuf, str) ? PASSED : FAILED;
	}
	if(operation == TEST_OP_N_BYTES) {
		tot = n_count;
		while(r < tot || w < tot) {
			usleep(delay * 1000);
			if (w < tot)
				n = zts_write(sockfd, str, n_count);
			if (n > 0)
				w += n;
			if (r < tot)
				n = zts_read(sockfd, rbuf, n_count);
			if (n > 0)
				r += n;
		}
		err = zts_close(sockfd);
		return (r == tot && w == tot && !err) ? PASSED : FAILED;
	}
	return FAILED;
}

// Maintain transfer for n_count OR n_count
int ipv6_tcp_client_sustained_test(struct sockaddr_in6 *addr, int port, int operation, int n_count, int delay) 
{
	int w=0, r=0, sockfd, accfd, err, len = strlen(str);
	int tot, n=0;
	char rbuf[STR_SIZE];
	if((sockfd = zts_socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
		printf("error creating ZeroTier socket");
	}
	if((err = zts_connect(sockfd, (const struct sockaddr *)addr, sizeof(addr))) < 0) {
		printf("error connecting to remote host (%d)\n", err);
	}
	//zts_fcntl(sockfd, F_SETFL, O_NONBLOCK);
	if(operation == TEST_OP_N_TIMES) {
		tot = len*n_count;
		for(int i=0; i<n_count; i++) {
			usleep(delay * 1000);
			n = zts_write(sockfd, str, len);
			if (n > 0)
				w += n;
			n = zts_read(sockfd, rbuf, len);
			if (n > 0)
				r += n;
		}
		err = zts_close(sockfd);
		return (r == tot && w == tot && !err) && !strcmp(rbuf, str) ? PASSED : FAILED;
	}
	if(operation == TEST_OP_N_BYTES) {
		tot = n_count;
		while(r < tot || w < tot) {
			usleep(delay * 1000);
			if (w < tot)
				n = zts_write(sockfd, str, n_count);
			if (n > 0)
				w += n;
			if (r < tot)
				n = zts_read(sockfd, rbuf, n_count);
			if (n > 0)
				r += n;
		}
		err = zts_close(sockfd);
		return (r == tot && w == tot && !err) ? PASSED : FAILED;
	}
	return FAILED;
}





/****************************************************************************/
/* SUSTAINED SERVER                                                         */
/****************************************************************************/

// Maintain transfer for n_count OR n_count
int ipv4_tcp_server_sustained_test(struct sockaddr_in *addr, int port, int operation, int n_count, int delay)
{
	int w=0, r=0, sockfd, accfd, err, len = strlen(str);
	int tot, n=0;
	char rbuf[STR_SIZE];
	if((sockfd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("error creating ZeroTier socket");
	}
	if((err = zts_bind(sockfd, (struct sockaddr *)addr, (socklen_t)sizeof(struct sockaddr_in)) < 0)) {
		printf("error binding to interface (%d)\n", err);
	}
	if((err = zts_listen(sockfd, 1)) < 0) {
		printf("error placing socket in LISTENING state (%d)\n", err);
	}
	// TODO: handle new address
	if((accfd = zts_accept(sockfd, (struct sockaddr *)&addr, (socklen_t *)sizeof(addr))) < 0) {
		printf("error accepting connection (%d)\n", err);
	}
	//zts_fcntl(accfd, F_SETFL, O_NONBLOCK);
	if(operation == TEST_OP_N_TIMES) {
		tot = len*n_count;
		for(int i=0; i<n_count; i++) {
			usleep(delay * 1000);
			r += zts_read(accfd, rbuf, len);
			w += zts_write(accfd, rbuf, len);		
		}
		zts_close(sockfd);
		zts_close(accfd);
		return (r == tot && w == tot && !err) && !strcmp(rbuf, str) ? PASSED : FAILED;
	}
	if(operation == TEST_OP_N_BYTES) {
		tot = n_count;
		while(r < tot || w < tot) {
			usleep(delay * 1000);
			if (r < tot)
				n = zts_read(accfd, rbuf, n_count);
			if (n > 0)
				r += n;
			if (w < tot)
				n = zts_write(accfd, str, n_count);
			if (n > 0)
				w += n;
		}
		zts_close(sockfd);
		zts_close(accfd);
		return (r == tot && w == tot && !err) ? PASSED : FAILED;
	}
	return FAILED;
}

// Maintain transfer for n_count OR n_count
int ipv6_tcp_server_sustained_test(struct sockaddr_in6 *addr, int port, int operation, int n_count, int delay)
{
	int w=0, r=0, sockfd, accfd, err, len = strlen(str);
	int tot, n=0;
	char rbuf[STR_SIZE];
	if((sockfd = zts_socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
		printf("error creating ZeroTier socket");
	}
	if((err = zts_bind(sockfd, (struct sockaddr *)addr, (socklen_t)sizeof(struct sockaddr_in)) < 0)) {
		printf("error binding to interface (%d)\n", err);
	}
	if((err = zts_listen(sockfd, 1)) < 0) {
		printf("error placing socket in LISTENING state (%d)\n", err);
	}
	// TODO: handle new address
	if((accfd = zts_accept(sockfd, (struct sockaddr *)&addr, (socklen_t *)sizeof(addr))) < 0) {
		printf("error accepting connection (%d)\n", err);
	}
	//zts_fcntl(accfd, F_SETFL, O_NONBLOCK);
	if(operation == TEST_OP_N_TIMES) {
		tot = len*n_count;
		for(int i=0; i<n_count; i++) {
			usleep(delay * 1000);
			r += zts_read(accfd, rbuf, len);
			w += zts_write(accfd, rbuf, len);		
		}
		zts_close(sockfd);
		zts_close(accfd);
		return (r == tot && w == tot && !err) && !strcmp(rbuf, str) ? PASSED : FAILED;
	}
	if(operation == TEST_OP_N_BYTES) {
		tot = n_count;
		while(r < tot || w < tot) {
			usleep(delay * 1000);
			if (r < tot)
				n = zts_read(accfd, rbuf, n_count);
			if (n > 0)
				r += n;
			if (w < tot)
				n = zts_write(accfd, str, n_count);
			if (n > 0)
				w += n;
		}
		zts_close(sockfd);
		zts_close(accfd);
		return (r == tot && w == tot && !err) ? PASSED : FAILED;
	}
	return FAILED;}





/****************************************************************************/
/* SLAM API (multiple of each api call and/or plausible call sequence)      */
/****************************************************************************/

#define SLAM_NUMBER 16
#define SLAM_REPEAT 1

int slam_api_test()
{
	int err = 0;
	int results[SLAM_NUMBER*SLAM_REPEAT];

	struct hostent *server;
    struct sockaddr_in6 addr6;
	struct sockaddr_in addr;

	int start_stack_timer_count = pico_ntimers(); // number of picoTCP timers allocated

	// TESTS:
	// socket()
	// close()
	if(false)
	{
		// open and close SLAM_NUMBER*SLAM_REPEAT sockets
		for(int j=0; j<SLAM_REPEAT; j++) {
			std::cout << "slamming " << j << " time(s)" << std::endl;
			usleep(SLAM_INTERVAL);
			// create sockets
			int fds[SLAM_NUMBER];
			for(int i = 0; i<SLAM_NUMBER; i++) {
				if((err = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
					std::cout << "error creating socket (errno = " << errno << ")" << std::endl;
					if(errno == EMFILE)
						break;
					else
						return -1;
				}
				else
					fds[i] = err;
				std::cout << "\tcreating " << i << " socket(s) fd = " << err << std::endl;

			}
			// close sockets
			for(int i = 0; i<SLAM_NUMBER; i++) {
				//std::cout << "\tclosing " << i << " socket(s)" << std::endl;
				if((err = zts_close(fds[i])) < 0) {
					std::cout << "error closing socket (errno = " << errno << ")" << std::endl;
					//return -1;
				}
				else
					fds[i] = -1;
			}
		}
		if(zts_nsockets() == 0)
			std::cout << "PASSED [slam open and close]" << std::endl;
		else
			std::cout << "FAILED [slam open and close] - sockets left unclosed" << std::endl;
	}

	// ---

	// TESTS:
	// socket()
	// bind()
	// listen()
	// accept()
	// close()
	if(false) 
	{
		int sock = 0;
		std::vector<int> used_ports;

		for(int j=0; j<SLAM_REPEAT; j++) {
			std::cout << "slamming " << j << " time(s)" << std::endl;
			usleep(SLAM_INTERVAL);

			for(int i = 0; i<SLAM_NUMBER; i++) {
				if((sock = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
					std::cout << "error creating socket (errno = " << errno << ")" << std::endl;
					if(errno == EMFILE)
						break;
					else
						return -1;
				}
				std::cout << "socket() = " << sock << std::endl;
				usleep(SLAM_INTERVAL);

				int port;
				while(!(std::find(used_ports.begin(),used_ports.end(),port) == used_ports.end())) {
			    	port = MIN_PORT + (rand() % (int)(MAX_PORT - MIN_PORT + 1));
			    }
			    used_ports.push_back(port);
			    std::cout << "port = " << port << std::endl;
				
				if(false) {
					server = gethostbyname2("::",AF_INET6);
	    			memset((char *) &addr6, 0, sizeof(addr6));
	    			addr6.sin6_flowinfo = 0;
	    			addr6.sin6_family = AF_INET6;
	    			addr6.sin6_port = htons(port);
					addr6.sin6_addr = in6addr_any;
					err = zts_bind(sock, (struct sockaddr *)&addr6, (socklen_t)(sizeof addr6));
				}

				if(true) {
					addr.sin_port = htons(port);
					addr.sin_addr.s_addr = inet_addr("10.9.9.50");
					//addr.sin_addr.s_addr = htons(INADDR_ANY);
					addr.sin_family = AF_INET;
					err = zts_bind(sock, (struct sockaddr *)&addr, (socklen_t)(sizeof addr));
				}
				if(err < 0) {
					std::cout << "error binding socket (errno = " << errno << ")" << std::endl;
					return -1;
				}
				
				if(sock > 0) {
					if((err = zts_close(sock)) < 0) {
						std::cout << "error closing socket (errno = " << errno << ")" << std::endl;
						//return -1;
					}
				}
			}
		}
		used_ports.clear();
		if(zts_nsockets() == 0)
			std::cout << "PASSED [slam open, bind, listen, accept, close]" << std::endl;
		else
			std::cout << "FAILED [slam open, bind, listen, accept, close]" << std::endl;
	}

	// TESTS:
	// (1) socket()
	// (2) connect()
	// (3) close()
	int num_times = zts_maxsockets();
	std::cout << "socket/connect/close - " << num_times << " times" << std::endl;
	for(int i=0;i<(SLAM_NUMBER*SLAM_REPEAT); i++) { results[i] = 0; }
	if(true) 
	{
		int port = 4545;
		
		// open, bind, listen, accept, close
		for(int j=0; j<num_times; j++) {
			int sock = 0;
			errno = 0;

			usleep(SLAM_INTERVAL);

			// socket()
			printf("creating socket... (%d)\n", j);
			if((sock = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0)
				std::cout << "error creating socket (errno = " << errno << ")" << std::endl;
			results[j] = std::min(results[j], sock);
			
			// set O_NONBLOCK
			if((err = zts_fcntl(sock, F_SETFL, O_NONBLOCK) < 0))
				std::cout << "error setting O_NONBLOCK (errno=" << errno << ")" << std::endl;
			results[j] = std::min(results[j], err);

			// connect()
			if(false) {
				server = gethostbyname2("::",AF_INET6);
    			memset((char *) &addr6, 0, sizeof(addr6));
    			addr6.sin6_flowinfo = 0;
    			addr6.sin6_family = AF_INET6;
    			addr6.sin6_port = htons(port);
				addr6.sin6_addr = in6addr_any;
				err = zts_connect(sock, (struct sockaddr *)&addr6, (socklen_t)(sizeof addr6));
			}
			if(true) {
				addr.sin_port = htons(port);
				addr.sin_addr.s_addr = inet_addr("10.9.9.51");
				//addr.sin_addr.s_addr = htons(INADDR_ANY);
				addr.sin_family = AF_INET;
				err = zts_connect(sock, (struct sockaddr *)&addr, (socklen_t)(sizeof addr));
			}

			if(errno != EINPROGRESS) { // acceptable error for non-block mode
				if(err < 0)
					std::cout << "error connecting socket (errno = " << errno << ")" << std::endl;
				results[j] = std::min(results[j], err);
			}

			// close()
			if((err = zts_close(sock)) < 0)
				std::cout << "error closing socket (errno = " << errno << ")" << std::endl;
			results[j] = std::min(results[j], err);
		}

		//while(pico_ntimers() > start_stack_timer_count) {
		//	sleep(10);
		//	printf("timers = %d\n", pico_ntimers());
		//}

		displayResults(results, num_times);
		if(zts_nsockets() == 0)
			std::cout << "PASSED [slam open, connect, close]" << std::endl;
		else
			std::cout << "FAILED [slam open, connect, close]" << std::endl;
	}
	return 0;
}




/****************************************************************************/
/* RANDOMIZED API TEST                                                      */
/****************************************************************************/

int random_api_test()
{
	// PASSED implies we didn't segfault or hang anywhere

	//
	int calls_made = 0;

	// how many calls we'll make
    int num_of_api_calls = 10;

/*
zts_socket()
zts_connect()
zts_listen()
zts_accept()
zts_bind()
zts_getsockopt()
zts_setsockopt()
zts_fnctl()
zts_close()
*/

	// variables which will be populated with random values
	int fd, arg_val;
	struct sockaddr_in addr;
	struct sockaddr_in6 addr6;

	while(calls_made < num_of_api_calls)
	{
		fprintf(stderr, "calls_made = %d\n", calls_made);
		int random_call = 0;

/*
		switch(random_call)
		{
			default:
				printf()
		}
*/



		calls_made++;
	}
	return PASSED;
}


/****************************************************************************/
/* test driver, called from main()                                          */
/****************************************************************************/

/*
*
* path      = place where ZT keys, and config files will be stored 
* nwid      = network for app to join
* type      = simple, sustained
* protocol  = 4, 6
* mode      = client, server
* addr      = ip address string
* port      = integer
* operation = n_times, n_seconds, n_bytes, etc
* n_count   = number of operations of type
* delay     = delay between each operation
*
*/
int do_test(std::string path, std::string nwid, int type, int protocol, int mode, std::string ipstr, int port, int operation, int n_count, int delay)
{
	struct hostent *server;
    struct sockaddr_in6 addr6;
	struct sockaddr_in addr;

	printf("\npath      = %s\n", path.c_str());
	printf("nwid      = %s\n", nwid.c_str());
	printf("type      = %d\n", type);
	printf("protocol  = %d\n", protocol);
	printf("mode      = %d\n", mode);
	printf("ipstr     = %s\n", ipstr.c_str());
	printf("port      = %d\n", port);
	printf("operation = %d\n", operation);
	printf("n_count   = %d\n", n_count);
	printf("delay     = %d\n\n", delay);

	/****************************************************************************/
	/* SIMPLE                                                                   */
	/****************************************************************************/

	// SIMPLE
	// performs a one-off test of a particular subset of the API
	// For instance (ipv4 client, ipv6 server, etc)
	if(type == TEST_TYPE_SIMPLE) {		
		if(mode == TEST_MODE_CLIENT) {

			std::cout << "connecting to " << ipstr << " on port " << port << std::endl;
			// IPv4
			if(protocol == 4) {
				addr.sin_addr.s_addr = inet_addr(ipstr.c_str());
				addr.sin_family = AF_INET;
				addr.sin_port = htons(port);
				//printf(" running (%d) test as ipv=%d\n", mode, protocol);
				return ipv4_tcp_client_test(&addr, port);
			}
			// IPv6
			if(protocol == 6) {
				server = gethostbyname2(ipstr.c_str(),AF_INET6);
    			memset((char *) &addr6, 0, sizeof(addr6));
    			addr6.sin6_flowinfo = 0;
    			addr6.sin6_family = AF_INET6;
    			memmove((char *) &addr6.sin6_addr.s6_addr, (char *) server->h_addr, server->h_length);
    			addr6.sin6_port = htons(port);
				//printf(" running (%d) test as ipv=%d\n", mode, protocol);
				return ipv6_tcp_client_test(&addr6, port);
			}
		}

		if(mode == TEST_MODE_SERVER) {

			//printf("serving on port %s\n", port);
			// IPv4
			if(protocol == 4) {
				addr.sin_port = htons(port);
				addr.sin_addr.s_addr = inet_addr(ipstr.c_str());
				// addr.sin_addr.s_addr = htons(INADDR_ANY);
				addr.sin_family = AF_INET;
				//printf(" running (%d) test as ipv=%d\n", mode, protocol);
				return ipv4_tcp_server_test(&addr, port);
			}
			// IPv6
			if(protocol == 6) {
				server = gethostbyname2(ipstr.c_str(),AF_INET6);
				memset((char *) &addr6, 0, sizeof(addr6));
			    addr6.sin6_flowinfo = 0;
			    addr6.sin6_family = AF_INET6;
			    memmove((char *) &addr6.sin6_addr.s6_addr, (char *) server->h_addr, server->h_length);
			    addr6.sin6_port = htons(port);
				return ipv6_tcp_server_test(&addr6, port);
			}
		}
	}

	/****************************************************************************/
	/* SUSTAINED                                                                */
	/****************************************************************************/

	// ./unit zt2 c7cd7c9e1b0f52a2 simple 4 client 10.9.9.40 8787 n_seconds 10 50
	// ./unit zt2 c7cd7c9e1b0f52a2 simple 4 client 10.9.9.40 8787 n_bytes 100 50
	// ./unit zt2 c7cd7c9e1b0f52a2 simple 4 client 10.9.9.40 8787 n_times 100 50

	// SUSTAINED
	// Performs a stress test for benchmarking performance
	if(type == TEST_TYPE_SUSTAINED) {
		if(mode == TEST_MODE_CLIENT) {

			//printf("connecting to %s on port %d\n", ipstr, port);
			// IPv4
			if(protocol == 4) {
				addr.sin_port = htons(port);
				addr.sin_addr.s_addr = inet_addr(ipstr.c_str());
				addr.sin_family = AF_INET;
				//printf(" running (%d) test as ipv=%d\n", mode, protocol);
				return ipv4_tcp_client_sustained_test(&addr, port, operation, n_count, delay);
			}
			// IPv6
			if(protocol == 6) {
				server = gethostbyname2(ipstr.c_str(),AF_INET6);
				memset((char *) &addr6, 0, sizeof(addr6));
			    addr6.sin6_flowinfo = 0;
			    addr6.sin6_family = AF_INET6;
			    memmove((char *) &addr6.sin6_addr.s6_addr, (char *) server->h_addr, server->h_length);
			    addr6.sin6_port = htons(port);
				return ipv6_tcp_client_sustained_test(&addr6, port, operation, n_count, delay);
			}
		}

		if(mode == TEST_MODE_SERVER)
		{
			//printf("serving on port %d\n", port);
			// IPv4
			if(protocol == 4) {
				addr.sin_port = htons(port);
				addr.sin_addr.s_addr = inet_addr(ipstr.c_str());
				// addr.sin_addr.s_addr = htons(INADDR_ANY);
				addr.sin_family = AF_INET;
				//printf(" running (%d) test as ipv=%d\n", mode, protocol);
				return ipv4_tcp_server_sustained_test(&addr, port, operation, n_count, delay);
			}
			// IPv6
			if(protocol == 6) {
				server = gethostbyname2(ipstr.c_str(),AF_INET6);
    			memset((char *) &addr6, 0, sizeof(addr6));
    			addr6.sin6_flowinfo = 0;
    			addr6.sin6_family = AF_INET6;
    			addr6.sin6_port = htons(port);
				addr6.sin6_addr = in6addr_any;
    			//memmove((char *) &addr6.sin6_addr.s6_addr, (char *) server->h_addr, server->h_length);
				//printf(" running (%d) test as ipv=%d\n", mode, protocol);
				return ipv6_tcp_server_sustained_test(&addr6, port, operation, n_count, delay);
			}
		}
	}
	return 0;
}




/****************************************************************************/
/* main (calls test driver: do_test(...))                                   */
/****************************************************************************/

// zt2 c7cd7c9e1b0f52a2 simple 4 client 10.9.9.40 8787 n_seconds 10 50
// int do_test(std::string path, std::string nwid, int type, int protocol, int mode, char *ipstr, int port, int operation, int n_count, int delay)

int main(int argc , char *argv[])
{
    if(argc < 1) {
        printf("usage(1): ./unit <path> <nwid> <simple|sustained|random> <4|6> <client|server> <port> <operation> <count> <delay>\n");     
        printf("usage(2): selftest.conf\n");  
        return 1;
    }

    int err          = 0;
	int type         = 0;
    int protocol     = 0;
    int mode         = 0;
    int port         = 0;
    int local_port   = 0;
    int remote_port  = 0;
    int local_port6  = 0;
    int remote_port6 = 0;
    int operation    = 0;
	int n_count      = 0;
	int delay        = 0;

	std::string nwid, stype, path = argv[1];
	std::string ipstr, ipstr6, local_ipstr, local_ipstr6, remote_ipstr, remote_ipstr6;

	memcpy(str, "welcome to the machine", 22);

	// if a test config file was specified:
	// load addresses/path, perform comprehensive test
	if(path.find(".conf") != std::string::npos) 
	{
		loadTestConfigFile(path);
		nwid   = testConf["nwid"];
		path   = testConf["local_path"];
		stype  = "comprehensive";
		local_ipstr   = testConf["local_ipv4"];
		local_ipstr6  = testConf["local_ipv6"];
		remote_ipstr  = testConf["remote_ipv4"];
		remote_ipstr6 = testConf["remote_ipv6"];
		std::string smode   = testConf["mode"];

		if(strcmp(smode.c_str(), "server") == 0)
			mode = TEST_MODE_SERVER;
		else
			mode = TEST_MODE_CLIENT;

		local_port = atoi(testConf["local_port"].c_str());
		remote_port = atoi(testConf["remote_port"].c_str());

		local_port6 = atoi(testConf["local_port6"].c_str());
		remote_port6 = atoi(testConf["remote_port6"].c_str());

		fprintf(stderr, "local_ipstr       = %s\n", local_ipstr.c_str());
		fprintf(stderr, "local_ipstr6      = %s\n", local_ipstr6.c_str());
		fprintf(stderr, "remote_ipstr      = %s\n", remote_ipstr.c_str());
		fprintf(stderr, "remote_ipstr6     = %s\n", remote_ipstr6.c_str());
		
		fprintf(stderr, "remote_port       = %d\n", remote_port);
		fprintf(stderr, "remote_port6      = %d\n", remote_port6);
		fprintf(stderr, "local_port        = %d\n", local_port);
		fprintf(stderr, "local_port6       = %d\n", local_port6);
	}
	else
	{
		nwid  = argv[2];
		stype = argv[3];
	}

	fprintf(stderr, "path        = %s\n", path.c_str());
	fprintf(stderr, "nwid        = %s\n", nwid.c_str());
	fprintf(stderr, "type        = %s\n", stype.c_str());


	printf("waiting for libzt to come online\n");
	zts_simple_start(path.c_str(), nwid.c_str());
	// What follows is a long-form of zts_simple_start():
		// zts_start(path.c_str());
		// printf("waiting for service to start...\n");
		// while(!zts_running())
		//	sleep(1);
		// printf("joining network...\n");
		// zts_join(nwid.c_str());
		// printf("waiting for address assignment...\n");
		// while(!zts_has_address(nwid.c_str()))
		//	sleep(1);

	// SLAM
	// Perform thsouands of repetitions of the same plausible API sequences to detect faults
	if(stype == "slam")
	{
		slam_api_test();
		return 0;
	}

	// SIMPLE
	// performs a one-off test of a particular subset of the API
	// For instance (ipv4 client, ipv6 server, etc)
	if(stype == "simple")
	{
		printf("performing SIMPLE test\n");
		// Parse args
		type     = TEST_TYPE_SIMPLE;
		protocol = atoi(argv[4]);
		if(!strcmp(argv[5],"client"))
			mode = TEST_MODE_CLIENT;
		if(!strcmp(argv[5],"server"))
			mode = TEST_MODE_SERVER;
		ipstr = argv[6];
		port = atoi(argv[7]);
		
		// Perform test
	    if((err = do_test(path, nwid, type, protocol, mode, ipstr, port, operation, n_count, delay)) == PASSED)
	    	fprintf(stderr, "PASSED\n");
	    else
	    	fprintf(stderr, "FAILED\n");
	    return err;
	}

	// SUSTAINED
	// Performs a stress test for benchmarking performance
	if(stype == "sustained")
	{
		printf("performing SUSTAINED test\n");
		type     = TEST_TYPE_SUSTAINED;
		protocol = atoi(argv[4]);
		if(!strcmp(argv[5],"client"))
			mode = TEST_MODE_CLIENT;
		if(!strcmp(argv[5],"server"))
			mode = TEST_MODE_SERVER;
		ipstr = argv[6];
		port = atoi(argv[7]);


		std::string s_operation = argv[ 8];  // n_count, n_count, n_count
		n_count  = atoi(argv[ 9]); // 10, 100, 1000, ...
		delay    = atoi(argv[10]); // 100 (in ms)
		
		if(s_operation == "n_times")
			operation = TEST_OP_N_TIMES;
		if(s_operation == "n_bytes")
			operation = TEST_OP_N_BYTES;
		if(s_operation == "n_seconds")
			operation = TEST_OP_N_SECONDS;

		// Perform test
	    if((err = do_test(path, nwid, type, protocol, mode, ipstr, port, operation, n_count, delay)) == PASSED)
	    	fprintf(stderr, "PASSED\n");
	    else
	    	fprintf(stderr, "FAILED\n");
	    return err;
	}

	/****************************************************************************/
	/* COMPREHENSIVE                                                            */
	/****************************************************************************/

	// Use test/*.conf files to specify test setup
	// More information can be found in TESTING.md

	// COMPREHENSIVE
	// Tests ALL API calls
	if(stype == "comprehensive")
	{	

		//printf("performing COMPREHENSIVE ipv4 test\n");
		/* Each host must operate as the counterpart to the other, thus, each mode 
		 * will call the same test helper functions in different orders
		 * Additionally, the test will use the preset paremeters below for the test:
		 */

		delay     =  0;
		n_count   = 10;
		type      = TEST_TYPE_SIMPLE;
		operation = TEST_OP_N_TIMES;
		protocol  = 4;
		
		if(mode == TEST_MODE_SERVER) {
			printf("starting comprehensive test as SERVER\n");
			port  = local_port;
			ipstr = local_ipstr;
		}
		else if(mode == TEST_MODE_CLIENT) {
			printf("starting comprehensive test as CLIENT (waiting, giving server time to start)\n");
			sleep(10); // give the server some time to come online before beginning test
			port  = remote_port;
			ipstr = remote_ipstr;
		}

		// IPV4 (first test)
		
		do_test(path, nwid, type, protocol, mode, ipstr, port, operation, n_count, delay);

		// swtich modes (client/server)
		if(mode == TEST_MODE_SERVER) {
			printf("\nswitching from SERVER to CLIENT mode\n");
			port  = remote_port;
			ipstr = remote_ipstr;
			mode  = TEST_MODE_CLIENT;
		}
		else if(mode == TEST_MODE_CLIENT) {
			printf("switching from CLIENT to SERVER mode\n");
			port  = local_port;
			ipstr = local_ipstr;
			mode  = TEST_MODE_SERVER;
		}
		
		// IPV4 (second test)
		do_test(path, nwid, type, protocol, mode, ipstr, port, operation, n_count, delay);
		sleep(3);

// IPV6

		printf("performing COMPREHENSIVE ipv6 test\n");
		/* Each host must operate as the counterpart to the other, thus, each mode 
		 * will call the same test helper functions in different orders
		 * Additionally, the test will use the preset paremeters below for the test:
		 */

		delay     =  0;
		n_count   = 10;
		type      = TEST_TYPE_SIMPLE;
		operation = TEST_OP_N_TIMES;
		protocol  = 6;
		
		if(mode == TEST_MODE_SERVER) {
			printf("starting comprehensive test as SERVER\n");
			port  = local_port6;
			ipstr6 = local_ipstr6;
		}
		else if(mode == TEST_MODE_CLIENT) {
			printf("starting comprehensive test as CLIENT (waiting, giving server time to start)\n");
			sleep(10); // give the server some time to come online before beginning test
			port  = remote_port6;
			ipstr6 = remote_ipstr6;
		}

		// IPV4 (first test)
		
		do_test(path, nwid, type, protocol, mode, ipstr6, port, operation, n_count, delay);

		// swtich modes (client/server)
		if(mode == TEST_MODE_SERVER) {
			printf("\nswitching from SERVER to CLIENT mode\n");
			port  = remote_port6;
			ipstr6 = remote_ipstr6;
			mode  = TEST_MODE_CLIENT;
		}
		else if(mode == TEST_MODE_CLIENT) {
			printf("switching from CLIENT to SERVER mode\n");
			port  = local_port6;
			ipstr6 = local_ipstr6;
			mode  = TEST_MODE_SERVER;
		}
		
		// IPV4 (second test)
		do_test(path, nwid, type, protocol, mode, ipstr6, port, operation, n_count, delay);
		sleep(3);

	}


	/****************************************************************************/
	/* RANDOM                                                                   */
	/****************************************************************************/

	// RANDOM
	// performs random API calls with plausible (and random) arguments/data
	if(stype == "random")
	{
		random_api_test();
	}

	while(1)
		sleep(1);
	return 0;
}