/*
 * ZeroTier SDK - Network Virtualization Everywhere
 * Copyright (C) 2011-2019  ZeroTier, Inc.  https://www.zerotier.com/
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
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
 * ZeroTier socket API
 */

#ifndef LIBZT_H
#define LIBZT_H

#ifdef _WIN32
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

#include <stdint.h>
#include <vector>

#if !defined(_WIN32) && !defined(__ANDROID__)
typedef unsigned int socklen_t;
//#include <sys/socket.h>
#else
typedef int socklen_t;
//#include <sys/socket.h>
#endif

#if defined(_WIN32)
#include <WinSock2.h>
#include <stdint.h>
#include <WS2tcpip.h>
#include <Windows.h>
#endif

#ifdef _USING_LWIP_DEFINITIONS_
#include "lwip/sockets.h"
#endif

#include "Constants.hpp"
#include "Defs.hpp"
#include "ServiceControls.hpp"

class InetAddress;
class VirtualTap;

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

//////////////////////////////////////////////////////////////////////////////
// Common definitions and structures for interacting with the ZT socket API //
// This is a subset of lwip/sockets.h, lwip/arch.h, and lwip/inet.h         //
//                                                                          //
// These re-definitions exist here so that the user application's usage     //
// of the API is internally consistent with the underlying network stack.   //
// They have an attached prefix so that they can co-exist with the native   //
// platform's own definitions and structures.                               // 
//////////////////////////////////////////////////////////////////////////////

// Socket protocol types
#define ZTS_SOCK_STREAM     0x0001
#define ZTS_SOCK_DGRAM      0x0002
#define ZTS_SOCK_RAW        0x0003
// Socket family types
#define ZTS_AF_UNSPEC       0x0000
#define ZTS_AF_INET         0x0002
#define ZTS_AF_INET6        0x000a
#define ZTS_PF_INET         ZTS_AF_INET
#define ZTS_PF_INET6        ZTS_AF_INET6
#define ZTS_PF_UNSPEC       ZTS_AF_UNSPEC
// 
#define ZTS_IPPROTO_IP      0x0000
#define ZTS_IPPROTO_ICMP    0x0001
#define ZTS_IPPROTO_TCP     0x0006
#define ZTS_IPPROTO_UDP     0x0011
#define ZTS_IPPROTO_IPV6    0x0029
#define ZTS_IPPROTO_ICMPV6  0x003a
#define ZTS_IPPROTO_UDPLITE 0x0088
#define ZTS_IPPROTO_RAW     0x00ff
// send() and recv() flags
#define ZTS_MSG_PEEK        0x0001
#define ZTS_MSG_WAITALL     0x0002 // NOT YET SUPPORTED
#define ZTS_MSG_OOB         0x0004 // NOT YET SUPPORTED
#define ZTS_MSG_DONTWAIT    0x0008
#define ZTS_MSG_MORE        0x0010
// fnctl() commands
#define ZTS_F_GETFL         0x0003
#define ZTS_F_SETFL         0x0004
// fnctl() flags
#define ZTS_O_NONBLOCK      0x0001
#define ZTS_O_NDELAY        0x0001
//
#define ZTS_SHUT_RD         0x0000
#define ZTS_SHUT_WR         0x0001
#define ZTS_SHUT_RDWR       0x0002
// Socket level option number
#define ZTS_SOL_SOCKET      0x0fff
// Socket options
#define ZTS_SO_DEBUG        0x0001 // NOT YET SUPPORTED
#define ZTS_SO_ACCEPTCONN   0x0002
#define ZTS_SO_REUSEADDR    0x0004
#define ZTS_SO_KEEPALIVE    0x0008
#define ZTS_SO_DONTROUTE    0x0010 // NOT YET SUPPORTED
#define ZTS_SO_BROADCAST    0x0020
#define ZTS_SO_USELOOPBACK  0x0040 // NOT YET SUPPORTED
#define ZTS_SO_LINGER       0x0080
#define ZTS_SO_DONTLINGER   ((int)(~ZTS_SO_LINGER))
#define ZTS_SO_OOBINLINE    0x0100 // NOT YET SUPPORTED
#define ZTS_SO_REUSEPORT    0x0200 // NOT YET SUPPORTED
#define ZTS_SO_SNDBUF       0x1001 // NOT YET SUPPORTED
#define ZTS_SO_RCVBUF       0x1002
#define ZTS_SO_SNDLOWAT     0x1003 // NOT YET SUPPORTED
#define ZTS_SO_RCVLOWAT     0x1004 // NOT YET SUPPORTED
#define ZTS_SO_SNDTIMEO     0x1005
#define ZTS_SO_RCVTIMEO     0x1006
#define ZTS_SO_ERROR        0x1007
#define ZTS_SO_TYPE         0x1008
#define ZTS_SO_CONTIMEO     0x1009
#define ZTS_SO_NO_CHECK     0x100a
// IPPROTO_IP options
#define ZTS_IP_TOS          0x0001
#define ZTS_IP_TTL          0x0002
// IPPROTO_TCP options
#define ZTS_TCP_NODELAY     0x0001
#define ZTS_TCP_KEEPALIVE   0x0002
#define ZTS_TCP_KEEPIDLE    0x0003
#define ZTS_TCP_KEEPINTVL   0x0004
#define ZTS_TCP_KEEPCNT     0x0005
// IPPROTO_IPV6 options
#define ZTS_IPV6_CHECKSUM   0x0007  // RFC3542
#define ZTS_IPV6_V6ONLY     0x001b  // RFC3493
//
#define ZTS_IOCPARM_MASK    0x7fU
#define ZTS_IOC_VOID        0x20000000UL
#define ZTS_IOC_OUT         0x40000000UL
#define ZTS_IOC_IN          0x80000000UL
#define ZTS_IOC_INOUT       (ZTS_IOC_IN   | ZTS_IOC_OUT)
#define ZTS_IO(x,y)         (ZTS_IOC_VOID | ((x)<<8)|(y))
#define ZTS_IOR(x,y,t)      (ZTS_IOC_OUT  | (((long)sizeof(t) & ZTS_IOCPARM_MASK)<<16) | ((x)<<8) | (y))
#define ZTS_IOW(x,y,t)      (ZTS_IOC_IN   | (((long)sizeof(t) & ZTS_IOCPARM_MASK)<<16) | ((x)<<8) | (y))
//
#define ZTS_FIONREAD        ZTS_IOR('f', 127, unsigned long)
#define ZTS_FIONBIO         ZTS_IOW('f', 126, unsigned long)


