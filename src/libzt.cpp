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

/**
 * @file
 *
 * Application-facing, partially-POSIX-compliant socket API
 */

#include <cstring>

#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/ip_addr.h"

#include "libzt.h"

#ifdef __cplusplus
extern "C" {
#endif

void sys2lwip(int fd, const struct sockaddr *orig, struct sockaddr *modified) {
	
	/* Inelegant fix for lwIP 'sequential' API address error check (in sockets.c). For some reason
	lwIP seems to lose track of the sa_family for the socket internally, when lwip_connect()
	is called, it thus receives an AF_UNSPEC socket which fails. Here we use lwIP's own facilities
	to get the sa_family ourselves and rebuild the address structure and pass it to lwip_connect().
	I suspect this is due to a struct memory alignment issue. */

	struct sockaddr_storage ss;
	socklen_t namelen = sizeof(ss);
	int err = 0;
	if ((err = lwip_getsockname(fd, (struct sockaddr*)&ss, &namelen)) < 0) {
		DEBUG_ERROR("error while determining socket family");
		return;
	}

	if (ss.ss_family == AF_INET) {
#if defined(__linux__)	
		struct sockaddr_in *modified_ptr = (struct sockaddr_in *)modified;
		struct sockaddr_in *addr4 = (struct sockaddr_in*)orig;
		modified_ptr->sin_len = sizeof(struct sockaddr_in);
		modified_ptr->sin_family = ss.ss_family;
		modified_ptr->sin_port = addr4->sin_port;
		modified_ptr->sin_addr.s_addr = addr4->sin_addr.s_addr;
#else
		memcpy(modified, orig, sizeof(struct sockaddr_in));
#endif
	}
	if (ss.ss_family == AF_INET) {
#if defined(__linux__)	
#else
#endif
	}
}

int zts_socket(int socket_family, int socket_type, int protocol) 
{
	DEBUG_EXTRA("family=%d, type=%d, proto=%d", socket_family, socket_type, protocol);
	return lwip_socket(socket_family, socket_type, protocol);
}

int zts_connect(int fd, const struct sockaddr *addr, socklen_t addrlen) 
{
	DEBUG_EXTRA("fd=%d",fd);
	struct sockaddr_storage ss;
	sys2lwip(fd, addr, (struct sockaddr*)&ss);
	return lwip_connect(fd, (struct sockaddr*)&ss, addrlen);
}

int zts_bind(int fd, const struct sockaddr *addr, socklen_t addrlen) 
{
	DEBUG_EXTRA("fd=%d", fd);	
	struct sockaddr_storage ss;
	sys2lwip(fd, addr, (struct sockaddr*)&ss);
	return lwip_bind(fd, (struct sockaddr*)&ss, addrlen);
}

int zts_listen(int fd, int backlog) 
{
	DEBUG_EXTRA("fd=%d", fd);
	return lwip_listen(fd, backlog);
}

int zts_accept(int fd, struct sockaddr *addr, socklen_t *addrlen) 
{
	DEBUG_EXTRA("fd=%d", fd);
	return lwip_accept(fd, addr, addrlen);
}

#if defined(__linux__)
int zts_accept4(int fd, struct sockaddr *addr, socklen_t *addrlen, int flags)
{
	DEBUG_EXTRA("fd=%d", fd);
	return zts_accept(fd, addr, addrlen);
}
#endif

int zts_setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen)
{
	DEBUG_EXTRA("fd=%d, level=%d, optname=%d", fd, level, optname);
	return lwip_setsockopt(fd, level, optname, optval, optlen);
}

int zts_getsockopt(int fd, int level, int optname, void *optval, socklen_t *optlen)
{
	DEBUG_EXTRA("fd=%d, level=%d, optname=%d", fd, level, optname);
	return lwip_getsockopt(fd, level, optname, optval, optlen);
}

int zts_getsockname(int fd, struct sockaddr *addr, socklen_t *addrlen)
{
	DEBUG_EXTRA("fd=%p", fd);
	return lwip_getsockname(fd, addr, addrlen);
}

int zts_getpeername(int fd, struct sockaddr *addr, socklen_t *addrlen)
{
	DEBUG_EXTRA("fd=%d", fd);
	return lwip_getpeername(fd, addr, addrlen);
}

int zts_gethostname(char *name, size_t len)
{
	DEBUG_EXTRA();
	return -1;
}

int zts_sethostname(const char *name, size_t len)
{
	DEBUG_EXTRA();
	return -1;
}

int zts_close(int fd)
{
	DEBUG_EXTRA("fd=%d", fd);
	return lwip_close(fd);
}

int zts_poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
	DEBUG_EXTRA();
	return poll(fds, nfds, timeout);
}

int zts_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
	DEBUG_EXTRA();
	return select(nfds, readfds, writefds, exceptfds, timeout);
}

int zts_fcntl(int fd, int cmd, int flags)
{
	DEBUG_EXTRA("fd=%p, cmd=%d, flags=%d", cmd, flags);
	int translated_flags = 0;
	if (flags == 2048) {
		translated_flags = 1;
	}
	return lwip_fcntl(fd, cmd, translated_flags);
}

int zts_ioctl(int fd, unsigned long request, void *argp)
{
	DEBUG_EXTRA("fd=%d, req=%d", fd, request);
	return lwip_ioctl(fd, request, argp);
}

ssize_t zts_sendto(int fd, const void *buf, size_t len, int flags, const struct sockaddr *addr, socklen_t addrlen)
{
	DEBUG_TRANS("fd=%d, len=%d", fd, len);
	struct sockaddr_storage ss;
	sys2lwip(fd, addr, (struct sockaddr*)&ss);
	return lwip_sendto(fd, buf, len, flags, (struct sockaddr*)&ss, addrlen);
}

ssize_t zts_send(int fd, const void *buf, size_t len, int flags)
{
	DEBUG_TRANS("fd=%d, len=%d", fd, len);
	return lwip_send(fd, buf, len, flags);
}

ssize_t zts_sendmsg(int fd, const struct msghdr *msg, int flags)
{
	DEBUG_TRANS("fd=%d", fd);
	return lwip_sendmsg(fd, msg, flags);
}

ssize_t zts_recv(int fd, void *buf, size_t len, int flags)
{
	DEBUG_TRANS("fd=%d", fd);
	return lwip_recv(fd, buf, len, flags);
}

ssize_t zts_recvfrom(int fd, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t *addrlen)
{
	DEBUG_TRANS("fd=%d", fd);
	return lwip_recvfrom(fd, buf, len, flags, addr, addrlen);
}

ssize_t zts_recvmsg(int fd, struct msghdr *msg,int flags)
{
	DEBUG_TRANS("fd=%d", fd);
	return -1;
}

int zts_read(int fd, void *buf, size_t len) {
	DEBUG_TRANS("fd=%d, len=%d", fd, len);
	return lwip_read(fd, buf, len);
}

int zts_write(int fd, const void *buf, size_t len) {
	DEBUG_TRANS("fd=%d, len=%d", fd, len);
	return lwip_write(fd, buf, len);
}

int zts_shutdown(int fd, int how)
{
	DEBUG_EXTRA("fd=%d, how=%d", fd, how);
	return lwip_shutdown(fd, how);
}

int zts_add_dns_nameserver(struct sockaddr *addr)
{
	DEBUG_EXTRA();
	return -1;
}

int zts_del_dns_nameserver(struct sockaddr *addr)
{
	DEBUG_EXTRA();
	return -1;
}

#ifdef __cplusplus
}
#endif