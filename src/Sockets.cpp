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
#include "lwip/def.h"
#include "lwip/inet.h"
#include "lwip/stats.h"

#include "ZeroTierSockets.h"

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
	if (!(_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	return lwip_socket(socket_family, socket_type, protocol);
}

int zts_connect(int fd, const struct zts_sockaddr *addr, zts_socklen_t addrlen)
{
	if (!(_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (!addr) {
		return ZTS_ERR_ARG;
	}
	if (addrlen > (int)sizeof(struct zts_sockaddr_storage) || addrlen < (int)sizeof(struct zts_sockaddr_in)) {
		return ZTS_ERR_ARG;
	}
	return lwip_connect(fd, (sockaddr*)addr, addrlen);
}

int zts_bind(int fd, const struct zts_sockaddr *addr, zts_socklen_t addrlen)
{
	if (!(_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (!addr) {
		return ZTS_ERR_ARG;
	}
	if (addrlen > (int)sizeof(struct zts_sockaddr_storage) || addrlen < (int)sizeof(struct zts_sockaddr_in)) {
		return ZTS_ERR_ARG;
	}
	return lwip_bind(fd, (sockaddr*)addr, addrlen);
}

int zts_listen(int fd, int backlog)
{
	if (!(_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	return lwip_listen(fd, backlog);
}

int zts_accept(int fd, struct zts_sockaddr *addr, zts_socklen_t *addrlen)
{
	if (!(_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	return lwip_accept(fd, (sockaddr*)addr, (socklen_t*)addrlen);
}

int zts_setsockopt(int fd, int level, int optname, const void *optval,zts_socklen_t optlen)
{
	if (!(_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	return lwip_setsockopt(fd, level, optname, optval, optlen);
}

int zts_getsockopt(int fd, int level, int optname, void *optval, zts_socklen_t *optlen)
{
	if (!(_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	return lwip_getsockopt(fd, level, optname, optval, (socklen_t*)optlen);
}

int zts_getsockname(int fd, struct zts_sockaddr *addr, zts_socklen_t *addrlen)
{
	if (!(_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (!addr) {
		return ZTS_ERR_ARG;
	}
	if (*addrlen > (int)sizeof(struct zts_sockaddr_storage) || *addrlen < (int)sizeof(struct zts_sockaddr_in)) {
		return ZTS_ERR_ARG;
	}
	return lwip_getsockname(fd, (sockaddr*)addr, (socklen_t*)addrlen);
}

int zts_getpeername(int fd, struct zts_sockaddr *addr, zts_socklen_t *addrlen)
{
	if (!(_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (!addr) {
		return ZTS_ERR_ARG;
	}
	if (*addrlen > (int)sizeof(struct zts_sockaddr_storage) || *addrlen < (int)sizeof(struct zts_sockaddr_in)) {
		return ZTS_ERR_ARG;
	}
	return lwip_getpeername(fd, (sockaddr*)addr, (socklen_t*)addrlen);
}

int zts_close(int fd)
{
	if (!(_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	return lwip_close(fd);
}

int zts_select(int nfds, zts_fd_set *readfds, zts_fd_set *writefds, zts_fd_set *exceptfds,
	struct zts_timeval *timeout)
{
	if (!(_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	return lwip_select(nfds, (fd_set*)readfds, (fd_set*)writefds, (fd_set*)exceptfds, (timeval*)timeout);
}

int zts_fcntl(int fd, int cmd, int flags)
{
	if (!(_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	return lwip_fcntl(fd, cmd, flags);
}

int zts_poll(struct zts_pollfd *fds, nfds_t nfds, int timeout)
{
	if (!(_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	return lwip_poll((pollfd*)fds, nfds, timeout);
}

int zts_ioctl(int fd, unsigned long request, void *argp)
{
	if (!(_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (!argp) {
		return ZTS_ERR_ARG;
	}
	return lwip_ioctl(fd, request, argp);
}

ssize_t zts_send(int fd, const void *buf, size_t len, int flags)
{
	if (!(_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (!buf) {
		return ZTS_ERR_ARG;
	}
	return lwip_send(fd, buf, len, flags);
}

ssize_t zts_sendto(int fd, const void *buf, size_t len, int flags,
	const struct zts_sockaddr *addr,zts_socklen_t addrlen)
{
	if (!(_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (!addr || !buf) {
		return ZTS_ERR_ARG;
	}
	if (addrlen > (int)sizeof(struct zts_sockaddr_storage) || addrlen < (int)sizeof(struct zts_sockaddr_in)) {
		return ZTS_ERR_ARG;
	}
	return lwip_sendto(fd, buf, len, flags, (sockaddr*)addr, addrlen);
}

ssize_t zts_sendmsg(int fd, const struct msghdr *msg, int flags)
{
	if (!(_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	return lwip_sendmsg(fd, msg, flags);
}

ssize_t zts_recv(int fd, void *buf, size_t len, int flags)
{
	if (!(_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (!buf) {
		return ZTS_ERR_ARG;
	}
	return lwip_recv(fd, buf, len, flags);
}

ssize_t zts_recvfrom(int fd, void *buf, size_t len, int flags,
	struct zts_sockaddr *addr, zts_socklen_t *addrlen)
{
	if (!(_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (!buf) {
		return ZTS_ERR_ARG;
	}
	return lwip_recvfrom(fd, buf, len, flags, (sockaddr*)addr, (socklen_t*)addrlen);
}

ssize_t zts_recvmsg(int fd, struct msghdr *msg, int flags)
{
	if (!(_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (!msg) {
		return ZTS_ERR_ARG;
	}
	return lwip_recvmsg(fd, msg, flags);
}

ssize_t zts_read(int fd, void *buf, size_t len)
{
	if (!(_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (!buf) {
		return ZTS_ERR_ARG;
	}
	return lwip_read(fd, buf, len);
}

ssize_t zts_readv(int s, const struct zts_iovec *iov, int iovcnt)
{
	if (!(_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	return lwip_readv(s, (iovec*)iov, iovcnt);
}

ssize_t zts_write(int fd, const void *buf, size_t len)
{
	if (!(_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (!buf) {
		return ZTS_ERR_ARG;
	}
	return lwip_write(fd, buf, len);
}

ssize_t zts_writev(int fd, const struct zts_iovec *iov, int iovcnt)
{
	if (!(_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	return lwip_writev(fd, (iovec*)iov, iovcnt);
}

int zts_shutdown(int fd, int how)
{
	if (!(_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	return lwip_shutdown(fd, how);
}

int zts_add_dns_nameserver(struct zts_sockaddr *addr)
{
	if (!(_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	return ZTS_ERR_SERVICE; // TODO
}

int zts_del_dns_nameserver(struct zts_sockaddr *addr)
{
	if (!(_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	return ZTS_ERR_SERVICE; // TODO
}

uint16_t zts_htons(uint16_t n)
{
	return lwip_htons(n);
}

uint32_t zts_htonl(uint32_t n)
{
	return lwip_htonl(n);
}

uint16_t zts_ntohs(uint16_t n)
{
	return lwip_htons(n);
}

uint32_t zts_ntohl(uint32_t n)
{
	return lwip_htonl(n);
}

const char *zts_inet_ntop(int af, const void *src, char *dst,zts_socklen_t size)
{
	return lwip_inet_ntop(af,src,dst,size);
}

int zts_inet_pton(int af, const char *src, void *dst)
{
	return lwip_inet_pton(af,src,dst);
}

uint32_t zts_inet_addr(const char *cp)
{
	return ipaddr_addr(cp);
}

//////////////////////////////////////////////////////////////////////////////
// Statistics                                                               //
//////////////////////////////////////////////////////////////////////////////

#ifdef ZTS_ENABLE_STATS

extern struct stats_ lwip_stats;

int zts_get_all_stats(struct zts_stats *statsDest)
{
	if (!(_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
#if LWIP_STATS
	if (!statsDest) {
		return ZTS_ERR_ARG;
	}
	memset(statsDest, 0, sizeof(struct zts_stats));
	// Copy lwIP stats
	memcpy(&(statsDest->link), &(lwip_stats.link), sizeof(struct stats_proto));
	memcpy(&(statsDest->etharp), &(lwip_stats.etharp), sizeof(struct stats_proto));
	memcpy(&(statsDest->ip_frag), &(lwip_stats.ip_frag), sizeof(struct stats_proto));
	memcpy(&(statsDest->ip), &(lwip_stats.ip), sizeof(struct stats_proto));
	memcpy(&(statsDest->icmp), &(lwip_stats.icmp), sizeof(struct stats_proto));
	//memcpy(&(statsDest->igmp), &(lwip_stats.igmp), sizeof(struct stats_igmp));
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

int zts_get_protocol_stats(int protocolType, void *protoStatsDest)
{
	if (!(_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
#if LWIP_STATS
	if (!protoStatsDest) {
		return ZTS_ERR_ARG;
	}
	memset(protoStatsDest, 0, sizeof(struct stats_proto));
	switch (protocolType)
	{
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
		default:
			return ZTS_ERR_ARG;
	}
	return ZTS_ERR_OK;
#else
	return ZTS_ERR_NO_RESULT;
#endif
}

#endif // ZTS_ENABLE_STATS

#ifdef __cplusplus
}
#endif

} // namespace ZeroTier
