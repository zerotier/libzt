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

%module libzt

%include "stdint.i"

%{
#define SWIG_FILE_WITH_INIT
#include "../../include/libzt.h"
%}

int zts_start(const char *path, bool blocking);

int zts_startjoin(const char *path, const uint64_t nwid);

void zts_stop();

int zts_core_running();

int zts_stack_running();

int zts_ready();

int zts_join(const uint64_t nwid);

int zts_leave(const uint64_t nwid);

void zts_get_path(char *homePath, const size_t len);

uint64_t zts_get_node_id();

uint64_t zts_get_node_id_from_file(const char *filepath);

int zts_has_address(const uint64_t nwid);

int zts_get_num_assigned_addresses(const uint64_t nwid);

int zts_get_address_at_index(
	const uint64_t nwid, const int index, struct sockaddr_storage *addr);

int zts_get_address(
	const uint64_t nwid, struct sockaddr_storage *addr, const int address_family);

void zts_get_6plane_addr(
	struct sockaddr_storage *addr, const uint64_t nwid, const uint64_t nodeId);

void zts_get_rfc4193_addr(
	struct sockaddr_storage *addr, const uint64_t nwid, const uint64_t nodeId);

unsigned long zts_get_peer_count();

int zts_get_peer_address(char *peer, const uint64_t nodeId);

int zts_socket(int socket_family, int socket_type, int protocol);

int zts_connect(int fd, const struct sockaddr *addr, socklen_t addrlen);

int zts_bind(int fd, const struct sockaddr *addr, socklen_t addrlen);

int zts_listen(int fd, int backlog);

int zts_accept(int fd, struct sockaddr *addr, socklen_t *addrlen);

int zts_setsockopt(
	int fd, int level, int optname, const void *optval, socklen_t optlen);

int zts_getsockopt(
	int fd, int level, int optname, void *optval, socklen_t *optlen);

int zts_getsockname(int fd, struct sockaddr *addr, socklen_t *addrlen);

int zts_getpeername(int fd, struct sockaddr *addr, socklen_t *addrlen);

int zts_gethostname(char *name, size_t len);

int zts_sethostname(const char *name, size_t len);

struct hostent *zts_gethostbyname(const char *name);

int zts_close(int fd);

int zts_select(
	int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

int zts_fcntl(int fd, int cmd, int flags);

int zts_ioctl(int fd, unsigned long request, void *argp);

ssize_t zts_send(int fd, const void *buf, size_t len, int flags);

ssize_t zts_sendto(
	int fd, const void *buf, size_t len, int flags, const struct sockaddr *addr, socklen_t addrlen);

ssize_t zts_sendmsg(int fd, const struct msghdr *msg, int flags);

ssize_t zts_recv(int fd, void *buf, size_t len, int flags);

ssize_t zts_recvfrom(
	int fd, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t *addrlen);

ssize_t zts_recvmsg(int fd, struct msghdr *msg,int flags);

int zts_read(int fd, void *buf, size_t len);

int zts_write(int fd, const void *buf, size_t len);

int zts_shutdown(int fd, int how);

int zts_add_dns_nameserver(struct sockaddr *addr);

int zts_del_dns_nameserver(struct sockaddr *addr);

