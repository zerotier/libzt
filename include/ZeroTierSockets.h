/*
 * Copyright (c)2013-2021 ZeroTier, Inc.
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file in the project's root directory.
 *
 * Change Date: 2025-01-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2.0 of the Apache License.
 */
/****/

/**
 * @file
 *
 * This defines the external C API for ZeroTier Sockets
 */

#ifndef ZT_SOCKETS_H
#define ZT_SOCKETS_H

//////////////////////////////////////////////////////////////////////////////
// Configuration Options                                                    //
//////////////////////////////////////////////////////////////////////////////

#if !defined(ZTS_ENABLE_PYTHON) && !defined(ZTS_ENABLE_PINVOKE)
#define ZTS_C_API_ONLY 1
#endif

#if !ZTS_NO_STDINT_H
#include <stdint.h>
#endif

#if defined(_MSC_VER)
	#ifndef ssize_t
		// TODO: Should be SSIZE_T, would require lwIP patch
		// #include <BaseTsd.h>
		// typedef SSIZE_T ssize_t;
		typedef int ssize_t;
	#endif
#else
	#include <unistd.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ZTS_ENABLE_PINVOKE
	// Used by P/INVOKE wrappers
	typedef void (*CppCallback)(void *msg);
#endif

//////////////////////////////////////////////////////////////////////////////
// Event codes                                                              //
//////////////////////////////////////////////////////////////////////////////

// Node events
#define ZTS_EVENT_NODE_UP                  200
#define ZTS_EVENT_NODE_ONLINE              201
#define ZTS_EVENT_NODE_OFFLINE             202
#define ZTS_EVENT_NODE_DOWN                203
#define ZTS_EVENT_NODE_IDENTITY_COLLISION  204
#define ZTS_EVENT_NODE_UNRECOVERABLE_ERROR 205
#define ZTS_EVENT_NODE_NORMAL_TERMINATION  206
// Network events
#define ZTS_EVENT_NETWORK_NOT_FOUND        210
#define ZTS_EVENT_NETWORK_CLIENT_TOO_OLD   211
#define ZTS_EVENT_NETWORK_REQ_CONFIG       212
#define ZTS_EVENT_NETWORK_OK               213
#define ZTS_EVENT_NETWORK_ACCESS_DENIED    214
#define ZTS_EVENT_NETWORK_READY_IP4        215
#define ZTS_EVENT_NETWORK_READY_IP6        216
#define ZTS_EVENT_NETWORK_READY_IP4_IP6    217
#define ZTS_EVENT_NETWORK_DOWN             218
#define ZTS_EVENT_NETWORK_UPDATE           219
// Network Stack events
#define ZTS_EVENT_STACK_UP                 220
#define ZTS_EVENT_STACK_DOWN               221
// lwIP netif events
#define ZTS_EVENT_NETIF_UP                 230
#define ZTS_EVENT_NETIF_DOWN               231
#define ZTS_EVENT_NETIF_REMOVED            232
#define ZTS_EVENT_NETIF_LINK_UP            233
#define ZTS_EVENT_NETIF_LINK_DOWN          234
// Peer events
#define ZTS_EVENT_PEER_DIRECT              240
#define ZTS_EVENT_PEER_RELAY               241
#define ZTS_EVENT_PEER_UNREACHABLE         242
#define ZTS_EVENT_PEER_PATH_DISCOVERED     243
#define ZTS_EVENT_PEER_PATH_DEAD           244
// Route events
#define ZTS_EVENT_ROUTE_ADDED              250
#define ZTS_EVENT_ROUTE_REMOVED            251
// Address events
#define ZTS_EVENT_ADDR_ADDED_IP4           260
#define ZTS_EVENT_ADDR_REMOVED_IP4         261
#define ZTS_EVENT_ADDR_ADDED_IP6           262
#define ZTS_EVENT_ADDR_REMOVED_IP6         263

//////////////////////////////////////////////////////////////////////////////
// Return Error codes                                                       //
//////////////////////////////////////////////////////////////////////////////

#define ZTS_ERR_OK            0 // No error
#define ZTS_ERR_SOCKET       -1 // Socket error, see zts_errno
#define ZTS_ERR_SERVICE      -2 // You probably did something at the wrong time
#define ZTS_ERR_ARG          -3 // Invalid argument
#define ZTS_ERR_NO_RESULT    -4 // No result (not necessarily an error)
#define ZTS_ERR_GENERAL      -5 // Consider filing a bug report

//////////////////////////////////////////////////////////////////////////////
// zts_errno Error codes                                                    //
//////////////////////////////////////////////////////////////////////////////

// Error variable set after each zts_* call to provide additional information.
extern int zts_errno;

#define  ZTS_EPERM            1  /* Operation not permitted */
#define  ZTS_ENOENT           2  /* No such file or directory */
#define  ZTS_ESRCH            3  /* No such process */
#define  ZTS_EINTR            4  /* Interrupted system call */
#define  ZTS_EIO              5  /* I/O error */
#define  ZTS_ENXIO            6  /* No such device or address */
#define  ZTS_E2BIG            7  /* Arg list too long */
#define  ZTS_ENOEXEC          8  /* Exec format error */
#define  ZTS_EBADF            9  /* Bad file number */
#define  ZTS_ECHILD          10  /* No child processes */
#define  ZTS_EAGAIN          11  /* Try again */
#define  ZTS_ENOMEM          12  /* Out of memory */
#define  ZTS_EACCES          13  /* Permission denied */
#define  ZTS_EFAULT          14  /* Bad address */
#define  ZTS_ENOTBLK         15  /* Block device required */
#define  ZTS_EBUSY           16  /* Device or resource busy */
#define  ZTS_EEXIST          17  /* File exists */
#define  ZTS_EXDEV           18  /* Cross-device link */
#define  ZTS_ENODEV          19  /* No such device */
#define  ZTS_ENOTDIR         20  /* Not a directory */
#define  ZTS_EISDIR          21  /* Is a directory */
#define  ZTS_EINVAL          22  /* Invalid argument */
#define  ZTS_ENFILE          23  /* File table overflow */
#define  ZTS_EMFILE          24  /* Too many open files */
#define  ZTS_ENOTTY          25  /* Not a typewriter */
#define  ZTS_ETXTBSY         26  /* Text file busy */
#define  ZTS_EFBIG           27  /* File too large */
#define  ZTS_ENOSPC          28  /* No space left on device */
#define  ZTS_ESPIPE          29  /* Illegal seek */
#define  ZTS_EROFS           30  /* Read-only file system */
#define  ZTS_EMLINK          31  /* Too many links */
#define  ZTS_EPIPE           32  /* Broken pipe */
#define  ZTS_EDOM            33  /* Math argument out of domain of func */
#define  ZTS_ERANGE          34  /* Math result not representable */
#define  ZTS_EDEADLK         35  /* Resource deadlock would occur */
#define  ZTS_ENAMETOOLONG    36  /* File name too long */
#define  ZTS_ENOLCK          37  /* No record locks available */
#define  ZTS_ENOSYS          38  /* Function not implemented */
#define  ZTS_ENOTEMPTY       39  /* Directory not empty */
#define  ZTS_ELOOP           40  /* Too many symbolic links encountered */
#define  ZTS_EWOULDBLOCK     ZTS_EAGAIN  /* Operation would block */
#define  ZTS_ENOMSG          42  /* No message of desired type */
#define  ZTS_EIDRM           43  /* Identifier removed */
#define  ZTS_ECHRNG          44  /* Channel number out of range */
#define  ZTS_EL2NSYNC        45  /* Level 2 not synchronized */
#define  ZTS_EL3HLT          46  /* Level 3 halted */
#define  ZTS_EL3RST          47  /* Level 3 reset */
#define  ZTS_ELNRNG          48  /* Link number out of range */
#define  ZTS_EUNATCH         49  /* Protocol driver not attached */
#define  ZTS_ENOCSI          50  /* No CSI structure available */
#define  ZTS_EL2HLT          51  /* Level 2 halted */
#define  ZTS_EBADE           52  /* Invalid exchange */
#define  ZTS_EBADR           53  /* Invalid request descriptor */
#define  ZTS_EXFULL          54  /* Exchange full */
#define  ZTS_ENOANO          55  /* No anode */
#define  ZTS_EBADRQC         56  /* Invalid request code */
#define  ZTS_EBADSLT         57  /* Invalid slot */

#define  ZTS_EDEADLOCK       ZTS_EDEADLK

