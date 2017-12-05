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

#ifndef LIBZT_DEFINES_H
#define LIBZT_DEFINES_H

/**
 * Use ZeroTier Virtual Socket layer to abstract network stack raw API
 */
#define ZT_VIRTUAL_SOCKET  0
/**
 * Use lwIP sockets API
 */
#define ZT_LWIP_SEQ_SOCKET 1
/**
 * Use pico BSD socket API
 */
#define ZT_PICO_BSD_SOCKET 0

#define STACK_LWIP 1
#define STACK_PICO 0
#define NO_STACK   0 // for layer-2 only (this will omit all userspace network stack code)


/*  sanity checks for userspace network stack and socket API layer choices

 EX.
 zts_socket()
   1. ) ZT_VIRTUAL_SOCKET? -> virt_socket() --- Choose this if the default socket layer isn't doing what you need
                       STACK_LWIP? -> raw lwip_ API
                       STACK_PICO? -> raw pico_ API
                       otherStack? -> raw API

   2.) ZT_LWIP_SEQ_SOCKET? (default) -> lwip_socket() --- currently provides greatest safety and performance
   3.) ZT_PICO_BSD_SOCKET? -> pico_ socket API
   otherStack? -> other_stack_socket()

   Default is: STACK_LWIP=1 ZT_LWIP_SEQ_SOCKET=1

*/

#if (STACK_LWIP+STACK_PICO) > 1
#error "Multiple network stacks specified. Pick only one."
#endif
#if STACK_LWIP==0 && STACK_PICO==0 && NO_STACK==0
#error "No network stacks specified and NO_STACK wasn't set. Pick one."
#endif
#if ZT_VIRTUAL_SOCKET==0 && ZT_LWIP_SEQ_SOCKET==0 && ZT_PICO_BSD_SOCKET==0
#error "No socket handling layer specified. Pick one."
#endif
#if (ZT_VIRTUAL_SOCKET + ZT_LWIP_SEQ_SOCKET + ZT_PICO_BSD_SOCKET) > 1
#error "Multiple socket handling layers specified. Pick only one."
#endif
#if  ZT_LWIP_SEQ_SOCKET==1 && STACK_LWIP==0
#error "ZT_LWIP_SEQ_SOCKET is selected as socket handling layer, but STACK_LWIP isn't set"
#endif
#if  ZT_PICO_BSD_SOCKET==1 && STACK_PICO==0
#error "ZT_PICO_BSD_SOCKET is selected as socket handling layer, but STACK_PICO isn't set"
#endif

#if STACK_LWIP==1
  #undef STACK_PICO
  #undef NO_STACK
#endif
#if STACK_PICO==1
  #undef STACK_LWIP
  #undef NO_STACK
#endif
#if NO_STACK==1
  #undef STACK_LWIP
  #undef STACK_PICO
#endif
#if ZT_VIRTUAL_SOCKET==1
  #undef ZT_LWIP_SEQ_SOCKET
  #undef ZT_PICO_BSD_SOCKET
#endif
#if ZT_LWIP_SEQ_SOCKET==1
  #undef ZT_VIRTUAL_SOCKET
  #undef ZT_PICO_BSD_SOCKET
#endif
#if ZT_PICO_BSD_SOCKET==1
  #undef ZT_VIRTUAL_SOCKET
  #undef ZT_LWIP_SEQ_SOCKET
#endif

/**
 * Maximum MTU size for ZeroTier
 */
#define ZT_MAX_MTU 10000

/**
 * How fast service states are re-checked (in milliseconds)
 */
#define ZTO_WRAPPER_CHECK_INTERVAL 100

/**
 * Length of buffer required to hold a ztAddress/nodeID
 */
#define ZTO_ID_LEN                  16

#if !defined(__MINGW32__)
typedef unsigned int socklen_t;
#endif

/****************************************************************************/
/* For SOCK_RAW support, it will initially be modeled after linux's API, so */
/* below are the various things we need to define in order to make this API */
/* work on other platforms. Mayber later down the road we will customize    */
/* this for each different platform. Maybe.                                 */
/****************************************************************************/

#if !defined(__linux__)
#define SIOCGIFINDEX  101
#define SIOCGIFHWADDR 102

// Normally defined in linux/if_packet.h, defined here so we can offer a linux-like
// raw socket API on non-linux platforms
struct sockaddr_ll {
		unsigned short sll_family;   /* Always AF_PACKET */
		unsigned short sll_protocol; /* Physical layer protocol */
		int            sll_ifindex;  /* Interface number */
		unsigned short sll_hatype;   /* ARP hardware type */
		unsigned char  sll_pkttype;  /* Packet type */
		unsigned char  sll_halen;    /* Length of address */
		unsigned char  sll_addr[8];  /* Physical layer address */
};

