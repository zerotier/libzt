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
#include <netinet/tcp.h>
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

#define EXIT_ON_FAIL           false

#define PASSED                 1
#define FAILED                 0

#define ECHO_INTERVAL          1000000 // us
#define SLAM_INTERVAL          500000

#define WAIT_FOR_SERVER_TO_COME_ONLINE    2
#define WAIT_FOR_TEST_TO_CONCLUDE         15
#define WAIT_FOR_TRANSMISSION_TO_COMPLETE 5

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

#define UNIT_TEST_SIG_4        struct sockaddr_in  *addr, int operation, int count, int delay, char *details, bool *passed
#define UNIT_TEST_SIG_6        struct sockaddr_in6 *addr, int operation, int count, int delay, char *details, bool *passed

#define ECHOTEST_MODE_RX       333
#define ECHOTEST_MODE_TX       666

#define DATA_BUF_SZ            1024*32

#define MAX_RX_BUF_SZ          2048
#define MAX_TX_BUF_SZ          2048

#define ONE_MEGABYTE           1024 * 1024

#define DETAILS_STR_LEN        128

char str[STR_SIZE];

std::map<std::string, std::string> testConf;


// TODO: check for correct byte order in sustained and performance tests

/* Tests in this file:

	Basic RX/TX connect()/accept() Functionality:

	[ ?]                      slam - perform thousands of the same call per second
	[  ]                    random - act like a monkey, press all the buttons
	[OK]        simple client ipv4 - connect, send one message and wait for an echo
	[OK]        simple server ipv4 - accept, read one message and echo it back
	[OK]        simple client ipv6 - connect, send one message and wait for an echo
	[OK]        simple server ipv6 - accept, read one message and echo it back
	[OK]     sustained client ipv4 - connect and rx/tx many messages, VERIFIES data integrity
	[OK]     sustained server ipv4 - accept and echo messages, VERIFIES data integrity
	[OK]     sustained client ipv6 - connect and rx/tx many messages, VERIFIES data integrity
	[OK]     sustained server ipv6 - accept and echo messages, VERIFIES data integrity
	[OK] comprehensive client ipv4 - test all ipv4/6 client simple/sustained modes
	[OK] comprehensive server ipv6 - test all ipv4/6 server simple/sustained modes

	Performance: 
	     (See libzt.h, compile libzt with appropriate ZT_TCP_TX_BUF_SZ, ZT_TCP_RX_BUF_SZ, ZT_UDP_TX_BUF_SZ, and ZT_UDO_RX_BUF_SZ for your test)

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





/****************************************************************************/
/* Helper Functions                                                         */
/****************************************************************************/

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
	std::string key, value, prefix;
	std::ifstream testFile;
	testFile.open(filepath.c_str());
	while (testFile >> key >> value) {
		if(key == "name") {
			prefix = value;
		}
		if(key[0] != '#' && key[0] != ';') {
	    	testConf[prefix + "." + key] = value;
	        fprintf(stderr, "%s.%s = %s\n", prefix.c_str(), key.c_str(), testConf[prefix + "." + key].c_str());
	    }

	}
	testFile.close();
}