#define  ZTS_EBFONT          59  /* Bad font file format */
#define  ZTS_ENOSTR          60  /* Device not a stream */
#define  ZTS_ENODATA         61  /* No data available */
#define  ZTS_ETIME           62  /* Timer expired */
#define  ZTS_ENOSR           63  /* Out of streams resources */
#define  ZTS_ENONET          64  /* Machine is not on the network */
#define  ZTS_ENOPKG          65  /* Package not installed */
#define  ZTS_EREMOTE         66  /* Object is remote */
#define  ZTS_ENOLINK         67  /* Link has been severed */
#define  ZTS_EADV            68  /* Advertise error */
#define  ZTS_ESRMNT          69  /* Srmount error */
#define  ZTS_ECOMM           70  /* Communication error on send */
#define  ZTS_EPROTO          71  /* Protocol error */
#define  ZTS_EMULTIHOP       72  /* Multihop attempted */
#define  ZTS_EDOTDOT         73  /* RFS specific error */
#define  ZTS_EBADMSG         74  /* Not a data message */
#define  ZTS_EOVERFLOW       75  /* Value too large for defined data type */
#define  ZTS_ENOTUNIQ        76  /* Name not unique on network */
#define  ZTS_EBADFD          77  /* File descriptor in bad state */
#define  ZTS_EREMCHG         78  /* Remote address changed */
#define  ZTS_ELIBACC         79  /* Can not access a needed shared library */
#define  ZTS_ELIBBAD         80  /* Accessing a corrupted shared library */
#define  ZTS_ELIBSCN         81  /* .lib section in a.out corrupted */
#define  ZTS_ELIBMAX         82  /* Attempting to link in too many shared libraries */
#define  ZTS_ELIBEXEC        83  /* Cannot exec a shared library directly */
#define  ZTS_EILSEQ          84  /* Illegal byte sequence */
#define  ZTS_ERESTART        85  /* Interrupted system call should be restarted */
#define  ZTS_ESTRPIPE        86  /* Streams pipe error */
#define  ZTS_EUSERS          87  /* Too many users */
#define  ZTS_ENOTSOCK        88  /* Socket operation on non-socket */
#define  ZTS_EDESTADDRREQ    89  /* Destination address required */
#define  ZTS_EMSGSIZE        90  /* Message too long */
#define  ZTS_EPROTOTYPE      91  /* Protocol wrong type for socket */
#define  ZTS_ENOPROTOOPT     92  /* Protocol not available */
#define  ZTS_EPROTONOSUPPORT 93  /* Protocol not supported */
#define  ZTS_ESOCKTNOSUPPORT 94  /* Socket type not supported */
#define  ZTS_EOPNOTSUPP      95  /* Operation not supported on transport endpoint */
#define  ZTS_EPFNOSUPPORT    96  /* Protocol family not supported */
#define  ZTS_EAFNOSUPPORT    97  /* Address family not supported by protocol */
#define  ZTS_EADDRINUSE      98  /* Address already in use */
#define  ZTS_EADDRNOTAVAIL   99  /* Cannot assign requested address */
#define  ZTS_ENETDOWN       100  /* Network is down */
#define  ZTS_ENETUNREACH    101  /* Network is unreachable */
#define  ZTS_ENETRESET      102  /* Network dropped connection because of reset */
#define  ZTS_ECONNABORTED   103  /* Software caused connection abort */
#define  ZTS_ECONNRESET     104  /* Connection reset by peer */
#define  ZTS_ENOBUFS        105  /* No buffer space available */
#define  ZTS_EISCONN        106  /* Transport endpoint is already connected */
#define  ZTS_ENOTCONN       107  /* Transport endpoint is not connected */
#define  ZTS_ESHUTDOWN      108  /* Cannot send after transport endpoint shutdown */
#define  ZTS_ETOOMANYREFS   109  /* Too many references: cannot splice */
#define  ZTS_ETIMEDOUT      110  /* Connection timed out */
#define  ZTS_ECONNREFUSED   111  /* Connection refused */
#define  ZTS_EHOSTDOWN      112  /* Host is down */
#define  ZTS_EHOSTUNREACH   113  /* No route to host */
#define  ZTS_EALREADY       114  /* Operation already in progress */
#define  ZTS_EINPROGRESS    115  /* Operation now in progress */
#define  ZTS_ESTALE         116  /* Stale NFS file handle */
#define  ZTS_EUCLEAN        117  /* Structure needs cleaning */
#define  ZTS_ENOTNAM        118  /* Not a XENIX named type file */
#define  ZTS_ENAVAIL        119  /* No XENIX semaphores available */
#define  ZTS_EISNAM         120  /* Is a named type file */
#define  ZTS_EREMOTEIO      121  /* Remote I/O error */
#define  ZTS_EDQUOT         122  /* Quota exceeded */

#define  ZTS_ENOMEDIUM      123  /* No medium found */
#define  ZTS_EMEDIUMTYPE    124  /* Wrong medium type */

//////////////////////////////////////////////////////////////////////////////
// Common definitions and structures for interoperability between zts_* and //
// lwIP functions. Some of the code in the following section is a borrowed  //
// from the lwIP codebase so that the user doesn't need to include headers  //
// from that project in addition to the ZeroTier SDK headers. The license   //
// applying to this code borrowed from lwIP is produced below and only      //
// applies to the portions of code which are merely renamed versions of     //
// their lwIP counterparts. The rest of the code in this C API file is      //
// governed by the license text provided at the beginning of this file.     //
//////////////////////////////////////////////////////////////////////////////

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/**
 * Length of human-readable MAC address string
 */
#define ZTS_MAC_ADDRSTRLEN     18

#define ZTS_INET_ADDRSTRLEN    16
#define ZTS_INET6_ADDRSTRLEN   46

/** 255.255.255.255 */
#define ZTS_IPADDR_NONE         ((uint32_t)0xffffffffUL)
/** 127.0.0.1 */
#define ZTS_IPADDR_LOOPBACK     ((uint32_t)0x7f000001UL)
/** 0.0.0.0 */
#define ZTS_IPADDR_ANY          ((uint32_t)0x00000000UL)
/** 255.255.255.255 */
#define ZTS_IPADDR_BROADCAST    ((uint32_t)0xffffffffUL)

/** 255.255.255.255 */
#define ZTS_INADDR_NONE         ZTS_IPADDR_NONE
/** 127.0.0.1 */
#define ZTS_INADDR_LOOPBACK     ZTS_IPADDR_LOOPBACK
/** 0.0.0.0 */
#define ZTS_INADDR_ANY          ZTS_IPADDR_ANY
/** 255.255.255.255 */
#define ZTS_INADDR_BROADCAST    ZTS_IPADDR_BROADCAST

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

//////////////////////////////////////////////////////////////////////////////
// Custom but still mostly standard socket interface structures             //
//////////////////////////////////////////////////////////////////////////////

typedef uint32_t zts_socklen_t;
typedef uint32_t zts_in_addr_t;
typedef uint16_t zts_in_port_t;
typedef uint8_t zts_sa_family_t;

struct zts_in_addr {
#if defined(_WIN32)
	zts_in_addr_t S_addr;
#else
	// A definition in winsock may conflict with s_addr
	zts_in_addr_t s_addr;
#endif
};

struct zts_in6_addr {
	union un {
		uint32_t u32_addr[4];
		uint8_t  u8_addr[16];
	} un;
//#define s6_addr  un.u8_addr
};

struct zts_sockaddr_in {
	uint8_t            sin_len;
	zts_sa_family_t    sin_family;
	zts_in_port_t      sin_port;
	struct zts_in_addr sin_addr;
#define SIN_ZERO_LEN 8
	char sin_zero[SIN_ZERO_LEN];
};

struct zts_sockaddr_in6 {
	uint8_t             sin6_len;      // length of this structure
	zts_sa_family_t     sin6_family;   // ZTS_AF_INET6
	zts_in_port_t       sin6_port;     // Transport layer port #
	uint32_t            sin6_flowinfo; // IPv6 flow information
	struct zts_in6_addr sin6_addr;     // IPv6 address
	uint32_t            sin6_scope_id; // Set of interfaces for scope
};

struct zts_sockaddr {
	uint8_t         sa_len;
	zts_sa_family_t sa_family;
	char            sa_data[14];
};

struct zts_sockaddr_storage {
	uint8_t         s2_len;
	zts_sa_family_t ss_family;
	char            s2_data1[2];
	uint32_t        s2_data2[3];
	uint32_t        s2_data3[3];
};

//////////////////////////////////////////////////////////////////////////////
// Structures used to convey details during various callback events         //
//////////////////////////////////////////////////////////////////////////////

/**
 * Maximum address assignments per network
 */
#define ZTS_MAX_ASSIGNED_ADDRESSES 16

/**
 * Maximum routes per network
 */
#define ZTS_MAX_NETWORK_ROUTES 32

/**
 * Maximum number of direct network paths to a given peer
 */
#define ZTS_MAX_PEER_NETWORK_PATHS 16

/**
 * What trust hierarchy role does this peer have?
 */
enum zts_peer_role
{
	ZTS_PEER_ROLE_LEAF = 0,       // ordinary node
	ZTS_PEER_ROLE_MOON = 1,       // moon root
	ZTS_PEER_ROLE_PLANET = 2      // planetary root
};

/**
 * A structure used to convey details about the current node
 * to the user application
 */
struct zts_node_details
{
	/**
	 * The node ID
	 */
	uint64_t address;

	/**
	 * The port used by the service to send and receive
	 * all encapsulated traffic
	 */
	uint16_t primaryPort;
	uint16_t secondaryPort;
	uint16_t tertiaryPort;

	/**
	 * ZT version
	 */
	uint8_t versionMajor;
	uint8_t versionMinor;
	uint8_t versionRev;
};

/**
 * A structure used to convey information to a user application via
 * a callback function.
 */
