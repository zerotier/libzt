/**
 * @file
 *
 * libzt application-facing POSIX-like socket API
 */

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
/*
  Specifies the polling interval and the callback function that should
  be called to poll the application. The interval is specified in
  number of TCP coarse grained timer shots, which typically occurs
  twice a second. An interval of 10 means that the application would
  be polled every 5 seconds. */
#define LWIP_APPLICATION_POLL_FREQ         2

#define LWIP_TCP_TIMER_INTERVAL            25

// How often we check VirtualSocket statuses (in ms)
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

#define ZT_MAX_SOCKETS                     1024
#define ZT_SDK_MTU                         ZT_MAX_MTU
#define ZT_LEN_SZ                          4
#define ZT_ADDR_SZ                         128
#define ZT_SOCKET_MSG_BUF_SZ               ZT_SDK_MTU + ZT_LEN_SZ + ZT_ADDR_SZ


#define ZT_PHY_POLL_INTERVAL                2 // ms
#define ZT_ACCEPT_RECHECK_DELAY            100 // ms (for blocking zts_accept() calls)
#define ZT_CONNECT_RECHECK_DELAY           100 // ms (for blocking zts_connect() calls)
#define ZT_API_CHECK_INTERVAL              100 // ms

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
/****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/** \brief Start core ZeroTier service (generates cryptographic identity).
 *         
 *
 * ...
 * Errno values:
 * ~~~
 * Value  | Meaning
 * ------ | ----------------
 * EINVAL | Invalid argument
 * ~~~
 */
void zts_start(const char *path);

/**
 *  @brief      Should be called at the beginning of your application  
 *
 *  @usage      Blocks until all of the following conditions are met:
 *                - ZeroTier core service has been initialized
 *                - Cryptographic identity has been generated or loaded from directory specified by `path`
 *                - Virtual network is successfully joined
 *                - IP address is assigned by network controller service 
 * 
 *  @param      path directory where cryptographic identities and network configuration files are stored and retrieved 
 *              (`identity.public`, `identity.secret`)
 *  @param      nwid the 16-digit hexidecimal network identifier (e.g. Earth: `8056c2e21c000001`)
 *
 *  @return     0 if successful; or 1 if failed
 */
void zts_simple_start(const char *path, const char *nwid);

/**
 *  @brief      Stops the ZeroTier core service and disconnects from all virtual networks                
 *
 *  @usage      Called at the end of your application. This call will block until everything is shut down
 *  
 *  @return
 * 
 */
void zts_stop();

/**
 *  @brief      Joins a virtual network                
 *
 *  @usage      Called after zts_start() or zts_simple_start()
 * 
 *  @param      nwid the 16-digit hexidecimal network identifier
 *
 *  @return
 */
void zts_join(const char * nwid);

/**
 *  @brief      Joins a network (eventually), this will create the dir and conf file required, don't instruct the core 
 *              to do anything              
 *
 *  @usage      Candidate for deletion
 * 
 *  @param      filepath path to the `*.conf` file named after the network
 *
 *  @return
 */
void zts_join_soft(const char * filepath, const char * nwid);

/**
 *  @brief      Leaves a virtual network.
 *
 *  @usage      
 * 
 *  @param      nwid
 *
 *  @return
 *
 */
void zts_leave(const char * nwid);

/**
 *  @brief      Leave a network - Only delete the .conf file, this will prevent the service from joining upon next startup
 *
 *  @usage      
 * 
 *  @param      filepath
 *  @param      nwid
 *
 *  @return     
 */
void zts_leave_soft(const char * filepath, const char * nwid);

/**
 *  @brief      Return the home path for this instance of ZeroTier
 *
 *  @usage      
 * 
 *  @param      homePath
 *  @param      len
 *
 *  @return     
 */
void zts_get_homepath(char *homePath, const int len);

/**
 *  @brief      Copies ZeroTier core version string into `ver`
 *
 *  @usage      
 * 
 *  @param      ver
 *
 *  @return     
 *
 */
void zts_core_version(char *ver);

/**
 *  @brief      Copies libzt version string into `ver`
 *
 *  @usage      
 * 
 *  @param      ver
 *
 *  @return     
 *
 * Provides core libzt service version
 */
