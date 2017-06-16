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

/****************************************************************************/
/* PERFORMANCE                                                              */
/****************************************************************************/

// Maintain transfer for n_count OR n_count
void start_echo_mode(std::string ipstr, int port)
{
	DEBUG_TEST();
	/*
	int w=0, sockfd, err;
	int total_test_sz          = 1024*1024;
	int arbitrary_chunk_sz_max = 16384;
	int arbitrary_chunk_sz_min = 512;

	char rbuf[arbitrary_chunk_sz_max];

	for (int i=arbitrary_chunk_sz_min; (i*2) < arbitrary_chunk_sz_max; i*=2) {

		if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
			DEBUG_ERROR("error creating ZeroTier socket");
		if((err = connect(sockfd, (const struct sockaddr *)addr, sizeof(addr))) < 0)
			DEBUG_ERROR("error connecting to remote host (%d)\n", err);

		DEBUG_TEST("[TX] Testing (%d) byte chunks: ", i);

		int chunk_sz   = i;
		int iterations = total_test_sz / chunk_sz;

		long int start_time = get_now_ts();
		w = 0;
		while(w < total_test_sz)
			w += write(sockfd, rbuf, chunk_sz);
		long int end_time = get_now_ts();
		float ts_delta = (end_time - start_time) / (float)1000;
		float rate = (float)total_test_sz / (float)ts_delta;
		DEBUG_TEST("%d total bytes, time = %3f, rate = %3f KB/s", w, ts_delta, (rate / (float)1024) );

		close(sockfd);		
		// let things settle after test
		sleep(5);
	}	
	*passed = (w == total_test_sz && !err) ? PASSED : FAILED;
	*/
}

/*
	Mode 1 (Measure performance of other host's TX): 
	 - Receive incoming TX test config (total bytes intended)
	 - Prepare receiver
	 - Record time of first received byte
	 - Record time of last received byte
	 - Send results back to other host's selftest instance

	Mode 2 (Measure performance of other host's RX): 
	 - Receive incoming RX test config (total bytes requested)
	 - Prepare transmitter
	 - Send bytes as fast as possible
*/


int main(int argc , char *argv[])
{
    if(argc < 1) {
        fprintf(stderr, "usage: echo <alice|bob>.conf\n");
        fprintf(stderr, " - Define your test environment in *.conf files.\n");     
        return 1;
    }

    int err          = 0;
	int type         = 0;
    int protocol     = 0;
    int mode         = 0;
    int port         = 0;
    int start_port   = 0;
    int port_offset  = 0;
    int operation    = 0;
	int n_count      = 0;
	int delay        = 0;

	std::string local_echo_ipv4;

	std::string nwid, stype, path = argv[1];
	std::string ipstr, ipstr6, local_ipstr, local_ipstr6, remote_ipstr, remote_ipstr6;

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
		std::string smode   = testConf["mode"];
/*
		if(strcmp(smode.c_str(), "server") == 0)
			mode = TEST_MODE_SERVER;
		else
			mode = TEST_MODE_CLIENT;
*/
		fprintf(stderr, "\tlocal_ipstr   = %s\n", local_ipstr.c_str());
		fprintf(stderr, "\tremote_ipstr  = %s\n", remote_ipstr.c_str());		
		fprintf(stderr, "\tstart_port   = %d\n", start_port);
		fprintf(stderr, "\tport_offset  = %d\n", port_offset);
		fprintf(stderr, "\tlocal_echo_ipv4   = %s\n", local_echo_ipv4.c_str());
	}

	fprintf(stderr, "\tpath          = %s\n", path.c_str());
	fprintf(stderr, "\tnwid          = %s\n", nwid.c_str());
	fprintf(stderr, "\ttype          = %s\n\n", stype.c_str());
	
	DEBUG_TEST("Starting echo mode...");
	start_echo_mode(local_echo_ipv4, start_port+port_offset);
	return 1;
}