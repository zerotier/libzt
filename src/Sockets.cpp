/*
 * Copyright (c)2013-2021 ZeroTier, Inc.
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file in the project's root directory.
 *
 * Change Date: 2026-01-01
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

#include "Events.hpp"
#include "ZeroTierSockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

int zts_errno;

namespace ZeroTier {

#ifdef __cplusplus
extern "C" {
#endif

int zts_bsd_socket(const int socket_family, const int socket_type, const int protocol)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    return lwip_socket(socket_family, socket_type, protocol);
}

int zts_bsd_connect(int fd, const struct zts_sockaddr* addr, zts_socklen_t addrlen)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    if (! addr) {
        return ZTS_ERR_ARG;
    }
    if (addrlen > (zts_socklen_t)sizeof(struct zts_sockaddr_storage)
        || addrlen < (zts_socklen_t)sizeof(struct zts_sockaddr_in)) {
        return ZTS_ERR_ARG;
    }
    return lwip_connect(fd, (sockaddr*)addr, addrlen);
}

int zts_bsd_bind(int fd, const struct zts_sockaddr* addr, zts_socklen_t addrlen)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    if (! addr) {
        return ZTS_ERR_ARG;
    }
    if (addrlen > (int)sizeof(struct zts_sockaddr_storage) || addrlen < (int)sizeof(struct zts_sockaddr_in)) {
        return ZTS_ERR_ARG;
    }
    return lwip_bind(fd, (sockaddr*)addr, addrlen);
}

int zts_bsd_listen(int fd, int backlog)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    return lwip_listen(fd, backlog);
}

int zts_bsd_accept(int fd, struct zts_sockaddr* addr, zts_socklen_t* addrlen)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    return lwip_accept(fd, (sockaddr*)addr, (socklen_t*)addrlen);
}

int zts_bsd_setsockopt(int fd, int level, int optname, const void* optval, zts_socklen_t optlen)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    return lwip_setsockopt(fd, level, optname, optval, optlen);
}

int zts_bsd_getsockopt(int fd, int level, int optname, void* optval, zts_socklen_t* optlen)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    return lwip_getsockopt(fd, level, optname, optval, (socklen_t*)optlen);
}

int zts_bsd_getsockname(int fd, struct zts_sockaddr* addr, zts_socklen_t* addrlen)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    if (! addr) {
        return ZTS_ERR_ARG;
    }
    if (*addrlen > (int)sizeof(struct zts_sockaddr_storage) || *addrlen < (int)sizeof(struct zts_sockaddr_in)) {
        return ZTS_ERR_ARG;
    }
    return lwip_getsockname(fd, (sockaddr*)addr, (socklen_t*)addrlen);
}

int zts_bsd_getpeername(int fd, struct zts_sockaddr* addr, zts_socklen_t* addrlen)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    if (! addr) {
        return ZTS_ERR_ARG;
    }
    if (*addrlen > (int)sizeof(struct zts_sockaddr_storage) || *addrlen < (int)sizeof(struct zts_sockaddr_in)) {
        return ZTS_ERR_ARG;
    }
    return lwip_getpeername(fd, (sockaddr*)addr, (socklen_t*)addrlen);
}

int zts_bsd_close(int fd)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    return lwip_close(fd);
}

int zts_bsd_select(
    int nfds,
    zts_fd_set* readfds,
    zts_fd_set* writefds,
    zts_fd_set* exceptfds,
    struct zts_timeval* timeout)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    return lwip_select(nfds, (fd_set*)readfds, (fd_set*)writefds, (fd_set*)exceptfds, (timeval*)timeout);
}

int zts_bsd_fcntl(int fd, int cmd, int flags)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    return lwip_fcntl(fd, cmd, flags);
}

int zts_bsd_poll(struct zts_pollfd* fds, nfds_t nfds, int timeout)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    return lwip_poll((pollfd*)fds, nfds, timeout);
}

int zts_bsd_ioctl(int fd, unsigned long request, void* argp)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    if (! argp) {
        return ZTS_ERR_ARG;
    }
    return lwip_ioctl(fd, request, argp);
}

ssize_t zts_bsd_send(int fd, const void* buf, size_t len, int flags)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    if (! buf) {
        return ZTS_ERR_ARG;
    }
    return lwip_send(fd, buf, len, flags);
}

ssize_t
zts_bsd_sendto(int fd, const void* buf, size_t len, int flags, const struct zts_sockaddr* addr, zts_socklen_t addrlen)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    if (! addr || ! buf) {
        return ZTS_ERR_ARG;
    }
    if (addrlen > (int)sizeof(struct zts_sockaddr_storage) || addrlen < (int)sizeof(struct zts_sockaddr_in)) {
        return ZTS_ERR_ARG;
    }
    return lwip_sendto(fd, buf, len, flags, (sockaddr*)addr, addrlen);
}

ssize_t zts_bsd_sendmsg(int fd, const struct zts_msghdr* msg, int flags)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    return lwip_sendmsg(fd, (const struct msghdr*)msg, flags);
}

ssize_t zts_bsd_recv(int fd, void* buf, size_t len, int flags)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    if (! buf) {
        return ZTS_ERR_ARG;
    }
    return lwip_recv(fd, buf, len, flags);
}

ssize_t zts_bsd_recvfrom(int fd, void* buf, size_t len, int flags, struct zts_sockaddr* addr, zts_socklen_t* addrlen)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    if (! buf) {
        return ZTS_ERR_ARG;
    }
    return lwip_recvfrom(fd, buf, len, flags, (sockaddr*)addr, (socklen_t*)addrlen);
}

ssize_t zts_bsd_recvmsg(int fd, struct zts_msghdr* msg, int flags)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    if (! msg) {
        return ZTS_ERR_ARG;
    }
    return lwip_recvmsg(fd, (struct msghdr*)msg, flags);
}

ssize_t zts_bsd_read(int fd, void* buf, size_t len)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    if (! buf) {
        return ZTS_ERR_ARG;
    }
    return lwip_read(fd, buf, len);
}

ssize_t zts_bsd_readv(int fd, const struct zts_iovec* iov, int iovcnt)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    return lwip_readv(fd, (iovec*)iov, iovcnt);
}

ssize_t zts_bsd_write(int fd, const void* buf, size_t len)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    if (! buf) {
        return ZTS_ERR_ARG;
    }
    return lwip_write(fd, buf, len);
}

ssize_t zts_bsd_writev(int fd, const struct zts_iovec* iov, int iovcnt)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    return lwip_writev(fd, (iovec*)iov, iovcnt);
}

int zts_bsd_shutdown(int fd, int how)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    return lwip_shutdown(fd, how);
}

struct zts_hostent* zts_bsd_gethostbyname(const char* name)
{
    if (! transport_ok()) {
        return NULL;
    }
    if (! name) {
        return NULL;
    }
    return (struct zts_hostent*)lwip_gethostbyname(name);
}

int zts_dns_set_server(uint8_t index, const zts_ip_addr* addr)
{
    if (! transport_ok()) {
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
    if (! transport_ok()) {
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

int zts_util_ipstr_to_saddr(
    const char* src_ipstr,
    unsigned short port,
    struct zts_sockaddr* dest_addr,
    zts_socklen_t* addrlen)
{
    int family = zts_util_get_ip_family(src_ipstr);

    if (family == ZTS_AF_INET) {
        struct zts_sockaddr_in* in4 = (struct zts_sockaddr_in*)dest_addr;
        in4->sin_port = htons(port);
        in4->sin_family = family;
#if defined(_WIN32)
        zts_inet_pton(family, src_ipstr, &(in4->sin_addr.S_addr));
#else
        zts_inet_pton(family, src_ipstr, &(in4->sin_addr.s_addr));
#endif
        *addrlen = sizeof(struct zts_sockaddr_in);
        return ZTS_ERR_OK;
    }
    int any = 0;
    if (family == ZTS_AF_INET6) {
        struct zts_sockaddr_in6* in6 = (struct zts_sockaddr_in6*)dest_addr;
        in6->sin6_port = htons(port);
        in6->sin6_family = family;
        // Handle the unspecified address
        any = ((strlen(src_ipstr) >= 2) && ! strncmp(src_ipstr, "::", 2))
                      || ((strlen(src_ipstr) >= 15) && ! strncmp(src_ipstr, "0:0:0:0:0:0:0:0", 15))
                  ? 1
                  : 0;
        if (! any) {
            zts_inet_pton(family, src_ipstr, &(in6->sin6_addr));
        }
        else {
            memset((void*)&(in6->sin6_addr), 0, sizeof(zts_in6_addr));
        }
        *addrlen = sizeof(struct zts_sockaddr_in6);
        return ZTS_ERR_OK;
    }
    return ZTS_ERR_ARG;
}

int zts_socket(int family, int type, int protocol)
{
    return zts_bsd_socket(family, type, protocol);
}

int zts_connect(int fd, const char* ipstr, unsigned short port, int timeout_ms)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
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

    // Convert to standard address structure

    addrlen = sizeof(ss);
    zts_util_ipstr_to_saddr(ipstr, port, (struct zts_sockaddr*)&ss, &addrlen);
    sa = (struct zts_sockaddr*)&ss;

    if (addrlen > 0 && sa != NULL) {
        if (zts_get_blocking(fd)) {
            do {
                err = zts_bsd_connect(fd, sa, addrlen);
                zts_util_delay(connect_delay);
                n_tries--;
            } while ((err < 0) && (zts_errno != 0) && (n_tries > 0));
        }
        return err;
    }
    return ZTS_ERR_ARG;
}

int zts_bind(int fd, const char* ipstr, unsigned short port)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    zts_socklen_t addrlen = 0;
    struct zts_sockaddr_storage ss = { 0, 0, { 0 }, { 0 }, { 0 } };
    struct zts_sockaddr* sa = NULL;

    addrlen = sizeof(ss);
    int err = ZTS_ERR_OK;
    if ((err = zts_util_ipstr_to_saddr(ipstr, port, (struct zts_sockaddr*)&ss, &addrlen)) != ZTS_ERR_OK) {
        return err;
    }
    sa = (struct zts_sockaddr*)&ss;
    return zts_bsd_bind(fd, sa, addrlen);
}

int zts_listen(int fd, int backlog)
{
    return zts_bsd_listen(fd, backlog);
}

int zts_accept(int fd, char* remote_addr, int len, unsigned short* port)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    if (len != ZTS_INET6_ADDRSTRLEN) {
        return ZTS_ERR_ARG;
    }
    zts_sockaddr_storage ss;
    zts_socklen_t addrlen = sizeof(ss);

    int acc_fd = zts_bsd_accept(fd, (zts_sockaddr*)&ss, (zts_socklen_t*)&addrlen);
    int err = ZTS_ERR_OK;
    if ((err = zts_util_ntop((struct zts_sockaddr*)&ss, addrlen, remote_addr, len, port)) < ZTS_ERR_OK) {
        return err;
    }
    return acc_fd;
}

ssize_t zts_send(int fd, const void* buf, size_t len, int flags)
{
    return zts_bsd_send(fd, buf, len, flags);
}

ssize_t zts_recv(int fd, void* buf, size_t len, int flags)
{
    return zts_bsd_recv(fd, buf, len, flags);
}

ssize_t zts_read(int fd, void* buf, size_t len)
{
    return zts_bsd_read(fd, buf, len);
}

ssize_t zts_write(int fd, const void* buf, size_t len)
{
    return zts_bsd_write(fd, buf, len);
}

int zts_shutdown_rd(int fd)
{
    return zts_bsd_shutdown(fd, ZTS_SHUT_RD);
}

int zts_shutdown_wr(int fd)
{
    return zts_bsd_shutdown(fd, ZTS_SHUT_WR);
}

int zts_shutdown_rdwr(int fd)
{
    return zts_bsd_shutdown(fd, ZTS_SHUT_RDWR);
}

int zts_close(int fd)
{
    return zts_bsd_close(fd);
}

int zts_getpeername(int fd, char* remote_addr_str, int len, unsigned short* port)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    if (len != ZTS_INET6_ADDRSTRLEN) {
        return ZTS_ERR_ARG;
    }
    struct zts_sockaddr_storage ss;
    struct zts_sockaddr* sa = (struct zts_sockaddr*)&ss;
    int err = ZTS_ERR_OK;
    zts_socklen_t addrlen = sizeof(ss);
    if ((err = zts_bsd_getpeername(fd, sa, &addrlen)) < 0) {
        return err;
    }
    return zts_util_ntop(sa, addrlen, remote_addr_str, len, port);
}

int zts_getsockname(int fd, char* local_addr_str, int len, unsigned short* port)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    if (len != ZTS_INET6_ADDRSTRLEN) {
        return ZTS_ERR_ARG;
    }
    struct zts_sockaddr_storage ss;
    struct zts_sockaddr* sa = (struct zts_sockaddr*)&ss;
    int err = ZTS_ERR_OK;
    zts_socklen_t addrlen = sizeof(ss);
    if ((err = zts_bsd_getsockname(fd, sa, &addrlen)) < 0) {
        return err;
    }
    return zts_util_ntop(sa, addrlen, local_addr_str, len, port);
}

int zts_tcp_client(const char* remote_ipstr, unsigned short remote_port)
{
    int fd, family = zts_util_get_ip_family(remote_ipstr);
    if ((fd = zts_bsd_socket(family, ZTS_SOCK_STREAM, 0)) < 0) {
        return fd;   // Failed to create socket
    }
    int timeout = 0;
    if ((fd = zts_connect(fd, remote_ipstr, remote_port, timeout)) < 0) {
        zts_bsd_close(fd);
        return fd;   // Failed to connect
    }
    return fd;
}

int zts_tcp_server(
    const char* local_ipstr,
    unsigned short local_port,
    char* remote_ipstr,
    int len,
    unsigned short* remote_port)
{
    int listen_fd, family = zts_util_get_ip_family(local_ipstr);
    if ((listen_fd = zts_bsd_socket(family, ZTS_SOCK_STREAM, 0)) < 0) {
        return listen_fd;   // Failed to create socket
    }
    if ((listen_fd = zts_bind(listen_fd, local_ipstr, local_port)) < 0) {
        return listen_fd;   // Failed to bind
    }
    int backlog = 0;
    if ((listen_fd = zts_bsd_listen(listen_fd, backlog)) < 0) {
        return listen_fd;   // Failed to listen
    }
    int acc_fd = 0;
    if ((acc_fd = zts_accept(listen_fd, remote_ipstr, len, remote_port)) < 0) {
        return acc_fd;   // Failed to accept
    }
    zts_bsd_close(listen_fd);
    return acc_fd;
}

int zts_udp_server(const char* local_ipstr, unsigned short local_port)
{
    int fd, family = zts_util_get_ip_family(local_ipstr);
    if ((fd = zts_bsd_socket(family, ZTS_SOCK_DGRAM, 0)) < 0) {
        return fd;   // Failed to create socket
    }
    if ((fd = zts_bind(fd, local_ipstr, local_port)) < 0) {
        zts_bsd_close(fd);
        return fd;   // Failed to connect
    }
    return fd;
}

int zts_udp_client(const char* remote_ipstr)
{
    int fd, family = zts_util_get_ip_family(remote_ipstr);
    if ((fd = zts_bsd_socket(family, ZTS_SOCK_DGRAM, 0)) < 0) {
        return fd;   // Failed to create socket
    }
    return fd;
}

int zts_get_last_socket_error(int fd)
{
    int optval = 0;
    zts_socklen_t optlen = sizeof(optval);
    int err = ZTS_ERR_OK;
    if ((err = zts_bsd_getsockopt(fd, ZTS_SOL_SOCKET, ZTS_SO_ERROR, &optval, &optlen)) < 0) {
        return err;
    }
    return optval;
}

size_t zts_get_data_available(int fd)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    int err = ZTS_ERR_OK;
    size_t bytes_available = 0;
    if ((err = zts_bsd_ioctl(fd, ZTS_FIONREAD, &bytes_available)) < 0) {
        return err;
    }
    return bytes_available;
}

int zts_set_no_delay(int fd, int enabled)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    if (enabled != 0 && enabled != 1) {
        return ZTS_ERR_ARG;
    }
    return zts_bsd_setsockopt(fd, ZTS_IPPROTO_TCP, ZTS_TCP_NODELAY, (void*)&enabled, sizeof(int));
}

int zts_get_no_delay(int fd)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    int err, optval = 0;
    zts_socklen_t len = sizeof(optval);
    if ((err = zts_bsd_getsockopt(fd, ZTS_IPPROTO_TCP, ZTS_TCP_NODELAY, (void*)&optval, &len)) < 0) {
        return err;
    }
    return optval != 0;
}

int zts_set_linger(int fd, int enabled, int value)
{
    if (! transport_ok()) {
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
    return zts_bsd_setsockopt(fd, ZTS_SOL_SOCKET, ZTS_SO_LINGER, (void*)&linger, sizeof(linger));
}

int zts_get_linger_enabled(int fd)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    struct zts_linger linger;
    zts_socklen_t len = sizeof(linger);
    int err;
    if ((err = zts_bsd_getsockopt(fd, ZTS_SOL_SOCKET, ZTS_SO_LINGER, (void*)&linger, &len)) < 0) {
        return err;
    }
    return linger.l_onoff;
}

int zts_get_linger_value(int fd)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    struct zts_linger linger;
    zts_socklen_t len = sizeof(linger);
    int err;
    if ((err = zts_bsd_getsockopt(fd, ZTS_SOL_SOCKET, ZTS_SO_LINGER, (void*)&linger, &len)) < 0) {
        return err;
    }
    return linger.l_linger;
}

int zts_get_pending_data_size(int fd)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    int bytes_available = 0;
    int err = ZTS_ERR_OK;
    if ((err = zts_bsd_ioctl(fd, ZTS_FIONREAD, &bytes_available)) < 0) {
        return err;
    }
    return bytes_available;
}

int zts_set_reuse_addr(int fd, int enabled)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    if (enabled != 0 && enabled != 1) {
        return ZTS_ERR_ARG;
    }
    return zts_bsd_setsockopt(fd, ZTS_SOL_SOCKET, ZTS_SO_REUSEADDR, (void*)&enabled, sizeof(enabled));
}

int zts_get_reuse_addr(int fd)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    int err, optval = 0;
    zts_socklen_t optlen = sizeof(optval);
    if ((err = zts_bsd_getsockopt(fd, ZTS_SOL_SOCKET, ZTS_SO_REUSEADDR, (void*)&optval, &optlen)) < 0) {
        return err;
    }
    return optval != 0;
}

int zts_set_recv_timeout(int fd, int seconds, int microseconds)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    if (seconds < 0 || microseconds < 0) {
        return ZTS_ERR_ARG;
    }
    struct timeval tv;
    tv.tv_sec = seconds;
    tv.tv_usec = microseconds;
    return zts_bsd_setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (void*)&tv, sizeof(tv));
}

int zts_get_recv_timeout(int fd)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    struct timeval tv;
    zts_socklen_t optlen = sizeof(tv);
    int err;
    if ((err = zts_bsd_getsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (void*)&tv, &optlen)) < 0) {
        return err;
    }
    return tv.tv_sec;   // TODO microseconds
}

int zts_set_send_timeout(int fd, int seconds, int microseconds)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    if (seconds < 0 || microseconds < 0) {
        return ZTS_ERR_ARG;
    }
    struct timeval tv;
    tv.tv_sec = seconds;
    tv.tv_usec = microseconds;
    return zts_bsd_setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (void*)&tv, sizeof(tv));
}

int zts_get_send_timeout(int fd)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    struct zts_timeval tv;
    zts_socklen_t optlen = sizeof(tv);
    int err;
    if ((err = zts_bsd_getsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (void*)&tv, &optlen)) < 0) {
        return err;
    }
    return tv.tv_sec;   // TODO microseconds
}

int zts_set_send_buf_size(int fd, int size)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    if (size < 0) {
        return ZTS_ERR_ARG;
    }
    return zts_bsd_setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void*)&size, sizeof(int));
}

int zts_get_send_buf_size(int fd)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    int err, optval = 0;
    zts_socklen_t optlen = sizeof(optval);
    if ((err = zts_bsd_getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&optval, &optlen)) < 0) {
        return err;
    }
    return optval;
}

int zts_set_recv_buf_size(int fd, int size)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    if (size < 0) {
        return ZTS_ERR_ARG;
    }
    return zts_bsd_setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void*)&size, sizeof(int));
}

int zts_get_recv_buf_size(int fd)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    int err, optval = 0;
    zts_socklen_t optlen = sizeof(optval);
    if ((err = zts_bsd_getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char*)&optval, &optlen)) < 0) {
        return err;
    }
    return optval;
}

int zts_set_ttl(int fd, int ttl)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    if (ttl < 0 || ttl > 255) {
        return ZTS_ERR_ARG;
    }
    return zts_bsd_setsockopt(fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));
}

int zts_get_ttl(int fd)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    int err, ttl = 0;
    zts_socklen_t optlen = sizeof(ttl);
    if ((err = zts_bsd_getsockopt(fd, IPPROTO_IP, IP_TTL, &ttl, &optlen)) < 0) {
        return err;
    }
    return ttl;
}

int zts_set_blocking(int fd, int enabled)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    if (enabled != 0 && enabled != 1) {
        return ZTS_ERR_ARG;
    }
    int flags = zts_bsd_fcntl(fd, ZTS_F_GETFL, 0);
    if (! enabled) {
        return zts_bsd_fcntl(fd, ZTS_F_SETFL, flags | ZTS_O_NONBLOCK);
    }
    else {
        // Default
        return zts_bsd_fcntl(fd, ZTS_F_SETFL, flags & (~ZTS_O_NONBLOCK));
    }
}

int zts_get_blocking(int fd)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    int flags = zts_bsd_fcntl(fd, ZTS_F_GETFL, 0);
    if (flags < 0) {
        return flags;
    }
    return ! (flags & ZTS_O_NONBLOCK);
}

int zts_set_keepalive(int fd, int enabled)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    if (enabled != 0 && enabled != 1) {
        return ZTS_ERR_ARG;
    }
    int keepalive = enabled;
    return zts_bsd_setsockopt(fd, ZTS_SOL_SOCKET, ZTS_SO_KEEPALIVE, &keepalive, sizeof(keepalive));
}

int zts_get_keepalive(int fd)
{
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
    int err, optval = 0;
    zts_socklen_t optlen = sizeof(optval);
    if ((err = zts_bsd_getsockopt(fd, ZTS_SOL_SOCKET, ZTS_SO_KEEPALIVE, (void*)&optval, &optlen)) < 0) {
        return err;
    }
    return optval != 0;
}

int zts_util_ntop(struct zts_sockaddr* addr, zts_socklen_t addrlen, char* dst_str, int len, unsigned short* port)
{
    if (! addr || addrlen < sizeof(struct zts_sockaddr_in) || addrlen > sizeof(struct zts_sockaddr_storage) || ! dst_str
        || len != ZTS_INET6_ADDRSTRLEN) {
        return ZTS_ERR_ARG;
    }
    if (addr->sa_family == ZTS_AF_INET) {
        struct zts_sockaddr_in* in4 = (struct zts_sockaddr_in*)addr;
        zts_inet_ntop(ZTS_AF_INET, &(in4->sin_addr), dst_str, len);
        *port = ntohs(in4->sin_port);
        return ZTS_ERR_OK;
    }
    if (addr->sa_family == ZTS_AF_INET6) {
        struct zts_sockaddr_in6* in6 = (struct zts_sockaddr_in6*)addr;
        zts_inet_ntop(ZTS_AF_INET6, &(in6->sin6_addr), dst_str, len);
        *port = ntohs(in6->sin6_port);
        return ZTS_ERR_OK;
    }
    return ZTS_ERR_ARG;
}

#ifdef __cplusplus
}
#endif

}   // namespace ZeroTier
