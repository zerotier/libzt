/*
 * Copyright (c)2013-2021 ZeroTier, Inc.
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file in the project's root directory.
 *
 * Change Date: 2025-01-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2.0 of the Apache License.
 */
/****/

/**
 * @file
 *
 * ZeroTier Socket API
 */

#include "lwip/sockets.h"

#include "ZeroTierSockets.h"
#include "lwip/def.h"
#include "lwip/dns.h"
#include "lwip/inet.h"
#include "lwip/ip_addr.h"
#include "lwip/netdb.h"
#include "lwip/stats.h"

#define ZTS_STATE_NODE_RUNNING        0x01
#define ZTS_STATE_STACK_RUNNING       0x02
#define ZTS_STATE_NET_SERVICE_RUNNING 0x04
#define ZTS_STATE_CALLBACKS_RUNNING   0x08
#define ZTS_STATE_FREE_CALLED         0x10

extern int zts_errno;

namespace ZeroTier {

extern uint8_t _serviceStateFlags;

#ifdef __cplusplus
extern "C" {
#endif

int zts_socket(const int socket_family, const int socket_type, const int protocol)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	return lwip_socket(socket_family, socket_type, protocol);
}

int zts_connect(int fd, const struct zts_sockaddr* addr, zts_socklen_t addrlen)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (! addr) {
		return ZTS_ERR_ARG;
	}
	if (addrlen > (int)sizeof(struct zts_sockaddr_storage)
	    || addrlen < (int)sizeof(struct zts_sockaddr_in)) {
		return ZTS_ERR_ARG;
	}
	return lwip_connect(fd, (sockaddr*)addr, addrlen);
}

int zts_connect_easy(int fd, int family, char* ipstr, int port, int timeout_ms)
{
	if (timeout_ms < 0) {
		return ZTS_ERR_ARG;
	}
	if (timeout_ms == 0) {
		timeout_ms = 30000;   // Default
	}
	int div = 4;   // Must be > 0, Four connection attempts per second
	int n_tries = (timeout_ms / 1000) * div;
	int connect_delay = 1000 / div;
	int err = ZTS_ERR_SOCKET;

	zts_socklen_t addrlen = 0;
	struct zts_sockaddr_storage ss;
	struct zts_sockaddr* sa = NULL;

	if (family == ZTS_AF_INET) {
		addrlen = sizeof(ss);
		ipstr2sockaddr(family, ipstr, port, (struct zts_sockaddr*)&ss, &addrlen);
		sa = (struct zts_sockaddr*)&ss;
	}
	if (family == ZTS_AF_INET6) {
		addrlen = sizeof(ss);
		ipstr2sockaddr(family, ipstr, port, (struct zts_sockaddr*)&ss, &addrlen);
		sa = (struct zts_sockaddr*)&ss;
	}
	if (addrlen > 0 && sa != NULL) {
		if (zts_get_blocking(fd)) {
			do {
				err = zts_connect(fd, sa, addrlen);
				zts_delay_ms(connect_delay);
				n_tries--;
			} while ((err < 0) && (zts_errno != 0) && (n_tries > 0));
		}
		return err;
	}
	return ZTS_ERR_ARG;
}

int zts_bind(int fd, const struct zts_sockaddr* addr, zts_socklen_t addrlen)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (! addr) {
		return ZTS_ERR_ARG;
	}
	if (addrlen > (int)sizeof(struct zts_sockaddr_storage)
	    || addrlen < (int)sizeof(struct zts_sockaddr_in)) {
		return ZTS_ERR_ARG;
	}
	return lwip_bind(fd, (sockaddr*)addr, addrlen);
}

int zts_bind_easy(int fd, int family, char* ipstr, int port)
{
	if (family == ZTS_AF_INET) {
		struct zts_sockaddr_in in4;
		zts_socklen_t addrlen = sizeof(in4);
		ipstr2sockaddr(family, ipstr, port, (struct zts_sockaddr*)&in4, &addrlen);
		struct zts_sockaddr* sa = (struct zts_sockaddr*)&in4;
		return zts_bind(fd, sa, addrlen);
	}
	if (family == ZTS_AF_INET6) {
		struct zts_sockaddr_in6 in6;
		zts_socklen_t addrlen = sizeof(in6);
		ipstr2sockaddr(family, ipstr, port, (struct zts_sockaddr*)&in6, &addrlen);
		struct zts_sockaddr* sa = (struct zts_sockaddr*)&in6;
		return zts_bind(fd, sa, addrlen);
	}
	return ZTS_ERR_ARG;
}