struct zts_callback_msg
{
	/**
	 * Event identifier
	 */
	int16_t eventCode;

	struct zts_node_details *node;
	struct zts_network_details *network;
	struct zts_netif_details *netif;
	struct zts_virtual_network_route *route;
	struct zts_peer_details *peer;
	struct zts_addr_details *addr;
};

struct zts_addr_details
{
	uint64_t nwid;
	struct zts_sockaddr_storage addr;
};

/**
 * A structure used to convey information about a virtual network
 * interface (netif) to a user application.
 */
struct zts_netif_details
{
	/**
	 * The virtual network that this interface was commissioned for.
	 */
	uint64_t nwid;

	/**
	 * The hardware address assigned to this interface
	 */
	uint64_t mac;

	/**
	 * The MTU for this interface
	 */
	int mtu;
};

/**
 * A structure used to represent a virtual network route
 */
struct zts_virtual_network_route
{
	/**
	 * Target network / netmask bits (in port field) or NULL or 0.0.0.0/0 for default
	 */
	struct zts_sockaddr_storage target;

	/**
	 * Gateway IP address (port ignored) or NULL (family == 0) for LAN-local (no gateway)
	 */
	struct zts_sockaddr_storage via;

	/**
	 * Route flags
	 */
	uint16_t flags;

	/**
	 * Route metric (not currently used)
	 */
	uint16_t metric;
};


/**
 * Maximum length of network short name
 */
#define ZTS_MAX_NETWORK_SHORT_NAME_LENGTH 127

/**
 * Maximum number of pushed routes on a network
 */
#define ZTS_MAX_NETWORK_ROUTES 32

/**
 * Maximum number of statically assigned IP addresses per network endpoint using ZT address management (not DHCP)
 */
#define ZTS_MAX_ZT_ASSIGNED_ADDRESSES 16

/**
 * Maximum number of multicast groups a device / network interface can be subscribed to at once
 */
#define ZTS_MAX_MULTICAST_SUBSCRIPTIONS 1024

/**
 * Virtual network status codes
 */
enum ZTS_VirtualNetworkStatus
{
	/**
	 * Waiting for network configuration (also means revision == 0)
	 */
	ZTS_NETWORK_STATUS_REQUESTING_CONFIGURATION = 0,

	/**
	 * Configuration received and we are authorized
	 */
	ZTS_NETWORK_STATUS_OK = 1,

	/**
	 * Netconf master told us 'nope'
	 */
	ZTS_NETWORK_STATUS_ACCESS_DENIED = 2,

	/**
	 * Netconf master exists, but this virtual network does not
	 */
	ZTS_NETWORK_STATUS_NOT_FOUND = 3,

	/**
	 * Initialization of network failed or other internal error
	 */
	ZTS_NETWORK_STATUS_PORT_ERROR = 4,

	/**
	 * ZeroTier core version too old
	 */
	ZTS_NETWORK_STATUS_CLIENT_TOO_OLD = 5
};

/**
 * Virtual network type codes
 */
enum ZTS_VirtualNetworkType
{
	/**
	 * Private networks are authorized via certificates of membership
	 */
	ZTS_NETWORK_TYPE_PRIVATE = 0,

	/**
	 * Public networks have no access control -- they'll always be AUTHORIZED
	 */
	ZTS_NETWORK_TYPE_PUBLIC = 1
};

/**
 * A route to be pushed on a virtual network
 */
typedef struct
{
	/**
	 * Target network / netmask bits (in port field) or NULL or 0.0.0.0/0 for default
	 */
	struct zts_sockaddr_storage target;

	/**
	 * Gateway IP address (port ignored) or NULL (family == 0) for LAN-local (no gateway)
	 */
	struct zts_sockaddr_storage via;

	/**
	 * Route flags
	 */
	uint16_t flags;

	/**
	 * Route metric (not currently used)
	 */
	uint16_t metric;
} ZTS_VirtualNetworkRoute;

/**
 * Virtual network configuration
 */
struct zts_network_details
{
	/**
	 * 64-bit ZeroTier network ID
	 */
	uint64_t nwid;

	/**
	 * Ethernet MAC (48 bits) that should be assigned to port
	 */
	uint64_t mac;

	/**
	 * Network name (from network configuration master)
	 */
	char name[ZTS_MAX_NETWORK_SHORT_NAME_LENGTH + 1];

	/**
	 * Network configuration request status
	 */
	enum ZTS_VirtualNetworkStatus status;

	/**
	 * Network type
	 */
	enum ZTS_VirtualNetworkType type;

	/**
	 * Maximum interface MTU
	 */
	unsigned int mtu;

	/**
	 * If nonzero, the network this port belongs to indicates DHCP availability
	 *
	 * This is a suggestion. The underlying implementation is free to ignore it
	 * for security or other reasons. This is simply a netconf parameter that
	 * means 'DHCP is available on this network.'
	 */
	int dhcp;

	/**
	 * If nonzero, this port is allowed to bridge to other networks
	 *
	 * This is informational. If this is false (0), bridged packets will simply
	 * be dropped and bridging won't work.
	 */
	int bridge;

	/**
	 * If nonzero, this network supports and allows broadcast (ff:ff:ff:ff:ff:ff) traffic
	 */
	int broadcastEnabled;

	/**
	 * If the network is in PORT_ERROR state, this is the (negative) error code most recently reported
	 */
	int portError;

	/**
	 * Revision number as reported by controller or 0 if still waiting for config
	 */
	unsigned long netconfRevision;

	/**
	 * Number of assigned addresses
	 */
	unsigned int assignedAddressCount;

	/**
	 * ZeroTier-assigned addresses (in sockaddr_storage structures)
	 *
	 * For IP, the port number of the sockaddr_XX structure contains the number
	 * of bits in the address netmask. Only the IP address and port are used.
	 * Other fields like interface number can be ignored.
	 *
	 * This is only used for ZeroTier-managed address assignments sent by the
	 * virtual network's configuration master.
	 */
	struct zts_sockaddr_storage assignedAddresses[ZTS_MAX_ZT_ASSIGNED_ADDRESSES];

	/**
	 * Number of ZT-pushed routes
	 */
	unsigned int routeCount;

	/**
	 * Routes (excluding those implied by assigned addresses and their masks)
	 */
	ZTS_VirtualNetworkRoute routes[ZTS_MAX_NETWORK_ROUTES];

	/**
	 * Number of multicast groups subscribed
	 */
	unsigned int multicastSubscriptionCount;

	/**
	 * Multicast groups to which this network's device is subscribed
	 */
	struct {
		uint64_t mac; /* MAC in lower 48 bits */
		uint32_t adi; /* Additional distinguishing information, usually zero except for IPv4 ARP groups */
	} multicastSubscriptions[ZTS_MAX_MULTICAST_SUBSCRIPTIONS];
};

/**
 * Physical network path to a peer
 */
struct zts_physical_path
{
	/**
	 * Address of endpoint
	 */
	struct zts_sockaddr_storage address;

	/**
	 * Time of last send in milliseconds or 0 for never
	 */
	uint64_t lastSend;

	/**
	 * Time of last receive in milliseconds or 0 for never
	 */
	uint64_t lastReceive;

	/**
	 * Is this a trusted path? If so this will be its nonzero ID.
	 */
	uint64_t trustedPathId;

	/**
	 * Is path expired?
	 */
	int expired;

	/**
	 * Is path preferred?
	 */
	int preferred;
};

/**
 * Peer status result buffer
 */
struct zts_peer_details
{
	/**
	 * ZeroTier address (40 bits)
	 */
	uint64_t address;

	/**
	 * Remote major version or -1 if not known
	 */
	int versionMajor;

	/**
	 * Remote minor version or -1 if not known
	 */
	int versionMinor;

	/**
	 * Remote revision or -1 if not known
	 */
	int versionRev;

	/**
	 * Last measured latency in milliseconds or -1 if unknown
	 */
	int latency;

	/**
	 * What trust hierarchy role does this device have?
	 */
	enum zts_peer_role role;

	/**
	 * Number of paths (size of paths[])
	 */
	unsigned int pathCount;

	/**
	 * Known network paths to peer
	 */
	struct zts_physical_path paths[ZTS_MAX_PEER_NETWORK_PATHS];
};

/**
 * List of peers
 */
struct zts_peer_list
{
	struct zts_peer_details *peers;
	unsigned long peerCount;
};

//////////////////////////////////////////////////////////////////////////////
// Python Bindings (Subset of regular socket API)                           //
//////////////////////////////////////////////////////////////////////////////

#ifdef ZTS_ENABLE_PYTHON

#include "Python.h"

/**
 * Abstract class used as a director. Pointer to an instance of this class
 * is provided to the Python layer.
 *
 * See: https://rawgit.com/swig/swig/master/Doc/Manual/SWIGPlus.html#SWIGPlus_target_language_callbacks
 */
class PythonDirectorCallbackClass
{
public:
	/**
	 * Called by native code on event. Implemented in Python
	 */
	virtual void on_zerotier_event(struct zts_callback_msg *msg);
	virtual ~PythonDirectorCallbackClass() {};
};

extern PythonDirectorCallbackClass *_userEventCallback;