/* FD_SET used for lwip_select */

#ifndef ZTS_FD_SET
#undef  ZTS_FD_SETSIZE
// Make FD_SETSIZE match NUM_SOCKETS in socket.c
#define ZTS_FD_SETSIZE    MEMP_NUM_NETCONN
#define ZTS_FDSETSAFESET(n, code) do { \
  if (((n) - LWIP_SOCKET_OFFSET < MEMP_NUM_NETCONN) && (((int)(n) - LWIP_SOCKET_OFFSET) >= 0)) { \
  code; }} while(0)
#define ZTS_FDSETSAFEGET(n, code) (((n) - LWIP_SOCKET_OFFSET < MEMP_NUM_NETCONN) && (((int)(n) - LWIP_SOCKET_OFFSET) >= 0) ?\
  (code) : 0)
#define ZTS_FD_SET(n, p)  ZTS_FDSETSAFESET(n, (p)->fd_bits[((n)-LWIP_SOCKET_OFFSET)/8] |=  (1 << (((n)-LWIP_SOCKET_OFFSET) & 7)))
#define ZTS_FD_CLR(n, p)  ZTS_FDSETSAFESET(n, (p)->fd_bits[((n)-LWIP_SOCKET_OFFSET)/8] &= ~(1 << (((n)-LWIP_SOCKET_OFFSET) & 7)))
#define ZTS_FD_ISSET(n,p) ZTS_FDSETSAFEGET(n, (p)->fd_bits[((n)-LWIP_SOCKET_OFFSET)/8] &   (1 << (((n)-LWIP_SOCKET_OFFSET) & 7)))
#define ZTS_FD_ZERO(p)    memset((void*)(p), 0, sizeof(*(p)))

#elif LWIP_SOCKET_OFFSET
#error LWIP_SOCKET_OFFSET does not work with external FD_SET!
#elif ZTS_FD_SETSIZE < MEMP_NUM_NETCONN
#error "external ZTS_FD_SETSIZE too small for number of sockets"
#endif // FD_SET

#if !defined(_USING_LWIP_DEFINITIONS_)

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t   u8_t;
typedef int8_t    s8_t;
typedef uint16_t  u16_t;
typedef int16_t   s16_t;
typedef uint32_t  u32_t;
typedef int32_t   s32_t;
typedef uintptr_t mem_ptr_t;