#endif

// Provide missing optnames for setsockopt() implementations
#ifdef _WIN32
   #ifdef _WIN64
   #else
   #endif
#elif __APPLE__
#define IP_BIND_ADDRESS_NO_PORT 201
#define IP_FREEBIND             202
#define IP_MTU                  203
#define IP_MTU_DISCOVER         204
#define IP_MULTICAST_ALL        205
#define IP_NODEFRAG             206
#define IP_RECVORIGDSTADDR      207
#define IP_ROUTER_ALERT         208
#define IP_TRANSPARENT          209
#define TCP_INFO                210
#define SO_STYLE                100
#define TCP_CORK                101
#define TCP_DEFER_ACCEPT        102
//#ifndef TCP_KEEPIDLE
//#define TCP_KEEPIDLE            103
//#endif
#define TCP_LINGER2             104
#define TCP_QUICKACK            105
#define TCP_SYNCNT              106
#define TCP_WINDOW_CLAMP        107
#define UDP_CORK                108
#elif __linux__
#define SO_STYLE                100
#define UDP_CORK                101
#define IP_BIND_ADDRESS_NO_PORT 201
#define IP_NODEFRAG             206
#elif __unix__
#elif defined(_POSIX_VERSION)
#else
#   error "Unknown platform"
#endif

/****************************************************************************/
/* Legend                                                                   */
/****************************************************************************/

/*
	NSLWIP network_stack_lwip
	NSPICO network_stack_pico
	NSRXBF network_stack_pico guarded frame buffer RX
	ZTVIRT zt_virtual_wire
	APPFDS app_fd
	VSRXBF app_fd TX buf
	VSTXBF app_fd RX buf
*/

/****************************************************************************/
/* lwIP                                                                     */
/****************************************************************************/

// For LWIP configuration see: include/lwipopts.h

#if defined(STACK_LWIP)

typedef signed char err_t;

#define ND6_DISCOVERY_INTERVAL 1000
#define ARP_DISCOVERY_INTERVAL ARP_TMR_INTERVAL

/**
  Specifies the polling interval and the callback function that should
  be called to poll the application. The interval is specified in
  number of TCP coarse grained timer shots, which typically occurs
  twice a second. An interval of 10 means that the application would
  be polled every 5 seconds. (only for raw lwIP driver)
  */
#define LWIP_APPLICATION_POLL_FREQ         2

/**
 * TCP timer interval in milliseconds (only for raw lwIP driver)
 */
#define LWIP_TCP_TIMER_INTERVAL            25

/**
 * How often we check VirtualSocket statuses in milliseconds (only for raw lwIP driver)
 */
#define LWIP_STATUS_TMR_INTERVAL           500

// #define LWIP_CHKSUM <your_checksum_routine>, See: RFC1071 for inspiration
#endif

/****************************************************************************/
/* picoTCP                                                                  */
/****************************************************************************/

#if defined(STACK_PICO)
#endif

/****************************************************************************/
/* Defines                                                                  */
/****************************************************************************/

/**
 * Maximum number of sockets that libzt can administer
 */
#define ZT_MAX_SOCKETS                     1024

/**
 * Maximum MTU size for libzt (must be less than or equal to ZT_MAX_MTU)
 */
#define ZT_SDK_MTU                         ZT_MAX_MTU

/**
 *
 */
#define ZT_LEN_SZ                          4

/**
 *
 */
#define ZT_ADDR_SZ                         128

/**
 * Size of message buffer for VirtualSockets
 */
#define ZT_SOCKET_MSG_BUF_SZ               ZT_SDK_MTU + ZT_LEN_SZ + ZT_ADDR_SZ

/**
 * Polling interval (in ms) for file descriptors wrapped in the Phy I/O loop (for raw drivers only)
 */
#define ZT_PHY_POLL_INTERVAL               5

/**
 * State check interval (in ms) for VirtualSocket state
 */
#define ZT_ACCEPT_RECHECK_DELAY            50

/**
 * State check interval (in ms) for VirtualSocket state
 */
#define ZT_CONNECT_RECHECK_DELAY           50

/**
 * State check interval (in ms) for VirtualSocket state
 */
#define ZT_API_CHECK_INTERVAL              50

/**
 * Maximum size of guarded RX buffer (for picoTCP raw driver only)
 */
#define MAX_PICO_FRAME_RX_BUF_SZ           ZT_MAX_MTU * 128

/**
 * Size of TCP TX buffer for VirtualSockets used in raw network stack drivers
 */
#define ZT_TCP_TX_BUF_SZ                   1024 * 1024 * 128

/**
 * Size of TCP RX buffer for VirtualSockets used in raw network stack drivers
 */
#define ZT_TCP_RX_BUF_SZ                   1024 * 1024 * 128

