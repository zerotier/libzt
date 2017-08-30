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

#ifndef ZT_LIBZT_H
#define ZT_LIBZT_H

#include <sys/socket.h>
#include <poll.h>
#include <net/if.h>
#include <stdlib.h>
#include <stdint.h>
#include <vector>

#include "ZeroTierOne.h"

// See test/selftest.cpp for examples

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

/*
struct zts_ifreq {
	 char ifr_name[IFNAMSIZ]; // Interface name
	 union {
			 struct sockaddr ifr_addr;
			 struct sockaddr ifr_dstaddr;
			 struct sockaddr ifr_broadaddr;
			 struct sockaddr ifr_netmask;
			 struct sockaddr ifr_hwaddr;
			 short           ifr_flags;
			 int             ifr_ifindex;
			 int             ifr_metric;
			 int             ifr_mtu;
			 struct ifmap    ifr_map;
			 char            ifr_slave[IFNAMSIZ];
			 char            ifr_newname[IFNAMSIZ];
			 char           *ifr_data;
	 };
};
*/

#endif

/****************************************************************************/
/* LWIP                                                                     */
/****************************************************************************/

#define LWIP_APPLICATION_POLL_FREQ         2
#define LWIP_TCP_TIMER_INTERVAL            50
#define LWIP_STATUS_TMR_INTERVAL           500 // How often we check VirtualSocket statuses (in ms)

/****************************************************************************/
/* Defines                                                                  */
/****************************************************************************/

#define ZT_SDK_MTU                         ZT_MAX_MTU
#define ZT_LEN_SZ                          4
#define ZT_ADDR_SZ                         128
#define ZT_SOCKET_MSG_BUF_SZ               ZT_SDK_MTU + ZT_LEN_SZ + ZT_ADDR_SZ


#define ZT_PHY_POLL_INTERVAL               2 // ms
#define ZT_ACCEPT_RECHECK_DELAY            100 // ms (for blocking zts_accept() calls)
#define ZT_CONNECT_RECHECK_DELAY           100 // ms (for blocking zts_connect() calls)
#define ZT_API_CHECK_INTERVAL              500 // ms

#define MAX_PICO_FRAME_RX_BUF_SZ           ZT_MAX_MTU * 128

#define ZT_TCP_TX_BUF_SZ                   1024 * 1024 * 128
#define ZT_TCP_RX_BUF_SZ                   1024 * 1024 * 128
#define ZT_UDP_TX_BUF_SZ                   ZT_MAX_MTU
#define ZT_UDP_RX_BUF_SZ                   ZT_MAX_MTU * 10

// Send and Receive buffer sizes for the network stack
// By default picoTCP sets them to 16834, this is good for embedded-scale
// stuff but you might want to consider higher values for desktop and mobile
// applications.
#define ZT_STACK_TCP_SOCKET_TX_SZ          ZT_TCP_TX_BUF_SZ
#define ZT_STACK_TCP_SOCKET_RX_SZ          ZT_TCP_RX_BUF_SZ

// Maximum size we're allowed to read or write from a stack socket
// This is put in place because picoTCP seems to fail at higher values.
// If you use another stack you can probably bump this up a bit.
#define ZT_STACK_SOCKET_WR_MAX             4096
#define ZT_STACK_SOCKET_RD_MAX             4096*4

#define ZT_CORE_VERSION_MAJOR              1
#define ZT_CORE_VERSION_MINOR              2
#define ZT_CORE_VERSION_REVISION           5

#define ZT_LIB_VERSION_MAJOR               1
#define ZT_LIB_VERSION_MINOR               1
#define ZT_LIB_VERSION_REVISION            4

#define ZT_ID_LEN                          16
#define ZT_VER_STR_LEN                     6
#define ZT_HOME_PATH_MAX_LEN               128
#define ZT_MAC_ADDRSTRLEN                  18

#define ZT_SOCK_STATE_NONE                 100
#define ZT_SOCK_STATE_UNHANDLED_CONNECTED  101
#define ZT_SOCK_STATE_CONNECTED            102
#define ZT_SOCK_STATE_LISTENING            103