typedef u32_t zts_in_addr_t;
typedef u16_t zts_in_port_t;
typedef u8_t zts_sa_family_t;

struct zts_in_addr {
  zts_in_addr_t s_addr;
};

struct zts_in6_addr {
  union {
    u32_t u32_addr[4];
    u8_t  u8_addr[16];
  } un;
#define s6_addr  un.u8_addr
};

struct zts_sockaddr_in {
  u8_t            sin_len;
  zts_sa_family_t     sin_family;
  zts_in_port_t       sin_port;
  struct zts_in_addr  sin_addr;
#define SIN_ZERO_LEN 8
  char            sin_zero[SIN_ZERO_LEN];
};

struct zts_sockaddr_in6 {
  u8_t            sin6_len;      /* length of this structure    */
  zts_sa_family_t     sin6_family;   /* AF_INET6                    */
  zts_in_port_t       sin6_port;     /* Transport layer port #      */
  u32_t           sin6_flowinfo; /* IPv6 flow information       */
  struct zts_in6_addr sin6_addr;     /* IPv6 address                */
  u32_t           sin6_scope_id; /* Set of interfaces for scope */
};

struct zts_sockaddr {
  u8_t        sa_len;
  zts_sa_family_t sa_family;
  char        sa_data[14];
};

struct zts_sockaddr_storage {
  u8_t        s2_len;
  zts_sa_family_t ss_family;
  char        s2_data1[2];
  u32_t       s2_data2[3];
  u32_t       s2_data3[3];
};

#if !defined(zts_iovec)
struct zts_iovec {
  void  *iov_base;
  size_t iov_len;
};
#endif

struct zts_msghdr {
  void         *msg_name;
  socklen_t     msg_namelen;
  struct iovec *msg_iov;
  int           msg_iovlen;
  void         *msg_control;
  socklen_t     msg_controllen;
  int           msg_flags;
};

/*
 * Structure used for manipulating linger option.
 */
struct zts_linger {
       int l_onoff;                /* option on/off */
       int l_linger;               /* linger time in seconds */
};

/*
typedef struct fd_set
{
  unsigned char fd_bits [(FD_SETSIZE+7)/8];
} fd_set;
*/

#ifdef __cplusplus
}
#endif

#endif // _USING_LWIP_DEFINITIONS_

//////////////////////////////////////////////////////////////////////////////
// For SOCK_RAW support, it will initially be modeled after linux's API, so //
// below are the various things we need to define in order to make this API //
// work on other platforms. Maybe later down the road we will customize	    //
// this for each different platform. Maybe.									//
//////////////////////////////////////////////////////////////////////////////

#if !defined(__linux__)
#define SIOCGIFINDEX 101
#define SIOCGIFHWADDR 102

// Normally defined in linux/if_packet.h, defined here so we can offer a linux-like
// raw socket API on non-linux platforms
struct sockaddr_ll {
		unsigned short sll_family;	/* Always AF_PACKET */
		unsigned short sll_protocol; /* Physical layer protocol */
		int	sll_ifindex;  /* Interface number */
		unsigned short sll_hatype;	/* ARP hardware type */
		unsigned char sll_pkttype;  /* Packet type */
		unsigned char sll_halen;	 /* Length of address */
		unsigned char sll_addr[8];  /* Physical layer address */
};

#endif

//////////////////////////////////////////////////////////////////////////////
// Socket API                                                               //
//////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

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
ZT_SOCKET_API zts_err_t ZTCALL zts_socket(int socket_family, int socket_type, int protocol);

/**
 * @brief Connect a socket to a remote host
 *
 * @usage Call this after zts_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param addr Remote host address to connect to
 * @param addrlen Length of address
 * @return
 */
ZT_SOCKET_API zts_err_t ZTCALL zts_connect(int fd, const struct sockaddr *addr, socklen_t addrlen);

/**
 * @brief Bind a socket to a virtual interface
 *
 * @usage Call this after zts_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param addr Local interface address to bind to
 * @param addrlen Length of address
 * @return
 */
ZT_SOCKET_API zts_err_t ZTCALL zts_bind(int fd, const struct sockaddr *addr, socklen_t addrlen);

/**
 * @brief Listen for incoming connections
 *
 * @usage Call this after zts_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param backlog Number of backlogged connection allowed
 * @return
 */
ZT_SOCKET_API zts_err_t ZTCALL zts_listen(int fd, int backlog);

