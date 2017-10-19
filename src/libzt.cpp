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

#if defined(STACK_LWIP)
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/ip_addr.h"
#include "lwip/netdb.h"
#include "dns.h"
#endif
#if defined(NO_STACK)
#include <sys/socket.h>
#endif

#include "libzt.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The rationale for the following correctional methods is as follows:

	Since we don't want the user of this library to worry about naming conflicts 
	with their native OS/platform's socket facilities we deliberately isolate what 
	is used by the userspace network stack and stack drivers from the user's
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
#if defined(__MINGW32__) || defined(__MINGW64__)
	return family == 23 ? AF_INET6 : family; // 10
#endif
}

void fix_addr_socket_family(struct sockaddr *addr) 
{
#if defined(__linux__)
	/* linux's socket.h's sockaddr definition doesn't contain an sa_len field
	so we must compensate here before feeding it into the stack. Due to this limitation 
	we must cast the pointer to the address into two different address 
	*/
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

int zts_socket(int socket_family, int socket_type, int protocol)
{
	int err = -1;
	int socket_family_adj = platform_adjusted_socket_family(socket_family);
	DEBUG_EXTRA("family=%d, type=%d, proto=%d", socket_family, socket_type, protocol);
#if defined(STACK_LWIP)
	err = lwip_socket(socket_family_adj, socket_type, protocol);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_connect(int fd, const struct sockaddr *addr, socklen_t addrlen)
{
	int err = -1;
	DEBUG_EXTRA("fd=%d",fd);
#if defined(STACK_LWIP)
	struct sockaddr_storage ss;
	memcpy(&ss, addr, addrlen);
	fix_addr_socket_family((struct sockaddr*)&ss);
	err = lwip_connect(fd, (struct sockaddr*)&ss, addrlen);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_bind(int fd, const struct sockaddr *addr, socklen_t addrlen)
{
	int err = -1;
	DEBUG_EXTRA("fd=%d", fd);
#if defined(STACK_LWIP)
	struct sockaddr_storage ss;
	memcpy(&ss, addr, addrlen);
	fix_addr_socket_family((struct sockaddr*)&ss);
	err = lwip_bind(fd, (struct sockaddr*)&ss, addrlen);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_listen(int fd, int backlog)
{
	int err = -1;
	DEBUG_EXTRA("fd=%d", fd);
#if defined(STACK_LWIP)
	err = lwip_listen(fd, backlog);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_accept(int fd, struct sockaddr *addr, socklen_t *addrlen)
{
	int err = -1;
	DEBUG_EXTRA("fd=%d", fd);
#if defined(STACK_LWIP)
	err = lwip_accept(fd, addr, addrlen);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

#if defined(__linux__)
int zts_accept4(int fd, struct sockaddr *addr, socklen_t *addrlen, int flags)
{
	int err = -1;
	DEBUG_EXTRA("fd=%d", fd);
#if defined(STACK_LWIP)
	err = zts_accept(fd, addr, addrlen);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}
#endif

int zts_setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen)
{
	int err = -1;
	DEBUG_EXTRA("fd=%d, level=%d, optname=%d", fd, level, optname);
#if defined(STACK_LWIP)
	err = lwip_setsockopt(fd, level, optname, optval, optlen);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_getsockopt(int fd, int level, int optname, void *optval, socklen_t *optlen)
{
	int err = -1;
	DEBUG_EXTRA("fd=%d, level=%d, optname=%d", fd, level, optname);
#if defined(STACK_LWIP)
	err = lwip_getsockopt(fd, level, optname, optval, optlen);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_getsockname(int fd, struct sockaddr *addr, socklen_t *addrlen)
{
	int err = -1;
	DEBUG_EXTRA("fd=%p", fd);
#if defined(STACK_LWIP)
	err = lwip_getsockname(fd, addr, addrlen);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_getpeername(int fd, struct sockaddr *addr, socklen_t *addrlen)
{
	int err = -1;
	DEBUG_EXTRA("fd=%d", fd);
#if defined(STACK_LWIP)
	err = lwip_getpeername(fd, addr, addrlen);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_gethostname(char *name, size_t len)
{
	DEBUG_EXTRA();
	int err = -1;
#if defined(STACK_LWIP)
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_sethostname(const char *name, size_t len)
{
	DEBUG_EXTRA();
	int err = -1;
#if defined(STACK_LWIP)
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

struct hostent *zts_gethostbyname(const char *name)
{
#if defined(STACK_LWIP)
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
	*/
	return lwip_gethostbyname(name);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
	return NULL;
#endif
}

int zts_close(int fd)
{
	int err = -1;
	DEBUG_EXTRA("fd=%d", fd);
#if defined(STACK_LWIP)
	err = lwip_close(fd);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

#if defined(__linux__)
int zts_poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
	int err = -1;
#if defined(STACK_LWIP)
	DEBUG_ERROR("warning, this is not implemented");
	return poll(fds, nfds, timeout);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}
#endif

int zts_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, 
	struct timeval *timeout)
{
	int err = -1;
	//DEBUG_EXTRA();
#if defined(STACK_LWIP)
	err = lwip_select(nfds, readfds, writefds, exceptfds, timeout);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_fcntl(int fd, int cmd, int flags)
{
	int err = -1;
	DEBUG_EXTRA("fd=%d, cmd=%d, flags=%d", fd, cmd, flags);
#if defined(STACK_LWIP)
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
	err = lwip_fcntl(fd, cmd, translated_flags);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_ioctl(int fd, unsigned long request, void *argp)
{
	int err = -1;
	DEBUG_EXTRA("fd=%d, req=%d", fd, request);
#if defined(STACK_LWIP)
	err = lwip_ioctl(fd, request, argp);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

ssize_t zts_sendto(int fd, const void *buf, size_t len, int flags, 
	const struct sockaddr *addr, socklen_t addrlen)
{
	int err = -1;
	DEBUG_TRANS("fd=%d, len=%d", fd, len);
#if defined(STACK_LWIP)
	struct sockaddr_storage ss;
	memcpy(&ss, addr, addrlen);
	fix_addr_socket_family((struct sockaddr*)&ss);
	err = lwip_sendto(fd, buf, len, flags, (struct sockaddr*)&ss, addrlen);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

ssize_t zts_send(int fd, const void *buf, size_t len, int flags)
{
	int err = -1;
	DEBUG_TRANS("fd=%d, len=%d", fd, len);
#if defined(STACK_LWIP)
	err = lwip_send(fd, buf, len, flags);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

ssize_t zts_sendmsg(int fd, const struct msghdr *msg, int flags)
{
	int err = -1;
	DEBUG_TRANS("fd=%d", fd);
#if defined(STACK_LWIP)
	err = lwip_sendmsg(fd, msg, flags);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

ssize_t zts_recv(int fd, void *buf, size_t len, int flags)
{
	int err = -1;
	DEBUG_TRANS("fd=%d", fd);
#if defined(STACK_LWIP)
	err = lwip_recv(fd, buf, len, flags);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

ssize_t zts_recvfrom(int fd, void *buf, size_t len, int flags, 
	struct sockaddr *addr, socklen_t *addrlen)
{
	int err = -1;
	DEBUG_TRANS("fd=%d", fd);
#if defined(STACK_LWIP)
	err = lwip_recvfrom(fd, buf, len, flags, addr, addrlen);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

ssize_t zts_recvmsg(int fd, struct msghdr *msg,int flags)
{
	DEBUG_TRANS("fd=%d", fd);
	int err = -1;
#if defined(STACK_LWIP)
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_read(int fd, void *buf, size_t len)
{
	int err = -1;
	//DEBUG_TRANS("fd=%d, len=%d", fd, len);
#if defined(STACK_LWIP)
	err = lwip_read(fd, buf, len);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_write(int fd, const void *buf, size_t len) 
{
	//DEBUG_TRANS("fd=%d, len=%d", fd, len);
	int err = -1;
#if defined(STACK_LWIP)
	err = lwip_write(fd, buf, len);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_shutdown(int fd, int how)
{
	int err = -1;
	DEBUG_EXTRA("fd=%d, how=%d", fd, how);
#if defined(STACK_LWIP)
	err = lwip_shutdown(fd, how);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_add_dns_nameserver(struct sockaddr *addr)
{
	DEBUG_EXTRA();
	int err = -1;
#if defined(STACK_LWIP) && defined(LIBZT_IPV4)
	struct sockaddr_in *in4 = (struct sockaddr_in*)&addr;
	static ip4_addr_t ipaddr;
	ipaddr.addr = in4->sin_addr.s_addr;
	// TODO: manage DNS server indices 
	dns_setserver(0, (const ip_addr_t*)&ipaddr);
	err = 0;
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_del_dns_nameserver(struct sockaddr *addr)
{
	DEBUG_EXTRA();
	int err = -1;
#if defined(STACK_LWIP)
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

#ifdef __cplusplus
}
#endif
