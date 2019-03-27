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
 * Common constants used throughout the SDK
 */

#ifndef ZEROTIER_CONSTANTS_H
#define ZEROTIER_CONSTANTS_H

//////////////////////////////////////////////////////////////////////////////
// Control API error codes                                                  //
//////////////////////////////////////////////////////////////////////////////

// Everything is ok
#define ZTS_ERR_OK                          0
// A argument provided by the user application is invalid (e.g. out of range, NULL, etc)
#define ZTS_ERR_INVALID_ARG                -1
// The service isn't initialized or is for some reason currently unavailable. Try again.
#define ZTS_ERR_SERVICE                    -2
// For some reason this API operation is not permitted or doesn't make sense at this time.
#define ZTS_ERR_INVALID_OP                 -3
// The call succeeded, but no object or relevant result was available
#define ZTS_ERR_NO_RESULT                  -4
// General internal failure
#define ZTS_ERR_GENERAL                    -5

/**
 * The system port upon which ZT traffic is sent and received
 */
#define ZTS_DEFAULT_PORT 9994

//////////////////////////////////////////////////////////////////////////////
// Control API event codes                                                  //
//////////////////////////////////////////////////////////////////////////////

#define ZTS_EVENT_NONE                     -1
#define ZTS_EVENT_NODE_UP                   0
// Standard node events
#define ZTS_EVENT_NODE_OFFLINE              1
#define ZTS_EVENT_NODE_ONLINE               2
#define ZTS_EVENT_NODE_DOWN                 3
#define ZTS_EVENT_NODE_IDENTITY_COLLISION   4
#define ZTS_EVENT_NODE_UNRECOVERABLE_ERROR  16
#define ZTS_EVENT_NODE_NORMAL_TERMINATION   17
// Network events
#define ZTS_EVENT_NETWORK_NOT_FOUND         32
#define ZTS_EVENT_NETWORK_CLIENT_TOO_OLD    33
#define ZTS_EVENT_NETWORK_REQUESTING_CONFIG 34
#define ZTS_EVENT_NETWORK_OK                35
#define ZTS_EVENT_NETWORK_ACCESS_DENIED     36
#define ZTS_EVENT_NETWORK_READY_IP4         37
#define ZTS_EVENT_NETWORK_READY_IP6         38
#define ZTS_EVENT_NETWORK_READY_IP4_IP6     39
#define ZTS_EVENT_NETWORK_DOWN              40
// Network Stack events
#define ZTS_EVENT_STACK_UP                  48
#define ZTS_EVENT_STACK_DOWN                49
// lwIP netif events
#define ZTS_EVENT_NETIF_UP                  64
#define ZTS_EVENT_NETIF_DOWN                65
#define ZTS_EVENT_NETIF_REMOVED             66
#define ZTS_EVENT_NETIF_LINK_UP             67
#define ZTS_EVENT_NETIF_LINK_DOWN           68
// Peer events
#define ZTS_EVENT_PEER_P2P                  96
#define ZTS_EVENT_PEER_RELAY                97
#define ZTS_EVENT_PEER_UNREACHABLE          98
// Path events
#define ZTS_EVENT_PATH_DISCOVERED           112
#define ZTS_EVENT_PATH_ALIVE                113
#define ZTS_EVENT_PATH_DEAD                 114
// Route events
#define ZTS_EVENT_ROUTE_ADDED               128
#define ZTS_EVENT_ROUTE_REMOVED             129
// Address events
#define ZTS_EVENT_ADDR_ADDED_IP4            144
#define ZTS_EVENT_ADDR_REMOVED_IP4          145
#define ZTS_EVENT_ADDR_ADDED_IP6            146
#define ZTS_EVENT_ADDR_REMOVED_IP6          147

// Macros for legacy behaviour
#define NODE_EVENT_TYPE(code) code >= ZTS_EVENT_NODE_UP && code <= ZTS_EVENT_NODE_NORMAL_TERMINATION
#define NETWORK_EVENT_TYPE(code) code >= ZTS_EVENT_NETWORK_NOT_FOUND && code <= ZTS_EVENT_NETWORK_DOWN
#define STACK_EVENT_TYPE(code) code >= ZTS_EVENT_STACK_UP && code <= ZTS_EVENT_STACK_DOWN
#define NETIF_EVENT_TYPE(code) code >= ZTS_EVENT_NETIF_UP && code <= ZTS_EVENT_NETIF_LINK_DOWN
#define PEER_EVENT_TYPE(code) code >= ZTS_EVENT_PEER_P2P && code <= ZTS_EVENT_PEER_UNREACHABLE
#define PATH_EVENT_TYPE(code) code >= ZTS_EVENT_PATH_DISCOVERED && code <= ZTS_EVENT_PATH_DEAD
#define ROUTE_EVENT_TYPE(code) code >= ZTS_EVENT_ROUTE_ADDED && code <= ZTS_EVENT_ROUTE_REMOVED
#define ADDR_EVENT_TYPE(code) code >= ZTS_EVENT_ADDR_ADDED_IP4 && code <= ZTS_EVENT_ADDR_REMOVED_IP6

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
// Protocol command types
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
// Shutdown commands
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
// Macro's for defining ioctl() command values
#define ZTS_IOCPARM_MASK    0x7fU
#define ZTS_IOC_VOID        0x20000000UL
#define ZTS_IOC_OUT         0x40000000UL
#define ZTS_IOC_IN          0x80000000UL
#define ZTS_IOC_INOUT       (ZTS_IOC_IN   | ZTS_IOC_OUT)
#define ZTS_IO(x,y)         (ZTS_IOC_VOID | ((x)<<8)|(y))
#define ZTS_IOR(x,y,t)      (ZTS_IOC_OUT  | (((long)sizeof(t) & ZTS_IOCPARM_MASK)<<16) | ((x)<<8) | (y))
#define ZTS_IOW(x,y,t)      (ZTS_IOC_IN   | (((long)sizeof(t) & ZTS_IOCPARM_MASK)<<16) | ((x)<<8) | (y))
// ioctl() commands
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

//////////////////////////////////////////////////////////////////////////////
// Statistics                                                               //
//////////////////////////////////////////////////////////////////////////////

#define ZTS_STATS_PROTOCOL_LINK      0
#define ZTS_STATS_PROTOCOL_ETHARP    1
#define ZTS_STATS_PROTOCOL_IP        2
#define ZTS_STATS_PROTOCOL_UDP       3
#define ZTS_STATS_PROTOCOL_TCP       4
#define ZTS_STATS_PROTOCOL_ICMP      5
#define ZTS_STATS_PROTOCOL_IP_FRAG   6
#define ZTS_STATS_PROTOCOL_IP6       7
#define ZTS_STATS_PROTOCOL_ICMP6     8
#define ZTS_STATS_PROTOCOL_IP6_FRAG  9

//#if defined(_USING_LWIP_DEFINITIONS_)

#endif // ZEROTIER_CONSTANTS_H