int zts_py_bind(int fd, int family, int type, PyObject *addro);
int zts_py_connect(int fd, int family, int type, PyObject *addro);
PyObject * zts_py_accept(int fd);
int zts_py_listen(int fd, int backlog);
PyObject * zts_py_recv(int fd, int len, int flags);
int zts_py_send(int fd, PyObject *buf, int flags);
int zts_py_close(int fd);
int zts_py_setblocking(int fd, int flag);
int zts_py_getblocking(int fd);

#endif // ZTS_ENABLE_PYTHON

//////////////////////////////////////////////////////////////////////////////
// ZeroTier Service Controls                                                //
//////////////////////////////////////////////////////////////////////////////

#if defined(_WIN32)
	#ifdef ADD_EXPORTS
		#define ZTS_API __declspec(dllexport)
	#else
		#define ZTS_API __declspec(dllimport)
	#endif
	#define ZTCALL __cdecl
#else
	#define ZTS_API
	#define ZTCALL
#endif

//////////////////////////////////////////////////////////////////////////////
// Central API                                                              //
//////////////////////////////////////////////////////////////////////////////

#ifdef ZTS_ENABLE_CENTRAL_API

#define ZTS_CENTRAL_DEFAULT_URL         "https://my.zerotier.com"
#define ZTS_CENRTAL_MAX_URL_LEN         128
#define ZTS_CENTRAL_TOKEN_LEN           32
#define ZTS_CENTRAL_RESP_BUF_DEFAULT_SZ (128*1024)

#define ZTS_HTTP_GET    0
#define ZTS_HTTP_POST   1
#define ZTS_HTTP_DELETE 2

#define ZTS_CENTRAL_NODE_AUTH_FALSE 0
#define ZTS_CENTRAL_NODE_AUTH_TRUE  1

#define ZTS_CENTRAL_READ  1
#define ZTS_CENTRAL_WRITE 2

/**
 * @brief Enables read/write capability. Default before calling this is
 * read-only (ZTS_CENTRAL_READ.)
 *
 * @param modes Whether the API allows read, write, or both
 */
ZTS_API void ZTCALL zts_central_set_access_mode(int8_t modes);

/**
 * @brief Enables or disables libcurl verbosity
 *
 * @param is_verbose Whether debug information is desired
 */
ZTS_API void ZTCALL zts_central_set_verbose(int8_t is_verbose);

ZTS_API void ZTCALL zts_central_clear_resp_buf();

/**
 * @brief Set the Central API URL and user API token.
 *
 * @param url_str The URL to the Central API server
 * @param token_str User API token
 * @param resp_buf Destination buffer for raw JSON output
 * @param buf_len Size of buffer for server response (specify 0 for default size)
 * @return ZTS_ERR_OK on success. ZTS_ERR_ARG if invalid arguments provided.
 */
ZTS_API int ZTCALL zts_central_init(
	const char *url_str, const char *token_str, char *resp_buf, uint32_t buf_len);

ZTS_API void ZTCALL zts_central_cleanup();

/**
 * @brief Copies the JSON-formatted string buffer from the last request into
 *        a user-provided buffer.
 *
 * @param dest_buffer User-provided destination buffer
 * @param dest_buf_len Length of aforementioned buffer
 * @return ZTS_ERR_OK if all contents were copied successfully.
 *         ZTS_ERR_ARG if provided buffer was too small.
 */
ZTS_API int ZTCALL zts_central_get_last_response_buf(
	char *dest_buffer, int dest_buf_len);

/**
 * @brief Get the status of the Central API server.
 *
 * @return Standard HTTP response codes.
 */
ZTS_API int ZTCALL zts_central_get_status(int *http_response_code);

/**
 * @brief Get the currently authenticated user’s user record.
 *
 * @return Standard HTTP response codes.
 */
ZTS_API int ZTCALL zts_central_get_self(int *http_response_code);

/**
 * @brief Retrieve a Network.
 *
 * @return Standard HTTP response codes.
 */
ZTS_API int ZTCALL zts_central_get_network(
	int *http_response_code, uint64_t nwid);

/**
 * @brief Update or create a Network.
 *
 * Only fields marked as [rw] can be directly modified. If other fields are
 * present in the posted request they are ignored. New networks can be
 * created by POSTing to /api/network with no networkId parameter. The server
 * will create a random unused network ID and return the new network record.
 *
 * @return Standard HTTP response codes.
 */
ZTS_API int ZTCALL zts_central_update_network(
	int *http_response_code, uint64_t nwid);

/**
 * @brief Delete a Network.
 *
 * Delete a network and all its related information permanently.
 * Use extreme caution as this cannot be undone!
 *
 * @return Standard HTTP response codes.
 */
ZTS_API int ZTCALL zts_central_delete_network(
	int *http_response_code, uint64_t nwid);

/**
 * @brief Get All Viewable Networks.
 *
 * Get all networks for which you have at least read access.
 *
 * @return Standard HTTP response codes.
 */
ZTS_API int ZTCALL zts_central_get_networks(int *http_response_code);
/**
 * @brief Retrieve a Member.
 *
 * @return Standard HTTP response codes.
 */
ZTS_API int ZTCALL zts_central_get_member(
	int *http_response_code, uint64_t nwid, uint64_t nodeid);

/**
 * @brief Update or add a Member.
 *
 * New members can be added to a network by POSTing them.
 *
 * @return Standard HTTP response codes.
 */
ZTS_API int ZTCALL zts_central_update_member(
	int *http_response_code, uint64_t nwid, uint64_t nodeid, char *post_data);

/**
 * @brief Authorize or (De)authorize a node on a network. This operation
 * is idempotent.
 *
 * @param nwid The network ID
 * @param nodeid The node ID
 * @param is_authed Boolean value for whether this node should be authorized
 * @return Standard HTTP response codes. ZTS_ERR_ARG invalid argument specified.
 */
ZTS_API int ZTCALL zts_central_set_node_auth(
	int *http_response_code, uint64_t nwid, uint64_t nodeid, uint8_t is_authed);

/**
 * @brief Get All Members of a Network.
 *
 * Get all members of a network for which you have at least read access.
 *
 * @return Standard HTTP response codes.
 */
ZTS_API int ZTCALL zts_central_get_members_of_network(
	int *http_response_code, uint64_t nwid);

#endif // NO_CENTRAL_API

//////////////////////////////////////////////////////////////////////////////
// Identity Management                                                      //
//////////////////////////////////////////////////////////////////////////////

/**
 * @brief Generates a node identity (public/secret keypair) and stores it in a user-provided buffer.
 *
 * @param key_pair_str User-provided destination buffer
 * @param key_buf_len Length of user-provided destination buffer. Will be set to number of bytes copied.
 * @return ZTS_ERR_OK on success. ZTS_ERR_SERVICE on failure.
 */
ZTS_API int ZTCALL zts_generate_orphan_identity(char *key_pair_str, uint16_t *key_buf_len);

/**
 * @brief Verifies that a keypair is valid for use.
 *
 * @param key_pair_str Buffer containing keypair
 * @return ZTS_ERR_OK on success. ZTS_ERR_SERVICE on failure.
 */
ZTS_API int ZTCALL zts_verify_identity(const char *key_pair_str);

/**
 * @brief Copies the current node's identity into a buffer
 *
 * @param key_pair_str User-provided destination buffer
 * @param key_buf_len Length of user-provided destination buffer. Will be set to number of bytes copied.
 * @return ZTS_ERR_OK on success. ZTS_ERR_SERVICE on failure.
 */
ZTS_API int ZTCALL zts_get_node_identity(char *key_pair_str, uint16_t *key_buf_len);

/**
 * @brief Starts the ZeroTier service and notifies user application of events via callback. This
 * variant will assign a user-provided identity to the node.
 *
 * @param path path directory where configuration files are stored
 * @param callback User-specified callback for ZTS_EVENT_* events
 * @param port Port that the library should use for talking to other ZeroTier nodes
 * @return ZTS_ERR_OK on success. ZTS_ERR_SERVICE or ZTS_ERR_ARG on failure
 */
#ifdef ZTS_ENABLE_PYTHON
int zts_start_with_identity(const char *key_pair_str, uint16_t key_buf_len,
		PythonDirectorCallbackClass *callback, uint16_t port);
#endif
#ifdef ZTS_ENABLE_PINVOKE
int zts_start_with_identity(const char *key_pair_str, uint16_t key_buf_len,
	CppCallback callback, uint16_t port);
#endif
#ifdef ZTS_C_API_ONLY
int zts_start_with_identity(const char *key_pair_str, uint16_t key_buf_len,
	void (*callback)(void *), uint16_t port);
#endif

/**
 * @brief Enable or disable whether the service will cache network details (enabled by default)
 *
 * This can potentially shorten (startup) times. This allows the service to nearly instantly
 * inform the network stack of an address to use for this peer so that it can
 * create an interface. This can be disabled for cases where one may not want network
 * config details to be written to storage. This is especially useful for situations where
 * address assignments do not change often.
 *
 * @usage Should be called before zts_start() if you intend on changing its state.
 *
 * @param enabled Whether or not this feature is enabled
 * @return ZTS_ERR_OK on success. ZTS_ERR_SERVICE on failure.
 */