#define ZT_ERR_OK                          0
#define ZT_ERR_GENERAL_FAILURE             -88

// Since extra time is required to send a mesage via a socket through the
// stack and ZT service, calling a zts_close() immediately after a "successful"
// zts_write() might cause data loss, for this reason, sockets will SO_LINGER for
// a short period of time by default as a precaution.

#define ZT_SOCK_BEHAVIOR_LINGER            true
#define ZT_SOCK_BEHAVIOR_LINGER_TIME       3  // s

// Wait time for socket closure if data is still present in the write queue
#define ZT_SDK_CLTIME                      60

// After closing a pico_socket, other threads might still try to use the 
// VirtualSocket object for remaining data I/O, as a safety measure we will wait to 
// delete this VirtualSocket object until the socket has been closed for some arbitrary
// amount of time and it is safe to assume any clients interacting with this
// socket have read some sort of error code from the API.
#define ZT_VirtualSocket_DELETE_WAIT_TIME     30 // s

// Interval for performing cleanup tasks on Tap/Stack objects
#define ZT_HOUSEKEEPING_INTERVAL           10 // s 

// Whether or not we want libzt to shit its pants
#define ZT_EXIT_ON_GENERAL_FAIL            false

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

/****************************************************************************/
/* SDK Socket API (ZeroTier Service Controls)                               */
/* Implemented in libzt.cpp                                                 */ 
/****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Start core ZeroTier service (generates identity)
 */
void zts_start(const char *path);

/**
 * Convenience function: Starts, waits for service, joins one network, waits for address, returns
 */
void zts_simple_start(const char *path, const char *nwid);

/**
 * Stops the core ZeroTier service
 */
void zts_stop();

/**
 * Join a network
 */
void zts_join(const char * nwid);

/**
 * Join a network - Just create the dir and conf file required, don't instruct the core to do anything
 */
void zts_join_soft(const char * filepath, const char * nwid);

/**
 * Leave a network
 */
void zts_leave(const char * nwid);

/**
 * Leave a network - Only delete the .conf file, this will prevent the service from joining upon next startup
 */
void zts_leave_soft(const char * filepath, const char * nwid);

/**
 * Return the home path for this instance of ZeroTier
 * FIXME: double check this is correct on all platforms
 */
void zts_get_homepath(char *homePath, const int len);

/**
 * Provides core ZeroTier service version
 */
void zts_core_version(char *ver);

/**
 * Provides core libzt service version
 */
void zts_lib_version(char *ver);

/**
 * Get device ID
 * 10-digit hex + NULL byte
 */
int zts_get_device_id(char *devID);

/**
 * Check whether the service is running
 */
int zts_running();

/**
 * Returns whether any IPv6 address has been assigned to the SockTap for this network
 * - This is used as an indicator of readiness for service for the ZeroTier core and stack
 */
int zts_has_ipv4_address(const char *nwid);

/**
 * Returns whether any IPv4 address has been assigned to the SockTap for this network
 * - This is used as an indicator of readiness for service for the ZeroTier core and stack
 */
int zts_has_ipv6_address(const char *nwid);

/**
 * Returns whether any address has been assigned to the SockTap for this network
 * - This is used as an indicator of readiness for service for the ZeroTier core and stack
 */
int zts_has_address(const char *nwid);

/**
 * Get IPV4 Address for this device on a given network
 * FIXME: Only returns first address found for given protocol and network (should be enough for now)
 */
void zts_get_ipv4_address(const char *nwid, char *addrstr, const int addrlen);

/**
 * Get IPV6 Address for this device on a given network
 * FIXME: Only returns first address found for given protocol and network (should be enough for now)
 */
void zts_get_ipv6_address(const char *nwid, char *addrstr, const int addrlen);

/**
 * Returns a 6PLANE IPv6 address given a network ID and zerotier ID
 */
void zts_get_6plane_addr(char *addr, const char *nwid, const char *devID);

/**
 * Returns an RFC 4193 IPv6 address given a network ID and zerotier ID
 */
void zts_get_rfc4193_addr(char *addr, const char *nwid, const char *devID);

