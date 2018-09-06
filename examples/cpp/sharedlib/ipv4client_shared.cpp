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

#if !defined(_MSC_VER)

#include <stdio.h>
#include <string.h>
#include <string>
#include <inttypes.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dlfcn.h>

#include "libzt.h"

// function pointers which will have values assigned once the dynamic library is loaded

int (*_zts_set_service_port)(int portno);
int (*_zts_start)(const char *path, bool blocking);
int (*_zts_startjoin)(const char*, uint64_t);
void (*_zts_stop)(void);
int (*_zts_core_running)(void);
int (*_zts_stack_running)(void);
int (*_zts_ready)(void);
int (*_zts_join)(const uint64_t);
int (*_zts_leave)(const uint64_t);
void (*_zts_get_path)(char *homePath, const size_t len);
uint64_t (*_zts_get_node_id)(void);
int (*_zts_has_address)(const uint64_t);
int (*_zts_get_num_assigned_addresses)(const uint64_t nwid);
int (*_zts_get_address_at_index)(const uint64_t nwid, const int index, struct sockaddr_storage *addr);
int (*_zts_get_address)(const uint64_t nwid, struct sockaddr_storage *addr, const int address_family);
void (*_zts_get_6plane_addr)(struct sockaddr_storage *addr, const uint64_t nwid, const uint64_t nodeId);
void (*_zts_get_rfc4193_addr)(struct sockaddr_storage *addr, const uint64_t nwid, const uint64_t nodeId);
unsigned long (*_zts_get_peer_count)(void);
int (*_zts_get_peer_address)(char *peer, const uint64_t nodeId);
int (*_zts_socket)(int socket_family, int socket_type, int protocol);
int (*_zts_connect)(int fd, const struct sockaddr *addr, socklen_t addrlen);
int (*_zts_bind)(int fd, const struct sockaddr *addr, socklen_t addrlen);
int (*_zts_listen)(int fd, int backlog);
int (*_zts_accept)(int fd, struct sockaddr *addr, socklen_t *addrlen);
#if defined(__linux__)
int (*_zts_accept4)(int fd, struct sockaddr *addr, socklen_t *addrlen, int flags);
#endif
int (*_zts_setsockopt)(int fd, int level, int optname, const void *optval, socklen_t optlen);
int (*_zts_getsockopt)(int fd, int level, int optname, void *optval, socklen_t *optlen);
int (*_zts_getsockname)(int fd, struct sockaddr *addr, socklen_t *addrlen);
int (*_zts_getpeername)(int fd, struct sockaddr *addr, socklen_t *addrlen);
int (*_zts_gethostname)(char *name, size_t len);
int (*_zts_sethostname)(const char *name, size_t len);
struct hostent *(*_zts_gethostbyname)(const char *name);
int (*_zts_close)(int fd);
int(*_zts_fcntl)(int fd, int cmd, int flags);
int (*_zts_ioctl)(int fd, unsigned long request, void *argp);
ssize_t (*_zts_send)(int fd, const void *buf, size_t len, int flags);
ssize_t (*_zts_sendto)(int fd, const void *buf, size_t len, int flags, const struct sockaddr *addr, socklen_t addrlen);
ssize_t (*_zts_sendmsg)(int fd, const struct msghdr *msg, int flags);
ssize_t (*_zts_recv)(int fd, void *buf, size_t len, int flags);
ssize_t (*_zts_recvfrom)(int fd, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t *addrlen);
ssize_t (*_zts_recvmsg)(int fd, struct msghdr *msg,int flags);
int (*_zts_read)(int fd, void *buf, size_t len);
int (*_zts_write)(int fd, const void *buf, size_t len);
int (*_zts_shutdown)(int fd, int how);
int (*_zts_add_dns_nameserver)(struct sockaddr *addr);
int (*_zts_del_dns_nameserver)(struct sockaddr *addr);