ZTS_API int ZTCALL zts_allow_network_caching(uint8_t allowed);

/**
 * @brief Enable or disable whether the service will cache peer details (enabled by default)
 *
 * This can potentially shorten (connection) times. This allows the service to
 * re-use previously discovered paths to a peer, this prevents the service from having
 * to go through the entire transport-triggered link provisioning process. This is especially
 * useful for situations where paths to peers do not change often. This is enabled by default
 * and can be disabled for cases where one may not want peer details to be written to storage.
 *
 * @usage Should be called before zts_start() if you intend on changing its state.
 *
 * @param enabled Whether or not this feature is enabled
 * @return ZTS_ERR_OK on success. ZTS_ERR_SERVICE on failure.
 */
ZTS_API int ZTCALL zts_allow_peer_caching(uint8_t allowed);

/**
 * @brief Enable or disable whether the service will read node configuration settings from a local.conf
 *
 * @usage Should be called before zts_start() if you intend on changing its state.
 *
 * @param enabled Whether or not this feature is enabled
 * @return ZTS_ERR_OK on success. ZTS_ERR_SERVICE on failure.
 */
ZTS_API int ZTCALL zts_allow_local_conf(uint8_t allowed);

/**
 * @brief Enable or disable whether the service will read or write config data to local storage
 *
 * @usage Should be called before zts_start() if you intend on changing its state.
 *
 * @param enabled Whether or not this feature is enabled
 * @return ZTS_ERR_OK on success. ZTS_ERR_SERVICE on failure.
 */
ZTS_API int ZTCALL zts_disable_local_storage(uint8_t disabled);

/**
 * @brief Starts the ZeroTier service and notifies user application of events via callback
 *
 * @param path path directory where configuration files are stored
 * @param callback User-specified callback for ZTS_EVENT_* events
 * @param port Port that the library should use for talking to other ZeroTier nodes
 * @return ZTS_ERR_OK on success. ZTS_ERR_SERVICE or ZTS_ERR_ARG on failure
 */
#ifdef ZTS_ENABLE_PYTHON
	ZTS_API int ZTCALL zts_start(const char *path, PythonDirectorCallbackClass *callback, uint16_t port);
#endif
#ifdef ZTS_ENABLE_PINVOKE
	ZTS_API int ZTCALL zts_start(const char *path, CppCallback callback, uint16_t port);
#endif
#ifdef ZTS_C_API_ONLY
	ZTS_API int ZTCALL zts_start(const char *path, void (*callback)(void *), uint16_t port);
#endif

/**
 * @brief Stops the ZeroTier service and brings down all virtual network interfaces
 *
 * @usage While the ZeroTier service will stop, the stack driver (with associated timers)
 * will remain active in case future traffic processing is required. To stop all activity
 * and free all resources use zts_free() instead.
 * @return ZTS_ERR_OK on success. ZTS_ERR_SERVICE on failure.
 */
ZTS_API int ZTCALL zts_stop();

/**
 * @brief Restart the ZeroTier service.
 *
 * @usage This call will block until the service has been brought offline. Then
 * it will return and the user application can then watch for the appropriate
 * startup callback events.
 * @return ZTS_ERR_OK on success. ZTS_ERR_SERVICE on failure.
 */
ZTS_API int ZTCALL zts_restart();

/**
 * @brief Stop all background services, bring down all interfaces, free all resources. After
 * calling this function an application restart will be required before the library can be
 * used again.
 *
 * @usage This should be called at the end of your program or when you do not anticipate
 *        communicating over ZeroTier
 * @return ZTS_ERR_OK on success. ZTS_ERR_SERVICE on failure.
 */
ZTS_API int ZTCALL zts_free();

/**
 * @brief Join a network
 *
 * @param networkId A 16-digit hexadecimal virtual network ID
 * @return ZTS_ERR_OK on success. ZTS_ERR_SERVICE or ZTS_ERR_ARG on failure.
 */
ZTS_API int ZTCALL zts_join(const uint64_t networkId);

/**
 * @brief Leave a network
 *
 * @param networkId A 16-digit hexadecimal virtual network ID
 * @return ZTS_ERR_OK on success. ZTS_ERR_SERVICE or ZTS_ERR_ARG on failure.
 */
ZTS_API int ZTCALL zts_leave(const uint64_t networkId);

/**
 * @brief Orbit a given moon (user-defined root server)
 *
 * @param moonWorldId A 16-digit hexadecimal world ID
 * @param moonSeed A 16-digit hexadecimal seed ID
 * @return ZTS_ERR_OK on success. ZTS_ERR_SERVICE or ZTS_ERR_ARG on failure.
 */
ZTS_API int ZTCALL zts_orbit(uint64_t moonWorldId, uint64_t moonSeed);

/**
 * @brief De-orbit a given moon (user-defined root server)
 *
 * @param moonWorldId A 16-digit world ID
 * @return ZTS_ERR_OK on success. ZTS_ERR_SERVICE or ZTS_ERR_ARG on failure.
 */
ZTS_API int ZTCALL zts_deorbit(uint64_t moonWorldId);

/**
 * @brief Compute a 6PLANE IPv6 address for the given Network ID and Node ID
 *
 * @param addr Destination structure for address
 * @param networkId Network ID
 * @param nodeId Node ID
 * @return ZTS_ERR_OK on success. ZTS_ERR_ARG on failure.
 */
ZTS_API int ZTCALL zts_get_6plane_addr(struct zts_sockaddr_storage *addr, const uint64_t networkId, const uint64_t nodeId);

/**
 * @brief Compute a RFC4193 IPv6 address for the given Network ID and Node ID
 *
 * @param addr Destination structure for address
 * @param networkId Network ID
 * @param nodeId Node ID
 * @return ZTS_ERR_OK on success. ZTS_ERR_ARG on failure.
 */
ZTS_API int ZTCALL zts_get_rfc4193_addr(
	struct zts_sockaddr_storage *addr, const uint64_t networkId, const uint64_t nodeId);

/**
 * @brief Compute a RFC4193 IPv6 address for the given Network ID and Node ID
 *
 * Ad-hoc Network:
 *
 * ffSSSSEEEE000000
 * | |   |   |
 * | |   |   Reserved for future use, must be 0
 * | |   End of port range (hex)
 * | Start of port range (hex)
 * Reserved ZeroTier address prefix indicating a controller-less network.
 *
 * Ad-hoc networks are public (no access control) networks that have no network controller. Instead
 * their configuration and other credentials are generated locally. Ad-hoc networks permit only IPv6
 * UDP and TCP unicast traffic (no multicast or broadcast) using 6plane format NDP-emulated IPv6
 * addresses. In addition an ad-hoc network ID encodes an IP port range. UDP packets and TCP SYN
 * (connection open) packets are only allowed to destination ports within the encoded range.
 *
 * For example ff00160016000000 is an ad-hoc network allowing only SSH, while ff0000ffff000000 is an
 * ad-hoc network allowing any UDP or TCP port.
 *
 * Keep in mind that these networks are public and anyone in the entire world can join them. Care must
 * be taken to avoid exposing vulnerable services or sharing unwanted files or other resources.
 *
 *
 * @param startPortOfRange Start of port allowed port range
 * @param endPortOfRange End of allowed port range
 * @return An Ad-hoc network ID
 */
ZTS_API uint64_t ZTCALL zts_generate_adhoc_nwid_from_range(
	uint16_t startPortOfRange, uint16_t endPortOfRange);

/**
 * @brief Platform-agnostic delay (provided for convenience)
 *
 * @param interval_ms Number of milliseconds to delay
 */
ZTS_API void ZTCALL zts_delay_ms(long interval_ms);

//////////////////////////////////////////////////////////////////////////////
// Statistics                                                               //
//////////////////////////////////////////////////////////////////////////////

#ifdef ZTS_ENABLE_STATS

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

/** Protocol related stats */
struct zts_stats_proto {
  uint32_t xmit;             /* Transmitted packets.  */
  uint32_t recv;             /* Received packets.     */
  uint32_t fw;               /* Forwarded packets.    */
  uint32_t drop;             /* Dropped packets.      */
  uint32_t chkerr;           /* Checksum error.       */
  uint32_t lenerr;           /* Invalid length error. */
  uint32_t memerr;           /* Out of memory error.  */
  uint32_t rterr;            /* Routing error.        */
  uint32_t proterr;          /* Protocol error.       */
  uint32_t opterr;           /* Error in options.     */
  uint32_t err;              /* Misc error.           */
  uint32_t cachehit;
};

/** IGMP stats */
struct zts_stats_igmp {
  uint32_t xmit;             /* Transmitted packets.             */
  uint32_t recv;             /* Received packets.                */
  uint32_t drop;             /* Dropped packets.                 */
  uint32_t chkerr;           /* Checksum error.                  */
  uint32_t lenerr;           /* Invalid length error.            */
  uint32_t memerr;           /* Out of memory error.             */
  uint32_t proterr;          /* Protocol error.                  */
  uint32_t rx_v1;            /* Received v1 frames.              */
  uint32_t rx_group;         /* Received group-specific queries. */
  uint32_t rx_general;       /* Received general queries.        */
  uint32_t rx_report;        /* Received reports.                */
  uint32_t tx_join;          /* Sent joins.                      */
  uint32_t tx_leave;         /* Sent leaves.                     */
  uint32_t tx_report;        /* Sent reports.                    */
};

