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
 * VirtualSocket management layer
 */

//#include "InetAddress.hpp"

namespace ZeroTier {
	class Mutex;
	struct InetAddress;
}

class VirtualSocket;
class VirtualTap;

extern ZeroTier::Mutex _multiplexer_lock;

VirtualSocket *get_virt_socket(int fd);
int del_virt_socket(int fd);
int add_unassigned_virt_socket(int fd, VirtualSocket *vs);
int del_unassigned_virt_socket(int fd);
int add_assigned_virt_socket(void *tap, VirtualSocket *vs, int fd);
int del_assigned_virt_socket(void *tap, VirtualSocket *vs, int fd);
//void *get_assigned_virtual_pair(int fd);

/**
 * @brief Stops all VirtualTap interfaces and associated I/O loops
 *
 * @usage For internal use only.
 * @param
 * @return
 */
void disableTaps();


/**
 * @brief Create a socket
 *
 * This function will return an integer which can be used in much the same way as a
 * typical file descriptor, however it is only valid for use with libzt library calls
 * as this is merely a facade which is associated with the internal socket representation
 * of both the network stacks and drivers.
 *
 * @usage Call this after virt_start() has succeeded
 * @param socket_family Address family (AF_INET, AF_INET6)
 * @param socket_type Type of socket (SOCK_STREAM, SOCK_DGRAM, SOCK_RAW)
 * @param protocol Protocols supported on this socket
 * @return
 */
int virt_socket(int socket_family, int socket_type, int protocol);

/**
 * @brief Connect a socket to a remote host
 *
 * @usage Call this after virt_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param addr Remote host address to connect to
 * @param addrlen Length of address
 * @return
 */
int virt_connect(int fd, const struct sockaddr *addr, socklen_t addrlen);

/**
 * @brief Bind a socket to a virtual interface
 *
 * @usage Call this after virt_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param addr Local interface address to bind to
 * @param addrlen Length of address
 * @return
 */
int virt_bind(int fd, const struct sockaddr *addr, socklen_t addrlen);

/**
 * @brief Listen for incoming connections
 *
 * @usage Call this after virt_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param backlog Number of backlogged connection allowed
 * @return
 */
int virt_listen(int fd, int backlog);

/**
 * @brief Accept an incoming connection
 *
 * @usage Call this after virt_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param addr Address of remote host for accepted connection
 * @param addrlen Length of address
 * @return
 */
int virt_accept(int fd, struct sockaddr *addr, socklen_t *addrlen);

/**
 * @brief Accept an incoming connection
 *
 * @usage Call this after virt_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param addr Address of remote host for accepted connection
 * @param addrlen Length of address
 * @param flags
 * @return
 */
#if defined(__linux__)
	int virt_accept4(int fd, struct sockaddr *addr, socklen_t *addrlen, int flags);
#endif

/**
 * @brief Set socket options
 *
 * @usage Call this after virt_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param level Protocol level to which option name should apply
 * @param optname Option name to set
 * @param optval Source of option value to set
 * @param optlen Length of option value
 * @return
 */
int virt_setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen);

/**
 * @brief Get socket options
 *
 * @usage Call this after virt_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param level Protocol level to which option name should apply
 * @param optname Option name to get
 * @param optval Where option value will be stored
 * @param optlen Length of value
 * @return
 */
int virt_getsockopt(int fd, int level, int optname, void *optval, socklen_t *optlen);

/**
 * @brief Get socket name
 *
 * @usage Call this after virt_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param addr Name associated with this socket
 * @param addrlen Length of name
 * @return
 */
int virt_getsockname(int fd, struct sockaddr *addr, socklen_t *addrlen);

/**
 * @brief Get the peer name for the remote end of a connected socket
 *
 * @usage Call this after virt_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param addr Name associated with remote end of this socket
 * @param addrlen Length of name
 * @return
 */
int virt_getpeername(int fd, struct sockaddr *addr, socklen_t *addrlen);

/**
 * @brief Gets current hostname
 *
 * @usage Call this after virt_start() has succeeded
 * @param name
 * @param len
 * @return
 */
int virt_gethostname(char *name, size_t len);

/**
 * @brief Sets current hostname
 *
 * @usage Call this after virt_start() has succeeded
 * @param name
 * @param len
 * @return
 */
int virt_sethostname(const char *name, size_t len);

/**
 * @brief Return a pointer to an object with the following structure describing an internet host referenced by name
 *
 * @usage Call this after virt_start() has succeeded
 * @param name
 * @return Returns pointer to hostent structure otherwise NULL if failure
 */
struct hostent *virt_gethostbyname(const char *name);

/**
 * @brief Close a socket
 *
 * @usage Call this after virt_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @return
 */
int virt_close(int fd);

/**
 * @brief Waits for one of a set of file descriptors to become ready to perform I/O.
 *
 * @usage Call this after virt_start() has succeeded
 * @param fds
 * @param nfds
 * @param timeout
 * @return
 */