void zts_lib_version(char *ver);

/**
 *  @brief      Get device ID (10-digit hex + NULL byte)
 *
 *  @usage      
 * 
 *  @param      devID   
 *
 *  @return     
 */
int zts_get_device_id(char *devID);

/**
 *  @brief      Check whether the service is running
 *
 *  @usage      
 *  @return     
 */
int zts_running();

/**
 *  @brief      Returns whether any IPv6 address has been assigned to the SockTap for this network
 *
 *  @usage      This is used as an indicator of readiness for service for the ZeroTier core and stack
 * 
 *  @param      nwid    
 *
 *  @return     
 */
int zts_has_ipv4_address(const char *nwid);

/**
 *  @brief      Returns whether any IPv4 address has been assigned to the SockTap for this network
 *
 *  @usage      This is used as an indicator of readiness for service for the ZeroTier core and stack
 * 
 *  @param      nwid
 *  @return     
 */
int zts_has_ipv6_address(const char *nwid);

/**
 *  @brief      Returns whether any address has been assigned to the SockTap for this network
 *
 *  @usage      This is used as an indicator of readiness for service for the ZeroTier core and stack
 * 
 *  @param      nwid
 *  @return     
 */
int zts_has_address(const char *nwid);

/**
 *  @brief      Get IPV4 Address for this device on a given network
 *
 *  @usage      FIXME: Only returns first address found for given protocol and network (should be enough for now)
 * 
 *  @param      nwid
 *  @param      addrstr
 *  @param      addrlen
 *
 *  @return     
 */
void zts_get_ipv4_address(const char *nwid, char *addrstr, const int addrlen);

/**
 *  @brief      Get IPV6 Address for this device on a given network
 *
 *  @usage      FIXME: Only returns first address found for given protocol and network (should be enough for now)
 * 
 *  @param      
 *  @param      
 *  @param      
 *
 *  @return     
 */
void zts_get_ipv6_address(const char *nwid, char *addrstr, const int addrlen);

/**
 *  @brief      Returns a 6PLANE IPv6 address given a network ID and zerotier ID
 *
 *  @usage      
 * 
 *  @param      
 *  @param      
 *  @param      
 *
 *  @return     
 */
void zts_get_6plane_addr(char *addr, const char *nwid, const char *devID);

/**
 *  @brief      Returns an RFC 4193 IPv6 address given a network ID and zerotier ID
 *
 *  @usage      
 * 
 *  @param      
 *  @param      
 *  @param      
 *
 *  @return     
 */
void zts_get_rfc4193_addr(char *addr, const char *nwid, const char *devID);

/**
 *  @brief      Return the number of peers on this network
 *
 *  @usage      
 * 
 *  @param      
 *  @param      
 *  @param      
 *
 *  @return     
 */
unsigned long zts_get_peer_count();

/**
 *  @brief      Get the IP address of a peer if a direct path is available
 *
 *  @usage      
 * 
 *  @param      peer
 *  @param      devID
 *
 *  @return     
 */
int zts_get_peer_address(char *peer, const char *devID);

/**
 *  @brief      Enable HTTP control plane (traditionally used by zerotier-cli)
 *              - Allows one to control the ZeroTier core via HTTP requests
 *              FIXME: Implement
 *
 *  @usage          
 *
 *  @return     
 */
void zts_enable_http_control_plane();

/**
 *  @brief      Disable HTTP control plane (traditionally used by zerotier-cli)
 *              - Allows one to control the ZeroTier core via HTTP requests
 *              FIXME: Implement
 *
 *  @usage          
 *
 *  @return     
 */
void zts_disable_http_control_plane();

/****************************************************************************/
/* SDK Socket API (Socket User Controls)                                    */
/* - These functions are designed to work just like ordinary socket calls   */
/*   but are provisioned and handled by ZeroTier                            */
/****************************************************************************/

/**
 *  @brief      Create a socket
 *
 *  @usage      
 * 
 *  @param      
 *  @param      
 *  @param      
 *
 *  @return     
 */
int zts_socket(int socket_family, int socket_type, int protocol);

