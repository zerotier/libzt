/*
 * Copyright (c)2013-2020 ZeroTier, Inc.
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file in the project's root directory.
 *
 * Change Date: 2024-01-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2.0 of the Apache License.
 */
/****/

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