void load_library_symbols(char *library_path)
{
	void *libHandle = dlopen(library_path, RTLD_LAZY);
	if (libHandle == NULL) {
		DEBUG_ERROR("unable to load dynamic libs: %s", dlerror());
		exit(-1);
	}

	// Load symbols from library (call these directly)

	_zts_set_service_port = (int(*)(int portno))dlsym(libHandle, "zts_set_service_port");
	_zts_start = (int(*)(const char *path, bool blocking))dlsym(libHandle, "zts_start");
	_zts_startjoin = (int(*)(const char*, uint64_t))dlsym(libHandle, "zts_startjoin");
	_zts_stop = (void(*)(void))dlsym(libHandle, "zts_stop");
	_zts_core_running = (int(*)(void))dlsym(libHandle, "zts_core_running");
	_zts_stack_running = (int(*)(void))dlsym(libHandle, "zts_stack_running");
	_zts_ready = (int(*)(void))dlsym(libHandle, "zts_ready");
	_zts_join = (int(*)(const uint64_t))dlsym(libHandle, "zts_join");
	_zts_leave = (int(*)(const uint64_t))dlsym(libHandle, "zts_leave");
	_zts_get_path = (void(*)(char *homePath, const size_t len))dlsym(libHandle, "zts_get_path");
	_zts_get_node_id = (uint64_t(*)(void))dlsym(libHandle, "zts_get_node_id");
	_zts_has_address = (int(*)(const uint64_t))dlsym(libHandle, "zts_has_address");
	_zts_get_num_assigned_addresses = (int(*)(const uint64_t nwid))dlsym(libHandle, "zts_get_num_assigned_addresses");
	_zts_get_address_at_index = (int(*)(const uint64_t nwid, const int index, struct sockaddr_storage *addr))dlsym(libHandle, "zts_get_address_at_index");
	_zts_get_address = (int(*)(const uint64_t nwid, struct sockaddr_storage *addr, const int address_family))dlsym(libHandle, "zts_get_address");
	_zts_get_6plane_addr = (void(*)(struct sockaddr_storage *addr, const uint64_t nwid, const uint64_t nodeId))dlsym(libHandle, "zts_get_6plane_addr");
	_zts_get_rfc4193_addr = (void(*)(struct sockaddr_storage *addr, const uint64_t nwid, const uint64_t nodeId))dlsym(libHandle, "zts_get_rfc4193_addr");
	_zts_get_peer_count = (unsigned long(*)(void))dlsym(libHandle, "zts_get_peer_count");
	_zts_get_peer_address = (int(*)(char *peer, const uint64_t nodeId))dlsym(libHandle, "zts_get_peer_address");
	_zts_socket = (int(*)(int socket_family, int socket_type, int protocol))dlsym(libHandle, "zts_socket");
	_zts_connect = (int(*)(int fd, const struct sockaddr *addr, socklen_t addrlen))dlsym(libHandle, "zts_connect");
	_zts_bind = (int(*)(int fd, const struct sockaddr *addr, socklen_t addrlen))dlsym(libHandle, "zts_bind");
	_zts_listen = (int(*)(int fd, int backlog))dlsym(libHandle, "zts_listen");
	_zts_accept = (int(*)(int fd, struct sockaddr *addr, socklen_t *addrlen))dlsym(libHandle, "zts_accept");
	#if defined(__linux__)
	_zts_accept4 = (int(*)(int fd, struct sockaddr *addr, socklen_t *addrlen, int flags))dlsym(libHandle, "zts_accept4");
	#endif
	_zts_setsockopt = (int(*)(int fd, int level, int optname, const void *optval, socklen_t optlen))dlsym(libHandle, "zts_setsockopt");
	_zts_getsockopt = (int(*)(int fd, int level, int optname, void *optval, socklen_t *optlen))dlsym(libHandle, "zts_getsockopt");
	_zts_getsockname = (int(*)(int fd, struct sockaddr *addr, socklen_t *addrlen))dlsym(libHandle, "zts_getsockname");
	_zts_getpeername = (int(*)(int fd, struct sockaddr *addr, socklen_t *addrlen))dlsym(libHandle, "zts_getpeername");
	_zts_gethostname = (int(*)(char *name, size_t len))dlsym(libHandle, "zts_gethostname");
	_zts_sethostname = (int(*)(const char *name, size_t len))dlsym(libHandle, "zts_sethostname");
	_zts_gethostbyname = (struct hostent*(*)(const char *name))dlsym(libHandle, "zts_gethostbyname");
	_zts_close = (int(*)(int fd))dlsym(libHandle, "zts_close");
	_zts_fcntl = (int(*)(int fd, int cmd, int flags))dlsym(libHandle, "zts_fcntl");
	_zts_ioctl = (int(*)(int fd, unsigned long request, void *argp))dlsym(libHandle, "zts_ioctl");
	_zts_send = (ssize_t(*)(int fd, const void *buf, size_t len, int flags))dlsym(libHandle, "zts_send");
	_zts_sendto = (ssize_t(*)(int fd, const void *buf, size_t len, int flags, const struct sockaddr *addr, socklen_t addrlen))dlsym(libHandle, "zts_sendto");
	_zts_sendmsg = (ssize_t(*)(int fd, const struct msghdr *msg, int flags))dlsym(libHandle, "zts_sendmsg");
	_zts_recv = (ssize_t(*)(int fd, void *buf, size_t len, int flags))dlsym(libHandle, "zts_recv");
	_zts_recvfrom = (ssize_t(*)(int fd, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t *addrlen))dlsym(libHandle, "zts_recvfrom");
	_zts_recvmsg = (ssize_t(*)(int fd, struct msghdr *msg,int flags))dlsym(libHandle, "zts_recvmsg");
	_zts_read = (int(*)(int fd, void *buf, size_t len))dlsym(libHandle, "zts_read");
	_zts_write = (int(*)(int fd, const void *buf, size_t len))dlsym(libHandle, "zts_write");
	_zts_shutdown = (int(*)(int fd, int how))dlsym(libHandle, "zts_shutdown");
	_zts_add_dns_nameserver = (int(*)(struct sockaddr *addr))dlsym(libHandle, "zts_add_dns_nameserver");
}