/**
 *  @brief      Connect a socket to a remote host
 *
 *  @usage      
 * 
 *  @param      
 *  @param      
 *  @param      
 *
 *  @return     
 */
int zts_connect(int fd, const struct sockaddr *addr, socklen_t addrlen);

/**
 *  @brief      Bind a socket to an interface (VirtualTap)
 *
 *  @usage      
 * 
 *  @param      
 *  @param      
 *  @param      
 *
 *  @return     
 */
int zts_bind(int fd, const struct sockaddr *addr, socklen_t addrlen);

/**
 *  @brief      Listen for incoming VirtualSockets
 *
 *  @usage      
 * 
 *  @param      
 *  @param      
 *  @param      
 *
 *  @return     
 */
int zts_listen(int fd, int backlog);

/**
 *  @brief      Accept a VirtualSocket
 *
 *  @usage      
 * 
 *  @param      
 *  @param      
 *  @param      
 *
 *  @return     
 */
int zts_accept(int fd, struct sockaddr *addr, socklen_t *addrlen);

/**
 *  @brief      Accept a VirtualSocket
 *
 *  @usage      
 * 
 *  @param      
 *  @param      
 *  @param      
 *
 *  @return     
 */
#if defined(__linux__)
	int zts_accept4(int fd, struct sockaddr *addr, socklen_t *addrlen, int flags);
#endif

/**
 *  @brief      Set socket options
 *
 *  @usage      
 * 
 *  @param      
 *  @param      
 *  @param      
 *
 *  @return     
 */
int zts_setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen);

/**
 *  @brief      Get socket options
 *
 *  @usage      
 * 
 *  @param      
 *  @param      
 *  @param      
 *
 *  @return     
 */
int zts_getsockopt(int fd, int level, int optname, void *optval, socklen_t *optlen);

/**
 *  @brief      Get socket name
 *
 *  @usage      
 * 
 *  @param      
 *  @param      
 *  @param      
 *
 *  @return     
 */
int zts_getsockname(int fd, struct sockaddr *addr, socklen_t *addrlen);

/**
 *  @brief      Get a peer name
 *
 *  @usage      
 * 
 *  @param      
 *  @param      
 *  @param      
 *
 *  @return     
 */
int zts_getpeername(int fd, struct sockaddr *addr, socklen_t *addrlen);

/**
 *  @brief      Gets current hostname
 *
 *  @usage      
 * 
 *  @param      
 *  @param      
 *  @param      
 *
 *  @return     
 */
int zts_gethostname(char *name, size_t len);

/**
 *  @brief      Sets current hostname
 *
 *  @usage      
 * 
 *  @param      
 *  @param      
 *  @param      
 *
 *  @return     
 */
int zts_sethostname(const char *name, size_t len);

/**
 *  @brief      lose a socket
 *
 *  @usage      
 * 
 *  @param      fd
 *  @param      
 *  @param      
 *
 *  @return     
 */
int zts_close(int fd);

/**
 *  @brief      waits for one of a set of file descriptors to become ready to perform I/O.
 *
 *  @usage      
 * 
 *  @param      fds
 *  @param      nfds
 *  @param      timeout
 *
 *  @return     
 */
int zts_poll(struct pollfd *fds, nfds_t nfds, int timeout);

/**
 *  @brief      monitor multiple file descriptors, waiting until one or more of the file descriptors become "ready"
 *
 *  @usage      
 * 
 *  @param      nfds
 *  @param      readfds
 *  @param      writefds
 *  @param      exceptfds
 *  @param      timeout
 *
 *  @return     
 */
int zts_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

/**
 *  @brief      Issue file control commands on a socket
 *
 *  @usage      
 * 
 *  @param      fd
 *  @param      cmd
 *  @param      flags
 *
 *  @return     
 * 
 */
int zts_fcntl(int fd, int cmd, int flags);

/**
 *  @brief    Control a device  
 *
 *  @usage      
 * 
 *  @param      fd
 *  @param      request
 *  @param      argp
 *
 *  @return     
 */
int zts_ioctl(int fd, unsigned long request, void *argp);