int zts_listen(int fd, int backlog)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	return lwip_listen(fd, backlog);
}

int zts_accept(int fd, struct zts_sockaddr* addr, zts_socklen_t* addrlen)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	return lwip_accept(fd, (sockaddr*)addr, (socklen_t*)addrlen);
}

int zts_accept_easy(int fd, char* remoteIpStr, int len, int* port)
{
	if (len != ZTS_INET6_ADDRSTRLEN) {
		return ZTS_ERR_ARG;
	}
	char ipstr[ZTS_INET6_ADDRSTRLEN];
	memset(ipstr, 0, ZTS_INET6_ADDRSTRLEN);

	zts_sockaddr_storage ss;
	zts_socklen_t addrlen = sizeof(ss);

	int acc_fd = zts_accept(fd, (zts_sockaddr*)&ss, (zts_socklen_t*)&addrlen);
	struct zts_sockaddr* sa = (struct zts_sockaddr*)&ss;
	if (sa->sa_family == ZTS_AF_INET) {
		struct zts_sockaddr_in* in4 = (struct zts_sockaddr_in*)sa;
		zts_inet_ntop(ZTS_AF_INET, &(in4->sin_addr), remoteIpStr, ZTS_INET_ADDRSTRLEN);
		*port = ntohs(in4->sin_port);
	}
	if (sa->sa_family == ZTS_AF_INET6) {
		struct zts_sockaddr_in6* in6 = (struct zts_sockaddr_in6*)sa;
		zts_inet_ntop(ZTS_AF_INET6, &(in6->sin6_addr), remoteIpStr, ZTS_INET6_ADDRSTRLEN);
		*port = ntohs(in6->sin6_port);
	}
	return acc_fd;
}

int zts_setsockopt(int fd, int level, int optname, const void* optval, zts_socklen_t optlen)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	return lwip_setsockopt(fd, level, optname, optval, optlen);
}

int zts_getsockopt(int fd, int level, int optname, void* optval, zts_socklen_t* optlen)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	return lwip_getsockopt(fd, level, optname, optval, (socklen_t*)optlen);
}

int zts_getsockname(int fd, struct zts_sockaddr* addr, zts_socklen_t* addrlen)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (! addr) {
		return ZTS_ERR_ARG;
	}
	if (*addrlen > (int)sizeof(struct zts_sockaddr_storage)
	    || *addrlen < (int)sizeof(struct zts_sockaddr_in)) {
		return ZTS_ERR_ARG;
	}
	return lwip_getsockname(fd, (sockaddr*)addr, (socklen_t*)addrlen);
}

int zts_getpeername(int fd, struct zts_sockaddr* addr, zts_socklen_t* addrlen)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (! addr) {
		return ZTS_ERR_ARG;
	}
	if (*addrlen > (int)sizeof(struct zts_sockaddr_storage)
	    || *addrlen < (int)sizeof(struct zts_sockaddr_in)) {
		return ZTS_ERR_ARG;
	}
	return lwip_getpeername(fd, (sockaddr*)addr, (socklen_t*)addrlen);
}

int zts_close(int fd)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	return lwip_close(fd);
}

int zts_select(
    int nfds,
    zts_fd_set* readfds,
    zts_fd_set* writefds,
    zts_fd_set* exceptfds,
    struct zts_timeval* timeout)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	return lwip_select(
	    nfds,
	    (fd_set*)readfds,
	    (fd_set*)writefds,
	    (fd_set*)exceptfds,
	    (timeval*)timeout);
}

int zts_fcntl(int fd, int cmd, int flags)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	return lwip_fcntl(fd, cmd, flags);
}

int zts_poll(struct zts_pollfd* fds, nfds_t nfds, int timeout)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	return lwip_poll((pollfd*)fds, nfds, timeout);
}

int zts_ioctl(int fd, unsigned long request, void* argp)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (! argp) {
		return ZTS_ERR_ARG;
	}
	return lwip_ioctl(fd, request, argp);
}

