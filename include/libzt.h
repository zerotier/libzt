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

#ifndef LIBZT_H
#define LIBZT_H

#include <stdlib.h>
#include <stdint.h>
#include <vector>

#if defined(__linux__)
 #include <poll.h>
#endif

#include "Debug.hpp"
#include "Defs.h"

/****************************************************************************/
/* DLL export for Windows (and other cruft)                                 */
/****************************************************************************/

#if (defined(_WIN32) || defined(_WIN64)) && !(defined(__MINGW32__) || defined(__MINGW64__))
typedef int ssize_t;
#endif

#if defined(__MING32__) || defined(__MING64__)
#ifdef ADD_EXPORTS
#define ZT_SOCKET_API __declspec(dllexport)
#else
#define ZT_SOCKET_API __declspec(dllimport)
#endif
#define ZTCALL __cdecl
#else
#define ZT_SOCKET_API
#define ZTCALL
#endif

/****************************************************************************/
/* ZeroTier Service Controls                                                */
/****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// forward declarations from ZT1Service.h
ZT_SOCKET_API int ZTCALL zts_get_id(char *devID);
ZT_SOCKET_API void ZTCALL init_network_stack();

/**
 * @brief Starts libzt
 *
 * @usage Should be called at the beginning of your application. Will blocks until all of the following conditions are met:
 * - ZeroTier core service has been initialized
 * - Cryptographic identity has been generated or loaded from directory specified by `path`
 * - Virtual network is successfully joined
 * - IP address is assigned by network controller service
 * @param path path directory where cryptographic identities and network configuration files are stored and retrieved
 *              (`identity.public`, `identity.secret`)
 * @param nwid A 16-digit hexidecimal network identifier (e.g. Earth: `8056c2e21c000001`)
 * @return 0 if successful; or 1 if failed
 */
ZT_SOCKET_API int ZTCALL zts_start(const char *path);

/**
 * @brief Starts libzt
 *
 * @usage Should be called at the beginning of your application. Will blocks until all of the following conditions are met:
 * - ZeroTier core service has been initialized
 * - Cryptographic identity has been generated or loaded from directory specified by `path`
 * - Virtual network is successfully joined
 * - IP address is assigned by network controller service
 * @param path path directory where cryptographic identities and network configuration files are stored and retrieved
 *              (`identity.public`, `identity.secret`)
 * @param nwid A 16-digit hexidecimal network identifier (e.g. Earth: `8056c2e21c000001`)
 * @return 0 if successful; or 1 if failed
 */
ZT_SOCKET_API int ZTCALL zts_startjoin(const char *path, const char *nwid);

/**
 * @brief Stops the ZeroTier core service and disconnects from all virtual networks
 *
 * @usage Called at the end of your application. This call will block until everything is shut down
 * @return
 */
ZT_SOCKET_API void ZTCALL zts_stop();

/**
 * @brief Check whether the service is running
 *
 * @usage
 * @return
 */
ZT_SOCKET_API int ZTCALL zts_running();

/**
 * @brief Joins a virtual network
 *
 * @usage Called after zts_start() or zts_startjoin()
 * @param nwid the 16-digit hexidecimal network identifier
 * @return
 */
ZT_SOCKET_API void ZTCALL zts_join(const char * nwid);

/**
 * @brief Joins a network (eventually), this will create the dir and conf file required, don't instruct the core
 * to do anything
 *
 * @usage Candidate for deletion
 * @param filepath path to the `*.conf` file named after the network
 * @param nwid
 * @return
 */
ZT_SOCKET_API void ZTCALL zts_join_soft(const char * filepath, const char * nwid);

/**
 * @brief Leaves a virtual network.
 *
 * @usage
 * @param nwid
 * @return
 */
ZT_SOCKET_API void ZTCALL zts_leave(const char * nwid);

/**
 * @brief Leave a network - Only delete the .conf file, this will prevent the service from joining upon next startup
 *
 * @usage
 * @param filepath
 * @param nwid
 * @return
 */
ZT_SOCKET_API void ZTCALL zts_leave_soft(const char * filepath, const char * nwid);

/**
 * @brief Returns path used by ZeroTier/libzt for storing identity and config files
 *
 * @usage
 * @param homePath
 * @param len
 * @return
 */
