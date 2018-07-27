/*
 * ZeroTier SDK - Network Virtualization Everywhere
 * Copyright (C) 2011-2018  ZeroTier, Inc.  https://www.zerotier.com/
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
 * Application-facing, socket-like API
 */

#include "libztDefs.h"

#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "lwip/netdb.h"

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

int platform_adjusted_socket_family(int family);
void fix_addr_socket_family(struct sockaddr *addr);
bool zts_ready();

int zts_socket(int socket_family, int socket_type, int protocol)
{
	int socket_family_adj = platform_adjusted_socket_family(socket_family);
	return !zts_ready() ? -1 : lwip_socket(socket_family_adj, socket_type, protocol);
}

int zts_connect(int fd, const struct sockaddr *addr, socklen_t addrlen)
{
	struct sockaddr_storage ss;
	memcpy(&ss, addr, addrlen);
	fix_addr_socket_family((struct sockaddr*)&ss);
	return !zts_ready() ? -1 : lwip_connect(fd, (struct sockaddr*)&ss, addrlen);
}

int zts_bind(int fd, const struct sockaddr *addr, socklen_t addrlen)
{
	struct sockaddr_storage ss;
	memcpy(&ss, addr, addrlen);
	fix_addr_socket_family((struct sockaddr*)&ss);
	return !zts_ready() ? -1 : lwip_bind(fd, (struct sockaddr*)&ss, addrlen);
}

int zts_listen(int fd, int backlog)
{
	return !zts_ready() ? -1 : lwip_listen(fd, backlog);
}

int zts_accept(int fd, struct sockaddr *addr, socklen_t *addrlen)
{
	return !zts_ready() ? -1 : lwip_accept(fd, addr, addrlen);
}

#if defined(__linux__)
int zts_accept4(int fd, struct sockaddr *addr, socklen_t *addrlen, int flags)
{
	return !zts_ready() ? -1 : -1; // lwip_accept4(fd, addr, addrlen, flags);
}
#endif

int zts_setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen)
{
	return !zts_ready() ? -1 : lwip_setsockopt(fd, level, optname, optval, optlen);
}

int zts_getsockopt(int fd, int level, int optname, void *optval, socklen_t *optlen)
{
	return !zts_ready() ? -1 : lwip_getsockopt(fd, level, optname, optval, optlen);
}

int zts_getsockname(int fd, struct sockaddr *addr, socklen_t *addrlen)
{
	return !zts_ready() ? -1 : lwip_getsockname(fd, addr, addrlen);
}

int zts_getpeername(int fd, struct sockaddr *addr, socklen_t *addrlen)
{
	return !zts_ready() ? -1 : lwip_getpeername(fd, addr, addrlen);
}

int zts_gethostname(char *name, size_t len)
{
	return !zts_ready() ? -1 : -1; // TODO
}

int zts_sethostname(const char *name, size_t len)
{
	return !zts_ready() ? -1 : -1; // TODO
}

struct hostent *zts_gethostbyname(const char *name)
{
	if (zts_ready() == false) {
		return NULL;
	}
	// TODO: Test thread safety
	/*
	char buf[256];
	int buflen = 256;
	int h_err = 0;
	struct hostent hret;
	struct hostent **result = NULL;
	int err = 0;
	if ((err = lwip_gethostbyname_r(name, &hret, buf, buflen, result, &h_err)) != 0) {
		DEBUG_ERROR("err = %d", err);
		DEBUG_ERROR("h_err = %d", h_err);
		errno = h_err;
		return NULL; // failure
	}
	return *result;
	
	return lwip_gethostbyname(name);
	*/
	return NULL;
}

int zts_close(int fd)
{
	return !zts_ready() ? -1 : lwip_close(fd);
}

int zts_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
	struct timeval *timeout)
{
	return !zts_ready() ? -1 : lwip_select(nfds, readfds, writefds, exceptfds, timeout);
}

int zts_fcntl(int fd, int cmd, int flags)
{
	// translation from platform flag values to stack flag values
	int translated_flags = 0;
#if defined(__linux__)
	if (flags == 2048) {
		translated_flags = 1;
	}
#endif
#if defined(__APPLE__)
	if (flags == 4) {
		translated_flags = 1;
	}
#endif
	return !zts_ready() ? -1 : lwip_fcntl(fd, cmd, translated_flags);
}