char *msg = (char*)"welcome to the machine";

int main(int argc, char **argv)
{
	if (argc != 5) {
		printf("\nlibzt example client\n");
		printf("client [config_file_path] [nwid] [remote_addr] [remote_port]\n");
		exit(0);
	}

#ifdef __linux__
	char *library_path = (char*)"./libzt.so";
#endif
#ifdef __APPLE__
	char *library_path = (char*)"./libzt.dylib";
#endif
	load_library_symbols(library_path);

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

	//

	printf("Waiting for libzt to come online...\n");
	uint64_t nwid = strtoull(nwidstr.c_str(),NULL,16);
	printf("nwid=%llx\n", (unsigned long long)nwid);
	_zts_startjoin(path.c_str(), nwid);
	uint64_t nodeId = _zts_get_node_id();
	printf("I am %llx\n", (unsigned long long)nodeId);

	if ((sockfd = _zts_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("error creating ZeroTier socket\n");
	}

	if ((err = _zts_connect(sockfd, (const struct sockaddr *)&in4, sizeof(in4))) < 0) {
		printf("error connecting to remote host (%d)\n", err);
	}

	printf("sending to server...\n");
	w = _zts_write(sockfd, msg, strlen(msg));
	
	printf("reading from server...\n");
	r = _zts_read(sockfd, rbuf, strlen(msg));

	printf("Sent     : %s\n", msg);
	printf("Received : %s\n", rbuf);

	err = _zts_close(sockfd);

	return err;
}

#endif // _MSC_VER