ssize_t zts_send(int fd, const void* buf, size_t len, int flags)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (! buf) {
		return ZTS_ERR_ARG;
	}
	return lwip_send(fd, buf, len, flags);
}

ssize_t zts_sendto(
    int fd,
    const void* buf,
    size_t len,
    int flags,
    const struct zts_sockaddr* addr,
    zts_socklen_t addrlen)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (! addr || ! buf) {
		return ZTS_ERR_ARG;
	}
	if (addrlen > (int)sizeof(struct zts_sockaddr_storage)
	    || addrlen < (int)sizeof(struct zts_sockaddr_in)) {
		return ZTS_ERR_ARG;
	}
	return lwip_sendto(fd, buf, len, flags, (sockaddr*)addr, addrlen);
}

ssize_t zts_sendmsg(int fd, const struct zts_msghdr* msg, int flags)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}

	return lwip_sendmsg(fd, (const struct msghdr*)msg, flags);
}

ssize_t zts_recv(int fd, void* buf, size_t len, int flags)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (! buf) {
		return ZTS_ERR_ARG;
	}
	return lwip_recv(fd, buf, len, flags);
}

ssize_t zts_recvfrom(
    int fd,
    void* buf,
    size_t len,
    int flags,
    struct zts_sockaddr* addr,
    zts_socklen_t* addrlen)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (! buf) {
		return ZTS_ERR_ARG;
	}
	return lwip_recvfrom(fd, buf, len, flags, (sockaddr*)addr, (socklen_t*)addrlen);
}

ssize_t zts_recvmsg(int fd, struct zts_msghdr* msg, int flags)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (! msg) {
		return ZTS_ERR_ARG;
	}
	return lwip_recvmsg(fd, (struct msghdr*)msg, flags);
}

ssize_t zts_read(int fd, void* buf, size_t len)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (! buf) {
		return ZTS_ERR_ARG;
	}
	return lwip_read(fd, buf, len);
}

ssize_t zts_readv(int fd, const struct zts_iovec* iov, int iovcnt)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}

	return lwip_readv(fd, (iovec*)iov, iovcnt);
}

ssize_t zts_write(int fd, const void* buf, size_t len)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}

	if (! buf) {
		return ZTS_ERR_ARG;
	}
	return lwip_write(fd, buf, len);
}

ssize_t zts_writev(int fd, const struct zts_iovec* iov, int iovcnt)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}

	return lwip_writev(fd, (iovec*)iov, iovcnt);
}

int zts_shutdown(int fd, int how)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	return lwip_shutdown(fd, how);
}

struct zts_hostent* zts_gethostbyname(const char* name)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return NULL;
	}
	if (! name) {
		return NULL;
	}
	return (struct zts_hostent*)lwip_gethostbyname(name);
}

int zts_dns_set_server(uint8_t index, const zts_ip_addr* addr)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (index >= DNS_MAX_SERVERS) {
		return ZTS_ERR_ARG;
	}
	if (! addr) {
		return ZTS_ERR_ARG;
	}
	dns_setserver(index, (const ip_addr_t*)addr);
	return ZTS_ERR_OK;
}

const zts_ip_addr* zts_dns_get_server(uint8_t index)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return NULL;
	}
	if (index >= DNS_MAX_SERVERS) {
		return NULL;
	}
	return (const zts_ip_addr*)dns_getserver(index);
}

char* zts_ipaddr_ntoa(const zts_ip_addr* addr)
{
	return ipaddr_ntoa((ip_addr_t*)addr);
}

int zts_ipaddr_aton(const char* cp, zts_ip_addr* addr)
{
	return ipaddr_aton(cp, (ip_addr_t*)addr);
}

const char* zts_inet_ntop(int family, const void* src, char* dst, zts_socklen_t size)
{
	return lwip_inet_ntop(family, src, dst, size);
}

int zts_inet_pton(int family, const char* src, void* dst)
{
	return lwip_inet_pton(family, src, dst);
}

