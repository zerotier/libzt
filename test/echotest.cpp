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

 // Echo program to aid in the operation of selftest

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

#define ECHOTEST_MODE_RX       333
#define ECHOTEST_MODE_TX       666

#define MAX_RX_BUF_SZ          16384
#define MAX_TX_BUF_SZ          16384

std::map<std::string, std::string> testConf;

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

void start_echo_mode(std::string ipstr, int listen_port)
{
	DEBUG_TEST();
	DEBUG_TEST("listening for connections on port (%d)", listen_port);

	int backlog = 128;
	int err = 0;
	int sockfd, accfd;


	struct sockaddr_in addr;
	struct sockaddr_in client;
	socklen_t clen = sizeof client;
	addr.sin_port = htons(listen_port);
	addr.sin_addr.s_addr = inet_addr(ipstr.c_str());
	addr.sin_family = AF_INET;

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		DEBUG_ERROR("error creating socket (err=%d, errno=%s)", err, strerror(errno));
	if((err = bind(sockfd, (struct sockaddr *)&addr, (socklen_t)sizeof(struct sockaddr_in)) < 0))
		DEBUG_ERROR("error binding to interface (err=%d, errno=%s)\n", err, strerror(errno));
	if((err = listen(sockfd, backlog)) < 0)
		DEBUG_ERROR("error placing socket in LISTENING state (err=%d, errno=%s)\n", err, strerror(errno));

	DEBUG_TEST("accepting test connections...");
	while(true)
	{
		if((accfd = accept(sockfd, (struct sockaddr *)&client, &clen)) < 0) {
			DEBUG_ERROR("error accepting connection (err=%d, errno=%s)", accfd, strerror(errno));
			return;
		}
		DEBUG_TEST("connection accepted! (fd=%d)", accfd);

		// Read initial test parameters from other host
		int err = 0;
		int mode = 0; // rx/tx
		int count = 0; // of incoming byte stream, or requested outgoing
		char pbuf[64]; // test parameter buffer
		char rbuf[MAX_RX_BUF_SZ];
		int len = sizeof mode + sizeof count;
		int tot = 0; // total bytes read from remote test stream (selftest)

		memset(pbuf, 0, sizeof pbuf);

		DEBUG_TEST("reading %d bytes (test parameters)", len);
		if((err = read(accfd, pbuf, len)) < 0) {
			DEBUG_ERROR("error while reading test parameters from remote selftest host (err=%d, errno=%s)", err, strerror(errno));
			return;
		}

		memcpy(&mode, pbuf, sizeof mode);
		memcpy(&count, pbuf + sizeof mode, sizeof count);

		DEBUG_TEST("mode = %d, count = %d", mode, count);

		float totKB=0, totMB=0;
		/*
			Mode 1 (Measure performance of other host's TX): 
			 - Receive incoming TX test config (total bytes intended)
			 - Prepare receiver
			 - Record time of first received byte
			 - Record time of last received byte
			 - Send results back to other host's selftest instance
		*/

		// read 'count' bytes and send back before/after timestamps
		if(mode == ECHOTEST_MODE_TX)
		{
			DEBUG_TEST("entering READ mode, as soon as bytes are read we will start keeping time...");
			if((err = read(accfd, rbuf, sizeof rbuf)) < 0) {
				DEBUG_ERROR("there was an error reading the test stream. aborting (err=%d, errno=%s)", err, errno);
				return;
			}

			tot += err;

			long int start_time = get_now_ts();	
			totKB=0;
			totMB=0;
			DEBUG_TEST("Received first set of bytes in test stream. now keeping time");

			while(tot < count) {
				if((err = read(accfd, rbuf, sizeof rbuf)) < 0) {
					DEBUG_ERROR("there was an error reading the test stream. aborting");
					return;
				}
				tot += err;
				totKB = (float)tot / (float)1024;
				totMB = (float)tot / (float)(1024*1024);
				//DEBUG_TEST("read = %d, totB = %d, totKB = %3f, totMB = %3f", err, tot, totKB, totMB);

			}
			//DEBUG_TEST("total received = %d (%d MB)", tot);
			long int end_time = get_now_ts();	
			DEBUG_TEST("read last byte (tot=%d). stopping timer. sending test data back to remote selftest", tot);

			memset(pbuf, 0, sizeof pbuf);
			memcpy(pbuf, &start_time, sizeof start_time);
			memcpy(pbuf + sizeof start_time, &end_time, sizeof end_time);
			DEBUG_TEST("copied test data, sending...");

			if((err = write(accfd, pbuf, sizeof start_time + sizeof end_time)) < 0) {
				DEBUG_ERROR("error while sending test data to remote selftest host (err=%d, errno=%s)", err, strerror(errno));
				return;
			}
			DEBUG_TEST("sleeping before closing socket and accepting further selftest connections\n\n");
			sleep(3);
		}

		/* 
			Mode 2 (Measure performance of other host's RX): 
			 - Receive incoming RX test config (total bytes requested)
			 - Prepare transmitter
			 - Send bytes as fast as possible
		*/

		// send 'count' bytes as quickly as possible
		if(mode == ECHOTEST_MODE_RX)
		{
			totKB=0;
			totMB=0;
			while(tot < count) {
				if((err = write(accfd, rbuf, sizeof rbuf)) < 0) {
					DEBUG_ERROR("error while sending test byte stream to echotest");
					return;
				}
				tot += err;
				totKB = (float)tot / (float)1024;
				totMB = (float)tot / (float)(1024*1024);
				//DEBUG_TEST("read = %d, totB = %d, totKB = %3f, totMB = %3f", err, tot, totKB, totMB);
			}
			DEBUG_TEST("sleeping before closing socket and accepting further selftest connections");
			sleep(3);
		}
		close(accfd);
	}
	close(sockfd);
}

