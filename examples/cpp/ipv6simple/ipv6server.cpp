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

	struct sockaddr_in6 in6, acc_in6;
	in6.sin6_port = htons(bind_port);
	in6.sin6_family = AF_INET6;
	in6.sin6_addr = in6addr_any;

	// --- BEGIN EXAMPLE CODE

	printf("Waiting for libzt to come online...\n");
	uint64_t nwid = strtoull(nwidstr.c_str(),NULL,16);
	printf("nwid=%llx\n", (unsigned long long)nwid);
	zts_startjoin(path.c_str(), nwid);
	uint64_t nodeId = zts_get_node_id();
	printf("I am %llx\n", (unsigned long long)nodeId);

	if ((sockfd = zts_socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
		printf("error creating ZeroTier socket\n");
	}
	
	if ((err = zts_bind(sockfd, (struct sockaddr *)&in6, sizeof(struct sockaddr_in6)) < 0)) {
		printf("error binding to interface (%d)\n", err);
	}
	
	if ((err = zts_listen(sockfd, 100)) < 0) {
		printf("error placing socket in LISTENING state (%d)\n", err);
	}
	
	socklen_t client_addrlen = sizeof(sockaddr_in6);
	if ((accfd = zts_accept(sockfd, (struct sockaddr *)&acc_in6, &client_addrlen)) < 0) {
		printf("error accepting connection (%d)\n", err);
	}

	socklen_t peer_addrlen = sizeof(struct sockaddr_storage);
	zts_getpeername(accfd, (struct sockaddr*)&acc_in6, &peer_addrlen);
	//DEBUG_INFO("accepted connection from %s : %d", inet_ntoa(acc_in6.sin6_addr), ntohs(acc_in6.sin6_port));

	printf("reading from client...\n");
	r = zts_read(accfd, rbuf, sizeof rbuf);

	printf("sending to client...\n");
	w = zts_write(accfd, rbuf, strlen(rbuf));

	printf("Received : %s\n", rbuf);

	err = zts_close(sockfd);
	err = zts_close(accfd);

	return err;
}