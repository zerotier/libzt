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

#define ZT_MAX_MTU 10000

/**
 * How fast service states are re-checked (in milliseconds)
 */
#define ZTO_WRAPPER_CHECK_INTERVAL 50

/**
 * 
 */
#define ZTO_ID_LEN                  16

typedef uint32_t socklen_t;

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
#define TCP_KEEPIDLE            103
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

/*
  Specifies the polling interval and the callback function that should
  be called to poll the application. The interval is specified in
  number of TCP coarse grained timer shots, which typically occurs
  twice a second. An interval of 10 means that the application would
  be polled every 5 seconds. 
  */
#define LWIP_APPLICATION_POLL_FREQ         2

/**
 * 
 */
#define LWIP_TCP_TIMER_INTERVAL            25

/**
 * How often we check VirtualSocket statuses (in ms)
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
 * 
 */
#define ZT_MAX_SOCKETS                     1024

/**
 * 
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
 * 
 */
#define ZT_SOCKET_MSG_BUF_SZ               ZT_SDK_MTU + ZT_LEN_SZ + ZT_ADDR_SZ

/**
 * 
 */
#define ZT_PHY_POLL_INTERVAL                 5 // ms

/**
 * 
 */
#define ZT_ACCEPT_RECHECK_DELAY            100 // ms (for blocking zts_accept() calls)

/**
 * 
 */
#define ZT_CONNECT_RECHECK_DELAY           100 // ms (for blocking zts_connect() calls)

/**
 * 
 */
#define ZT_API_CHECK_INTERVAL              100 // ms

/**
 * 
 */
#define MAX_PICO_FRAME_RX_BUF_SZ           ZT_MAX_MTU * 128

/**
 * 
 */
#define ZT_TCP_TX_BUF_SZ                   1024 * 1024 * 128

/**
 * 
 */
#define ZT_TCP_RX_BUF_SZ                   1024 * 1024 * 128

/**
 * 
 */
#define ZT_UDP_TX_BUF_SZ                   ZT_MAX_MTU

/**
 * 
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
 * 
 */
#define ZT_STACK_SOCKET_WR_MAX             4096

/**
 * 
 */
#define ZT_STACK_SOCKET_RD_MAX             4096*4

/**
 * 
 */
#define ZT_CORE_VERSION_MAJOR              1
#define ZT_CORE_VERSION_MINOR              2
#define ZT_CORE_VERSION_REVISION           5

/**
 * 
 */
#define ZT_LIB_VERSION_MAJOR               1
#define ZT_LIB_VERSION_MINOR               1
#define ZT_LIB_VERSION_REVISION            4

/**
 * 
 */
#define ZT_ID_LEN                          16

/**
 * 
 */
#define ZT_VER_STR_LEN                     6

/**
 * 
 */
#define ZT_HOME_PATH_MAX_LEN               128

/**
 * 
 */
#define ZT_MAC_ADDRSTRLEN                  18

/**
 * 
 */
#define ZT_ERR_OK                          0

/**
 * 
 */
#define ZT_ERR_GENERAL_FAILURE             -88

// Since extra time is required to send a mesage via a socket through the
// stack and ZT service, calling a zts_close() immediately after a "successful"
// zts_write() might cause data loss, for this reason, sockets will SO_LINGER for
// a short period of time by default as a precaution.

/**
 * 
 */
#define ZT_SOCK_BEHAVIOR_LINGER            true

/**
 * 
 */
#define ZT_SOCK_BEHAVIOR_LINGER_TIME       3  // s

/**
 * Wait time for socket closure if data is still present in the write queue
 */
#define ZT_SDK_CLTIME                      60

// After closing a pico_socket, other threads might still try to use the
// VirtualSocket object for remaining data I/O, as a safety measure we will wait to
// delete this VirtualSocket object until the socket has been closed for some arbitrary
// amount of time and it is safe to assume any clients interacting with this
// socket have read some sort of error code from the API.

/**
 * Interval for performing cleanup tasks on Tap/Stack objects (in seconds)
 */
#define ZT_HOUSEKEEPING_INTERVAL           10

/**
 * Whether or not we want libzt to exit on internal failure
 */
#define ZT_EXIT_ON_GENERAL_FAIL            false

#endif // LIBZT_DEFINES_H