/**
 *  @brief      Send data to remote host
 *
 *  @usage      
 * 
 *  @param      fd
 *  @param      buf
 *  @param      len
 *  @param      flags
 *
 *  @return     
 */
ssize_t zts_send(int fd, const void *buf, size_t len, int flags);

/**
 *  @brief      Send data to remote host
 *
 *  @usage      
 * 
 *  @param      fd
 *  @param      buf
 *  @param      len
 *  @param      flags
 *  @param      addr
 *  @param      addrlen
 *
 *  @return     
 */
ssize_t zts_sendto(int fd, const void *buf, size_t len, int flags, const struct sockaddr *addr, socklen_t addrlen);

/**
 *  @brief      Send message to remote host
 *
 *  @usage      
 * 
 *  @param      fd
 *  @param      msg
 *  @param      flags
 *
 *  @return     
 */
ssize_t zts_sendmsg(int fd, const struct msghdr *msg, int flags);

/**
 *  @brief      Receive data from remote host
 *
 *  @usage      
 * 
 *  @param      fd
 *  @param      buf
 *  @param      len
 *  @param      flags
 *
 *  @return     
 */
ssize_t zts_recv(int fd, void *buf, size_t len, int flags);

/**
 *  @brief      Receive data from remote host
 *
 *  @usage      
 * 
 *  @param      fd
 *  @param      buf
 *  @param      len
 *  @param      flags
 *  @param      addr
 *  @param      addrlen
 *
 *  @return     
 */
ssize_t zts_recvfrom(int fd, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t *addrlen);

/**
 *  @brief      Receive a message from remote host
 *
 *  @usage      
 * 
 *  @param      fd
 *  @param      msg
 *  @param      flags
 *
 *  @return     
 */
ssize_t zts_recvmsg(int fd, struct msghdr *msg,int flags);

/**
 *  @brief      Read bytes from socket onto buffer
 *
 *  @usage      Note, this function isn't strictly necessary, you can use a regular read() 
 *              call as long as the socket file descriptor was created via a zts_socket() call.
 * 
 *  @param      fd
 *  @param      buf
 *  @param      len
 *
 *  @return     
 *
 */
int zts_read(int fd, void *buf, size_t len);

/**
 *  @brief      Write bytes from buffer to socket
 *
 *  @usage      Note, this function isn't strictly necessary, you can use a regular write() 
 *              call as long as the socket file descriptor was created via a zts_socket() call.
 * 
 *  @param      fd
 *  @param      buf
 *  @param      len
 *
 *  @return     
 */
int zts_write(int fd, const void *buf, size_t len);

/**
 *  @brief      Sends a FIN segment
 *
 *  @usage      
 * 
 *  @param      fd
 *  @param      how
 *
 *  @return     
 */
int zts_shutdown(int fd, int how);

/**
 *  @brief      Returns a vector of network routes { target, via, metric, etc... }
 *
 *  @usage      
 * 
 *  @param      nwid   
 *
 *  @return     
 */
std::vector<ZT_VirtualNetworkRoute> *zts_get_network_routes(char *nwid);

/**
 *  @brief      Adds a DNS nameserver for the network stack to use
 *
 *  @usage      
 * 
 *  @param      addr   
 *
 *  @return     
 */
int zts_add_dns_nameserver(struct sockaddr *addr);

/**
 *  @brief      Removes a DNS nameserver
 *
 *  @usage      
 * 
 *  @param      addr     
 *
 *  @return     
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
	class VirtualSocket;
	struct InetAddress;
}

/**
 *  @brief      Whether we can add a new socket or not. Depends on stack in use
 *
 *  @usage      
 * 
 *  @param      socket_type
 *  @param      
 *  @param      
 *
 *  @return     
 */
bool can_provision_new_socket(int socket_type);

/**
 *  @brief      Returns the number of sockets either already provisioned or waiting to be
 * Some network stacks may have a limit on the number of sockets that they can
 * safely handle due to timer construction, this is a way to check that we
 * haven't passed that limit. Someday if multiple stacks are used simultaneously
 * the logic for this function should change accordingly.
 *
 *  @usage          
 *
 *  @return     
 */
int zts_num_active_virt_sockets();