int ipstr2sockaddr(
    int family,
    char* src_ipstr,
    int port,
    struct zts_sockaddr* dest_addr,
    zts_socklen_t* addrlen)
{
	if (family == ZTS_AF_INET) {
		struct zts_sockaddr_in in4;
		in4.sin_port = htons(port);
		in4.sin_family = family;
#if defined(_WIN32)
		zts_inet_pton(family, src_ipstr, &(in4.sin_addr.S_addr));
#else
		zts_inet_pton(family, src_ipstr, &(in4.sin_addr.s_addr));
#endif
		dest_addr = (struct zts_sockaddr*)&in4;
		*addrlen = sizeof(in4);
		return ZTS_ERR_OK;
	}
	if (family == ZTS_AF_INET6) {
		struct zts_sockaddr_in6 in6;
		in6.sin6_port = htons(port);
		in6.sin6_family = family;
#if defined(_WIN32)
		zts_inet_pton(family, src_ipstr, &(in6.sin6_addr));
#else
		zts_inet_pton(family, src_ipstr, &(in6.sin6_addr));
#endif
		dest_addr = (struct zts_sockaddr*)&in6;
		*addrlen = sizeof(in6);
		return ZTS_ERR_OK;
	}
	return ZTS_ERR_ARG;
}

//----------------------------------------------------------------------------//
// Convenience functions                                                      //
//----------------------------------------------------------------------------//

/**
 * Helper functions that simplify API wrapper generation and usage in other
 * non-C-like languages. Use simple integer types instead of bit flags, limit
 * the number of operations each function performs, prevent the user from
 * needing to manipulate the content of structures in a non-native language.
 */

int zts_set_no_delay(int fd, int enabled)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (enabled != 0 && enabled != 1) {
		return ZTS_ERR_ARG;
	}
	return zts_setsockopt(fd, ZTS_IPPROTO_TCP, ZTS_TCP_NODELAY, (void*)&enabled, sizeof(int));
}

int zts_get_no_delay(int fd)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	int err, optval = 0;
	zts_socklen_t len = sizeof(optval);
	if ((err = zts_getsockopt(fd, ZTS_IPPROTO_TCP, ZTS_TCP_NODELAY, (void*)&optval, &len)) < 0) {
		return err;
	}
	return optval != 0;
}

int zts_set_linger(int fd, int enabled, int value)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (enabled != 0 && enabled != 1) {
		return ZTS_ERR_ARG;
	}
	if (value < 0) {
		return ZTS_ERR_ARG;
	}
	struct zts_linger linger;
	linger.l_onoff = enabled;
	linger.l_linger = value;
	return zts_setsockopt(fd, ZTS_SOL_SOCKET, ZTS_SO_LINGER, (void*)&linger, sizeof(linger));
}

int zts_get_linger_enabled(int fd)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	int err;
	struct zts_linger linger;
	zts_socklen_t len = sizeof(linger);
	if ((err = zts_getsockopt(fd, ZTS_SOL_SOCKET, ZTS_SO_LINGER, (void*)&linger, &len)) < 0) {
		return err;
	}
	return linger.l_onoff;
}

int zts_get_linger_value(int fd)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	int err;
	struct zts_linger linger;
	zts_socklen_t len = sizeof(linger);
	if ((err = zts_getsockopt(fd, ZTS_SOL_SOCKET, ZTS_SO_LINGER, (void*)&linger, &len)) < 0) {
		return err;
	}
	return linger.l_linger;
}

int zts_set_reuse_addr(int fd, int enabled)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (enabled != 0 && enabled != 1) {
		return ZTS_ERR_ARG;
	}
	return zts_setsockopt(fd, ZTS_SOL_SOCKET, ZTS_SO_REUSEADDR, (void*)&enabled, sizeof(enabled));
}

int zts_get_reuse_addr(int fd)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	int err;
	int optval = 0;
	zts_socklen_t optlen = sizeof(optval);
	if ((err = zts_getsockopt(fd, ZTS_SOL_SOCKET, ZTS_SO_REUSEADDR, (void*)&optval, &optlen)) < 0) {
		return err;
	}
	return optval != 0;
}

int zts_set_recv_timeout(int fd, int seconds, int microseconds)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (seconds < 0 || microseconds < 0) {
		return ZTS_ERR_ARG;
	}
	struct timeval tv;
	tv.tv_sec = seconds;
	tv.tv_usec = microseconds;
	return zts_setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (void*)&tv, sizeof(tv));
}