long int get_now_ts()
{
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

void generate_random_data(void *buf, size_t n)
{
	char *b = (char*)buf;
	int min = 0, max = 9;
	srand((unsigned)time(0));
	for(int i=0; i<n; i++) {
		b[i] = min + (rand() % static_cast<int>(max - min + 1));
	}
}

void create_addr(std::string ipstr, int port, int ipv, struct sockaddr *saddr)
{
	struct hostent *server;
	if(ipv == 4) {
		struct sockaddr_in *in4 = (struct sockaddr_in*)saddr;
		in4->sin_port = htons(port);
		in4->sin_addr.s_addr = inet_addr(ipstr.c_str());
		in4->sin_family = AF_INET;
	}
	if(ipv == 6) {
		struct sockaddr_in6 *in6 = (struct sockaddr_in6*)saddr;
		server = gethostbyname2(ipstr.c_str(),AF_INET6);
		memset((char *) in6, 0, sizeof(struct sockaddr_in6));
		in6->sin6_flowinfo = 0;
		in6->sin6_family = AF_INET6;
		memmove((char *) in6->sin6_addr.s6_addr, (char *) server->h_addr, server->h_length);
		in6->sin6_port = htons(port);
	}
}


void RECORD_RESULTS(int *test_number, bool passed, char *details, std::vector<std::string> *results)
{
	(*test_number) = 0;
	char *ok_str   = (char*)"[  OK  ]";
	char *fail_str = (char*)"[ FAIL ]";

	if(passed == PASSED) {
		DEBUG_TEST("[%d]%s", *test_number, ok_str);
		results->push_back(std::string(ok_str) + " " + std::string(details));
	}
	else {
		DEBUG_ERROR("[%d]%s", *test_number, fail_str);		
		results->push_back(std::string(fail_str) + " " + std::string(details));
	}
	if(EXIT_ON_FAIL && !passed) {
		fprintf(stderr, "%s\n", results->at(results->size()-1).c_str());
		exit(0);
	}
	memset(details, 0, DETAILS_STR_LEN);
}





/****************************************************************************/
/* SIMPLE                                                                   */
/****************************************************************************/

// 
void tcp_client_4(UNIT_TEST_SIG_4)
{
	fprintf(stderr, "\n\n\ntcp_client_4\n");
	int r, w, sockfd, err, len = strlen(str);
	char rbuf[STR_SIZE];
	memset(rbuf, 0, sizeof rbuf);
	if((sockfd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0)
		DEBUG_ERROR("error creating ZeroTier socket");
	if((err = zts_connect(sockfd, (const struct sockaddr *)addr, sizeof(addr))) < 0)
		DEBUG_ERROR("error connecting to remote host (%d)", err);
	w = zts_write(sockfd, str, len);
	r = zts_read(sockfd, rbuf, len);
	DEBUG_TEST("Sent     : %s", str);
	DEBUG_TEST("Received : %s", rbuf);
	sleep(WAIT_FOR_TRANSMISSION_TO_COMPLETE);
	err = zts_close(sockfd);
	sprintf(details, "tcp_client_4, n=%d, err=%d, r=%d, w=%d", count, err, r, w);
	*passed = (w == len && r == len && !err) && !strcmp(rbuf, str);
}

//
void tcp_server_4(UNIT_TEST_SIG_4)
{
	fprintf(stderr, "\n\n\ntcp_server_4\n");
	int w=0, r=0, sockfd, accfd, err, len = strlen(str);
	char rbuf[STR_SIZE];
	memset(rbuf, 0, sizeof rbuf);
	if((sockfd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0)
		DEBUG_ERROR("error creating ZeroTier socket");
	if((err = zts_bind(sockfd, (struct sockaddr *)addr, sizeof(struct sockaddr_in)) < 0))
		DEBUG_ERROR("error binding to interface (%d)", err);
	if((err = zts_listen(sockfd, 100)) < 0)
		printf("error placing socket in LISTENING state (%d)", err);
	if((accfd = zts_accept(sockfd, (struct sockaddr *)&addr, (socklen_t *)sizeof(addr))) < 0)
		DEBUG_ERROR("error accepting connection (%d)", err);
	r = zts_read(accfd, rbuf, sizeof rbuf);
	w = zts_write(accfd, rbuf, len);
	DEBUG_TEST("Received : %s", rbuf);
	sleep(WAIT_FOR_TRANSMISSION_TO_COMPLETE);
	err = zts_close(sockfd);
	err = zts_close(accfd);
	sprintf(details, "tcp_server_4, n=%d, err=%d, r=%d, w=%d", count, err, r, w);
	*passed = (w == len && r == len && !err) && !strcmp(rbuf, str);
}

// 
void tcp_client_6(UNIT_TEST_SIG_6)
{
	fprintf(stderr, "\n\n\ntcp_client_6\n");
	int r, w, sockfd, err, len = strlen(str);
	char rbuf[STR_SIZE];
	memset(rbuf, 0, sizeof rbuf);
	if((sockfd = zts_socket(AF_INET6, SOCK_STREAM, 0)) < 0)
		DEBUG_ERROR("error creating ZeroTier socket");
	if((err = zts_connect(sockfd, (const struct sockaddr *)addr, sizeof(addr))) < 0)
		DEBUG_ERROR("error connecting to remote host (%d)", err);
	w = zts_write(sockfd, str, len);
	r = zts_read(sockfd, rbuf, len);
	sleep(WAIT_FOR_TRANSMISSION_TO_COMPLETE);
	err = zts_close(sockfd);
	sprintf(details, "tcp_client_6, n=%d, err=%d, r=%d, w=%d", count, err, r, w);
	DEBUG_TEST("Sent     : %s", str);
	DEBUG_TEST("Received : %s", rbuf);
	*passed = (w == len && r == len && !err) && !strcmp(rbuf, str);
}

//
void tcp_server_6(UNIT_TEST_SIG_6)
{
	fprintf(stderr, "\n\n\ntcp_server_6\n");
	int w=0, r=0, sockfd, accfd, err, len = strlen(str);
	char rbuf[STR_SIZE];
	memset(rbuf, 0, sizeof rbuf);
	if((sockfd = zts_socket(AF_INET6, SOCK_STREAM, 0)) < 0)
		DEBUG_ERROR("error creating ZeroTier socket");
	if((err = zts_bind(sockfd, (struct sockaddr *)addr, sizeof(struct sockaddr_in)) < 0))
		DEBUG_ERROR("error binding to interface (%d)", err);
	if((err = zts_listen(sockfd, 100)) < 0)
		DEBUG_ERROR("error placing socket in LISTENING state (%d)", err);
	if((accfd = zts_accept(sockfd, (struct sockaddr *)&addr, (socklen_t *)sizeof(addr))) < 0)
		DEBUG_ERROR("error accepting connection (%d)", err);
	r = zts_read(accfd, rbuf, sizeof rbuf);
	w = zts_write(accfd, rbuf, len);
	DEBUG_TEST("Received : %s", rbuf);
	sleep(WAIT_FOR_TRANSMISSION_TO_COMPLETE);
	err = zts_close(sockfd);
	err = zts_close(accfd);
	sprintf(details, "tcp_server_6, n=%d, err=%d, r=%d, w=%d", count, err, r, w);
	*passed = (w == len && r == len && !err) && !strcmp(rbuf, str);
}





/****************************************************************************/
/* SUSTAINED                                                                */
/****************************************************************************/

// Maintain transfer for count OR count
void tcp_client_sustained_4(UNIT_TEST_SIG_4)
{
	fprintf(stderr, "\n\n\ntcp_client_sustained_4\n");
	int n=0, w=0, r=0, sockfd, err;	
	char *rxbuf = (char*)malloc(count*sizeof(char));
	char *txbuf = (char*)malloc(count*sizeof(char));
	generate_random_data(txbuf, count);

	if((sockfd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0)
		DEBUG_ERROR("error creating ZeroTier socket");
	if((err = zts_connect(sockfd, (const struct sockaddr *)addr, sizeof(addr))) < 0)
		DEBUG_ERROR("error connecting to remote host (%d)", err);

	if(operation == TEST_OP_N_BYTES) {
		int wrem = count, rrem = count;

		// TX
		long int tx_ti = get_now_ts();	
		while(wrem) {
			int next_write = std::min(4096, wrem);
			n = zts_write(sockfd, &txbuf[w], next_write);
			if (n > 0)
			{
				w += n;
				wrem -= n;
				err = n;
			}
		}
		long int tx_tf = get_now_ts();	
		DEBUG_TEST("wrote=%d", w);
		// RX
		long int rx_ti = 0;	
		while(rrem) {
			n = zts_read(sockfd, &rxbuf[r], rrem);
			if(!rx_ti) { // wait for first message
				rx_ti = get_now_ts();	
			}
			if (n > 0)
			{
				r += n;
				rrem -= n;
				err = n;
			}
		}
		long int rx_tf = get_now_ts();	
		DEBUG_TEST("read=%d", r);
		sleep(WAIT_FOR_TRANSMISSION_TO_COMPLETE);
		err = zts_close(sockfd);

		// Compare RX and TX buffer and detect mismatches
		bool match = true;
		for(int i=0; i<count; i++) {
			if(rxbuf[i] != txbuf[i]) {
				DEBUG_ERROR("buffer mismatch found at idx=%d", i);
				match=false;
			}
		}

		// Compute time deltas and transfer rates
		float tx_dt = (tx_tf - tx_ti) / (float)1000;
		float rx_dt = (rx_tf - rx_ti) / (float)1000;
		float tx_rate = (float)count / (float)tx_dt;
		float rx_rate = (float)count / (float)rx_dt;

		sprintf(details, "tcp_client_sustained_4, match=%d, n=%d, tx_dt=%.2f, rx_dt=%.2f, r=%d, w=%d, tx_rate=%.2f MB/s, rx_rate=%.2f MB/s", 
			match, count, tx_dt, rx_dt, r, w, (tx_rate / float(ONE_MEGABYTE) ), (rx_rate / float(ONE_MEGABYTE) ));	

		*passed = (r == count && w == count && match && err>=0);
	}
	free(rxbuf);
	free(txbuf);
}



// Maintain transfer for count OR count
void tcp_client_sustained_6(UNIT_TEST_SIG_6)
{
	fprintf(stderr, "\n\n\ntcp_client_sustained_6\n");
	int n=0, w=0, r=0, sockfd, err;	
	char *rxbuf = (char*)malloc(count*sizeof(char));
	char *txbuf = (char*)malloc(count*sizeof(char));
	generate_random_data(txbuf, count);

	if((sockfd = zts_socket(AF_INET6, SOCK_STREAM, 0)) < 0)
		DEBUG_ERROR("error creating ZeroTier socket");
	if((err = zts_connect(sockfd, (const struct sockaddr *)addr, sizeof(addr))) < 0)
		DEBUG_ERROR("error connecting to remote host (%d)", err);

	if(operation == TEST_OP_N_BYTES) {
		int wrem = count, rrem = count;

		// TX
		long int tx_ti = get_now_ts();	
		while(wrem) {
			int next_write = std::min(4096, wrem);
			n = zts_write(sockfd, &txbuf[w], next_write);
			if (n > 0)
			{
				w += n;
				wrem -= n;
				err = n;
			}
		}
		long int tx_tf = get_now_ts();	
		DEBUG_TEST("wrote=%d", w);
		// RX
		long int rx_ti = 0;	
		while(rrem) {
			n = zts_read(sockfd, &rxbuf[r], rrem);
			if(!rx_ti) { // wait for first message
				rx_ti = get_now_ts();	
			}
			if (n > 0)
			{
				r += n;
				rrem -= n;
				err = n;
			}
		}
		long int rx_tf = get_now_ts();	
		DEBUG_TEST("read=%d", r);

		sleep(WAIT_FOR_TRANSMISSION_TO_COMPLETE);
		err = zts_close(sockfd);

		// Compare RX and TX buffer and detect mismatches
		bool match = true;
		for(int i=0; i<count; i++) {
			if(rxbuf[i] != txbuf[i]) {
				DEBUG_ERROR("buffer mismatch found at idx=%d", i);
				match=false;
			}
		}

		// Compute time deltas and transfer rates
		float tx_dt = (tx_tf - tx_ti) / (float)1000;
		float rx_dt = (rx_tf - rx_ti) / (float)1000;
		float tx_rate = (float)count / (float)tx_dt;
		float rx_rate = (float)count / (float)rx_dt;

		sprintf(details, "tcp_client_sustained_6, match=%d, n=%d, tx_dt=%.2f, rx_dt=%.2f, r=%d, w=%d, tx_rate=%.2f MB/s, rx_rate=%.2f MB/s", 
			match, count, tx_dt, rx_dt, r, w, (tx_rate / float(ONE_MEGABYTE) ), (rx_rate / float(ONE_MEGABYTE) ));	

		*passed = (r == count && w == count && match && err>=0);
	}
	free(rxbuf);
	free(txbuf);
}


// Maintain transfer for count OR count
void tcp_server_sustained_4(UNIT_TEST_SIG_4)
{
	fprintf(stderr, "\n\n\ntcp_server_sustained_4\n");
	int n=0, w=0, r=0, sockfd, accfd, err;
	char *rxbuf = (char*)malloc(count*sizeof(char));
	memset(rxbuf, 0, count);

	if((sockfd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0)
		DEBUG_ERROR("error creating ZeroTier socket");
	if((err = zts_bind(sockfd, (struct sockaddr *)addr, (socklen_t)sizeof(struct sockaddr_in)) < 0))
		DEBUG_ERROR("error binding to interface (%d)", err);
	if((err = zts_listen(sockfd, 1)) < 0)
		DEBUG_ERROR("error placing socket in LISTENING state (%d)", err);
	if((accfd = zts_accept(sockfd, (struct sockaddr *)&addr, (socklen_t *)sizeof(addr))) < 0)
		DEBUG_ERROR("error accepting connection (%d)", err);
	if(operation == TEST_OP_N_BYTES) {
		int wrem = count, rrem = count;
		long int rx_ti = 0;
		while(rrem) {
			n = zts_read(accfd, &rxbuf[r], rrem);
			if (n > 0)
			{
				if(!rx_ti) { // wait for first message
					rx_ti = get_now_ts();	
				}
				r += n;
				rrem -= n;
				err = n;
			}
		}
		long int rx_tf = get_now_ts();	
		DEBUG_TEST("read=%d", r);
		
		long int tx_ti = get_now_ts();	
		while(wrem) {
			int next_write = std::min(1024, wrem);
			n = zts_write(accfd, &rxbuf[w], next_write);
			if (n > 0)
			{	
				w += n;
				wrem -= n;
				err = n;
			}
		}
		long int tx_tf = get_now_ts();	
		DEBUG_TEST("wrote=%d", w);

		sleep(WAIT_FOR_TRANSMISSION_TO_COMPLETE);
		err = zts_close(sockfd);

		// Compute time deltas and transfer rates
		float tx_dt = (tx_tf - tx_ti) / (float)1000;
		float rx_dt = (rx_tf - rx_ti) / (float)1000;
		float tx_rate = (float)count / (float)tx_dt;
		float rx_rate = (float)count / (float)rx_dt;

		sprintf(details, "tcp_server_sustained_4, n=%d, tx_dt=%.2f, rx_dt=%.2f, r=%d, w=%d, tx_rate=%.2f MB/s, rx_rate=%.2f MB/s", 
			count, tx_dt, rx_dt, r, w, (tx_rate / float(ONE_MEGABYTE) ), (rx_rate / float(ONE_MEGABYTE) ));

		*passed = (r == count && w == count && err>=0);
	}
	free(rxbuf);
}


// Maintain transfer for count OR count
void tcp_server_sustained_6(UNIT_TEST_SIG_6)
{
	fprintf(stderr, "\n\n\ntcp_server_sustained_6\n");
	int n=0, w=0, r=0, sockfd, accfd, err;
	char *rxbuf = (char*)malloc(count*sizeof(char));
	memset(rxbuf, 0, count);

	if((sockfd = zts_socket(AF_INET6, SOCK_STREAM, 0)) < 0)
		DEBUG_ERROR("error creating ZeroTier socket");
	if((err = zts_bind(sockfd, (struct sockaddr *)addr, (socklen_t)sizeof(struct sockaddr_in)) < 0))
		DEBUG_ERROR("error binding to interface (%d)", err);
	if((err = zts_listen(sockfd, 1)) < 0)
		DEBUG_ERROR("error placing socket in LISTENING state (%d)", err);
	if((accfd = zts_accept(sockfd, (struct sockaddr *)&addr, (socklen_t *)sizeof(addr))) < 0)
		DEBUG_ERROR("error accepting connection (%d)", err);
	if(operation == TEST_OP_N_BYTES) {
		int wrem = count, rrem = count;
		long int rx_ti = 0;
		while(rrem) {
			n = zts_read(accfd, &rxbuf[r], rrem);
			if (n > 0)
			{
				if(!rx_ti) { // wait for first message
					rx_ti = get_now_ts();	
				}
				r += n;
				rrem -= n;
				err = n;
			}
		}
		long int rx_tf = get_now_ts();	
		DEBUG_TEST("read=%d", r);
		long int tx_ti = get_now_ts();	
		while(wrem) {
			int next_write = std::min(1024, wrem);
			n = zts_write(accfd, &rxbuf[w], next_write);
			if (n > 0)
			{	
				w += n;
				wrem -= n;
				err = n;
			}
		}
		long int tx_tf = get_now_ts();	
		DEBUG_TEST("wrote=%d", w);
		sleep(WAIT_FOR_TRANSMISSION_TO_COMPLETE);
		err = zts_close(sockfd);

		// Compute time deltas and transfer rates
		float tx_dt = (tx_tf - tx_ti) / (float)1000;
		float rx_dt = (rx_tf - rx_ti) / (float)1000;
		float tx_rate = (float)count / (float)tx_dt;
		float rx_rate = (float)count / (float)rx_dt;

		sprintf(details, "tcp_server_sustained_6, n=%d, tx_dt=%.2f, rx_dt=%.2f, r=%d, w=%d, tx_rate=%.2f MB/s, rx_rate=%.2f MB/s", 
			count, tx_dt, rx_dt, r, w, (tx_rate / float(ONE_MEGABYTE) ), (rx_rate / float(ONE_MEGABYTE) ));

		*passed = (r == count && w == count && err>=0);
	}
	free(rxbuf);
}


/****************************************************************************/
/* PERFORMANCE (between library instances)                                  */
/****************************************************************************/

// Maintain transfer for count OR count
void tcp_client_perf_4(UNIT_TEST_SIG_4)
{
	fprintf(stderr, "\n\n\ntcp_client_perf_4\n");
	/*
	int w=0, sockfd, err;
	int total_test_sz          = count;
	int arbitrary_chunk_sz_max = MAX_RX_BUF_SZ;
	int arbitrary_chunk_sz_min = 512;

	char rbuf[arbitrary_chunk_sz_max];

	for (int i=arbitrary_chunk_sz_min; (i*2) < arbitrary_chunk_sz_max; i*=2) {

		if((sockfd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0)
			DEBUG_ERROR("error creating ZeroTier socket");
		if((err = zts_connect(sockfd, (const struct sockaddr *)addr, sizeof(addr))) < 0)
			DEBUG_ERROR("error connecting to remote host (%d)", err);

		DEBUG_TEST("[TX] Testing (%d) byte chunks: ", i);

		int chunk_sz   = i;
		long int start_time = get_now_ts();
		w = 0;

		// TX
		while(w < total_test_sz)
			w += zts_write(sockfd, rbuf, chunk_sz);
		
		long int end_time = get_now_ts();
		float ts_delta = (end_time - start_time) / (float)1000;
		float rate = (float)total_test_sz / (float)ts_delta;
		sprintf(details, "tot=%d, dt=%.2f, rate=%.2f MB/s", w, ts_delta, (rate / float(ONE_MEGABYTE) ));
		zts_close(sockfd);		
	}	
	*passed = (w == total_test_sz && !err) ? PASSED : FAILED;
	*/
}

// Maintain transfer for count OR count
void tcp_server_perf_4(UNIT_TEST_SIG_4)
{
	fprintf(stderr, "\n\n\ntcp_server_perf_4\n");
	/*
	int r=0, sockfd, accfd, err;
	int total_test_sz          = count;
	int arbitrary_chunk_sz_max = MAX_RX_BUF_SZ;
	int arbitrary_chunk_sz_min = 512;

	char rbuf[arbitrary_chunk_sz_max];

	for (int i=arbitrary_chunk_sz_min; (i*2) < arbitrary_chunk_sz_max; i*=2) {
		DEBUG_ERROR("TESTING chunk size = %d", i);
		if((sockfd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0)
			DEBUG_ERROR("error creating ZeroTier socket");
		if((err = zts_bind(sockfd, (struct sockaddr *)addr, (socklen_t)sizeof(struct sockaddr_in)) < 0))
			DEBUG_ERROR("error binding to interface (%d)", err);
		if((err = zts_listen(sockfd, 1)) < 0)
			DEBUG_ERROR("error placing socket in LISTENING state (%d)", err);
		if((accfd = zts_accept(sockfd, (struct sockaddr *)&addr, (socklen_t *)sizeof(addr))) < 0)
			DEBUG_ERROR("error accepting connection (%d)", err);

		DEBUG_TEST("[RX] Testing (%d) byte chunks: ", i);

		int chunk_sz   = i;
		long int start_time = get_now_ts();
		r = 0;

		// RX
		while(r < total_test_sz)
			r += zts_read(accfd, rbuf, chunk_sz);
		
		long int end_time = get_now_ts();

		float ts_delta = (end_time - start_time) / (float)1000;
		float rate = (float)total_test_sz / (float)ts_delta;

		sprintf(details, "tot=%d, dt=%.2f, rate=%.2f MB/s", r, ts_delta, (rate / float(ONE_MEGABYTE) ));		

		zts_close(sockfd);
		zts_close(accfd);
	}
	*passed = (r == total_test_sz && !err) ? PASSED : FAILED;
	*/
}





/****************************************************************************/
/* PERFORMANCE (between library and native)                                 */
/****************************************************************************/

void tcp_perf_tx_echo_4(UNIT_TEST_SIG_4)
{
	fprintf(stderr, "\n\n\ntcp_perf_tx_echo_4\n");

	int err   = 0;
	int tot   = 0;
	int w     = 0;
	int sockfd, mode;

	char pbuf[64]; // test parameter buffer
	char tbuf[MAX_TX_BUF_SZ];

	mode = ECHOTEST_MODE_TX;

	// connect to remote echotest host
	if((sockfd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		DEBUG_ERROR("error creating ZeroTier socket");
		return;
	}
	if((err = zts_connect(sockfd, (const struct sockaddr *)addr, sizeof(addr))) < 0) {
		DEBUG_ERROR("error connecting to remote host (%d)", err);
		return;
	}

	DEBUG_TEST("copying test parameters to buffer");
	memset(pbuf, 0, sizeof pbuf);
	memcpy(pbuf, &mode, sizeof mode);
	memcpy(pbuf + sizeof mode, &count, sizeof count);

	DEBUG_TEST("sending test parameters to echotest");
	if((w = zts_write(sockfd, pbuf, sizeof pbuf)) < 0) {
		DEBUG_ERROR("error while sending test parameters to echotest (err=%d)", w);
		return;
	}

	// begin
	DEBUG_TEST("beginning test, sending test byte stream...");
	while(tot < count) {
		if((w = zts_write(sockfd, tbuf, sizeof tbuf)) < 0) {
			DEBUG_ERROR("error while sending test byte stream to echotest (err=%d)", w);
			return;
		}
		tot += w;
		DEBUG_TEST("tot=%d, sent=%d", tot, w);
	}
	// read results
	memset(pbuf, 0, sizeof pbuf);
	DEBUG_TEST("reading test results from echotest");
	if((w = zts_read(sockfd, pbuf, sizeof tbuf)) < 0) {
		DEBUG_ERROR("error while reading results from echotest (err=%d)", w);
		return;
	}

	DEBUG_TEST("reading test results");
	long int start_time = 0, end_time = 0;
	memcpy(&start_time, pbuf, sizeof start_time);
	memcpy(&end_time, pbuf + sizeof start_time, sizeof end_time);

	float ts_delta = (end_time - start_time) / (float)1000;
	float rate = (float)tot / (float)ts_delta;
	sprintf(details, "tcp_perf_tx_echo_4, tot=%d, dt=%.2f, rate=%.2f MB/s", tot, ts_delta, (rate / float(ONE_MEGABYTE) ));

	sleep(WAIT_FOR_TRANSMISSION_TO_COMPLETE);
	err = zts_close(sockfd);
	*passed = (tot == count && !err) ? PASSED : FAILED;
}


void tcp_perf_rx_echo_4(UNIT_TEST_SIG_4)
{
	fprintf(stderr, "\n\n\ntcp_perf_rx_echo_4\n");

	int err   = 0;
	int mode  = 0;
	int tot   = 0;
	int r     = 0;
	
	char pbuf[64]; // test parameter buffer
	char tbuf[MAX_TX_BUF_SZ];
	int sockfd;

	mode = ECHOTEST_MODE_RX;

	// connect to remote echotest host
	if((sockfd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		DEBUG_ERROR("error creating ZeroTier socket");
		return;
	}
	if((err = zts_connect(sockfd, (const struct sockaddr *)addr, sizeof(addr))) < 0) {
		DEBUG_ERROR("error connecting to remote host (%d)", err);
		return;
	}

	DEBUG_TEST("copying test parameters to buffer");
	memset(pbuf, 0, sizeof pbuf);
	memcpy(pbuf, &mode, sizeof mode);
	memcpy(pbuf + sizeof mode, &count, sizeof count);

	DEBUG_TEST("sending test parameters to echotest");
	if((r = zts_write(sockfd, pbuf, sizeof pbuf)) < 0) {
		DEBUG_ERROR("error while sending test parameters to echotest (err=%d)", r);
		return;
	}

	// begin
	DEBUG_TEST("beginning test, as soon as bytes are read we will start keeping time...");
	if((r = read(sockfd, tbuf, sizeof tbuf)) < 0) {
		DEBUG_ERROR("there was an error reading the test stream. aborting (err=%d, errno=%s)", r, strerror(errno));
		return;
	}

	tot += r;

	long int start_time = get_now_ts();	
	DEBUG_TEST("Received first set of bytes in test stream. now keeping time");

	while(tot < count) {
		if((r = read(sockfd, tbuf, sizeof tbuf)) < 0) {
			DEBUG_ERROR("there was an error reading the test stream. aborting (err=%d)", r);
			return;
		}
		tot += r;
		DEBUG_TEST("r=%d, tot=%d", r, tot);
	}
	long int end_time = get_now_ts();	
	float ts_delta = (end_time - start_time) / (float)1000;
	float rate = (float)tot / (float)ts_delta;
	sprintf(details, "tcp_perf_rx_echo_4, tot=%d, dt=%.2f, rate=%.2f MB/s", tot, ts_delta, (rate / float(ONE_MEGABYTE) ));		
	
	sleep(WAIT_FOR_TRANSMISSION_TO_COMPLETE);
	err = zts_close(sockfd);
	*passed = (tot == count && !err) ? PASSED : FAILED;
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
					std::cout << "error creating socket (errno = " << strerror(errno) << ")" << std::endl;
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
					std::cout << "error closing socket (errno = " << strerror(errno) << ")" << std::endl;
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
					std::cout << "error creating socket (errno = " << strerror(errno) << ")" << std::endl;
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
					std::cout << "error binding socket (errno = " << strerror(errno) << ")" << std::endl;
					return -1;
				}
				
				if(sock > 0) {
					if((err = zts_close(sock)) < 0) {
						std::cout << "error closing socket (errno = " << strerror(errno) << ")" << std::endl;
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
				std::cout << "error creating socket (errno = " << strerror(errno) << ")" << std::endl;
			results[j] = std::min(results[j], sock);
			
			// set O_NONBLOCK
			if((err = zts_fcntl(sock, F_SETFL, O_NONBLOCK) < 0))
				std::cout << "error setting O_NONBLOCK (errno=" << strerror(errno) << ")" << std::endl;
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
					std::cout << "error connecting socket (errno = " << strerror(errno) << ")" << std::endl;
				results[j] = std::min(results[j], err);
			}

			// close()
			if((err = zts_close(sock)) < 0)
				std::cout << "error closing socket (errno = " << strerror(errno) << ")" << std::endl;
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
/* OBSCURE API CALL TESTS                                                   */
/****************************************************************************/

int obscure_api_test()
{
	// Disable Nagle's Algorithm on a socket
	int sock = zts_socket(AF_INET, SOCK_STREAM, 0);
	int flag = 1;
	int err = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
	 if (err < 0) {
	 	DEBUG_ERROR("error while disabling Nagle's algorithm on socket");
	 }
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
		fprintf(stderr, "calls_made=%d\n", calls_made);
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
	count   = number of operations of type
	delay     = delay between each operation
*/
int test_driver(std::string name, std::string path, std::string nwid, 
	int type, 
	int ipv, 
	int mode, 
	std::string ipstr, 
	int port, 
	int operation, 
	int count, 
	int delay, 
	std::vector<std::string> *results)
{
    return 0;
}



/*
 For each API call, test the following:
  - All possible combinations of plausible system-defined arguments
  - Common values in innappropriate locations {-1, 0, 1}
  - Check for specific errno values for each function

*/
void test_bad_args()
{
// Protocol Family test set
	int proto_families[] = {
		AF_UNIX, 
		AF_LOCAL,
		AF_INET,
		AF_INET6,
		AF_IPX,
		PF_LOCAL,
		PF_UNIX,
		PF_INET,
		PF_ROUTE,
		PF_KEY,
		PF_INET6,
#if !defined(__linux__)
		PF_SYSTEM,
		PF_NDRV,
#endif
#if !defined(__APPLE__)
		AF_NETLINK,
		AF_X25,
		AF_AX25,
		AF_ATMPVC,
		AF_ALG,
		AF_PACKET,
#endif
		AF_APPLETALK
	};
	int num_proto_families = sizeof(proto_families) / sizeof(int);

// Socket Type test set
	int socket_types[] = {
		SOCK_STREAM,
		SOCK_DGRAM,
		SOCK_RAW
	};
	int num_socket_types = 3;


// Protocol test set

	// int min = -1;
	int max =  2;
	int err =  0;

	int min_protocol_family_value = 0;
	int max_protocol_family_value = 0;

	int min_socket_type_value = 0;
	int max_socket_type_value = 0;

	int min_protocol_value = 0;
	int max_protocol_value = 0;

	// socket()
	DEBUG_TEST("testing bad arguments for socket()");

	// Try all plausible argument combinations
	for(int i=0; i<num_proto_families; i++) {
		for(int j=0; j<num_socket_types; j++) {
			for(int k=0; k<max; k++) {

				int protocol_family = proto_families[i];
				int socket_type = socket_types[j];
				int protocol = -1;

				min_protocol_family_value = std::min(protocol_family, min_protocol_family_value); 
				max_protocol_family_value = std::max(protocol_family, max_protocol_family_value); 

				min_socket_type_value = std::min(socket_type, min_socket_type_value); 
				max_socket_type_value = std::max(socket_type, max_socket_type_value); 

				min_protocol_value = std::min(protocol, min_protocol_value); 
				max_protocol_value = std::max(protocol, max_protocol_value); 

				err = zts_socket(protocol_family, socket_type, protocol);
				usleep(100000);
				if(err < 0) {
					DEBUG_ERROR("zts_socket(%d, %d, %d) = %d, errno=%d (%s)", protocol_family, socket_type, protocol, err, errno, strerror(errno));
				}
				else {
					DEBUG_TEST("zts_socket(%d, %d, %d) = %d, errno=%d (%s)", protocol_family, socket_type, protocol, err, errno, strerror(errno));
				}
			}	
		}
	}

	DEBUG_TEST("min_protocol_family_value=%d",min_protocol_family_value);
	DEBUG_TEST("max_protocol_family_value=%d",max_protocol_family_value);

	DEBUG_TEST("min_socket_type_value=%d",min_socket_type_value);
	DEBUG_TEST("max_socket_type_value=%d",max_socket_type_value);

	DEBUG_TEST("min_protocol_value=%d",min_protocol_value);
	DEBUG_TEST("max_protocol_value=%d",max_protocol_value);


	DEBUG_TEST("AF_INET = %d", AF_INET);
	DEBUG_TEST("AF_INET6 = %d", AF_INET6);
	DEBUG_TEST("SOCK_STREAM = %d", SOCK_STREAM);
	DEBUG_TEST("SOCK_DGRAM = %d", SOCK_DGRAM);

}


/****************************************************************************/
/* main(), calls test_driver(...)                                           */
/****************************************************************************/

int main(int argc , char *argv[])
{
    if(argc < 5) {
        fprintf(stderr, "usage: selftest <selftest.conf> <alice|bob|ted|carol> to <bob|alice|ted|carol>\n");
        fprintf(stderr, "e.g. : selftest test/selftest.conf alice to bob\n");
        return 1;
    }

	std::string from = argv[2];
	std::string   to = argv[4];
	std::string   me = from;

    std::vector<std::string> results;

    int err          = 0;
    int mode         = 0;
    int port         = 0;
    int operation    = 0;
    int start_port   = 0;
    int port_offset  = 0;
	int count        = 0;
	int delay        = 0;

	std::string remote_echo_ipv4, smode;
	std::string nwid, stype, path = argv[1];
	std::string ipstr, ipstr6, local_ipstr, local_ipstr6, remote_ipstr, remote_ipstr6;
	memcpy(str, "welcome to the machine", 22);

	// loaf config file
	if(path.find(".conf") == std::string::npos) {
		fprintf(stderr, "Possibly invalid conf file. Exiting...\n");
		exit(0);
	}
	loadTestConfigFile(path);

	// get origin details
	local_ipstr = testConf[me + ".ipv4"];
	local_ipstr6 = testConf[me + ".ipv6"];
	nwid = testConf[me + ".nwid"];
	path = testConf[me + ".path"];
	stype = testConf[me + ".test"];
	smode = testConf[me + ".mode"];
	start_port = atoi(testConf[me + ".port"].c_str());
	port_offset = 100;

	// get destination details
	remote_echo_ipv4 = testConf[to + ".echo_ipv4"];
	remote_ipstr  = testConf[to + ".ipv4"];
	remote_ipstr6 = testConf[to + ".ipv6"];

	if(strcmp(smode.c_str(), "server") == 0)
		mode = TEST_MODE_SERVER;
	else
		mode = TEST_MODE_CLIENT;

	fprintf(stderr, "ORIGIN:\n\n");
	fprintf(stderr, "\tlocal_ipstr      = %s\n", local_ipstr.c_str());
	fprintf(stderr, "\tlocal_ipstr6     = %s\n", local_ipstr6.c_str());
	fprintf(stderr, "\tstart_port       = %d\n", start_port);
	fprintf(stderr, "\tpath             = %s\n", path.c_str());
	fprintf(stderr, "\tnwid             = %s\n", nwid.c_str());
	fprintf(stderr, "\ttype             = %s\n\n", stype.c_str());

	fprintf(stderr, "DESTINATION:\n\n");
	fprintf(stderr, "\tremote_ipstr     = %s\n", remote_ipstr.c_str());
	fprintf(stderr, "\tremote_ipstr6    = %s\n", remote_ipstr6.c_str());
	fprintf(stderr, "\tremote_echo_ipv4 = %s\n", remote_echo_ipv4.c_str());

	DEBUG_TEST("Waiting for libzt to come online...\n");
	zts_simple_start(path.c_str(), nwid.c_str());
	char device_id[11];
	zts_get_device_id(device_id);
	DEBUG_TEST("I am %s, %s", device_id, me.c_str());
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

	/****************************************************************************/
	/* COMPREHENSIVE                                                            */
	/****************************************************************************/

	// More info can be found in TESTING.md

	// test purpposefully bad arguments

	//test_bad_args();
	//exit(0);


	int test_number = 0;
	int ipv;
	struct sockaddr addr;
	char details[128];
	memset(&details, 0, sizeof details);
	bool passed = 0; 

	// Tests ALL API calls
	if(stype == "comprehensive")
	{	

		port      = start_port;
		delay     = 0;
		count     = 1024*128;
		operation = TEST_OP_N_BYTES;


// ipv4 client/server
		ipv = 4;
		if(mode == TEST_MODE_SERVER) {
			create_addr(local_ipstr, port, ipv, (struct sockaddr *)&addr);
			tcp_server_4((struct sockaddr_in *)&addr, operation, count, delay, details, &passed); // tcp_server_4
		}
		else if(mode == TEST_MODE_CLIENT) {
			sleep(WAIT_FOR_SERVER_TO_COME_ONLINE);
			create_addr(remote_ipstr, port, ipv, (struct sockaddr *)&addr);
			tcp_client_4((struct sockaddr_in *)&addr, operation, count, delay, details, &passed); // tcp_client_4
		}
		RECORD_RESULTS(&test_number, passed, details, &results);
		mode = mode == TEST_MODE_SERVER ? TEST_MODE_CLIENT : TEST_MODE_SERVER; // switch roles
		port++; // move up one port
		if(mode == TEST_MODE_SERVER) {
			create_addr(local_ipstr, port, ipv, (struct sockaddr *)&addr);
			tcp_server_4((struct sockaddr_in *)&addr, operation, count, delay, details, &passed); // tcp_server_4
		}
		else if(mode == TEST_MODE_CLIENT) {
			sleep(WAIT_FOR_SERVER_TO_COME_ONLINE);
			create_addr(remote_ipstr, port, ipv, (struct sockaddr *)&addr);
			tcp_client_4((struct sockaddr_in *)&addr, operation, count, delay, details, &passed); // tcp_client_4
		}
		RECORD_RESULTS(&test_number, passed, details, &results);
		port++;


// ipv4 sustained transfer	
		ipv = 4;	
		if(mode == TEST_MODE_SERVER) {
			create_addr(local_ipstr, port, ipv, (struct sockaddr *)&addr);
			tcp_server_sustained_4((struct sockaddr_in *)&addr, operation, count, delay, details, &passed); // tcp_server_sustained_4
		}
		else if(mode == TEST_MODE_CLIENT) {
			sleep(WAIT_FOR_SERVER_TO_COME_ONLINE);
			create_addr(remote_ipstr, port, ipv, (struct sockaddr *)&addr);
			tcp_client_sustained_4((struct sockaddr_in *)&addr, operation, count, delay, details, &passed); // tcp_client_sustained_4
		}
		RECORD_RESULTS(&test_number, passed, details, &results); // swtich roles
		mode = mode == TEST_MODE_SERVER ? TEST_MODE_CLIENT : TEST_MODE_SERVER; // switch roles
		port++;
		if(mode == TEST_MODE_SERVER) {
			create_addr(local_ipstr, port, ipv, (struct sockaddr *)&addr);
			tcp_server_sustained_4((struct sockaddr_in *)&addr, operation, count, delay, details, &passed); // tcp_server_sustained_4
		}
		else if(mode == TEST_MODE_CLIENT) {
			sleep(WAIT_FOR_SERVER_TO_COME_ONLINE);
			create_addr(remote_ipstr, port, ipv, (struct sockaddr *)&addr);
			tcp_client_sustained_4((struct sockaddr_in *)&addr, operation, count, delay, details, &passed); // tcp_client_sustained_4
		}
		RECORD_RESULTS(&test_number, passed, details, &results);
		port++;


// ipv6 client/server
		ipv = 6;
		if(mode == TEST_MODE_SERVER) {
			create_addr(local_ipstr6, port, ipv, (struct sockaddr *)&addr);
			tcp_server_6((struct sockaddr_in6 *)&addr, operation, count, delay, details, &passed); // tcp_server_6
		}
		else if(mode == TEST_MODE_CLIENT) {
			DEBUG_TEST("waiting (15s) for other selftest to complete before continuing...");
			sleep(WAIT_FOR_TEST_TO_CONCLUDE);
			create_addr(remote_ipstr6, port, ipv, (struct sockaddr *)&addr);
			tcp_client_6((struct sockaddr_in6 *)&addr, operation, count, delay, details, &passed); // tcp_client_6
		}
		RECORD_RESULTS(&test_number, passed, details, &results);
		mode = mode == TEST_MODE_SERVER ? TEST_MODE_CLIENT : TEST_MODE_SERVER; // switch roles
		port++; // move up one port
		if(mode == TEST_MODE_SERVER) {
			create_addr(local_ipstr6, port, ipv, (struct sockaddr *)&addr);
			tcp_server_6((struct sockaddr_in6 *)&addr, operation, count, delay, details, &passed); // tcp_server_6
		}
		else if(mode == TEST_MODE_CLIENT) {
			sleep(WAIT_FOR_SERVER_TO_COME_ONLINE);
			create_addr(remote_ipstr6, port, ipv, (struct sockaddr *)&addr);
			tcp_client_6((struct sockaddr_in6 *)&addr, operation, count, delay, details, &passed); // tcp_client_6
		}
		RECORD_RESULTS(&test_number, passed, details, &results);
		port++;


// ipv6 sustained transfer
		ipv = 6;		
		if(mode == TEST_MODE_SERVER) {
			create_addr(local_ipstr6, port, ipv, (struct sockaddr *)&addr);
			tcp_server_sustained_6((struct sockaddr_in6 *)&addr, operation, count, delay, details, &passed); // tcp_server_sustained_4
		}
		else if(mode == TEST_MODE_CLIENT) {
			sleep(WAIT_FOR_SERVER_TO_COME_ONLINE);
			create_addr(remote_ipstr6, port, ipv, (struct sockaddr *)&addr);
			tcp_client_sustained_6((struct sockaddr_in6 *)&addr, operation, count, delay, details, &passed); // tcp_client_sustained_4
		}
		RECORD_RESULTS(&test_number, passed, details, &results); // swtich roles
		mode = mode == TEST_MODE_SERVER ? TEST_MODE_CLIENT : TEST_MODE_SERVER; // switch roles
		port++;
		if(mode == TEST_MODE_SERVER) {
			create_addr(local_ipstr6, port, ipv, (struct sockaddr *)&addr);
			tcp_server_sustained_6((struct sockaddr_in6 *)&addr, operation, count, delay, details, &passed); // tcp_server_sustained_4
		}
		else if(mode == TEST_MODE_CLIENT) {
			sleep(WAIT_FOR_SERVER_TO_COME_ONLINE);
			create_addr(remote_ipstr6, port, ipv, (struct sockaddr *)&addr);
			tcp_client_sustained_6((struct sockaddr_in6 *)&addr, operation, count, delay, details, &passed); // tcp_client_sustained_4
		}
		RECORD_RESULTS(&test_number, passed, details, &results);
		port++;

// PERFORMANCE (between this library instance and a native non library instance (echo) )
// Client/Server mode isn't being tested here, so it isn't important, we'll just set it to client

// ipv4 echo test
		ipv = 4;	
		if(me == "alice" || me == "ted") {
			port=start_port+100; // e.g. 7100
			create_addr(remote_echo_ipv4, port, ipv, (struct sockaddr *)&addr);
			tcp_perf_tx_echo_4((struct sockaddr_in *)&addr, operation, count, delay, details, &passed); // tcp_perf_tx_echo_4
			RECORD_RESULTS(&test_number, passed, details, &results);
			sleep(WAIT_FOR_SERVER_TO_COME_ONLINE);
			tcp_perf_rx_echo_4((struct sockaddr_in *)&addr, operation, count, delay, details, &passed); // tcp_perf_rx_echo_4
			RECORD_RESULTS(&test_number, passed, details, &results);
		}
		if(me == "bob" || me == "carol") {
			DEBUG_TEST("waiting (15s) for other selftest to complete before continuing...");
			sleep(WAIT_FOR_TEST_TO_CONCLUDE);			
			port=start_port+101; // e.g. 7101
			create_addr(remote_echo_ipv4, port, ipv, (struct sockaddr *)&addr);
			tcp_perf_rx_echo_4((struct sockaddr_in *)&addr, operation, count, delay, details, &passed); // tcp_perf_tx_echo_4
			RECORD_RESULTS(&test_number, passed, details, &results);
			sleep(WAIT_FOR_SERVER_TO_COME_ONLINE);
			tcp_perf_tx_echo_4((struct sockaddr_in *)&addr, operation, count, delay, details, &passed); // tcp_perf_rx_echo_4
			RECORD_RESULTS(&test_number, passed, details, &results);
		}
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

// zts_fcntl(accfd, F_SETFL, O_NONBLOCK);