/**
 *  @brief      Returns maximum number of sockets allowed by network stack
 *
 *  @usage      
 * 
 *  @param      socket_type   
 *
 *  @return     
 */
int zts_maxsockets(int socket_type);

int pico_ntimers();

/****************************************************************************/
/* ZeroTier Core helper functions for libzt - DON'T CALL THESE DIRECTLY     */
/****************************************************************************/

ZeroTier::VirtualTap *getTapByNWID(uint64_t nwid);
ZeroTier::VirtualTap *getTapByAddr(ZeroTier::InetAddress *addr);
ZeroTier::VirtualTap *getTapByName(char *ifname);
ZeroTier::VirtualTap *getTapByIndex(int index);
ZeroTier::VirtualTap *getAnyTap();

/**
 *  @brief      Returns a pointer to a VirtualSocket for a given file descriptor
 *
 *  @usage      Don't call this directly from application. For internal use only.  
 * 
 *  @return     
 * 
 */
ZeroTier::VirtualSocket *get_virt_socket(int fd);

/**
 *  @brief      Removes a VirtualSocket
 *
 *  @usage      Don't call this directly from application. For internal use only.  
 * 
 *  @param      fd   
 *
 *  @return     
 * 
 */
int del_virt_socket(int fd);

/**
 *  @brief      Adds a virtualSocket
 *
 *  @usage      Don't call this directly from application. For internal use only.  
 * 
 *  @param      fd
 *  @param      vs
 *
 *  @return     
 * 
 */
int add_unassigned_virt_socket(int fd, ZeroTier::VirtualSocket *vs);

/**
 *  @brief      Removes unassigned VirtualSocket
 *
 *  @usage      Don't call this directly from application. For internal use only.  
 * 
 *  @param      fd
 *
 *  @return     
 * 
 */
int del_unassigned_virt_socket(int fd);

/**
 *  @brief      Adds an assigned VirtualSocket
 *
 *  @usage      Don't call this directly from application. For internal use only.  
 * 
 *  @param      tap
 *  @param      vs
 *  @param      fd
 *
 *  @return     
 * 
 */
int add_assigned_virt_socket(ZeroTier::VirtualTap *tap, ZeroTier::VirtualSocket *vs, int fd);

/**
 *  @brief      Removes an assigned VirtualSocket
 *
 *  @usage      Don't call this directly from application. For internal use only.  
 * 
 *  @param      tap
 *  @param      vs
 *  @param      fd
 *
 *  @return     
 * 
 */
int del_assigned_virt_socket(ZeroTier::VirtualTap *tap, ZeroTier::VirtualSocket *vs, int fd);

/**
 *  @brief      Gets a pair of associated virtual objects (VirtualSocket bound to a VirtualTap)
 *
 *  @usage      Don't call this directly from application. For internal use only.  
 * 
 *  @param      fd  
 *
 *  @return     
 * 
 */
std::pair<ZeroTier::VirtualSocket*, ZeroTier::VirtualTap*> *get_assigned_virtual_pair(int fd);

/**
 *  @brief      Disables all VirtualTap devices
 *
 *  @usage      Don't call this directly from application. For internal use only.    
 *
 *  @return     
 * 
 */
void disableTaps();

/**
 *  @brief      Reads a 10-digit hexidecimal device ID (aka. nodeId, ztAddress, etc. from file)
 *
 *  @usage      Don't call this directly from application. For internal use only.
 * 
 *  @param      filepath
 *  @param      devID
 *
 *  @return     
 * 
 */
int zts_get_device_id_from_file(const char *filepath, char *devID);

/**
 *  @brief      Starts an instance of the ZeroTier core service.
 *
 *  @usage      Don't call this directly from application. For internal use only.
 * 
 *  @param      thread_id    
 *
 *  @return     
 * 
 */
void *zts_start_service(void *thread_id);

/**
 *  @brief      Should be called wherever libzt enters a condition where undefined behaviour might occur.  
 *
 *  @usage      Don't call this directly from application. For internal use only.  
 *
 *  @return     
 *  
 */
void handle_general_failure();

#include "Debug.hpp"

#ifdef __cplusplus
} // extern "C"
#endif

#endif // ZT_ZEROTIERSDK_H
