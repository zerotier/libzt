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

#include <stdio.h>
#include <string.h>
#include <string>
#include <inttypes.h>

#if defined(_WIN32)
#include <WinSock2.h>
#include <stdint.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#include "libzt.h"

char *msg = (char*)"welcome to the machine";

uint64_t generate_adhoc_nwid_from_port(int port)
{
	std::string padding;
	if(port < 10) {
		padding = "000";
	} else if(port < 100) {
		padding = "00";
	} else if(port < 1000) {
		padding = "0";
	}
	// We will join ad-hoc network ffSSSSEEEE000000
	// Where SSSS = start port
	//       EEEE =   end port
	padding = padding + std::to_string(port); // SSSS
	std::string nwidstr = "ff" + padding + padding + "000000"; // ff + SSSS + EEEE + 000000
	return strtoull(nwidstr.c_str(), NULL, 16);
}

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

	struct sockaddr_in6 in6;
	in6.sin6_port = htons(remote_port);
	inet_pton(AF_INET6, remote_addr.c_str(), &(in6.sin6_addr));
	in6.sin6_family = AF_INET6;

	// --- BEGIN EXAMPLE CODE

	printf("Waiting for libzt to come online...\n");
	uint64_t nwid = generate_adhoc_nwid_from_port(remote_port);
	printf("nwid=%llx\n", (unsigned long long)nwid);
	zts_startjoin(path.c_str(), nwid);
	uint64_t nodeId = zts_get_node_id();
	printf("I am %llx\n", (unsigned long long)nodeId);

	if ((sockfd = zts_socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
		printf("error creating ZeroTier socket\n");
	}

	if ((err = zts_connect(sockfd, (const struct sockaddr *)&in6, sizeof(in6))) < 0) {
		printf("error connecting to remote host (%d)\n", err);
	}

	printf("sending to server...\n");
	w = zts_write(sockfd, msg, strlen(msg));
	
	printf("reading from server...\n");
	r = zts_read(sockfd, rbuf, strlen(msg));

	printf("Sent     : %s\n", msg);
	printf("Received : %s\n", rbuf);

	err = zts_close(sockfd);

	return err;
}