/**
 * @brief Accept an incoming connection
 *
 * @usage Call this after zts_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param addr Address of remote host for accepted connection
 * @param addrlen Length of address
 * @return
 */
ZT_SOCKET_API zts_err_t ZTCALL zts_accept(int fd, struct sockaddr *addr, socklen_t *addrlen);

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
	zts_err_t zts_accept4(int fd, struct sockaddr *addr, socklen_t *addrlen, int flags);
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
ZT_SOCKET_API zts_err_t ZTCALL zts_setsockopt(
	int fd, int level, int optname, const void *optval, socklen_t optlen);

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
ZT_SOCKET_API zts_err_t ZTCALL zts_getsockopt(
	int fd, int level, int optname, void *optval, socklen_t *optlen);

/**
 * @brief Get socket name
 *
 * @usage Call this after zts_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param addr Name associated with this socket
 * @param addrlen Length of name
 * @return
 */
ZT_SOCKET_API zts_err_t ZTCALL zts_getsockname(int fd, struct sockaddr *addr, socklen_t *addrlen);

/**
 * @brief Get the peer name for the remote end of a connected socket
 *
 * @usage Call this after zts_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param addr Name associated with remote end of this socket
 * @param addrlen Length of name
 * @return
 */
ZT_SOCKET_API zts_err_t ZTCALL zts_getpeername(int fd, struct sockaddr *addr, socklen_t *addrlen);

/**
 * @brief Gets current hostname
 *
 * @usage Call this after zts_start() has succeeded
 * @param name
 * @param len
 * @return
 */
ZT_SOCKET_API zts_err_t ZTCALL zts_gethostname(char *name, size_t len);

/**
 * @brief Sets current hostname
 *
 * @usage Call this after zts_start() has succeeded
 * @param name
 * @param len
 * @return
 */
ZT_SOCKET_API zts_err_t ZTCALL zts_sethostname(const char *name, size_t len);

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
ZT_SOCKET_API zts_err_t ZTCALL zts_close(int fd);

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
/*
typedef unsigned int nfds_t;
int zts_poll(struct pollfd *fds, nfds_t nfds, int timeout);
*/
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
ZT_SOCKET_API zts_err_t ZTCALL zts_select(
	int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

/**
 * @brief Issue file control commands on a socket
 *
 * @usage Call this after zts_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param cmd
 * @param flags
 * @return
 */
#if defined(_WIN32)
#define F_SETFL 0
#define O_NONBLOCK 0
#endif
ZT_SOCKET_API zts_err_t ZTCALL zts_fcntl(int fd, int cmd, int flags);

/**
 * @brief Control a device
 *
 * @usage Call this after zts_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param request
 * @param argp
 * @return
 */
ZT_SOCKET_API zts_err_t ZTCALL zts_ioctl(int fd, unsigned long request, void *argp);

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
ZT_SOCKET_API ssize_t ZTCALL zts_sendto(
	int fd, const void *buf, size_t len, int flags, const struct sockaddr *addr, socklen_t addrlen);

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
ZT_SOCKET_API ssize_t ZTCALL zts_recvfrom(
	int fd, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t *addrlen);

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
ZT_SOCKET_API zts_err_t ZTCALL zts_read(int fd, void *buf, size_t len);

/**
 * @brief Write bytes from buffer to socket
 *
 * @usage Call this after zts_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param buf Pointer to data buffer
 * @param len Length of buffer to write
 * @return
 */
ZT_SOCKET_API zts_err_t ZTCALL zts_write(int fd, const void *buf, size_t len);

/**
 * @brief Shut down some aspect of a socket (read, write, or both)
 *
 * @usage Call this after zts_start() has succeeded
 * @param fd File descriptor (only valid for use with libzt calls)
 * @param how Which aspects of the socket should be shut down
 * @return
 */
ZT_SOCKET_API zts_err_t ZTCALL zts_shutdown(int fd, int how);

/**
 * @brief Adds a DNS nameserver for the network stack to use
 *
 * @usage Call this after zts_start() has succeeded
 * @param addr Address for DNS nameserver
 * @return
 */
ZT_SOCKET_API zts_err_t ZTCALL zts_add_dns_nameserver(struct sockaddr *addr);

/**
 * @brief Removes a DNS nameserver
 *
 * @usage Call this after zts_start() has succeeded
 * @param addr Address for DNS nameserver
 * @return
 */
ZT_SOCKET_API zts_err_t ZTCALL zts_del_dns_nameserver(struct sockaddr *addr);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _H