/*
#ifdef __linux__
int virt_poll(struct pollfd *fds, nfds_t nfds, int timeout);
#endif
*/
/**
 * @brief Monitor multiple file descriptors, waiting until one or more of the file descriptors become "ready"
 *
 * @usage Call this after virt_start() has succeeded
 * @param nfds
 * @param readfds
 * @param writefds
 * @param exceptfds
 * @param timeout
 * @return
 */
int virt_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

/**
 * @brief Issue file control commands on a socket
 *
 * @usage Call this after virt_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param cmd
 * @param flags
 * @return
 */
int virt_fcntl(int fd, int cmd, int flags);

/**
 * @brief Control a device
 *
 * @usage Call this after virt_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param request
 * @param argp
 * @return
 */
int virt_ioctl(int fd, unsigned long request, void *argp);

/**
 * @brief Send data to remote host
 *
 * @usage Call this after virt_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param buf Pointer to data buffer
 * @param len Length of data to write
 * @param flags
 * @return
 */
ssize_t virt_send(int fd, const void *buf, size_t len, int flags);

/**
 * @brief Send data to remote host
 *
 * @usage Call this after virt_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param buf Pointer to data buffer
 * @param len Length of data to write
 * @param flags
 * @param addr Destination address
 * @param addrlen Length of destination address
 * @return
 */
ssize_t virt_sendto(int fd, const void *buf, size_t len, int flags, const struct sockaddr *addr, socklen_t addrlen);

/**
 * @brief Send message to remote host
 *
 * @usage Call this after virt_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param msg
 * @param flags
 * @return
 */
ssize_t virt_sendmsg(int fd, const struct msghdr *msg, int flags);

/**
 * @brief Receive data from remote host
 *
 * @usage Call this after virt_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param buf Pointer to data buffer
 * @param len Length of data buffer
 * @param flags
 * @return
 */
ssize_t virt_recv(int fd, void *buf, size_t len, int flags);

/**
 * @brief Receive data from remote host
 *
 * @usage Call this after virt_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param buf Pointer to data buffer
 * @param len Length of data buffer
 * @param flags
 * @param addr
 * @param addrlen
 * @return
 */
ssize_t virt_recvfrom(int fd, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t *addrlen);

/**
 * @brief Receive a message from remote host
 *
 * @usage Call this after virt_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param msg
 * @param flags
 * @return
 */
ssize_t virt_recvmsg(int fd, struct msghdr *msg,int flags);

/**
 * @brief Read bytes from socket onto buffer
 *
 * @usage Call this after virt_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param buf Pointer to data buffer
 * @param len Length of data buffer to receive data
 * @return
 */
int virt_read(int fd, void *buf, size_t len);

/**
 * @brief Write bytes from buffer to socket
 *
 * @usage Call this after virt_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param buf Pointer to data buffer
 * @param len Length of buffer to write
 * @return
 */
int virt_write(int fd, const void *buf, size_t len);

/**
 * @brief Shut down some aspect of a socket (read, write, or both)
 *
 * @usage Call this after virt_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param how Which aspects of the socket should be shut down
 * @return
 */
int virt_shutdown(int fd, int how);

/**
 * @brief Adds a DNS nameserver for the network stack to use
 *
 * @usage Call this after virt_start() has succeeded
 * @param addr Address for DNS nameserver
 * @return
 */
int virt_add_dns_nameserver(struct sockaddr *addr);

/**
 * @brief Removes a DNS nameserver
 *
 * @usage Call this after virt_start() has succeeded
 * @param addr Address for DNS nameserver
 * @return
 */
int virt_del_dns_nameserver(struct sockaddr *addr);

/**
 * @brief Returns whether one can add a new socket or not. This depends on network stack in use.
 *
 * @usage Call this after zts_start() has succeeded
 * @param socket_type
 * @return
 */
bool virt_can_provision_new_socket(int socket_type);

/**
 * @brief Returns the number of VirtualSockets either already provisioned or waiting to be
 * Some network stacks may have a limit on the number of sockets that they can
 * safely handle due to timer construction, this is a way to check that we
 * haven't passed that limit. Someday if multiple stacks are used simultaneously
 * the logic for this function should change accordingly.
 *
 * @usage Call this after zts_start() has succeeded
 * @return
 */
int virt_num_active_sockets();

/**
 * @brief Return the maximum number of sockets allowable by platform/stack configuration
 *
 * @usage Call this after zts_start() has succeeded
 * @param socket_type
 * @return
 */
int virt_maxsockets(int socket_type);

/**
 * @brief Return the number of currently active picoTCP timers
 *
 * @usage Call this after zts_start() has succeeded
 * @return
 */
//int pico_ntimers();

/**
 * @brief Convert a struct sockaddr to a ZeroTier::InetAddress
 *
 * @usage For internal use only.
 * @param socket_family
 * @param addr
 * @param inet
 * @return
 */
void sockaddr2inet(int socket_family, const struct sockaddr *addr, ZeroTier::InetAddress *inet);