/**
 * Size of UDP TX buffer for VirtualSockets used in raw network stack drivers
 */
#define ZT_UDP_TX_BUF_SZ                   ZT_MAX_MTU

/**
 * Size of UDP RX buffer for VirtualSockets used in raw network stack drivers
 */
#define ZT_UDP_RX_BUF_SZ                   ZT_MAX_MTU * 10

// Send and Receive buffer sizes for the network stack
// By default picoTCP sets them to 16834, this is good for embedded-scale
// stuff but you might want to consider higher values for desktop and mobile
// applications.

/**
 *
 */
#define ZT_STACK_TCP_SOCKET_TX_SZ          ZT_TCP_TX_BUF_SZ

/**
 *
 */
#define ZT_STACK_TCP_SOCKET_RX_SZ          ZT_TCP_RX_BUF_SZ

// Maximum size we're allowed to read or write from a stack socket
// This is put in place because picoTCP seems to fail at higher values.
// If you use another stack you can probably bump this up a bit.

/**
 * Maximum size of write operation to a network stack
 */
#define ZT_STACK_SOCKET_WR_MAX             4096

/**
 * Maximum size of read operation from a network stack
 */
#define ZT_STACK_SOCKET_RD_MAX             4096*4

/**
 * Maximum length of libzt/ZeroTier home path (where keys, and config files are stored)
 */
#define ZT_HOME_PATH_MAX_LEN               256

/**
 * Length of human-readable MAC address string
 */
#define ZT_MAC_ADDRSTRLEN                  18

/**
 * Everything is ok
 */
#define ZT_ERR_OK                          0

/**
 * Value returned during an internal failure at the VirtualSocket/VirtualTap layer
 */
#define ZT_ERR_GENERAL_FAILURE             -88

/**
 * Whether sockets created will have SO_LINGER set by default
 */
#define ZT_SOCK_BEHAVIOR_LINGER            false

/**
 * Length of time that VirtualSockets should linger (in seconds)
 */
#define ZT_SOCK_BEHAVIOR_LINGER_TIME       3

/**
 * Maximum wait time for socket closure if data is still present in the write queue
 */
#define ZT_SDK_CLTIME                      60

/**
 * Interval for performing background tasks (such as adding routes) on VirtualTap objects (in seconds)
 */
#define ZT_HOUSEKEEPING_INTERVAL           1

/****************************************************************************/
/* Socket API Signatures                                                    */
/****************************************************************************/

#define ZT_SETSOCKOPT_SIG int fd, int level, int optname, const void *optval, socklen_t optlen
#define ZT_GETSOCKOPT_SIG int fd, int level, int optname, void *optval, socklen_t *optlen
#define ZT_SENDMSG_SIG int fd, const struct msghdr *msg, int flags
#define ZT_SENDTO_SIG int fd, const void *buf, size_t len, int flags, const struct sockaddr *addr, socklen_t addrlen
#define ZT_RECV_SIG int fd, void *buf, size_t len, int flags
#define ZT_RECVFROM_SIG int fd, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t *addrlen
#define ZT_RECVMSG_SIG int fd, struct msghdr *msg,int flags
#define ZT_SEND_SIG int fd, const void *buf, size_t len, int flags
#define ZT_READ_SIG int fd, void *buf, size_t len
#define ZT_WRITE_SIG int fd, const void *buf, size_t len
#define ZT_SHUTDOWN_SIG int fd, int how
#define ZT_SOCKET_SIG int socket_family, int socket_type, int protocol
#define ZT_CONNECT_SIG int fd, const struct sockaddr *addr, socklen_t addrlen
#define ZT_BIND_SIG int fd, const struct sockaddr *addr, socklen_t addrlen
#define ZT_LISTEN_SIG int fd, int backlog
#define ZT_ACCEPT4_SIG int fd, struct sockaddr *addr, socklen_t *addrlen, int flags
#define ZT_ACCEPT_SIG int fd, struct sockaddr *addr, socklen_t *addrlen
#define ZT_CLOSE_SIG int fd
#define ZT_POLL_SIG struct pollfd *fds, nfds_t nfds, int timeout
#define ZT_SELECT_SIG int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout
#define ZT_GETSOCKNAME_SIG int fd, struct sockaddr *addr, socklen_t *addrlen
#define ZT_GETPEERNAME_SIG int fd, struct sockaddr *addr, socklen_t *addrlen
#define ZT_GETHOSTNAME_SIG char *name, size_t len
#define ZT_SETHOSTNAME_SIG const char *name, size_t len
#define ZT_FCNTL_SIG int fd, int cmd, int flags
#define ZT_IOCTL_SIG int fd, unsigned long request, void *argp
#define ZT_SYSCALL_SIG long number, ...

#endif // _H