int main(int argc , char *argv[])
{
    if(argc < 1) {
        fprintf(stderr, "usage: echo <alice|bob>.conf\n");
        fprintf(stderr, " - Define your test environment in *.conf files.\n");     
        return 1;
    }

    int start_port       = 0;
    int port_offset      = 0;
	int echo_listen_port = 0;

	std::string local_echo_ipv4;

	std::string nwid, stype, path = argv[1];
	std::string ipstr, ipstr6, local_ipstr, local_ipstr6, remote_ipstr, remote_ipstr6;

	// if a test config file was specified:
	if(path.find(".conf") != std::string::npos) {
		loadTestConfigFile(path);
		start_port = atoi(testConf["start_port"].c_str());
		port_offset = atoi(testConf["port_offset"].c_str());
		local_ipstr   = testConf["local_ipv4"];
		local_ipstr6  = testConf["local_ipv6"];
		local_echo_ipv4 = testConf["local_echo_ipv4"];
		remote_ipstr  = testConf["remote_ipv4"];
		remote_ipstr6 = testConf["remote_ipv6"];

		if(strcmp(testConf["name"].c_str(), "alice") == 0)
			echo_listen_port = start_port+port_offset;
		else if(strcmp(testConf["name"].c_str(), "bob") == 0)
			echo_listen_port = start_port+port_offset+1;

		fprintf(stderr, "\tlocal_ipstr     = %s\n", local_ipstr.c_str());
		fprintf(stderr, "\tremote_ipstr    = %s\n", remote_ipstr.c_str());		
		fprintf(stderr, "\tstart_port      = %d\n", start_port);
		fprintf(stderr, "\tport_offset     = %d\n", port_offset);
		fprintf(stderr, "\tlocal_echo_ipv4 = %s\n", local_echo_ipv4.c_str());
	}
	
	DEBUG_TEST("Starting echo mode... %s : %d", local_echo_ipv4.c_str(), echo_listen_port);
	start_echo_mode(local_echo_ipv4, echo_listen_port);
	return 1;
}