int zts_get_recv_timeout(int fd)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	struct timeval tv;
	zts_socklen_t optlen = sizeof(tv);
	int err;
	if ((err = zts_getsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (void*)&tv, &optlen)) < 0) {
		return err;
	}
	return tv.tv_sec;   // TODO microseconds
}

int zts_set_send_timeout(int fd, int seconds, int microseconds)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (seconds < 0 || microseconds < 0) {
		return ZTS_ERR_ARG;
	}
	struct timeval tv;
	tv.tv_sec = seconds;
	tv.tv_usec = microseconds;
	return zts_setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (void*)&tv, sizeof(tv));
}

int zts_get_send_timeout(int fd)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	struct zts_timeval tv;
	zts_socklen_t optlen = sizeof(tv);
	int err;
	if ((err = zts_getsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (void*)&tv, &optlen)) < 0) {
		return err;
	}
	return tv.tv_sec;   // TODO microseconds
}

int zts_set_send_buf_size(int fd, int size)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (size < 0) {
		return ZTS_ERR_ARG;
	}
	return zts_setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void*)&size, sizeof(int));
}

int zts_get_send_buf_size(int fd)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	int err, optval = 0;
	zts_socklen_t optlen = sizeof(optval);
	if ((err = zts_getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&optval, &optlen)) < 0) {
		return err;
	}
	return optval;
}

int zts_set_recv_buf_size(int fd, int size)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (size < 0) {
		return ZTS_ERR_ARG;
	}
	return zts_setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void*)&size, sizeof(int));
}

int zts_get_recv_buf_size(int fd)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	int err, optval = 0;
	zts_socklen_t optlen = sizeof(optval);
	if ((err = zts_getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char*)&optval, &optlen)) < 0) {
		return err;
	}
	return optval;
}

int zts_set_ttl(int fd, int ttl)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (ttl < 0 || ttl > 255) {
		return ZTS_ERR_ARG;
	}
	return zts_setsockopt(fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));
}

int zts_get_ttl(int fd)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	int err, ttl = 0;
	zts_socklen_t optlen = sizeof(ttl);
	if ((err = zts_getsockopt(fd, IPPROTO_IP, IP_TTL, &ttl, &optlen)) < 0) {
		return err;
	}
	return ttl;
}

int zts_set_blocking(int fd, int enabled)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (enabled != 0 && enabled != 1) {
		return ZTS_ERR_ARG;
	}
	int flags = zts_fcntl(fd, ZTS_F_GETFL, 0);
	if (! enabled) {
		return zts_fcntl(fd, ZTS_F_SETFL, flags | ZTS_O_NONBLOCK);
	}
	else {
		// Default
		return zts_fcntl(fd, ZTS_F_SETFL, flags & (~ZTS_O_NONBLOCK));
	}
}

int zts_get_blocking(int fd)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	int flags = zts_fcntl(fd, ZTS_F_GETFL, 0);
	if (flags < 0) {
		return flags;
	}
	return ! (flags & ZTS_O_NONBLOCK);
}

int zts_set_keepalive(int fd, int enabled)
{
	//
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (enabled != 0 && enabled != 1) {
		return ZTS_ERR_ARG;
	}
	int keepalive = enabled;
	return zts_setsockopt(fd, ZTS_SOL_SOCKET, ZTS_SO_KEEPALIVE, &keepalive, sizeof(keepalive));
}

int zts_get_keepalive(int fd)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	int err;
	int optval = 0;
	zts_socklen_t optlen = sizeof(optval);
	if ((err = zts_getsockopt(fd, ZTS_SOL_SOCKET, ZTS_SO_KEEPALIVE, (void*)&optval, &optlen)) < 0) {
		return err;
	}
	return optval != 0;
}

//----------------------------------------------------------------------------//
// Statistics                                                                 //
//----------------------------------------------------------------------------//

#ifdef ZTS_ENABLE_STATS

extern struct stats_ lwip_stats;