ZT_SOCKET_API void ZTCALL zts_get_homepath(char *homePath, const size_t len);

/**
 * @brief Get device ID (10-digit hex + NULL byte)
 *
 * @usage
 * @param devID
 * @return
 */
ZT_SOCKET_API int ZTCALL zts_get_id(char *devID);

/**
 * @brief Returns whether any IPv6 address has been assigned to the SockTap for this network
 *
 * @usage This is used as an indicator of readiness for service for the ZeroTier core and stack
 * @param nwid
 * @return
 */
ZT_SOCKET_API int ZTCALL zts_has_ipv4_address(const char *nwid);

/**
 * @brief Returns whether any IPv4 address has been assigned to the SockTap for this network
 *
 * @usage This is used as an indicator of readiness for service for the ZeroTier core and stack
 * @param nwid
 * @return
 */
ZT_SOCKET_API int ZTCALL zts_has_ipv6_address(const char *nwid);

/**
 * @brief Returns whether any address has been assigned to the SockTap for this network
 *
 * @usage This is used as an indicator of readiness for service for the ZeroTier core and stack
 * @param nwid
 * @return
 */
ZT_SOCKET_API int ZTCALL zts_has_address(const char *nwid);

/**
 * @brief Get IPV4 Address for this device on a given network
 *
 * @usage FIXME: Only returns first address found for given protocol and network (should be enough for now)
 * @param nwid
 * @param addrstr
 * @param addrlen
 * @return
 */
ZT_SOCKET_API void ZTCALL zts_get_ipv4_address(const char *nwid, char *addrstr, const size_t addrlen);

/**
 * @brief Get IPV6 Address for this device on a given network
 *
 * @usage FIXME: Only returns first address found for given protocol and network (should be enough for now)
 * @param nwid
 * @param addrstr
 * @param addrlen
 * @return
 */
ZT_SOCKET_API void ZTCALL zts_get_ipv6_address(const char *nwid, char *addrstr, const size_t addrlen);

/**
 * @brief Returns a 6PLANE IPv6 address given a network ID and zerotier ID
 *
 * @usage
 * @param addr
 * @param nwid
 * @param devID
 * @return
 */
ZT_SOCKET_API void ZTCALL zts_get_6plane_addr(char *addr, const char *nwid, const char *devID);

/**
 * @brief Returns an RFC 4193 IPv6 address given a network ID and zerotier ID
 *
 * @usage
 * @param addr
 * @param nwid
 * @param devID
 * @return
 */
ZT_SOCKET_API void ZTCALL zts_get_rfc4193_addr(char *addr, const char *nwid, const char *devID);

/**
 * @brief Return the number of peers on this network
 *
 * @usage
 * @return
 */
ZT_SOCKET_API unsigned long zts_get_peer_count();

/**
 * @brief Get the IP address of a peer if a direct path is available
 *
 * @usage
 * @param peer
 * @param devID
 * @return
 */
ZT_SOCKET_API int ZTCALL zts_get_peer_address(char *peer, const char *devID);

/**
 * @brief Enable HTTP control plane (traditionally used by zerotier-cli)
 *              - Allows one to control the ZeroTier core via HTTP requests
 *              FIXME: Implement
 *
 * @usage
 * @return
 */
ZT_SOCKET_API void ZTCALL zts_enable_http_control_plane();

/**
 * @brief Disable HTTP control plane (traditionally used by zerotier-cli)
 *              - Allows one to control the ZeroTier core via HTTP requests
 *              FIXME: Implement
 *
 * @usage
 * @return
 */
ZT_SOCKET_API void ZTCALL zts_disable_http_control_plane();

/****************************************************************************/
/* POSIX-like socket API                                                    */
/****************************************************************************/

/**
 * @brief Create a socket
 *
 * This function will return an integer which can be used in much the same way as a
 * typical file descriptor, however it is only valid for use with libzt library calls
 * as this is merely a facade which is associated with the internal socket representation
 * of both the network stacks and drivers.
 *
 * @usage Call this after zts_start() has succeeded
 * @param socket_family Address family (AF_INET, AF_INET6)
 * @param socket_type Type of socket (SOCK_STREAM, SOCK_DGRAM, SOCK_RAW)
 * @param protocol Protocols supported on this socket
 * @return
 */