/**
 * Return the number of peers on this network
 */
unsigned long zts_get_peer_count();

/**
 * Get the IP address of a peer if a direct path is available
 */
int zts_get_peer_address(char *peer, const char *devID);

/**
 * Enable HTTP control plane (traditionally used by zerotier-cli)
 * - Allows one to control the ZeroTier core via HTTP requests
 * FIXME: Implement
 */
void zts_enable_http_control_plane();

/**
 * Disable HTTP control plane (traditionally used by zerotier-cli)
 * - Allows one to control the ZeroTier core via HTTP requests
 * FIXME: Implement
 */
void zts_disable_http_control_plane();

/****************************************************************************/
/* SDK Socket API (Socket User Controls)                                    */
/* - These functions are designed to work just like regular socket calls    */
/*   but are provisioned and handled by ZeroTier                            */
/* Implemented in Socket.c                                                  */ 
/****************************************************************************/

/**
 * Creates a socket
 */
int zts_socket(ZT_SOCKET_SIG);

 /**
 * Connect a socket to a remote host
 */
int zts_connect(ZT_CONNECT_SIG);

/**
 * Binds a socket to a specific address
 *  - To accept VirtualSockets on a specific ZeroTier network you must
 *    use this bind call with an address which is associated with that network
 *
 *  For instance, given the following networks:
 *     - nwid = 97afaf1963cc6a90 (10.9.0.0/24)
 *     - nwid = 23bfae5663c8b188 (192.168.0.0/24)
 *
 *  In order to accept a VirtualSocket on 97afaf1963cc6a90, you 
 *  should bind to 10.9.0.0
 */
int zts_bind(ZT_BIND_SIG);

/**
 * Listen for incoming VirtualSockets
 */
int zts_listen(ZT_LISTEN_SIG);

/**
 * Accept a VirtualSocket
 */
int zts_accept(ZT_ACCEPT_SIG);

/**
 * Accept a VirtualSocket
 */
#if defined(__linux__)
	int zts_accept4(ZT_ACCEPT4_SIG);
#endif

/**
 * Set socket options
 */
int zts_setsockopt(ZT_SETSOCKOPT_SIG);

/**
 * Get socket options
 */
int zts_getsockopt(ZT_GETSOCKOPT_SIG);

/**
 * Get socket name
 */
int zts_getsockname(ZT_GETSOCKNAME_SIG);

/**
 * Get a peer name
 */
int zts_getpeername(ZT_GETPEERNAME_SIG);

/**
 * Gets current hostname
 */
int zts_gethostname(ZT_GETHOSTNAME_SIG);

/**
 * Sets current hostname
 */
int zts_sethostname(ZT_SETHOSTNAME_SIG);

/**
 * Close a socket
 * TODO: Check that closing a socket immediately after writing doesn't behave in 
 * an undefined manner
 */
int zts_close(ZT_CLOSE_SIG);

/**
 * waits for one of a set of file descriptors to become ready to perform I/O.
 */
int zts_poll(ZT_POLL_SIG);

/**
 * monitor multiple file descriptors, waiting until one or more of the file descriptors become "ready"
 */
int zts_select(ZT_SELECT_SIG);

/**
 * Issue file control commands on a socket
 */
int zts_fcntl(ZT_FCNTL_SIG);

/**
 * Control a device
 */
int zts_ioctl(ZT_IOCTL_SIG);

/**
 * Send data to a remote host
 */
ssize_t zts_send(ZT_SEND_SIG);

/**
 * Send data to a remote host
 */
ssize_t zts_sendto(ZT_SENDTO_SIG);

/**
 * Send a message to a remote host
 */
ssize_t zts_sendmsg(ZT_SENDMSG_SIG);

/**
 * Receive data from a remote host
 */
ssize_t zts_recv(ZT_RECV_SIG);

/**
 * Receive data from a remote host
 */
ssize_t zts_recvfrom(ZT_RECVFROM_SIG);

/**
 * Receive a message from a remote host
 */
ssize_t zts_recvmsg(ZT_RECVMSG_SIG);

