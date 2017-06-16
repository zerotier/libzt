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
#include <ctime>
#include <sys/time.h>

#include "libzt.h"

#define EXIT_ON_FAIL           true

#define PASSED                 1
#define FAILED                 0

#define ECHO_INTERVAL          100000 // us
#define SLAM_INTERVAL          500000
#define STR_SIZE               32

#define TEST_OP_N_BYTES        10
#define TEST_OP_N_SECONDS      11
#define TEST_OP_N_TIMES        12

#define TEST_MODE_CLIENT       20
#define TEST_MODE_SERVER       21

#define TEST_TYPE_SIMPLE       30
#define TEST_TYPE_SUSTAINED    31
#define TEST_TYPE_PERF         32
#define TEST_TYPE_PERF_TO_ECHO 33

#define MIN_PORT               5000
#define MAX_PORT               50000

#define UNIT_TEST_SIG_4        struct sockaddr_in  *addr, int operation, int n_count, int delay, char *details, bool *passed
#define UNIT_TEST_SIG_6        struct sockaddr_in6 *addr, int operation, int n_count, int delay, char *details, bool *passed

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
	[OK] comprehensive client ipv4 - test all ipv4/6 client simple/sustained modes
	[OK] comprehensive server ipv6 - test all ipv4/6 server simple/sustained modes

	Performance:

	[OK]                Throughput - Test maximum RX/TX speeds
	[  ]              Memory Usage - Test memory consumption profile
	[  ]                 CPU Usage - Test processor usage
	[  ]               

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
	while (testFile >> key >> value) {
		if(key[0] != '#')
	    	testConf[key] = value;
	}
	testFile.close();
}

