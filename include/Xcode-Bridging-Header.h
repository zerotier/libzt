/*
 * ZeroTier SDK - Network Virtualization Everywhere
 * Copyright (C) 2011-2019  ZeroTier, Inc.  https://www.zerotier.com/
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * --
 *
 * You can be released from the requirements of the license by purchasing
 * a commercial license. Buying such a license is mandatory as soon as you
 * develop commercial closed-source software that incorporates or links
 * directly against ZeroTier software without disclosing the source code
 * of your own application.
 */

/**
 * @file
 *
 * ZeroTier socket API
 */

#ifndef LIBZT_BRIDGING_HEADER_H
#define LIBZT_BRIDGING_HEADER_H

#include <sys/socket.h>
#include "ZeroTier.h"

//////////////////////////////////////////////////////////////////////////////
// Service Controls                                                         //
//////////////////////////////////////////////////////////////////////////////

int zts_start(const char *path, void *callbackFunc, int port);
void zts_stop();
int zts_join(uint64_t nwid);
int zts_leave(uint64_t nwid);
uint64_t zts_get_node_id();
uint64_t zts_get_node_status();
int get_peer_status(uint64_t peerId);

//////////////////////////////////////////////////////////////////////////////
// Socket API                                                               //
//////////////////////////////////////////////////////////////////////////////

int zts_socket(int socket_family, int socket_type, int protocol);
int zts_connect(int fd, const struct sockaddr *addr, socklen_t addrlen);
int zts_bind(int fd, const struct sockaddr *addr, socklen_t addrlen);
int zts_listen(int fd, int backlog);
int zts_accept(int fd, struct sockaddr *addr, socklen_t *addrlen);
int zts_setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen);
int zts_getsockopt(int fd, int level, int optname, void *optval, socklen_t *optlen);
int zts_read(int fd, void *buf, size_t len);
int zts_write(int fd, const void *buf, size_t len);
ssize_t zts_send(int fd, const void *buf, size_t len, int flags);
ssize_t zts_sendto(int fd, const void *buf, size_t len, int flags, const struct sockaddr *addr, socklen_t addrlen);
ssize_t zts_sendmsg(int fd, const struct msghdr *msg, int flags);
ssize_t zts_recv(int fd, void *buf, size_t len, int flags);
ssize_t zts_recvfrom(int fd, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t *addrlen);
ssize_t zts_recvmsg(int fd, struct msghdr *msg,int flags);
int zts_shutdown(int fd, int how);
int zts_close(int fd);
int zts_getsockname(int fd, struct sockaddr *addr, socklen_t *addrlen);
int zts_getpeername(int fd, struct sockaddr *addr, socklen_t *addrlen);
int zts_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
int zts_fcntl(int fd, int cmd, int flags);
int zts_ioctl(int fd, unsigned long request, void *argp);

#endif // _H