/** System element stats */
struct zts_stats_syselem {
  uint32_t used;
  uint32_t max;
  uint32_t err;
};

/** System stats */
struct zts_stats_sys {
  struct zts_stats_syselem sem;
  struct zts_stats_syselem mutex;
  struct zts_stats_syselem mbox;
};

/** lwIP stats container */
struct zts_stats {
	/** Link level */
	struct zts_stats_proto link;
	/** ARP */
	struct zts_stats_proto etharp;
	/** Fragmentation */
	struct zts_stats_proto ip_frag;
	/** IP */
	struct zts_stats_proto ip;
	/** ICMP */
	struct zts_stats_proto icmp;
	/** IGMP */
	struct zts_stats_igmp igmp;
	/** UDP */
	struct zts_stats_proto udp;
	/** TCP */
	struct zts_stats_proto tcp;
	/** System */
	struct zts_stats_sys sys;
	/** IPv6 */
	struct zts_stats_proto ip6;
	/** ICMP6 */
	struct zts_stats_proto icmp6;
	/** IPv6 fragmentation */
	struct zts_stats_proto ip6_frag;
	/** Multicast listener discovery */
	struct zts_stats_igmp mld6;
	/** Neighbor discovery */
	struct zts_stats_proto nd6;
};

/**
 * @brief Return all statistical counters for all protocols (inefficient)
 *
 * @usage This function can only be used in debug builds.
 * @return ZTS_ERR_OK on success. ZTS_ERR_ARG or ZTS_ERR_NO_RESULT on failure.
 */
ZTS_API int ZTCALL zts_get_all_stats(struct zts_stats *statsDest);

/**
 * @brief Populate the given structure with the requested protocol's
 * statistical counters (from network stack)
 *
 * @usage This function can only be used in debug builds.
 * @return ZTS_ERR_OK on success. ZTS_ERR_ARG or ZTS_ERR_NO_RESULT on failure.
 */
ZTS_API int ZTCALL zts_get_protocol_stats(int protocolType, void *protoStatsDest);

#endif // ZTS_ENABLE_STATS

//////////////////////////////////////////////////////////////////////////////
// Socket API                                                               //
//////////////////////////////////////////////////////////////////////////////

/**
 * @brief Create a socket (sets zts_errno)
 *
 * @param socket_family Address family (ZTS_AF_INET, ZTS_AF_INET6)
 * @param socket_type Type of socket (ZTS_SOCK_STREAM, ZTS_SOCK_DGRAM, ZTS_SOCK_RAW)
 * @param protocol Protocols supported on this socket
 * @return Numbered file descriptor on success. ZTS_ERR_SERVICE or ZTS_ERR_SOCKET on failure.
 */
ZTS_API int ZTCALL zts_socket(
	const int socket_family, const int socket_type, const int protocol);

/**
 * @brief Connect a socket to a remote host (sets zts_errno)
 *
 * @param fd Socket file descriptor
 * @param addr Remote host address to connect to
 * @param addrlen Length of address
 * @return ZTS_ERR_OK on success. ZTS_ERR_SOCKET, ZTS_ERR_SERVICE, ZTS_ERR_ARG on failure.
 */
ZTS_API int ZTCALL zts_connect(
	int fd, const struct zts_sockaddr *addr, zts_socklen_t addrlen);

/**
 * @brief Bind a socket to a virtual interface (sets zts_errno)
 *
 * @param fd Socket file descriptor
 * @param addr Local interface address to bind to
 * @param addrlen Length of address
 * @return ZTS_ERR_OK on success. ZTS_ERR_SOCKET, ZTS_ERR_SERVICE, ZTS_ERR_ARG on failure.
 */
ZTS_API int ZTCALL zts_bind(
	int fd, const struct zts_sockaddr *addr, zts_socklen_t addrlen);

/**
 * @brief Listen for incoming connections on socket (sets zts_errno)
 *
 * @param fd Socket file descriptor
 * @param backlog Number of backlogged connections allowed
 * @return ZTS_ERR_OK on success. ZTS_ERR_SOCKET, ZTS_ERR_SERVICE, ZTS_ERR_ARG on failure.
 */
ZTS_API int ZTCALL zts_listen(int fd, int backlog);

/**
 * @brief Accept an incoming connection (sets zts_errno)
 *
 * @param fd Socket file descriptor
 * @param addr Address of remote host for accepted connection
 * @param addrlen Length of address
 * @return New socket file descriptor on success. ZTS_ERR_SOCKET, ZTS_ERR_SERVICE, ZTS_ERR_ARG on failure.
 */
ZTS_API int ZTCALL zts_accept(int fd, struct zts_sockaddr *addr, zts_socklen_t *addrlen);

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

/*
 * Structure used for manipulating linger option.
 */
struct zts_linger {
	int l_onoff;  // option on/off
	int l_linger; // linger time in seconds
};

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
#define ZTS_SO_BINDTODEVICE 0x100b
// IPPROTO_IP options
#define ZTS_IP_TOS          0x0001
#define ZTS_IP_TTL          0x0002
#define ZTS_IP_PKTINFO      0x0008
// IPPROTO_TCP options
#define ZTS_TCP_NODELAY     0x0001
#define ZTS_TCP_KEEPALIVE   0x0002
#define ZTS_TCP_KEEPIDLE    0x0003
#define ZTS_TCP_KEEPINTVL   0x0004
#define ZTS_TCP_KEEPCNT     0x0005
// IPPROTO_IPV6 options
#define ZTS_IPV6_CHECKSUM   0x0007  /* RFC3542: calculate and insert the ICMPv6 checksum for raw sockets. */
#define ZTS_IPV6_V6ONLY     0x001b  /* RFC3493: boolean control to restrict ZTS_AF_INET6 sockets to IPv6 communications only. */
// UDPLITE options
#define ZTS_UDPLITE_SEND_CSCOV 0x01 /* sender checksum coverage */
#define ZTS_UDPLITE_RECV_CSCOV 0x02 /* minimal receiver checksum coverage */
// UDPLITE options
#define ZTS_IP_MULTICAST_TTL   5
#define ZTS_IP_MULTICAST_IF    6
#define ZTS_IP_MULTICAST_LOOP  7

// Multicast options
#define ZTS_IP_ADD_MEMBERSHIP  3
#define ZTS_IP_DROP_MEMBERSHIP 4

typedef struct zts_ip_mreq {
	struct zts_in_addr imr_multiaddr; /* IP multicast address of group */
	struct zts_in_addr imr_interface; /* local IP address of interface */
} zts_ip_mreq;

struct zts_in_pktinfo {
	unsigned int       ipi_ifindex;  /* Interface index */
	struct zts_in_addr ipi_addr;     /* Destination (from header) address */
};

#define ZTS_IPV6_JOIN_GROUP      12
#define ZTS_IPV6_ADD_MEMBERSHIP  ZTS_IPV6_JOIN_GROUP
#define ZTS_IPV6_LEAVE_GROUP     13
#define ZTS_IPV6_DROP_MEMBERSHIP ZTS_IPV6_LEAVE_GROUP

typedef struct zts_ipv6_mreq {
	struct zts_in6_addr ipv6mr_multiaddr; /*  IPv6 multicast addr */
	unsigned int        ipv6mr_interface; /*  interface index, or 0 */
} zts_ipv6_mreq;

/*
 * The Type of Service provides an indication of the abstract
 * parameters of the quality of service desired.  These parameters are
 * to be used to guide the selection of the actual service parameters
 * when transmitting a datagram through a particular network.  Several
 * networks offer service precedence, which somehow treats high
 * precedence traffic as more important than other traffic (generally
 * by accepting only traffic above a certain precedence at time of high
 * load).  The major choice is a three way tradeoff between low-delay,
 * high-reliability, and high-throughput.
 * The use of the Delay, Throughput, and Reliability indications may
 * increase the cost (in some sense) of the service.  In many networks
 * better performance for one of these parameters is coupled with worse
 * performance on another.  Except for very unusual cases at most two
 * of these three indications should be set.
 */
#define ZTS_IPTOS_TOS_MASK          0x1E
#define ZTS_IPTOS_TOS(tos)          ((tos) & ZTS_IPTOS_TOS_MASK)
#define ZTS_IPTOS_LOWDELAY          0x10
#define ZTS_IPTOS_THROUGHPUT        0x08
#define ZTS_IPTOS_RELIABILITY       0x04
#define ZTS_IPTOS_LOWCOST           0x02
#define ZTS_IPTOS_MINCOST           ZTS_IPTOS_LOWCOST

/*
 * The Network Control precedence designation is intended to be used
 * within a network only.  The actual use and control of that
 * designation is up to each network. The Internetwork Control
 * designation is intended for use by gateway control originators only.
 * If the actual use of these precedence designations is of concern to
 * a particular network, it is the responsibility of that network to
 * control the access to, and use of, those precedence designations.
 */