long int get_now_ts()
{
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

/****************************************************************************/
/* SIMPLE                                                                   */
/****************************************************************************/

// 
void tcp_client_4(UNIT_TEST_SIG_4)
{
	DEBUG_TEST();
	int r, w, sockfd, err, len = strlen(str);
	char rbuf[STR_SIZE];
	memset(rbuf, 0, sizeof rbuf);
	if((sockfd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0)
		printf("error creating ZeroTier socket");
	if((err = zts_connect(sockfd, (const struct sockaddr *)addr, sizeof(addr))) < 0)
		printf("error connecting to remote host (%d)\n", err);
	w = zts_write(sockfd, str, len);
	r = zts_read(sockfd, rbuf, len);
	err = zts_close(sockfd);
	sprintf(details, "n_count = %d, err = %d, r = %d, w = %d", n_count, err, r, w);
	*passed = (w == len && r == len && !err) && !strcmp(rbuf, str);
}

// 
void tcp_client_6(UNIT_TEST_SIG_6)
{
	DEBUG_TEST();
	int r, w, sockfd, err, len = strlen(str);
	char rbuf[STR_SIZE];
	memset(rbuf, 0, sizeof rbuf);
	if((sockfd = zts_socket(AF_INET6, SOCK_STREAM, 0)) < 0)
		printf("error creating ZeroTier socket");
	if((err = zts_connect(sockfd, (const struct sockaddr *)addr, sizeof(addr))) < 0)
		printf("error connecting to remote host (%d)\n", err);
	w = zts_write(sockfd, str, len);
	r = zts_read(sockfd, rbuf, len);
	err = zts_close(sockfd);
	sprintf(details, "n_count = %d, err = %d, r = %d, w = %d", n_count, err, r, w);
	DEBUG_TEST("%s", rbuf);
	DEBUG_TEST("%s", str);
	DEBUG_TEST("%d", strcmp(rbuf, str));
	*passed = (w == len && r == len && !err) && !strcmp(rbuf, str);
}

//
void tcp_server_4(UNIT_TEST_SIG_4)
{
	DEBUG_TEST();
	int w=0, r=0, sockfd, accfd, err, len = strlen(str);
	char rbuf[STR_SIZE];
	memset(rbuf, 0, sizeof rbuf);
	if((sockfd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0)
		printf("error creating ZeroTier socket");
	if((err = zts_bind(sockfd, (struct sockaddr *)addr, sizeof(struct sockaddr_in)) < 0))
		printf("error binding to interface (%d)\n", err);
	if((err = zts_listen(sockfd, 100)) < 0)
		printf("error placing socket in LISTENING state (%d)\n", err);
	if((accfd = zts_accept(sockfd, (struct sockaddr *)&addr, (socklen_t *)sizeof(addr))) < 0)
		printf("error accepting connection (%d)\n", err);
	r = zts_read(accfd, rbuf, sizeof rbuf);
	w = zts_write(accfd, rbuf, len);
	zts_close(sockfd);
	zts_close(accfd);
	sprintf(details, "n_count = %d, err = %d, r = %d, w = %d", n_count, err, r, w);
	*passed = (w == len && r == len && !err) && !strcmp(rbuf, str);
}

//
void tcp_server_6(UNIT_TEST_SIG_6)
{
	DEBUG_TEST();
	int w=0, r=0, sockfd, accfd, err, len = strlen(str);
	char rbuf[STR_SIZE];
	memset(rbuf, 0, sizeof rbuf);
	if((sockfd = zts_socket(AF_INET6, SOCK_STREAM, 0)) < 0)
		printf("error creating ZeroTier socket");
	if((err = zts_bind(sockfd, (struct sockaddr *)addr, sizeof(struct sockaddr_in)) < 0))
		printf("error binding to interface (%d)\n", err);
	if((err = zts_listen(sockfd, 100)) < 0)
		printf("error placing socket in LISTENING state (%d)\n", err);
	if((accfd = zts_accept(sockfd, (struct sockaddr *)&addr, (socklen_t *)sizeof(addr))) < 0)
		printf("error accepting connection (%d)\n", err);
	r = zts_read(accfd, rbuf, sizeof rbuf);
	w = zts_write(accfd, rbuf, len);
	zts_close(sockfd);
	zts_close(accfd);
	sprintf(details, "n_count = %d, err = %d, r = %d, w = %d", n_count, err, r, w);
	*passed = (w == len && r == len && !err) && !strcmp(rbuf, str);
}





/****************************************************************************/
/* SUSTAINED                                                                */
/****************************************************************************/

// Maintain transfer for n_count OR n_count
void tcp_client_sustained_4(UNIT_TEST_SIG_4)
{
	DEBUG_TEST();
	int tot=0, n=0, w=0, r=0, sockfd, err, len = strlen(str);
	char rbuf[STR_SIZE];
	if((sockfd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0)
		printf("error creating ZeroTier socket");
	if((err = zts_connect(sockfd, (const struct sockaddr *)addr, sizeof(addr))) < 0)
		printf("error connecting to remote host (%d)\n", err);

	if(operation == TEST_OP_N_TIMES) {
		tot = len*n_count;
		std::time_t start_time = std::time(nullptr);
		for(int i=0; i<n_count; i++) {
			n = zts_write(sockfd, str, len);
			if (n > 0)
				w += n;
			n = zts_read(sockfd, rbuf, len);
			if (n > 0)
				r += n;
		}
		std::time_t end_time = std::time(nullptr);
		sleep(2);
		err = zts_close(sockfd);
		time_t ts_delta = end_time - start_time;
		sprintf(details, "n_count = %d, ts_delta = %d, r = %d, w = %d", n_count, ts_delta, r, w);
		*passed = (r == tot && w == tot && !err) && !strcmp(rbuf, str);
	}
	if(operation == TEST_OP_N_BYTES) {
		tot = n_count;
		while(r < tot || w < tot) {
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
		sprintf(details, "n_count = %d\n", n_count);
		*passed = (r == tot && w == tot && !err);
	}
}

// Maintain transfer for n_count OR n_count
void tcp_client_sustained_6(UNIT_TEST_SIG_6)
{
	DEBUG_TEST();
	int tot=0, n=0, w=0, r=0, sockfd, err, len = strlen(str);
	char rbuf[STR_SIZE];
	if((sockfd = zts_socket(AF_INET6, SOCK_STREAM, 0)) < 0)
		printf("error creating ZeroTier socket");
	if((err = zts_connect(sockfd, (const struct sockaddr *)addr, sizeof(addr))) < 0)
		printf("error connecting to remote host (%d)\n", err);
	//zts_fcntl(sockfd, F_SETFL, O_NONBLOCK);
	if(operation == TEST_OP_N_TIMES) {
		std::time_t start_time = std::time(nullptr);
		tot = len*n_count;
		for(int i=0; i<n_count; i++) {
			//usleep(delay * 1000);
			n = zts_write(sockfd, str, len);
			if (n > 0)
				w += n;
			n = zts_read(sockfd, rbuf, len);
			if (n > 0)
				r += n;
		}
		std::time_t end_time = std::time(nullptr);
		sleep(2);
		err = zts_close(sockfd);
		time_t ts_delta = end_time - start_time;
		sprintf(details, "n_count = %d, ts_delta = %d, r = %d, w = %d", n_count, ts_delta, r, w);
		*passed = (r == tot && w == tot && !err) && !strcmp(rbuf, str);
	}
	if(operation == TEST_OP_N_BYTES) {
		tot = n_count;
		while(r < tot || w < tot) {
			//usleep(delay * 1000);
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
		sprintf(details, "n_count = %d\n", n_count);
		*passed = (r == tot && w == tot && !err);
	}
}

// Maintain transfer for n_count OR n_count
void tcp_server_sustained_4(UNIT_TEST_SIG_4)
{
	DEBUG_TEST();
	int tot=0, n=0, w=0, r=0, sockfd, accfd, err, len = strlen(str);
	char rbuf[STR_SIZE];
	if((sockfd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0)
		printf("error creating ZeroTier socket");
	if((err = zts_bind(sockfd, (struct sockaddr *)addr, (socklen_t)sizeof(struct sockaddr_in)) < 0))
		printf("error binding to interface (%d)\n", err);
	if((err = zts_listen(sockfd, 1)) < 0)
		printf("error placing socket in LISTENING state (%d)", err);
	if((accfd = zts_accept(sockfd, (struct sockaddr *)&addr, (socklen_t *)sizeof(addr))) < 0)
		printf("error accepting connection (%d)\n", err);
	//zts_fcntl(accfd, F_SETFL, O_NONBLOCK);
	if(operation == TEST_OP_N_TIMES) {
		tot = len*n_count;
		std::time_t start_time = std::time(nullptr);
		for(int i=0; i<n_count; i++) {
			//usleep(delay * 1000);
			r += zts_read(accfd, rbuf, len);
			w += zts_write(accfd, rbuf, len);		
		}
		std::time_t end_time = std::time(nullptr);
		sleep(2);
		zts_close(sockfd);
		zts_close(accfd);
		time_t ts_delta = end_time - start_time;
		sprintf(details, "n_count = %d, ts_delta = %d, r = %d, w = %d", n_count, ts_delta, r, w);
		*passed = (r == tot && w == tot && !err) && !strcmp(rbuf, str);
	}
	if(operation == TEST_OP_N_BYTES) {
		tot = n_count;
		while(r < tot || w < tot) {
			//usleep(delay * 1000);
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
		sprintf(details, "n_count = %d", n_count);
		*passed = (r == tot && w == tot && !err);
	}
}

// Maintain transfer for n_count OR n_count
void tcp_server_sustained_6(UNIT_TEST_SIG_6)
{
	DEBUG_TEST();
	int tot=0, n=0, w=0, r=0, sockfd, accfd, err, len = strlen(str);
	char rbuf[STR_SIZE];
	if((sockfd = zts_socket(AF_INET6, SOCK_STREAM, 0)) < 0)
		printf("error creating ZeroTier socket");
	if((err = zts_bind(sockfd, (struct sockaddr *)addr, (socklen_t)sizeof(struct sockaddr_in)) < 0))
		printf("error binding to interface (%d)\n", err);
	if((err = zts_listen(sockfd, 1)) < 0)
		printf("error placing socket in LISTENING state (%d)\n", err);
	if((accfd = zts_accept(sockfd, (struct sockaddr *)&addr, (socklen_t *)sizeof(addr))) < 0)
		printf("error accepting connection (%d)\n", err);
	//zts_fcntl(accfd, F_SETFL, O_NONBLOCK);
	if(operation == TEST_OP_N_TIMES) {
		tot = len*n_count;
		std::time_t start_time = std::time(nullptr);
		for(int i=0; i<n_count; i++) {
			usleep(delay * 1000);
			r += zts_read(accfd, rbuf, len);
			w += zts_write(accfd, rbuf, len);		
		}
		std::time_t end_time = std::time(nullptr);
		sleep(2);
		zts_close(sockfd);
		zts_close(accfd);
		time_t ts_delta = end_time - start_time;
		sprintf(details, "n_count = %d, ts_delta = %d, r = %d, w = %d", n_count, ts_delta, r, w);
		*passed = (r == tot && w == tot && !err) && !strcmp(rbuf, str);
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
		sprintf(details, "n_count = %d", n_count);
		*passed = (r == tot && w == tot && !err);
	}
}



/****************************************************************************/
/* PERFORMANCE (between library instances)                                  */
/****************************************************************************/

// Maintain transfer for n_count OR n_count
void tcp_client_perf_4(UNIT_TEST_SIG_4)
{
	DEBUG_TEST();
	int w=0, sockfd, err;
	int total_test_sz          = 1024*1024;
	int arbitrary_chunk_sz_max = 16384;
	int arbitrary_chunk_sz_min = 512;

	char rbuf[arbitrary_chunk_sz_max];

	for (int i=arbitrary_chunk_sz_min; (i*2) < arbitrary_chunk_sz_max; i*=2) {

		if((sockfd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0)
			DEBUG_ERROR("error creating ZeroTier socket");
		if((err = zts_connect(sockfd, (const struct sockaddr *)addr, sizeof(addr))) < 0)
			DEBUG_ERROR("error connecting to remote host (%d)\n", err);

		DEBUG_TEST("[TX] Testing (%d) byte chunks: ", i);

		int chunk_sz   = i;
		int iterations = total_test_sz / chunk_sz;

		long int start_time = get_now_ts();
		w = 0;
		while(w < total_test_sz)
			w += zts_write(sockfd, rbuf, chunk_sz);
		long int end_time = get_now_ts();
		float ts_delta = (end_time - start_time) / (float)1000;
		float rate = (float)total_test_sz / (float)ts_delta;
		DEBUG_TEST("%d total bytes, time = %3f, rate = %3f KB/s", w, ts_delta, (rate / (float)1024) );

		zts_close(sockfd);		
		// let things settle after test
		sleep(5);
	}	
	*passed = (w == total_test_sz && !err) ? PASSED : FAILED;
}

// Maintain transfer for n_count OR n_count
void tcp_server_perf_4(UNIT_TEST_SIG_4)
{
	DEBUG_TEST();
	int r=0, sockfd, accfd, err;
	int total_test_sz          = 1024*1024;
	int arbitrary_chunk_sz_max = 16384;
	int arbitrary_chunk_sz_min = 512;

	char rbuf[arbitrary_chunk_sz_max];

	for (int i=arbitrary_chunk_sz_min; (i*2) < arbitrary_chunk_sz_max; i*=2) {
		if((sockfd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0)
			DEBUG_ERROR("error creating ZeroTier socket");
		if((err = zts_bind(sockfd, (struct sockaddr *)addr, (socklen_t)sizeof(struct sockaddr_in)) < 0))
			DEBUG_ERROR("error binding to interface (%d)\n", err);
		if((err = zts_listen(sockfd, 1)) < 0)
			DEBUG_ERROR("error placing socket in LISTENING state (%d)\n", err);
		if((accfd = zts_accept(sockfd, (struct sockaddr *)&addr, (socklen_t *)sizeof(addr))) < 0)
			DEBUG_ERROR("error accepting connection (%d)\n", err);

		DEBUG_TEST("[RX] Testing (%d) byte chunks: \n", i);

		int chunk_sz   = i;
		int iterations = total_test_sz / chunk_sz;

		long int start_time = get_now_ts();
		r = 0;
		while(r < total_test_sz)
			r += zts_read(accfd, rbuf, chunk_sz);
		long int end_time = get_now_ts();

		float ts_delta = (end_time - start_time) / (float)1000;
		float rate = (float)total_test_sz / (float)ts_delta;
		DEBUG_TEST("%d total bytes, time = %3f, rate = %3f KB/s", r, ts_delta, (rate / (float)1024) );		

		zts_close(sockfd);
		zts_close(accfd);
		// let things settle after test
		sleep(5);
	}
	*passed = (r == total_test_sz && !err) ? PASSED : FAILED;
}

/****************************************************************************/
/* PERFORMANCE (between library and native)                                 */
/****************************************************************************/

void tcp_client_perf_echo_4(UNIT_TEST_SIG_4)
{
	DEBUG_TEST();
	/*
	int w=0, sockfd, err;
	int total_test_sz          = 1024*1024;
	int arbitrary_chunk_sz_max = 16384;
	int arbitrary_chunk_sz_min = 512;

	char rbuf[arbitrary_chunk_sz_max];

	for (int i=arbitrary_chunk_sz_min; (i*2) < arbitrary_chunk_sz_max; i*=2) {

		if((sockfd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0)
			DEBUG_ERROR("error creating ZeroTier socket");
		if((err = zts_connect(sockfd, (const struct sockaddr *)addr, sizeof(addr))) < 0)
			DEBUG_ERROR("error connecting to remote host (%d)\n", err);

		DEBUG_TEST("[TX] Testing (%d) byte chunks: ", i);

		int chunk_sz   = i;
		int iterations = total_test_sz / chunk_sz;

		long int start_time = get_now_ts();
		w = 0;
		while(w < total_test_sz)
			w += zts_write(sockfd, rbuf, chunk_sz);
		long int end_time = get_now_ts();
		float ts_delta = (end_time - start_time) / (float)1000;
		float rate = (float)total_test_sz / (float)ts_delta;
		DEBUG_TEST("%d total bytes, time = %3f, rate = %3f KB/s", w, ts_delta, (rate / (float)1024) );

		zts_close(sockfd);		
		// let things settle after test
		sleep(5);
	}	
	*passed = (w == total_test_sz && !err) ? PASSED : FAILED;
	*/
}


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

	// int start_stack_timer_count = pico_ntimers(); // number of picoTCP timers allocated

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
	/*
	// PASSED implies we didn't segfault or hang anywhere

	//
	int calls_made = 0;

	// how many calls we'll make
    int num_of_api_calls = 10;


	zts_socket()
	zts_connect()
	zts_listen()
	zts_accept()
	zts_bind()
	zts_getsockopt()
	zts_setsockopt()
	zts_fnctl()
	zts_close()

	// variables which will be populated with random values
	int fd, arg_val;
	struct sockaddr_in addr;
	struct sockaddr_in6 addr6;

	while(calls_made < num_of_api_calls)
	{
		fprintf(stderr, "calls_made = %d\n", calls_made);
		int random_call = 0;

		switch(random_call)
		{
			default:
				printf()
		}

		calls_made++;
	}
	*/
	return PASSED;
}


/****************************************************************************/
/* test driver, called from main()                                          */
/****************************************************************************/
/*
	path      = place where ZT keys, and config files will be stored 
	nwid      = network for app to join
	type      = simple, sustained
	ipv  = 4, 6
	mode      = client, server
	addr      = ip address string
	port      = integer
	operation = n_times, n_seconds, n_bytes, etc
	n_count   = number of operations of type
	delay     = delay between each operation
*/
int test_driver(std::string name, std::string path, std::string nwid, 
	int type, 
	int ipv, 
	int mode, 
	std::string ipstr, 
	int port, 
	int operation, 
	int n_count, 
	int delay, 
	std::vector<std::string> *results)
{
	struct hostent *server;
    struct sockaddr_in6 addr6;
	struct sockaddr_in addr;
	int err = 0;
	char details[80];
	char result_str[80];
	memset(&details, 0, sizeof details);
	bool passed = 0; 
	char *ok_str = "[  OK  ]";
	char *fail_str = "[ FAIL ]";

	// Create sockadder_in objects for test calls
	if(ipv == 4) {
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = inet_addr(ipstr.c_str());
		addr.sin_family = AF_INET;
	}
	if(ipv == 6) {
		server = gethostbyname2(ipstr.c_str(),AF_INET6);
		memset((char *) &addr6, 0, sizeof(addr6));
		addr6.sin6_flowinfo = 0;
		addr6.sin6_family = AF_INET6;
		memmove((char *) &addr6.sin6_addr.s6_addr, (char *) server->h_addr, server->h_length);
		addr6.sin6_port = htons(port);
	}

	/****************************************************************************/
	/* SIMPLE                                                                   */
	/****************************************************************************/

	// performs a one-off test of a particular subset of the API
	// For instance (ipv4 client, ipv6 server, etc)
	if(type == TEST_TYPE_SIMPLE) {		
		if(mode == TEST_MODE_CLIENT) {
			sprintf(result_str, "tcp_client_%d, %s : %d, ", ipv, ipstr.c_str(), port);			
			if(ipv == 4)
				tcp_client_4(&addr, operation, n_count, delay, details, &passed);
			if(ipv == 6)
				tcp_client_6(&addr6, operation, n_count, delay, details, &passed);
		}

		if(mode == TEST_MODE_SERVER) {
			sprintf(result_str, "tcp_server_%d, %s : %d, ", ipv, ipstr.c_str(), port);			
			if(ipv == 4)
				tcp_server_4(&addr, operation, n_count, delay, details, &passed);
			if(ipv == 6)
				tcp_server_6(&addr6, operation, n_count, delay, details, &passed);
		}
	}

	/****************************************************************************/
	/* SUSTAINED                                                                */
	/****************************************************************************/

	// Performs a stress test for benchmarking performance
	if(type == TEST_TYPE_SUSTAINED) {
		if(mode == TEST_MODE_CLIENT) {
			sprintf(result_str, "tcp_client_sustained_%d, %s : %d, ", ipv, ipstr.c_str(), port);			
			if(ipv == 4)
				tcp_client_sustained_4(&addr, operation, n_count, delay, details, &passed);
			if(ipv == 6)
				tcp_client_sustained_6(&addr6, operation, n_count, delay, details, &passed);
		}

		if(mode == TEST_MODE_SERVER)
		{
			sprintf(result_str, "tcp_server_sustained_%d, %s : %d, ", ipv, ipstr.c_str(), port);			
			if(ipv == 4)				
				tcp_server_sustained_4(&addr, operation, n_count, delay, details, &passed);
			if(ipv == 6)
				tcp_server_sustained_6(&addr6, operation, n_count, delay, details, &passed);
		}
	}
	//
	if(type == TEST_TYPE_PERF) {
		if(mode == TEST_MODE_CLIENT) {
			sprintf(result_str, "tcp_client_perf_%d, %s : %d, ", ipv, ipstr.c_str(), port);
			if(ipv == 4)
				tcp_client_perf_4(&addr, operation, n_count, delay, details, &passed);
		}

		if(mode == TEST_MODE_SERVER) {
			sprintf(result_str, "tcp_server_perf_%d, %s : %d, ", ipv, ipstr.c_str(), port);
			if(ipv == 4)
				tcp_server_perf_4(&addr, operation, n_count, delay, details, &passed);
		}
	}
	//
	if(type == TEST_TYPE_PERF_TO_ECHO) {
		// Will only operate in client mode
		if(mode == TEST_MODE_CLIENT) {
			sprintf(result_str, "tcp_client_perf_echo_%d, %s : %d, ", ipv, ipstr.c_str(), port);
			if(ipv == 4)
				tcp_client_perf_echo_4(&addr, operation, n_count, delay, details, &passed);
		}
	}
	if(passed == PASSED) {
		DEBUG_TEST("%s",ok_str);
		results->push_back(std::string(ok_str) + " " + std::string(result_str) + " " + std::string(details));
	}
	else {
		DEBUG_ERROR("%s",fail_str);		
		results->push_back(std::string(fail_str) + " " + std::string(result_str) + " " + std::string(details));
	}
	if(EXIT_ON_FAIL && !passed) {
		fprintf(stderr, "%s\n", results->at(results->size()-1).c_str());
		exit(0);
	}
	return passed;
}




/****************************************************************************/
/* main(), calls test_driver(...)                                           */
/****************************************************************************/

int main(int argc , char *argv[])
{
    if(argc < 1) {
        fprintf(stderr, "usage: selftest <alice|bob>.conf\n");
        fprintf(stderr, " - Define your test environment in *.conf files.\n");     
        return 1;
    }

    std::vector<std::string> results;

    int err          = 0;
	int type         = 0;
    int ipv          = 0;
    int mode         = 0;
    int port         = 0;
    int operation    = 0;
    int start_port   = 0;
    int port_offset  = 0;
	int n_count      = 0;
	int delay        = 0;

	std::string remote_echo_ipv4;

	std::string nwid, stype, path = argv[1];
	std::string ipstr, ipstr6, local_ipstr, local_ipstr6, remote_ipstr, remote_ipstr6;
	memcpy(str, "welcome to the machine", 22);

	// if a test config file was specified:
	if(path.find(".conf") != std::string::npos) {
		//printf("\nTest config file contents:\n");
		loadTestConfigFile(path);
		nwid   = testConf["nwid"];
		path   = testConf["local_path"];
		stype  = testConf["test"];
		start_port = atoi(testConf["start_port"].c_str());
		port_offset = atoi(testConf["port_offset"].c_str());
		local_ipstr   = testConf["local_ipv4"];
		local_ipstr6  = testConf["local_ipv6"];
		remote_ipstr  = testConf["remote_ipv4"];
		remote_ipstr6 = testConf["remote_ipv6"];

		remote_echo_ipv4 = testConf["remote_echo_ipv4"];

		std::string smode   = testConf["mode"];

		if(strcmp(smode.c_str(), "server") == 0)
			mode = TEST_MODE_SERVER;
		else
			mode = TEST_MODE_CLIENT;

/*
		fprintf(stderr, "\tlocal_ipstr   = %s\n", local_ipstr.c_str());
		fprintf(stderr, "\tlocal_ipstr6  = %s\n", local_ipstr6.c_str());
		fprintf(stderr, "\tremote_ipstr  = %s\n", remote_ipstr.c_str());
		fprintf(stderr, "\tremote_ipstr6 = %s\n", remote_ipstr6.c_str());
		
		fprintf(stderr, "\tstart_port  = %d\n", start_port);
*/
	}
/*
	fprintf(stderr, "\tpath          = %s\n", path.c_str());
	fprintf(stderr, "\tnwid          = %s\n", nwid.c_str());
	fprintf(stderr, "\ttype          = %s\n\n", stype.c_str());
*/

	DEBUG_TEST("Waiting for libzt to come online...\n");
	zts_simple_start(path.c_str(), nwid.c_str());
	if(mode == TEST_MODE_SERVER)
		DEBUG_TEST("Ready. You should start selftest program on second host now...\n\n");
	if(mode == TEST_MODE_CLIENT)
		DEBUG_TEST("Ready. Contacting selftest program on first host.\n\n");

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
		DEBUG_TEST("performing SIMPLE test\n");
		// Parse args
		type     = TEST_TYPE_SIMPLE;
		ipv = atoi(argv[4]);
		if(!strcmp(argv[5],"client"))
			mode = TEST_MODE_CLIENT;
		if(!strcmp(argv[5],"server"))
			mode = TEST_MODE_SERVER;
		ipstr = argv[6];
		port = atoi(argv[7]);
		
		// Perform test
	    return test_driver(argv[5], path, nwid, type, ipv, mode, ipstr, port, operation, n_count, delay, &results);
	}

	// SUSTAINED
	// Performs a stress test for benchmarking performance
	if(stype == "sustained")
	{
		DEBUG_TEST("performing SUSTAINED test\n");
		type     = TEST_TYPE_SUSTAINED;
		ipv = atoi(argv[4]);
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
	    return test_driver(argv[5], path, nwid, type, ipv, mode, ipstr, port, operation, n_count, delay, &results);
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

// Establish initial IPV4 connection between Alice and Bob

		port      = start_port;
		delay     = 0;
		n_count   = 10;
		operation = TEST_OP_N_TIMES;
	
		if(mode == TEST_MODE_SERVER)
			ipstr = local_ipstr;
		else if(mode == TEST_MODE_CLIENT) {
			sleep(10); // give the server some time to come online before beginning test
			ipstr = remote_ipstr;
		}
		err += test_driver("ipv4", path, nwid, TEST_TYPE_SIMPLE, 4, mode, ipstr, port, operation, n_count, delay, &results);

// Perform sustained transfer

		port++;
		err += test_driver("ipv4_sustained", path, nwid, TEST_TYPE_SUSTAINED, 4, mode, ipstr, port, operation, n_count, delay, &results);

		// swtich modes (client/server)
		if(mode == TEST_MODE_SERVER) {
			ipstr = remote_ipstr;
			mode  = TEST_MODE_CLIENT;
		}
		else if(mode == TEST_MODE_CLIENT) {
			ipstr = local_ipstr;
			mode  = TEST_MODE_SERVER;
		}

		port++;
		err += test_driver("ipv4", path, nwid, TEST_TYPE_SIMPLE, 4, mode, ipstr, port, operation, n_count, delay, &results);

// IPV6

		if(mode == TEST_MODE_SERVER) {
			ipstr6 = local_ipstr6;
		}
		else if(mode == TEST_MODE_CLIENT) {
			sleep(10); // give the server some time to come online before beginning test
			ipstr6 = remote_ipstr6;
		}

		port++;
		err += test_driver("ipv6", path, nwid, TEST_TYPE_SIMPLE, 6, mode, ipstr6, port, operation, n_count, delay, &results);

// Perform sustained transfer

		port++;
		err += test_driver("ipv6_sustained", path, nwid, TEST_TYPE_SUSTAINED, 6, mode, ipstr6, port, operation, n_count, delay, &results);

		// swtich modes (client/server)
		if(mode == TEST_MODE_SERVER) {
			ipstr6 = remote_ipstr6;
			mode  = TEST_MODE_CLIENT;
		}
		else if(mode == TEST_MODE_CLIENT) {
			ipstr6 = local_ipstr6;
			mode  = TEST_MODE_SERVER;
		}

		port++;
		err += test_driver("ipv6", path, nwid, TEST_TYPE_SIMPLE, 6, mode, ipstr6, port, operation, n_count, delay, &results);


// PERFORMANCE (between library instances)
		
		n_count   = 1024*1024;
		operation = TEST_OP_N_BYTES;

		if(mode == TEST_MODE_SERVER) {
			ipstr = remote_ipstr;
			mode  = TEST_MODE_CLIENT;
		}
		else if(mode == TEST_MODE_CLIENT) {
			ipstr = local_ipstr;
			mode  = TEST_MODE_SERVER;
		}

		port++;
		err += test_driver("ipv4_perf", path, nwid, TEST_TYPE_PERF, 4, mode, ipstr, port, operation, n_count, delay, &results);		

// PERFORMANCE (between this library instance and a native non library instance (echo) )
// Client/Server mode isn't being tested here, so it isn't important, we'll just set it to client

		n_count   = 1024*1024;
		operation = TEST_OP_N_BYTES;

		mode = TEST_MODE_CLIENT;
		ipstr = remote_echo_ipv4;

		port=start_port+port_offset;
		err += test_driver("ipv4_perf_to_echo", path, nwid, TEST_TYPE_PERF_TO_ECHO, 4, mode, ipstr, port, operation, n_count, delay, &results);	

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

	printf("--------------------------------------------------------------------------------\n");
	for(int i=0;i<results.size(); i++) {
		fprintf(stderr, "%s\n", results[i].c_str());
	}

	return err;
}