/**
 * Read bytes from socket onto buffer
 *  - Note, this function isn't strictly necessary, you can
 *    use a regular read() call as long as the socket fd was
 *    created via a zts_socket() call. 
 */
int zts_read(ZT_READ_SIG);

/**
 * Write bytes from buffer to socket
 *  - Note, this function isn't strictly necessary, you can
 *    use a regular write() call as long as the socket fd was
 *    created via a zts_socket() call. 
 */
int zts_write(ZT_WRITE_SIG);

/*
 * Sends a FIN segment
 */
int zts_shutdown(ZT_SHUTDOWN_SIG);

/*
 * Returns a vector of network routes { target, via, metric, etc... }
 */
std::vector<ZT_VirtualNetworkRoute> *zts_get_network_routes(char *nwid);

/*
 * Adds a DNS nameserver for the network stack to use
 */
int zts_add_dns_nameserver(struct sockaddr *addr);

/*
 * Removes a DNS nameserver
 */
int zts_del_dns_nameserver(struct sockaddr *addr);

/****************************************************************************/
/* SDK Socket API Helper functions/objects --- DONT CALL THESE DIRECTLY     */
/****************************************************************************/

namespace ZeroTier
{
	class picoTCP;
	extern ZeroTier::picoTCP *picostack;

	class lwIP;
	extern ZeroTier::lwIP *lwipstack;

	class VirtualTap;
	struct VirtualSocket;
	struct InetAddress;
}

/*
 * Gets a pointer to a pico_socket given a file descriptor
 */
#if defined(STACK_PICO)
int zts_get_pico_socket(int fd, struct pico_socket **s);
#endif

/*
 * Whether we can add a new socket or not. Depends on stack in use
 */
bool can_provision_new_socket();

/**
 * Returns the number of sockets either already provisioned or waiting to be
 * Some network stacks may have a limit on the number of sockets that they can
 * safely handle due to timer construction, this is a way to check that we
 * haven't passed that limit. Someday if multiple stacks are used simultaneously
 * the logic for this function should change accordingly.
 */
int zts_nsockets();

/*
 * Returns maximum number of sockets allowed by network stack
 */
int zts_maxsockets();

int pico_ntimers();

/****************************************************************************/
/* ZeroTier Core helper functions for libzt - DON'T CALL THESE DIRECTLY     */
/****************************************************************************/

ZeroTier::VirtualTap *getTapByNWID(uint64_t nwid);
ZeroTier::VirtualTap *getTapByAddr(ZeroTier::InetAddress *addr);
ZeroTier::VirtualTap *getTapByName(char *ifname);
ZeroTier::VirtualTap *getTapByIndex(int index);
ZeroTier::VirtualTap *getAnyTap();

/*
 * Returns a pointer to a VirtualSocket for a given fd
 */
ZeroTier::VirtualSocket *get_virtual_socket(int fd);

/*
 * Removes a VirtualSocket
 */
void del_virtual_socket(int fd);

/*
 * Adds a virtualSocket
 */
void add_unassigned_virtual_socket(int fd, ZeroTier::VirtualSocket *vs);
/*
 * Removes unassigned VirtualSocket
 */
void del_unassigned_virtual_socket(int fd);

/*
 * Adds an assigned VirtualSocket
 */
void add_assigned_virtual_socket(ZeroTier::VirtualTap *tap, ZeroTier::VirtualSocket *vs, int fd);

/*
 * Removes an assigned VirtualSocket
 */
void del_assigned_virtual_socket(ZeroTier::VirtualTap *tap, ZeroTier::VirtualSocket *vs, int fd);

/*
 * Destroys all virtual tap devices
 */
void dismantleTaps();

/*
 * Get device ID (from file)
 */
int zts_get_device_id_from_file(const char *filepath, char *devID);

/**
 * Don't call this directly, use 'zts_start()'
 */
void *zts_start_service(void *thread_id);

/*
 *
 */
void handle_general_failure();

#include "Debug.hpp"

#ifdef __cplusplus
} // extern "C"
#endif

#endif // ZT_ZEROTIERSDK_H