int zts_get_all_stats(struct zts_stats* statsDest)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	#if LWIP_STATS
	if (! statsDest) {
		return ZTS_ERR_ARG;
	}
	memset(statsDest, 0, sizeof(struct zts_stats));
	// Copy lwIP stats
	memcpy(&(statsDest->link), &(lwip_stats.link), sizeof(struct stats_proto));
	memcpy(&(statsDest->etharp), &(lwip_stats.etharp), sizeof(struct stats_proto));
	memcpy(&(statsDest->ip_frag), &(lwip_stats.ip_frag), sizeof(struct stats_proto));
	memcpy(&(statsDest->ip), &(lwip_stats.ip), sizeof(struct stats_proto));
	memcpy(&(statsDest->icmp), &(lwip_stats.icmp), sizeof(struct stats_proto));
	// memcpy(&(statsDest->igmp), &(lwip_stats.igmp), sizeof(struct stats_igmp));
	memcpy(&(statsDest->udp), &(lwip_stats.udp), sizeof(struct stats_proto));
	memcpy(&(statsDest->tcp), &(lwip_stats.tcp), sizeof(struct stats_proto));
	// mem omitted
	// memp omitted
	memcpy(&(statsDest->sys), &(lwip_stats.sys), sizeof(struct stats_sys));
	memcpy(&(statsDest->ip6), &(lwip_stats.ip6), sizeof(struct stats_proto));
	memcpy(&(statsDest->icmp6), &(lwip_stats.icmp6), sizeof(struct stats_proto));
	memcpy(&(statsDest->ip6_frag), &(lwip_stats.ip6_frag), sizeof(struct stats_proto));
	memcpy(&(statsDest->mld6), &(lwip_stats.mld6), sizeof(struct stats_igmp));
	memcpy(&(statsDest->nd6), &(lwip_stats.nd6), sizeof(struct stats_proto));
	memcpy(&(statsDest->ip_frag), &(lwip_stats.ip_frag), sizeof(struct stats_proto));
	// mib2 omitted
	// Copy ZT stats
	// ...
	return ZTS_ERR_OK;
	#else
	return ZTS_ERR_NO_RESULT;
	#endif
}

int zts_get_protocol_stats(int protocolType, void* protoStatsDest)
{
	if (! (_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	#if LWIP_STATS
	if (! protoStatsDest) {
		return ZTS_ERR_ARG;
	}
	memset(protoStatsDest, 0, sizeof(struct stats_proto));
	switch (protocolType) {
		case ZTS_STATS_PROTOCOL_LINK:
			memcpy(protoStatsDest, &(lwip_stats.link), sizeof(struct stats_proto));
			break;
		case ZTS_STATS_PROTOCOL_ETHARP:
			memcpy(protoStatsDest, &(lwip_stats.etharp), sizeof(struct stats_proto));
			break;
		case ZTS_STATS_PROTOCOL_IP:
			memcpy(protoStatsDest, &(lwip_stats.ip), sizeof(struct stats_proto));
			break;
		case ZTS_STATS_PROTOCOL_UDP:
			memcpy(protoStatsDest, &(lwip_stats.udp), sizeof(struct stats_proto));
			break;
		case ZTS_STATS_PROTOCOL_TCP:
			memcpy(protoStatsDest, &(lwip_stats.tcp), sizeof(struct stats_proto));
			break;
		case ZTS_STATS_PROTOCOL_ICMP:
			memcpy(protoStatsDest, &(lwip_stats.icmp), sizeof(struct stats_proto));
			break;
		case ZTS_STATS_PROTOCOL_IP_FRAG:
			memcpy(protoStatsDest, &(lwip_stats.ip_frag), sizeof(struct stats_proto));
			break;
		case ZTS_STATS_PROTOCOL_IP6:
			memcpy(protoStatsDest, &(lwip_stats.ip6), sizeof(struct stats_proto));
			break;
		case ZTS_STATS_PROTOCOL_ICMP6:
			memcpy(protoStatsDest, &(lwip_stats.icmp6), sizeof(struct stats_proto));
			break;
		case ZTS_STATS_PROTOCOL_IP6_FRAG:
			memcpy(protoStatsDest, &(lwip_stats.ip6_frag), sizeof(struct stats_proto));
			break;
		default: return ZTS_ERR_ARG;
	}
	return ZTS_ERR_OK;
	#else
	return ZTS_ERR_NO_RESULT;
	#endif
}

#endif   // ZTS_ENABLE_STATS

#ifdef __cplusplus
}
#endif

}   // namespace ZeroTier