int zts_ioctl(int fd, unsigned long request, void *argp)
{
	return !zts_ready() ? -1 : lwip_ioctl(fd, request, argp);
}

ssize_t zts_sendto(int fd, const void *buf, size_t len, int flags, 
	const struct sockaddr *addr, socklen_t addrlen)
{
	struct sockaddr_storage ss;
	memcpy(&ss, addr, addrlen);
	fix_addr_socket_family((struct sockaddr*)&ss);
	return !zts_ready() ? -1 : lwip_sendto(fd, buf, len, flags, (struct sockaddr*)&ss, addrlen);
}

ssize_t zts_send(int fd, const void *buf, size_t len, int flags)
{
	return !zts_ready() ? -1 : lwip_send(fd, buf, len, flags);
}

ssize_t zts_sendmsg(int fd, const struct msghdr *msg, int flags)
{
	return !zts_ready() ? -1 : lwip_sendmsg(fd, msg, flags);
}

ssize_t zts_recv(int fd, void *buf, size_t len, int flags)
{
	return !zts_ready() ? -1 : lwip_recv(fd, buf, len, flags);
}

ssize_t zts_recvfrom(int fd, void *buf, size_t len, int flags, 
	struct sockaddr *addr, socklen_t *addrlen)
{
	return !zts_ready() ? -1 : lwip_recvfrom(fd, buf, len, flags, addr, addrlen);
}

ssize_t zts_recvmsg(int fd, struct msghdr *msg, int flags)
{
	return !zts_ready() ? -1 : -1; // Not currently implemented by stack
}

int zts_read(int fd, void *buf, size_t len)
{
	return !zts_ready() ? -1 : lwip_read(fd, buf, len);
}

int zts_write(int fd, const void *buf, size_t len)
{
	return !zts_ready() ? -1 : lwip_write(fd, buf, len);
}

int zts_shutdown(int fd, int how)
{
	return !zts_ready() ? -1 : lwip_shutdown(fd, how);
}

int zts_add_dns_nameserver(struct sockaddr *addr)
{
	return !zts_ready() ? -1 : -1; // TODO
}

int zts_del_dns_nameserver(struct sockaddr *addr)
{
	return !zts_ready() ? -1 : -1; // TODO
}

/* The rationale for the following correctional methods is as follows:

	Since we don't want the user of this library to worry about naming conflicts
	with their native OS/platform's socket facilities we deliberately isolate what
	is used by the user-space network stack and stack drivers from the user's
	application. As a result of this, we must compensate for a few things on our
	side. For instance, differing values for AF_INET6 on major operating systems, and
	differing structure definitions for sockaddr.
*/

/* adjust socket_family value (when AF_INET6) for various platforms:
	linux  : 10
	macOS  : 30
	windows: 23
*/
int platform_adjusted_socket_family(int family)
{
#if defined(__linux__)
	return family; // do nothing
#endif
#if defined(__APPLE__)
	 return family == 30 ? AF_INET6 : family; // 10
#endif
#if defined(_WIN32)
	 if (family == 23) {
		 return AF_INET6;
	 }
	 if (family == 2) {
		 return AF_INET;
	 }
	 return -1;
#endif
}

void fix_addr_socket_family(struct sockaddr *addr)
{
#if defined(__linux__) || defined(_WIN32)
	/* struct sockaddr on Linux and Windows don't contain an sa_len field
	so we must adjust it here before feeding it into the stack. */
	if (addr->sa_len == 2) {
		if (addr->sa_family == 0) {
			addr->sa_family = addr->sa_len;
			addr->sa_len = 0;
		}
	}
	if (addr->sa_len == 10 || addr->sa_len == 23 || addr->sa_len == 30) {
		if (addr->sa_family == 0) {
			addr->sa_family = addr->sa_len;
			addr->sa_len = 0;
		}
	}
	/* once we've moved the value to its anticipated location, convert it from
	its platform-specific value to one that the network stack can work with */
#endif
	addr->sa_family = platform_adjusted_socket_family(addr->sa_family);
}

#ifdef __cplusplus
}
#endif