#define ZTS_IPTOS_PREC_MASK                 0xe0
#define ZTS_IPTOS_PREC(tos)                ((tos) & ZTS_IPTOS_PREC_MASK)
#define ZTS_IPTOS_PREC_NETCONTROL           0xe0
#define ZTS_IPTOS_PREC_INTERNETCONTROL      0xc0
#define ZTS_IPTOS_PREC_CRITIC_ECP           0xa0
#define ZTS_IPTOS_PREC_FLASHOVERRIDE        0x80
#define ZTS_IPTOS_PREC_FLASH                0x60
#define ZTS_IPTOS_PREC_IMMEDIATE            0x40
#define ZTS_IPTOS_PREC_PRIORITY             0x20
#define ZTS_IPTOS_PREC_ROUTINE              0x00

/**
 * @brief Set socket options (sets zts_errno)
 *
 * @param fd Socket file descriptor
 * @param level Protocol level to which option name should apply
 * @param optname Option name to set
 * @param optval Source of option value to set
 * @param optlen Length of option value
 * @return ZTS_ERR_OK on success. ZTS_ERR_SOCKET, ZTS_ERR_SERVICE, ZTS_ERR_ARG on failure.
 */
ZTS_API int ZTCALL zts_setsockopt(
	int fd, int level, int optname, const void *optval, zts_socklen_t optlen);

/**
 * @brief Get socket options (sets zts_errno)
 *
 * @param fd Socket file descriptor
 * @param level Protocol level to which option name should apply
 * @param optname Option name to get
 * @param optval Where option value will be stored
 * @param optlen Length of value
 * @return ZTS_ERR_OK on success. ZTS_ERR_SOCKET, ZTS_ERR_SERVICE, ZTS_ERR_ARG on failure.
 */
ZTS_API int ZTCALL zts_getsockopt(
	int fd, int level, int optname, void *optval, zts_socklen_t *optlen);

/**
 * @brief Get socket name (sets zts_errno)
 *
 * @param fd Socket file descriptor
 * @param addr Name associated with this socket
 * @param addrlen Length of name
 * @return ZTS_ERR_OK on success. ZTS_ERR_SOCKET, ZTS_ERR_SERVICE, ZTS_ERR_ARG on failure.
 */
ZTS_API int ZTCALL zts_getsockname(int fd, struct zts_sockaddr *addr, zts_socklen_t *addrlen);

/**
 * @brief Get the peer name for the remote end of a connected socket
 *
 * @param fd Socket file descriptor
 * @param addr Name associated with remote end of this socket
 * @param addrlen Length of name
 * @return ZTS_ERR_OK on success. ZTS_ERR_SOCKET, ZTS_ERR_SERVICE, ZTS_ERR_ARG on failure.
 */
ZTS_API int ZTCALL zts_getpeername(int fd, struct zts_sockaddr *addr, zts_socklen_t *addrlen);

/**
 * @brief Close a socket (sets zts_errno)
 *
 * @param fd Socket file descriptor
 * @return ZTS_ERR_OK on success. ZTS_ERR_SOCKET, ZTS_ERR_SERVICE on failure.
 */
ZTS_API int ZTCALL zts_close(int fd);

/* FD_SET used for lwip_select */

#define LWIP_SOCKET_OFFSET 0
#define MEMP_NUM_NETCONN   1024

#ifndef ZTS_FD_SET
#undef  ZTS_FD_SETSIZE
// Make FD_SETSIZE match NUM_SOCKETS in socket.c
#define ZTS_FD_SETSIZE    MEMP_NUM_NETCONN
#define ZTS_FDSETSAFESET(n, code) do { \
  if (((n) - LWIP_SOCKET_OFFSET < MEMP_NUM_NETCONN) && (((int)(n) - LWIP_SOCKET_OFFSET) >= 0)) { \
  code; }} while(0)
#define ZTS_FDSETSAFEGET(n, code) \
  (((n) - LWIP_SOCKET_OFFSET < MEMP_NUM_NETCONN) && (((int)(n) - LWIP_SOCKET_OFFSET) >= 0) ? \
  (code) : 0)
#define ZTS_FD_SET(n, p)  \
	ZTS_FDSETSAFESET(n, (p)->fd_bits[((n)-LWIP_SOCKET_OFFSET)/8] |=  (1 << (((n)-LWIP_SOCKET_OFFSET) & 7)))
#define ZTS_FD_CLR(n, p)  \
	ZTS_FDSETSAFESET(n, (p)->fd_bits[((n)-LWIP_SOCKET_OFFSET)/8] &= ~(1 << (((n)-LWIP_SOCKET_OFFSET) & 7)))
#define ZTS_FD_ISSET(n,p) \
	ZTS_FDSETSAFEGET(n, (p)->fd_bits[((n)-LWIP_SOCKET_OFFSET)/8] &   (1 << (((n)-LWIP_SOCKET_OFFSET) & 7)))
#define ZTS_FD_ZERO(p) memset((void*)(p), 0, sizeof(*(p)))

#elif LWIP_SOCKET_OFFSET
#error LWIP_SOCKET_OFFSET does not work with external FD_SET!
#elif ZTS_FD_SETSIZE < MEMP_NUM_NETCONN
#error "external ZTS_FD_SETSIZE too small for number of sockets"
#endif // FD_SET

typedef struct zts_fd_set
{
  unsigned char fd_bits [(ZTS_FD_SETSIZE+7)/8];
} zts_fd_set;

struct zts_timeval {
  long    tv_sec;         /* seconds */
  long    tv_usec;        /* and microseconds */
};

/**
 * @brief Monitor multiple file descriptors for "readiness" (sets zts_errno)
 *
 * @param nfds Set to the highest numbered file descriptor in any of the given sets
 * @param readfds Set of file descriptors to monitor for READ readiness
 * @param writefds Set of file descriptors to monitor for WRITE readiness
 * @param exceptfds Set of file descriptors to monitor for exceptional conditions
 * @param timeout How long this call should block
 * @return Number of ready file descriptors on success. ZTS_ERR_SOCKET, ZTS_ERR_SERVICE on failure.
 */
ZTS_API int ZTCALL zts_select(
	int nfds, zts_fd_set *readfds, zts_fd_set *writefds, zts_fd_set *exceptfds, struct zts_timeval *timeout);

// fnctl() commands
#define ZTS_F_GETFL     0x0003
#define ZTS_F_SETFL     0x0004
/* File status flags and file access modes for fnctl,
   these are bits in an int. */
#define ZTS_O_NONBLOCK  1
#define ZTS_O_NDELAY    ZTS_O_NONBLOCK
#define ZTS_O_RDONLY    2
#define ZTS_O_WRONLY    4
#define ZTS_O_RDWR      (ZTS_O_RDONLY|ZTS_O_WRONLY)

/**
 * @brief Issue file control commands on a socket
 *
 * @param fd File descriptor
 * @param cmd
 * @param flags
 * @return
 */
ZTS_API int ZTCALL zts_fcntl(int fd, int cmd, int flags);

#define ZTS_POLLIN     0x001
#define ZTS_POLLOUT    0x002
#define ZTS_POLLERR    0x004
#define ZTS_POLLNVAL   0x008
/* Below values are unimplemented */
#define ZTS_POLLRDNORM 0x010
#define ZTS_POLLRDBAND 0x020
#define ZTS_POLLPRI    0x040
#define ZTS_POLLWRNORM 0x080
#define ZTS_POLLWRBAND 0x100
#define ZTS_POLLHUP    0x200

typedef unsigned int zts_nfds_t;

struct zts_pollfd
{
  int fd;
  short events;
  short revents;
};

/**
 * @brief Wait for some event on a file descriptor. (sets zts_errno)
 *
 * @param fds Set of file descriptors to monitor
 * @param nfds Number of elements in the fds array
 * @param timeout How long this call should block
 * @return Number of ready file descriptors on success. ZTS_ERR_SOCKET, ZTS_ERR_SERVICE on failure.
 */
ZTS_API int ZTCALL zts_poll(struct zts_pollfd *fds, zts_nfds_t nfds, int timeout);

/**
 * @brief Control a device (sets zts_errno)
 *
 * @param fd Socket file descriptor
 * @param request
 * @param argp
 * @return ZTS_ERR_OK on success, ZTS_ERR_SERVICE on failure.
 */
ZTS_API int ZTCALL zts_ioctl(int fd, unsigned long request, void *argp);

/**
 * @brief Send data to remote host (sets zts_errno)
 *
 * @param fd Socket file descriptor
 * @param buf Pointer to data buffer
 * @param len Length of data to write
 * @param flags (e.g. ZTS_MSG_DONTWAIT, ZTS_MSG_MORE)
 * @return Byte count sent on success. ZTS_ERR_SOCKET, ZTS_ERR_SERVICE, ZTS_ERR_ARG on failure.
 */
ZTS_API ssize_t ZTCALL zts_send(int fd, const void *buf, size_t len, int flags);

/**
 * @brief Send data to remote host (sets zts_errno)
 *
 * @param fd Socket file descriptor
 * @param buf Pointer to data buffer
 * @param len Length of data to write
 * @param flags
 * @param addr Destination address
 * @param addrlen Length of destination address
 * @return Byte count sent on success. ZTS_ERR_SOCKET, ZTS_ERR_SERVICE, ZTS_ERR_ARG on failure.
 */