ZT_SOCKET_API int ZTCALL zts_socket(int socket_family, int socket_type, int protocol);

/**
 * @brief Connect a socket to a remote host
 *
 * @usage Call this after zts_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param addr Remote host address to connect to
 * @param addrlen Length of address
 * @return
 */
ZT_SOCKET_API int ZTCALL zts_connect(int fd, const struct sockaddr *addr, socklen_t addrlen);

/**
 * @brief Bind a socket to a virtual interface
 *
 * @usage Call this after zts_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param addr Local interface address to bind to
 * @param addrlen Length of address
 * @return
 */
ZT_SOCKET_API int ZTCALL zts_bind(int fd, const struct sockaddr *addr, socklen_t addrlen);

/**
 * @brief Listen for incoming connections
 *
 * @usage Call this after zts_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param backlog Number of backlogged connection allowed
 * @return
 */
ZT_SOCKET_API int ZTCALL zts_listen(int fd, int backlog);

/**
 * @brief Accept an incoming connection
 *
 * @usage Call this after zts_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param addr Address of remote host for accepted connection
 * @param addrlen Length of address
 * @return
 */
ZT_SOCKET_API int ZTCALL zts_accept(int fd, struct sockaddr *addr, socklen_t *addrlen);

/**
 * @brief Accept an incoming connection
 *
 * @usage Call this after zts_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param addr Address of remote host for accepted connection
 * @param addrlen Length of address
 * @param flags
 * @return
 */
#if defined(__linux__)
	int zts_accept4(int fd, struct sockaddr *addr, socklen_t *addrlen, int flags);
#endif

/**
 * @brief Set socket options
 *
 * @usage Call this after zts_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param level Protocol level to which option name should apply
 * @param optname Option name to set
 * @param optval Source of option value to set
 * @param optlen Length of option value
 * @return
 */
ZT_SOCKET_API int ZTCALL zts_setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen);

/**
 * @brief Get socket options
 *
 * @usage Call this after zts_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param level Protocol level to which option name should apply
 * @param optname Option name to get
 * @param optval Where option value will be stored
 * @param optlen Length of value
 * @return
 */
ZT_SOCKET_API int ZTCALL zts_getsockopt(int fd, int level, int optname, void *optval, socklen_t *optlen);

/**
 * @brief Get socket name
 *
 * @usage Call this after zts_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param addr Name associated with this socket
 * @param addrlen Length of name
 * @return
 */
ZT_SOCKET_API int ZTCALL zts_getsockname(int fd, struct sockaddr *addr, socklen_t *addrlen);

/**
 * @brief Get the peer name for the remote end of a connected socket
 *
 * @usage Call this after zts_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param addr Name associated with remote end of this socket
 * @param addrlen Length of name
 * @return
 */
ZT_SOCKET_API int ZTCALL zts_getpeername(int fd, struct sockaddr *addr, socklen_t *addrlen);

/**
 * @brief Gets current hostname
 *
 * @usage Call this after zts_start() has succeeded
 * @param name
 * @param len
 * @return
 */
ZT_SOCKET_API int ZTCALL zts_gethostname(char *name, size_t len);

/**
 * @brief Sets current hostname
 *
 * @usage Call this after zts_start() has succeeded
 * @param name
 * @param len
 * @return
 */
ZT_SOCKET_API int ZTCALL zts_sethostname(const char *name, size_t len);

/**
 * @brief Return a pointer to an object with the following structure describing an internet host referenced by name
 *
 * @usage Call this after zts_start() has succeeded
 * @param name
 * @return Returns pointer to hostent structure otherwise NULL if failure
 */
ZT_SOCKET_API struct hostent *zts_gethostbyname(const char *name);

/**
 * @brief Close a socket
 *
 * @usage Call this after zts_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @return
 */
ZT_SOCKET_API int ZTCALL zts_close(int fd);

/**
 * @brief Waits for one of a set of file descriptors to become ready to perform I/O.
 *
 * @usage Call this after zts_start() has succeeded
 * @param fds
 * @param nfds
 * @param timeout
 * @return
 */
