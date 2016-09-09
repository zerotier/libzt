/*
 * ZeroTier One - Network Virtualization Everywhere
 * Copyright (C) 2011-2015  ZeroTier, Inc.
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
 * ZeroTier may be used and distributed under the terms of the GPLv3, which
 * are available at: http://www.gnu.org/licenses/gpl-3.0.html
 *
 * If you would like to embed ZeroTier into a commercial application or
 * redistribute it in a modified binary form, please contact ZeroTier Networks
 * LLC. Start here: http://www.zerotier.com/
 */

#include "SDK.h"
#include "SDK_XcodeWrapper.hpp"
#include "SDK_Signatures.h"

#define INTERCEPT_ENABLED   111
#define INTERCEPT_DISABLED  222

// ZEROTIER CONTROLS
// Starts a ZeroTier service at the specified path
// This will only support SOCKS5 Proxy
extern "C" void zt_start_service(const char *path, const char *nwid) {
    //zts_start_service(path);
    init_service(INTERCEPT_DISABLED, path);
}
// Starts a ZeroTier service at the specified path and initializes the RPC mechanism
// This will allow direct API calls
extern "C" void zt_start_service_and_rpc(const char *path, const char *nwid) {
    init_service_and_rpc(INTERCEPT_DISABLED, path, nwid);
}
//
extern "C" void zt_stop_service() {
    zts_stop_service();
}
//
extern "C" bool zt_service_is_running() {
    return zts_service_is_running();
}
// Joins a ZeroTier virtual network
extern "C" void zt_join_network(const char *nwid) {
    zts_join_network(nwid);
}
// Leaves a ZeroTier virtual network
extern "C" void zt_leave_network(const char *nwid) {
    zts_leave_network(nwid);
}
// Returns a list of addresses associated with this device on the given network
extern "C" void zt_get_ipv4_address(const char *nwid, char *addrstr) {
    zts_get_ipv4_address(nwid, addrstr);
}
// Returns a list of addresses associated with this device on the given network
extern "C" void zt_get_ipv6_address(const char *nwid, char *addrstr) {
    zts_get_ipv6_address(nwid, addrstr);
}


// PROXY SERVER CONTROLS
//
extern "C" void zt_start_proxy_server(const char *homepath, const char *nwid, struct sockaddr_storage *addr) {
    zts_start_proxy_server(homepath, nwid, addr);
}
//
extern "C" void zt_stop_proxy_server(const char *nwid) {
    zts_stop_proxy_server(nwid);
}
//
extern "C" void zt_proxy_running(const char *homepath, const char *nwid, struct sockaddr_storage *addr) {
    zts_start_proxy_server(homepath, nwid, addr);
}
//
extern "C" void zt_get_proxy_server_address(const char *nwid, struct sockaddr_storage *addr) {
    zts_get_proxy_server_address(nwid, addr);
}
// Explicit ZT API wrappers
#if !defined(__IOS__)
// This isn't available for iOS since function interposition isn't as reliable
extern "C" void zt_init_rpc(const char *path, const char *nwid) {
    zts_init_rpc(path, nwid);
}
#endif


// SOCKET API
extern "C" int zt_socket(SOCKET_SIG) {
    return zts_socket(socket_family, socket_type, protocol);
}
extern "C" int zt_connect(CONNECT_SIG) {
    return zts_connect(fd, addr, addrlen);
}
extern "C" int zt_bind(BIND_SIG){
    return zts_bind(fd, addr, addrlen);
}
extern "C" int zt_accept(ACCEPT_SIG) {
    return zts_accept(fd, addr, addrlen);
}
extern "C" int zt_listen(LISTEN_SIG) {
    return zts_listen(fd, backlog);
}
extern "C" int zt_setsockopt(SETSOCKOPT_SIG) {
    return zts_setsockopt(fd, level, optname, optval, optlen);
}
extern "C" int zt_getsockopt(GETSOCKOPT_SIG) {
    return zts_getsockopt(fd, level, optname, optval, optlen);
}
extern "C" int zt_close(CLOSE_SIG) {
    return zts_close(fd);
}
extern "C" int zt_getsockname(GETSOCKNAME_SIG) {
    return zts_getsockname(fd, addr, addrlen);
}
extern "C" int zt_getpeername(GETPEERNAME_SIG) {
    return zts_getpeername(fd, addr, addrlen);
}
extern "C" int zt_fcntl(FCNTL_SIG) {
    return zts_fcntl(fd, cmd, flags);
}
extern "C" ssize_t zt_recvfrom(RECVFROM_SIG) {
    return zts_recvfrom(fd, buf, len, flags, addr, addrlen);
}
extern "C" ssize_t zt_sendto(SENDTO_SIG) {
    return zts_sendto(fd, buf, len, flags, addr, addrlen);
}