ZTS_API ssize_t ZTCALL zts_sendto(
	int fd, const void *buf, size_t len, int flags, const struct zts_sockaddr *addr, zts_socklen_t addrlen);

struct zts_iovec {
	void  *iov_base;
	size_t iov_len;
};

/* */
struct zts_msghdr {
	void             *msg_name;
	zts_socklen_t     msg_namelen;
	struct zts_iovec *msg_iov;
	int               msg_iovlen;
	void             *msg_control;
	zts_socklen_t     msg_controllen;
	int               msg_flags;
};

/* struct msghdr->msg_flags bit field values */
#define ZTS_MSG_TRUNC   0x04
#define ZTS_MSG_CTRUNC  0x08

/**
 * @brief Send message to remote host (sets zts_errno)
 *
 * @param fd Socket file descriptor
 * @param msg
 * @param flags
 * @return Byte count sent on success. ZTS_ERR_SOCKET, ZTS_ERR_SERVICE, ZTS_ERR_ARG on failure.
 */
ZTS_API ssize_t ZTCALL zts_sendmsg(int fd, const struct zts_msghdr *msg, int flags);

/**
 * @brief Receive data from remote host (sets zts_errno)
 *
 * @param fd Socket file descriptor
 * @param buf Pointer to data buffer
 * @param len Length of data buffer
 * @param flags
 * @return Byte count received on success. ZTS_ERR_SOCKET, ZTS_ERR_SERVICE, ZTS_ERR_ARG on failure.
 */
ZTS_API ssize_t ZTCALL zts_recv(int fd, void *buf, size_t len, int flags);

/**
 * @brief Receive data from remote host (sets zts_errno)
 *
 * @param fd Socket file descriptor
 * @param buf Pointer to data buffer
 * @param len Length of data buffer
 * @param flags
 * @param addr
 * @param addrlen
 * @return Byte count received on success. ZTS_ERR_SOCKET, ZTS_ERR_SERVICE, ZTS_ERR_ARG on failure.
 */
ZTS_API ssize_t ZTCALL zts_recvfrom(
	int fd, void *buf, size_t len, int flags, struct zts_sockaddr *addr, zts_socklen_t *addrlen);

/**
 * @brief Receive a message from remote host (sets zts_errno)
 *
 * @param fd Socket file descriptor
 * @param msg
 * @param flags
 * @return Byte count received on success. ZTS_ERR_SOCKET, ZTS_ERR_SERVICE, ZTS_ERR_ARG on failure.
 */
ZTS_API ssize_t ZTCALL zts_recvmsg(int fd, struct zts_msghdr *msg,int flags);

/**
 * @brief Read bytes from socket onto buffer (sets zts_errno)
 *
 * @param fd Socket file descriptor
 * @param buf Pointer to data buffer
 * @param len Length of data buffer to receive data
 * @return Byte count received on success. ZTS_ERR_SOCKET, ZTS_ERR_SERVICE, ZTS_ERR_ARG on failure.
 */
ZTS_API ssize_t ZTCALL zts_read(int fd, void *buf, size_t len);

/**
 * @brief Read bytes from socket into multiple buffers (sets zts_errno)
 *
 * @param fd Socket file descriptor
 * @param iov Array of destination buffers
 * @param iovcnt Number of buffers to read into
 * @return Byte count received on success. ZTS_ERR_SOCKET, ZTS_ERR_SERVICE, ZTS_ERR_ARG on failure.
 */
ZTS_API ssize_t ZTCALL zts_readv(int fd, const struct zts_iovec *iov, int iovcnt);

/**
 * @brief Write bytes from buffer to socket (sets zts_errno)
 *
 * @param fd Socket file descriptor
 * @param buf Pointer to data buffer
 * @param len Length of buffer to write
 * @return Byte count sent on success. ZTS_ERR_SOCKET, ZTS_ERR_SERVICE, ZTS_ERR_ARG on failure.
 */
ZTS_API ssize_t ZTCALL zts_write(int fd, const void *buf, size_t len);

/**
 * @brief Write data from multiple buffers to socket. (sets zts_errno)
 *
 * @param fd Socket file descriptor
 * @param iov Array of source buffers
 * @param iovcnt Number of buffers to read from
 * @return Byte count sent on success. ZTS_ERR_SOCKET, ZTS_ERR_SERVICE, ZTS_ERR_ARG on failure.
 */
ZTS_API ssize_t ZTCALL zts_writev(int fd, const struct zts_iovec *iov, int iovcnt);

#define ZTS_SHUT_RD   0x0
#define ZTS_SHUT_WR   0x1
#define ZTS_SHUT_RDWR 0x2

/**
 * @brief Shut down some aspect of a socket (sets zts_errno)
 *
 * @param fd Socket file descriptor
 * @param how Which aspects of the socket should be shut down
 * @return ZTS_ERR_OK on success. ZTS_ERR_SOCKET, ZTS_ERR_SERVICE, ZTS_ERR_ARG on failure.
 */
ZTS_API int ZTCALL zts_shutdown(int fd, int how);

//////////////////////////////////////////////////////////////////////////////
// DNS                                                                      //
//////////////////////////////////////////////////////////////////////////////

struct zts_hostent {
	char  *h_name;            /* Official name of the host. */
	char **h_aliases;         /* A pointer to an array of pointers to alternative host names,
						         terminated by a null pointer. */
	int    h_addrtype;        /* Address type. */
	int    h_length;          /* The length, in bytes, of the address. */
	char **h_addr_list;       /* A pointer to an array of pointers to network addresses (in
						         network byte order) for the host, terminated by a null pointer. */
#define h_addr h_addr_list[0] /* for backward compatibility */
};

/**
 * @brief Resolve a hostname
 *
 * @param name A null-terminated string representating the name of the host
 * @return Pointer to struct zts_hostent if successful, NULL otherwise
 */
struct zts_hostent *zts_gethostbyname(const char *name);

enum zts_ip_addr_type {
	ZTS_IPADDR_TYPE_V4 =   0U,
	ZTS_IPADDR_TYPE_V6 =   6U,
	ZTS_IPADDR_TYPE_ANY = 46U // Dual stack
};

struct zts_ip4_addr {
	uint32_t addr;
};

/** This is the aligned version of ip6_addr_t,
	used as local variable, on the stack, etc. */
struct zts_ip6_addr {
	uint32_t addr[4];
#if LWIP_IPV6_SCOPES
	uint8_t zone;
#endif /* LWIP_IPV6_SCOPES */
};

/**
 * A union struct for both IP version's addresses.
 * ATTENTION: watch out for its size when adding IPv6 address scope!
 */
typedef struct zts_ip_addr {
	union {
		struct zts_ip6_addr ip6;
		struct zts_ip4_addr ip4;
	} u_addr;
	uint8_t type; // ZTS_IPADDR_TYPE_V4, ZTS_IPADDR_TYPE_V6
} zts_ip_addr;

/**
 * Initialize one of the DNS servers.
 *
 * @param index the index of the DNS server to set must be < DNS_MAX_SERVERS
 * @param addr IP address of the DNS server to set
 */
ZTS_API int ZTCALL zts_dns_set_server(uint8_t index, const zts_ip_addr *addr);

/**
 * Obtain one of the currently configured DNS server.
 *
 * @param index the index of the DNS server
 * @return IP address of the indexed DNS server or "ip_addr_any" if the DNS
 *         server has not been configured.
 */
ZTS_API const zts_ip_addr * ZTCALL zts_dns_get_server(uint8_t index);

//////////////////////////////////////////////////////////////////////////////
// Convenience functions pulled from lwIP                                   //
//////////////////////////////////////////////////////////////////////////////

/**
 * Convert numeric IP address (both versions) into ASCII representation.
 * returns ptr to static buffer; not reentrant!
 *
 * @param addr ip address in network order to convert
 * @return pointer to a global static (!) buffer that holds the ASCII
 *         representation of addr
 */
char *zts_ipaddr_ntoa(const zts_ip_addr *addr);

/**
 * Convert IP address string (both versions) to numeric.
 * The version is auto-detected from the string.
 *
 * @param cp IP address string to convert
 * @param addr conversion result is stored here
 * @return 1 on success, 0 on error
 */
int zts_ipaddr_aton(const char *cp, zts_ip_addr *addr);

/**
 * Convert IPv4 and IPv6 address structures to human-readable text form.
 *
 * @param af Address family (ZTS_AF_INET, ZTS_AF_INET6)
 * @param src Pointer to source address structure
 * @param dst Pointer to destination character array
 * @param size Size of the destination buffer
 * @return On success, returns a non-null pointer to the destination character array
 */
ZTS_API const char * ZTCALL zts_inet_ntop(
	int af, const void *src, char *dst, zts_socklen_t size);

/**
 * Convert C-string IPv4 and IPv6 addresses to binary form.
 *
 * @param af Address family (ZTS_AF_INET, ZTS_AF_INET6)
 * @param src Pointer to source character array
 * @param dst Pointer to destination address structure
 * @return return 1 on success. 0 or -1 on failure. (Does not follow zts_* conventions)
 */
ZTS_API int ZTCALL zts_inet_pton(int af, const char *src, void *dst);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _H