#if defined(__linux__)
int zts_poll(struct pollfd *fds, nfds_t nfds, int timeout);
#endif

/**
 * @brief Monitor multiple file descriptors, waiting until one or more of the file descriptors become "ready"
 *
 * @usage Call this after zts_start() has succeeded
 * @param nfds
 * @param readfds
 * @param writefds
 * @param exceptfds
 * @param timeout
 * @return
 */
ZT_SOCKET_API int ZTCALL zts_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

/**
 * @brief Issue file control commands on a socket
 *
 * @usage Call this after zts_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param cmd
 * @param flags
 * @return
 */
ZT_SOCKET_API int ZTCALL zts_fcntl(int fd, int cmd, int flags);

/**
 * @brief Control a device
 *
 * @usage Call this after zts_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param request
 * @param argp
 * @return
 */
ZT_SOCKET_API int ZTCALL zts_ioctl(int fd, unsigned long request, void *argp);

/**
 * @brief Send data to remote host
 *
 * @usage Call this after zts_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param buf Pointer to data buffer
 * @param len Length of data to write
 * @param flags
 * @return
 */
ZT_SOCKET_API ssize_t ZTCALL zts_send(int fd, const void *buf, size_t len, int flags);

/**
 * @brief Send data to remote host
 *
 * @usage Call this after zts_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param buf Pointer to data buffer
 * @param len Length of data to write
 * @param flags
 * @param addr Destination address
 * @param addrlen Length of destination address
 * @return
 */
ZT_SOCKET_API ssize_t ZTCALL zts_sendto(int fd, const void *buf, size_t len, int flags, const struct sockaddr *addr, socklen_t addrlen);

/**
 * @brief Send message to remote host
 *
 * @usage Call this after zts_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param msg
 * @param flags
 * @return
 */
ZT_SOCKET_API ssize_t ZTCALL zts_sendmsg(int fd, const struct msghdr *msg, int flags);

/**
 * @brief Receive data from remote host
 *
 * @usage Call this after zts_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param buf Pointer to data buffer
 * @param len Length of data buffer
 * @param flags
 * @return
 */
ZT_SOCKET_API ssize_t ZTCALL zts_recv(int fd, void *buf, size_t len, int flags);

/**
 * @brief Receive data from remote host
 *
 * @usage Call this after zts_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param buf Pointer to data buffer
 * @param len Length of data buffer
 * @param flags
 * @param addr
 * @param addrlen
 * @return
 */
ZT_SOCKET_API ssize_t ZTCALL zts_recvfrom(int fd, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t *addrlen);

/**
 * @brief Receive a message from remote host
 *
 * @usage Call this after zts_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param msg
 * @param flags
 * @return
 */
ZT_SOCKET_API ssize_t ZTCALL zts_recvmsg(int fd, struct msghdr *msg,int flags);

/**
 * @brief Read bytes from socket onto buffer
 *
 * @usage Call this after zts_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param buf Pointer to data buffer
 * @param len Length of data buffer to receive data
 * @return
 */
ZT_SOCKET_API int ZTCALL zts_read(int fd, void *buf, size_t len);

/**
 * @brief Write bytes from buffer to socket
 *
 * @usage Call this after zts_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param buf Pointer to data buffer
 * @param len Length of buffer to write
 * @return
 */
ZT_SOCKET_API int ZTCALL zts_write(int fd, const void *buf, size_t len);

/**
 * @brief Shut down some aspect of a socket (read, write, or both)
 *
 * @usage Call this after zts_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param how Which aspects of the socket should be shut down
 * @return
 */
ZT_SOCKET_API int ZTCALL zts_shutdown(int fd, int how);

/**
 * @brief Adds a DNS nameserver for the network stack to use
 *
 * @usage Call this after zts_start() has succeeded
 * @param addr Address for DNS nameserver
 * @return
 */
ZT_SOCKET_API int ZTCALL zts_add_dns_nameserver(struct sockaddr *addr);

/**
 * @brief Removes a DNS nameserver
 *
 * @usage Call this after zts_start() has succeeded
 * @param addr Address for DNS nameserver
 * @return
 */
ZT_SOCKET_API int ZTCALL zts_del_dns_nameserver(struct sockaddr *addr);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _H
