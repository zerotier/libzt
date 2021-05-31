/*
 * Copyright (c)2013-2021 ZeroTier, Inc.
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file in the project's root directory.
 *
 * Change Date: 2026-01-01
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

#ifndef ZTS_SOCKETS_H
#define ZTS_SOCKETS_H

#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------------------------------------------------//
// Error codes                                                                //
//----------------------------------------------------------------------------//

/** Common error return values */
typedef enum {
    /** No error */
    ZTS_ERR_OK = 0,
    /** Socket error, see `zts_errno` */
    ZTS_ERR_SOCKET = -1,
    /** This operation is not allowed at this time. Or possibly the node hasn't been started */
    ZTS_ERR_SERVICE = -2,
    /** Invalid argument */
    ZTS_ERR_ARG = -3,
    /** No result (not necessarily an error) */
    ZTS_ERR_NO_RESULT = -4,
    /** Consider filing a bug report */
    ZTS_ERR_GENERAL = -5
} zts_error_t;

//----------------------------------------------------------------------------//
// Event codes                                                                //
//----------------------------------------------------------------------------//

/** Event codes used by the (optional) callback API */
typedef enum {
    /**
     * Node has been initialized
     *
     * This is the first event generated, and is always sent. It may occur
     * before node's constructor returns.
     *
     */
    ZTS_EVENT_NODE_UP = 200,

    /**
     * Node is online -- at least one upstream node appears reachable
     *
     */
    ZTS_EVENT_NODE_ONLINE = 201,

    /**
     * Node is offline -- network does not seem to be reachable by any available
     * strategy
     *
     */
    ZTS_EVENT_NODE_OFFLINE = 202,

    /**
     * Node is shutting down
     *
     * This is generated within Node's destructor when it is being shut down.
     * It's done for convenience, since cleaning up other state in the event
     * handler may appear more idiomatic.
     *
     */
    ZTS_EVENT_NODE_DOWN = 203,

    /**
     * A fatal error has occurred. One possible reason is:
     *
     * Your identity has collided with another node's ZeroTier address
     *
     * This happens if two different public keys both hash (via the algorithm
     * in Identity::generate()) to the same 40-bit ZeroTier address.
     *
     * This is something you should "never" see, where "never" is defined as
     * once per 2^39 new node initializations / identity creations. If you do
     * see it, you're going to see it very soon after a node is first
     * initialized.
     *
     * This is reported as an event rather than a return code since it's
     * detected asynchronously via error messages from authoritative nodes.
     *
     * If this occurs, you must shut down and delete the node, delete the
     * identity.secret record/file from the data store, and restart to generate
     * a new identity. If you don't do this, you will not be able to communicate
     * with other nodes.
     *
     * We'd automate this process, but we don't think silently deleting
     * private keys or changing our address without telling the calling code
     * is good form. It violates the principle of least surprise.
     *
     * You can technically get away with not handling this, but we recommend
     * doing so in a mature reliable application. Besides, handling this
     * condition is a good way to make sure it never arises. It's like how
     * umbrellas prevent rain and smoke detectors prevent fires. They do, right?
     *
     * Meta-data: none
     */
    ZTS_EVENT_NODE_FATAL_ERROR = 204,

    /** Network ID does not correspond to a known network */
    ZTS_EVENT_NETWORK_NOT_FOUND = 210,
    /** The version of ZeroTier inside libzt is too old */
    ZTS_EVENT_NETWORK_CLIENT_TOO_OLD = 211,
    /** The configuration for a network has been requested (no action needed) */
    ZTS_EVENT_NETWORK_REQ_CONFIG = 212,
    /** The node joined the network successfully (no action needed) */
    ZTS_EVENT_NETWORK_OK = 213,
    /** The node is not allowed to join the network (you must authorize node) */
    ZTS_EVENT_NETWORK_ACCESS_DENIED = 214,
    /** The node has received an IPv4 address from the network controller */
    ZTS_EVENT_NETWORK_READY_IP4 = 215,
    /** The node has received an IPv6 address from the network controller */
    ZTS_EVENT_NETWORK_READY_IP6 = 216,
    /** Deprecated */
    ZTS_EVENT_NETWORK_READY_IP4_IP6 = 217,
    /** Network controller is unreachable */
    ZTS_EVENT_NETWORK_DOWN = 218,
    /** Network change received from controller */
    ZTS_EVENT_NETWORK_UPDATE = 219,

    /** TCP/IP stack (lwIP) is up (for debug purposes) */
    ZTS_EVENT_STACK_UP = 220,
    /** TCP/IP stack (lwIP) id down (for debug purposes) */
    ZTS_EVENT_STACK_DOWN = 221,

    /** lwIP netif up (for debug purposes) */
    ZTS_EVENT_NETIF_UP = 230,
    /** lwIP netif down (for debug purposes) */
    ZTS_EVENT_NETIF_DOWN = 231,
    /** lwIP netif removed (for debug purposes) */
    ZTS_EVENT_NETIF_REMOVED = 232,
    /** lwIP netif link up (for debug purposes) */
    ZTS_EVENT_NETIF_LINK_UP = 233,
    /** lwIP netif link down (for debug purposes) */
    ZTS_EVENT_NETIF_LINK_DOWN = 234,

    /** A direct P2P path to peer is known */
    ZTS_EVENT_PEER_DIRECT = 240,
    /** A direct P2P path to peer is NOT known. Traffic is now relayed  */
    ZTS_EVENT_PEER_RELAY = 241,
    /** A peer is unreachable. Check NAT/Firewall settings */
    ZTS_EVENT_PEER_UNREACHABLE = 242,
    /** A new path to a peer was discovered */
    ZTS_EVENT_PEER_PATH_DISCOVERED = 243,
    /** A known path to a peer is now considered dead */
    ZTS_EVENT_PEER_PATH_DEAD = 244,

    /** A new managed network route was added */
    ZTS_EVENT_ROUTE_ADDED = 250,
    /** A managed network route was removed */
    ZTS_EVENT_ROUTE_REMOVED = 251,

    /** A new managed IPv4 address was assigned to this peer */
    ZTS_EVENT_ADDR_ADDED_IP4 = 260,
    /** A managed IPv4 address assignment was removed from this peer  */
    ZTS_EVENT_ADDR_REMOVED_IP4 = 261,
    /** A new managed IPv4 address was assigned to this peer  */
    ZTS_EVENT_ADDR_ADDED_IP6 = 262,
    /** A managed IPv6 address assignment was removed from this peer  */
    ZTS_EVENT_ADDR_REMOVED_IP6 = 263,

    /** The node's secret key (identity) */
    ZTS_EVENT_STORE_IDENTITY_SECRET = 270,
    /** The node's public key (identity) */
    ZTS_EVENT_STORE_IDENTITY_PUBLIC = 271,
    /** The node has received an updated planet config */
    ZTS_EVENT_STORE_PLANET = 272,
    /** New reachability hints and peer configuration */
    ZTS_EVENT_STORE_PEER = 273,
    /** New network config */
    ZTS_EVENT_STORE_NETWORK = 274
} zts_event_t;

//----------------------------------------------------------------------------//
// zts_errno Error codes                                                      //
//----------------------------------------------------------------------------//

/**
 * Error variable set after each `zts_*` socket call. Provides additional error context.
 */
extern int zts_errno;

typedef enum {
    /** Operation not permitted */
    ZTS_EPERM = 1,
    /** No such file or directory */
    ZTS_ENOENT = 2,
    /** No such process */
    ZTS_ESRCH = 3,
    /** Interrupted system call */
    ZTS_EINTR = 4,
    /** I/O error */
    ZTS_EIO = 5,
    /** No such device or address */
    ZTS_ENXIO = 6,
    /** Bad file number */
    ZTS_EBADF = 9,
    /** Try again */
    ZTS_EAGAIN = 11,
    /** Operation would block */
    ZTS_EWOULDBLOCK = ZTS_EAGAIN,
    /** Out of memory */
    ZTS_ENOMEM = 12,
    /** Permission denied */
    ZTS_EACCES = 13,
    /** Bad address */
    ZTS_EFAULT = 14,
    /** Device or resource busy */
    ZTS_EBUSY = 16,
    /** File exists */
    ZTS_EEXIST = 17,
    /** No such device */
    ZTS_ENODEV = 19,
    /** Invalid argument */
    ZTS_EINVAL = 22,
    /** File table overflow */
    ZTS_ENFILE = 23,
    /** Too many open files */
    ZTS_EMFILE = 24,
    /** Function not implemented */
    ZTS_ENOSYS = 38,
    /** Socket operation on non-socket */
    ZTS_ENOTSOCK = 88,
    /** Destination address required */
    ZTS_EDESTADDRREQ = 89,
    /** Message too long */
    ZTS_EMSGSIZE = 90,
    /** Protocol wrong type for socket */
    ZTS_EPROTOTYPE = 91,
    /** Protocol not available */
    ZTS_ENOPROTOOPT = 92,
    /** Protocol not supported */
    ZTS_EPROTONOSUPPORT = 93,
    /** Socket type not supported */
    ZTS_ESOCKTNOSUPPORT = 94,
    /** Operation not supported on transport endpoint */
    ZTS_EOPNOTSUPP = 95,
    /** Protocol family not supported */
    ZTS_EPFNOSUPPORT = 96,
    /** Address family not supported by protocol */
    ZTS_EAFNOSUPPORT = 97,
    /** Address already in use */
    ZTS_EADDRINUSE = 98,
    /** Cannot assign requested address */
    ZTS_EADDRNOTAVAIL = 99,
    /** Network is down */
    ZTS_ENETDOWN = 100,
    /** Network is unreachable */
    ZTS_ENETUNREACH = 101,
    /** Software caused connection abort */
    ZTS_ECONNABORTED = 103,
    /** Connection reset by peer */
    ZTS_ECONNRESET = 104,
    /** No buffer space available */
    ZTS_ENOBUFS = 105,
    /** Transport endpoint is already connected */
    ZTS_EISCONN = 106,
    /** Transport endpoint is not connected */
    ZTS_ENOTCONN = 107,
    /** Connection timed out */
    ZTS_ETIMEDOUT = 110,
    /* Connection refused */
    ZTS_ECONNREFUSED = 111,
    /** No route to host */
    ZTS_EHOSTUNREACH = 113,
    /** Operation already in progress */
    ZTS_EALREADY = 114,
    /** Operation now in progress */
    ZTS_EINPROGRESS = 115
} zts_errno_t;

//----------------------------------------------------------------------------//
// Misc definitions                                                           //
//----------------------------------------------------------------------------//

/**
 * Length of human-readable MAC address string
 */
#define ZTS_MAC_ADDRSTRLEN 18

/**
 * Max length of human-readable IPv4 string
 */
#define ZTS_INET_ADDRSTRLEN 16

/**
 * Max length of human-readable IPv6 string
 */
#define ZTS_INET6_ADDRSTRLEN 46

/**
 * Maximum (and required) length of string buffers used to receive
 * string-format IP addresses from the API. This is set to `ZTS_INET6_ADDRSTRLEN`
 * to handle all cases: `ZTS_AF_INET` and `ZTS_AF_INET6`
 */
#define ZTS_IP_MAX_STR_LEN ZTS_INET6_ADDRSTRLEN

/**
 * Required buffer length to safely receive data store items
 */
#define ZTS_STORE_DATA_LEN 4096

/**
 * Maximum length of network short name
 */
#define ZTS_MAX_NETWORK_SHORT_NAME_LENGTH 127

/**
 * Maximum number of pushed routes on a network
 */
#define ZTS_MAX_NETWORK_ROUTES 32

/**
 * Maximum number of statically assigned IP addresses per network endpoint
 * using ZT address management (not DHCP)
 */
#define ZTS_MAX_ASSIGNED_ADDRESSES 16

/**
 * Maximum number of direct network paths to a given peer
 */
#define ZTS_MAX_PEER_NETWORK_PATHS 16

/**
 * Maximum number of multicast groups a device / network interface can be
 * subscribed to at once
 */
#define ZTS_MAX_MULTICAST_SUBSCRIPTIONS 1024

#define ZTS_MAX_ENDPOINT_STR_LEN ZTS_INET6_ADDRSTRLEN + 6

//----------------------------------------------------------------------------//
// Misc                                                                       //
//----------------------------------------------------------------------------//

#if ! defined(ZTS_ENABLE_PYTHON) && ! defined(ZTS_ENABLE_PINVOKE) && ! defined(ZTS_ENABLE_JAVA)
#define ZTS_C_API_ONLY 1
#endif

#if ! ZTS_NO_STDINT_H
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

#ifdef ZTS_ENABLE_PINVOKE
// Used by P/INVOKE wrappers
typedef void (*CppCallback)(void* msg);
#endif

//----------------------------------------------------------------------------//
// Common definitions and structures for interoperability between zts_* and   //
// lwIP functions. Some of the code in the following section is a borrowed    //
// from the lwIP codebase so that the user doesn't need to include headers    //
// from that project in addition to the ZeroTier SDK headers. The license     //
// applying to this code borrowed from lwIP is produced below and only        //
// applies to the portions of code which are merely renamed versions of       //
// their lwIP counterparts. The rest of the code in this C API file is        //
// governed by the license text provided at the beginning of this file.       //
//----------------------------------------------------------------------------//

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
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
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/** 255.255.255.255 */
#define ZTS_IPADDR_NONE ((uint32_t)0xffffffffUL)
/** 127.0.0.1 */
#define ZTS_IPADDR_LOOPBACK ((uint32_t)0x7f000001UL)
/** 0.0.0.0 */
#define ZTS_IPADDR_ANY ((uint32_t)0x00000000UL)
/** 255.255.255.255 */
#define ZTS_IPADDR_BROADCAST ((uint32_t)0xffffffffUL)

/** 255.255.255.255 */
#define ZTS_INADDR_NONE ZTS_IPADDR_NONE
/** 127.0.0.1 */
#define ZTS_INADDR_LOOPBACK ZTS_IPADDR_LOOPBACK
/** 0.0.0.0 */
#define ZTS_INADDR_ANY ZTS_IPADDR_ANY
/** 255.255.255.255 */
#define ZTS_INADDR_BROADCAST ZTS_IPADDR_BROADCAST

// Socket protocol types
#define ZTS_SOCK_STREAM 0x0001
#define ZTS_SOCK_DGRAM  0x0002
#define ZTS_SOCK_RAW    0x0003
// Socket family types
#define ZTS_AF_UNSPEC 0x0000
#define ZTS_AF_INET   0x0002
#define ZTS_AF_INET6  0x000a
#define ZTS_PF_INET   ZTS_AF_INET
#define ZTS_PF_INET6  ZTS_AF_INET6
#define ZTS_PF_UNSPEC ZTS_AF_UNSPEC
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
#define ZTS_MSG_PEEK     0x0001
#define ZTS_MSG_WAITALL  0x0002   // NOT YET SUPPORTED
#define ZTS_MSG_OOB      0x0004   // NOT YET SUPPORTED
#define ZTS_MSG_DONTWAIT 0x0008
#define ZTS_MSG_MORE     0x0010

// Macro's for defining ioctl() command values
#define ZTS_IOCPARM_MASK 0x7fU
#define ZTS_IOC_VOID     0x20000000UL
#define ZTS_IOC_OUT      0x40000000UL
#define ZTS_IOC_IN       0x80000000UL
#define ZTS_IOC_INOUT    (ZTS_IOC_IN | ZTS_IOC_OUT)
#define ZTS_IO(x, y)     (ZTS_IOC_VOID | ((x) << 8) | (y))
#define ZTS_IOR(x, y, t) (ZTS_IOC_OUT | (((long)sizeof(t) & ZTS_IOCPARM_MASK) << 16) | ((x) << 8) | (y))
#define ZTS_IOW(x, y, t) (ZTS_IOC_IN | (((long)sizeof(t) & ZTS_IOCPARM_MASK) << 16) | ((x) << 8) | (y))
// ioctl() commands
#define ZTS_FIONREAD ZTS_IOR('f', 127, unsigned long)
#define ZTS_FIONBIO  ZTS_IOW('f', 126, unsigned long)

//----------------------------------------------------------------------------//
// Custom but still mostly standard socket interface structures               //
//----------------------------------------------------------------------------//

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
        uint8_t u8_addr[16];
    } un;
    //#define s6_addr  un.u8_addr
};

/**
 * Address structure to specify an IPv4 endpoint
 */
struct zts_sockaddr_in {
    uint8_t sin_len;
    zts_sa_family_t sin_family;
    zts_in_port_t sin_port;
    struct zts_in_addr sin_addr;
#define SIN_ZERO_LEN 8
    char sin_zero[SIN_ZERO_LEN];
};

/**
 * Address structure to specify an IPv6 endpoint
 */
struct zts_sockaddr_in6 {
    uint8_t sin6_len;                // length of this structure
    zts_sa_family_t sin6_family;     // ZTS_AF_INET6
    zts_in_port_t sin6_port;         // Transport layer port #
    uint32_t sin6_flowinfo;          // IPv6 flow information
    struct zts_in6_addr sin6_addr;   // IPv6 address
    uint32_t sin6_scope_id;          // Set of interfaces for scope
};

/**
 * Pointers to socket address structures are often cast to this type
 */
struct zts_sockaddr {
    uint8_t sa_len;
    zts_sa_family_t sa_family;
    char sa_data[14];
};

/**
 * Address structure large enough to hold IPv4 and IPv6 addresses
 */
struct zts_sockaddr_storage {
    uint8_t s2_len;
    zts_sa_family_t ss_family;
    char s2_data1[2];
    uint32_t s2_data2[3];
    uint32_t s2_data3[3];
};

//----------------------------------------------------------------------------//
// Callback Structures                                                        //
//----------------------------------------------------------------------------//

/**
 * Runtime details about the current node
 */
typedef struct {
    /**
     * Node ID
     */
    uint64_t node_id;

    /**
     * Port used by ZeroTier to send and receive traffic
     */
    uint16_t port_primary;

    /**
     * Port used by ZeroTier to send and receive traffic
     */
    uint16_t port_secondary;

    /**
     * Port used by ZeroTier to send and receive traffic
     */
    uint16_t port_tertiary;

    /**
     * ZT Major version
     */
    uint8_t ver_major;

    /**
     * ZT Minor version
     */
    uint8_t ver_minor;

    /**
     * ZT Patch revision
     */
    uint8_t ver_rev;
} zts_node_info_t;

/**
 * Details about an assigned address that was added or removed
 */
typedef struct {
    uint64_t net_id;
    struct zts_sockaddr_storage addr;
} zts_addr_info_t;

/**
 * Virtual network status codes
 */
typedef enum {
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
} zts_network_status_t;

/**
 * Virtual network type codes
 */
typedef enum {
    /**
     * Private networks are authorized via certificates of membership
     */
    ZTS_NETWORK_TYPE_PRIVATE = 0,

    /**
     * Public networks have no access control -- they'll always be AUTHORIZED
     */
    ZTS_NETWORK_TYPE_PUBLIC = 1
} zts_net_info_type_t;

/**
 * A route to be pushed on a virtual network
 */
typedef struct {
    /**
     * Target network / netmask bits (in port field) or NULL or 0.0.0.0/0
     * for default
     */
    struct zts_sockaddr_storage target;

    /**
     * Gateway IP address (port ignored) or NULL (family == 0) for LAN-local
     * (no gateway)
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
} zts_route_info_t;

/**
 * An Ethernet multicast group
 */
typedef struct {
    /**
     * MAC address (least significant 48 bits)
     */
    uint64_t mac;

    /**
     * Additional distinguishing information (usually zero)
     */
    unsigned long adi;
} zts_multicast_group_t;

/**
 * The peer's trust hierarchy role
 */
typedef enum {
    /**
     * Ordinary node
     */
    ZTS_PEER_ROLE_LEAF = 0,
    /**
     * Moon root
     */
    ZTS_PEER_ROLE_MOON = 1,
    /**
     * Planetary root
     */
    ZTS_PEER_ROLE_PLANET = 2
} zts_peer_role_t;

/**
 * Virtual network configuration
 */
typedef struct {
    /**
     * 64-bit ZeroTier network ID
     */
    uint64_t net_id;

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
    zts_network_status_t status;

    /**
     * Network type
     */
    zts_net_info_type_t type;

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
     * If nonzero, this network supports and allows broadcast
     * (ff:ff:ff:ff:ff:ff) traffic
     */
    int broadcast_enabled;

    /**
     * If the network is in PORT_ERROR state, this is the (negative) error code
     * most recently reported
     */
    int port_error;

    /**
     * Revision number as reported by controller or 0 if still waiting for
     * config
     */
    unsigned long netconf_rev;

    /**
     * Number of assigned addresses
     */
    unsigned int assigned_addr_count;

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
    struct zts_sockaddr_storage assigned_addrs[ZTS_MAX_ASSIGNED_ADDRESSES];

    /**
     * Number of ZT-pushed routes
     */
    unsigned int route_count;

    /**
     * Routes (excluding those implied by assigned addresses and their masks)
     */
    zts_route_info_t routes[ZTS_MAX_NETWORK_ROUTES];

    /**
     * Number of multicast groups subscribed
     */
    unsigned int multicast_sub_count;

    /**
     * Multicast groups to which this network's device is subscribed
     */
    struct {
        uint64_t mac; /* MAC in lower 48 bits */
        uint32_t adi; /* Additional distinguishing information, usually zero
                         except for IPv4 ARP groups */
    } multicast_subs[ZTS_MAX_MULTICAST_SUBSCRIPTIONS];
} zts_net_info_t;

/**
 * Physical network path to a peer
 */
typedef struct {
    /**
     * Address of endpoint
     */
    struct zts_sockaddr_storage address;

    /**
     * Time of last send in milliseconds or 0 for never
     */
    uint64_t last_tx;

    /**
     * Time of last receive in milliseconds or 0 for never
     */
    uint64_t last_rx;

    /**
     * Is this a trusted path? If so this will be its nonzero ID.
     */
    uint64_t trusted_path_id;

    /**
     * One-way latency
     */
    float latency;

    float unused_0;
    float unused_1;
    float unused_2;
    float unused_3;
    float unused_4;
    uint64_t unused_5;
    uint64_t unused_6;
    float unused_7;

    /**
     * Name of physical interface (for monitoring)
     */
    char* ifname;

    /**
     * Is path expired?
     */
    int expired;

    /**
     * Is path preferred?
     */
    int preferred;
} zts_path_t;

/**
 * Peer status result buffer
 */
typedef struct {
    /**
     * ZeroTier address (40 bits)
     */
    uint64_t peer_id;

    /**
     * Remote major version or -1 if not known
     */
    int ver_major;

    /**
     * Remote minor version or -1 if not known
     */
    int ver_minor;

    /**
     * Remote revision or -1 if not known
     */
    int ver_rev;

    /**
     * Last measured latency in milliseconds or -1 if unknown
     */
    int latency;

    /**
     * What trust hierarchy role does this device have?
     */
    zts_peer_role_t role;

    /**
     * Number of paths (size of paths[])
     */
    unsigned int path_count;

    /**
     * Whether this peer was ever reachable via an aggregate link
     */
    int unused_0;

    /**
     * Known network paths to peer
     */
    zts_path_t paths[ZTS_MAX_PEER_NETWORK_PATHS];
} zts_peer_info_t;

#define ZTS_MAX_NUM_ROOTS          16
#define ZTS_MAX_ENDPOINTS_PER_ROOT 32

/**
 * Structure used to specify a root topology (aka a world)
 */
typedef struct {
    char* public_id_str[ZTS_MAX_NUM_ROOTS];
    char* endpoint_ip_str[ZTS_MAX_NUM_ROOTS][ZTS_MAX_ENDPOINTS_PER_ROOT];
} zts_root_set_t;

/**
 * Structure used to convey information about a virtual network
 * interface (netif) to a user application.
 */
typedef struct {
    /**
     * The virtual network that this interface was created for
     */
    uint64_t net_id;

    /**
     * The hardware address assigned to this interface
     */
    uint64_t mac;

    /**
     * The MTU for this interface
     */
    int mtu;
} zts_netif_info_t;

/**
 * Callback message
 */
typedef struct {
    /**
     * Event identifier
     */
    int16_t event_code;
    /**
     * Node status
     */
    zts_node_info_t* node;
    /**
     * Network information
     */
    zts_net_info_t* network;
    /**
     * Netif status
     */
    zts_netif_info_t* netif;
    /**
     * Managed routes
     */
    zts_route_info_t* route;
    /**
     * Peer info
     */
    zts_peer_info_t* peer;
    /**
     * Assigned address
     */
    zts_addr_info_t* addr;
    /**
     * Binary data (identities, planets, network configs, peer hints, etc)
     */
    void* cache;
    /**
     * Length of data message or structure
     */
    int len;
} zts_event_msg_t;

//----------------------------------------------------------------------------//
// ZeroTier Service and Network Controls                                      //
//----------------------------------------------------------------------------//

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

//----------------------------------------------------------------------------//
// Central API                                                                //
//----------------------------------------------------------------------------//

#define ZTS_DISABLE_CENTRAL_API 1

#ifndef ZTS_DISABLE_CENTRAL_API

#define ZTS_CENTRAL_DEFAULT_URL         "https://my.zerotier.com"
#define ZTS_CENRTAL_MAX_URL_LEN         128
#define ZTS_CENTRAL_TOKEN_LEN           32
#define ZTS_CENTRAL_RESP_BUF_DEFAULT_SZ (128 * 1024)

#define ZTS_HTTP_GET    0
#define ZTS_HTTP_POST   1
#define ZTS_HTTP_DELETE 2

#define ZTS_CENTRAL_NODE_AUTH_FALSE 0
#define ZTS_CENTRAL_NODE_AUTH_TRUE  1

#define ZTS_CENTRAL_READ  1
#define ZTS_CENTRAL_WRITE 2

/**
 * @brief Enable read/write capability. Default before calling this is
 * read-only: `ZTS_CENTRAL_READ`
 *
 * @param modes `ZTS_CENTRAL_READ` and/or `ZTS_CENTRAL_WRITE`. Whether the API allows read, write,
 * or both
 *
 * @return `ZTS_ERR_OK` if successful. `ZTS_ERR_ARG` if invalid argument.
 */
ZTS_API int ZTCALL zts_central_set_access_mode(int8_t modes);

/**
 * @brief Enable or disable libcurl verbosity
 *
 * @param is_verbose `[1, 0]`, Whether debug information is desired
 *
 * @return `ZTS_ERR_OK` if successful. `ZTS_ERR_ARG` if invalid argument.
 */
ZTS_API int ZTCALL zts_central_set_verbose(int8_t is_verbose);

ZTS_API void ZTCALL zts_central_clear_resp_buf();

/**
 * @brief Set the Central API `URL` and user API token.
 *
 * @param url_str The URL to the Central API server
 * @param token_str User API token
 * @param resp_buf Destination buffer for raw `JSON` output
 * @param buf_len Size of buffer for server response (specify `0` for default
 * size)
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_ARG` if invalid argument.
 */
ZTS_API int ZTCALL zts_central_init(const char* url_str, const char* token_str, char* resp_buf, uint32_t buf_len);

ZTS_API void ZTCALL zts_central_cleanup();

/**
 * @brief Copies the `JSON`-formatted string buffer from the last request into
 *        a user-provided buffer.
 *
 * @param dst User-provided destination buffer
 * @param len Length of aforementioned buffer
 * @return `ZTS_ERR_OK` if all contents were copied successfully.
 *     `ZTS_ERR_ARG` if provided buffer was too small.
 */
ZTS_API int ZTCALL zts_central_get_last_resp_buf(char* dst, int len);

/**
 * @brief Get the status of the Central API server.
 *
 * @return Standard HTTP response codes.
 */
ZTS_API int ZTCALL zts_central_status_get(int* http_resp_code);

/**
 * @brief Get the currently authenticated user’s record.
 *
 * @return Standard HTTP response codes.
 */
ZTS_API int ZTCALL zts_central_self_get(int* http_resp_code);

/**
 * @brief Retrieve a `Network`.
 *
 * @return Standard HTTP response codes.
 */
ZTS_API int ZTCALL zts_central_net_get(int* http_resp_code, uint64_t net_id);

/**
 * @brief Update or create a `Network`.
 *
 * Only fields marked as [rw] can be directly modified. If other fields are
 * present in the posted request they are ignored. New networks can be
 * created by POSTing to /api/network with no net_id parameter. The server
 * will create a random unused network ID and return the new network record.
 *
 * @return Standard HTTP response codes.
 */
ZTS_API int ZTCALL zts_central_net_update(int* http_resp_code, uint64_t net_id);

/**
 * @brief Delete a Network.
 *
 * Delete a network and all its related information permanently.
 * Use extreme caution as this cannot be undone!
 *
 * @return Standard HTTP response codes.
 */
ZTS_API int ZTCALL zts_central_net_delete(int* http_resp_code, uint64_t net_id);

/**
 * @brief Get All Viewable Networks.
 *
 * Get all networks for which you have at least read access.
 *
 * @return Standard HTTP response codes.
 */
ZTS_API int ZTCALL zts_central_net_get_all(int* http_resp_code);
/**
 * @brief Retrieve a Member.
 *
 * @return Standard HTTP response codes.
 */
ZTS_API int ZTCALL zts_central_member_get(int* http_resp_code, uint64_t net_id, uint64_t node_id);

/**
 * @brief Update or add a Member.
 *
 * New members can be added to a network by POSTing them.
 *
 * @return Standard HTTP response codes.
 */
ZTS_API int ZTCALL zts_central_member_update(int* http_resp_code, uint64_t net_id, uint64_t node_id, char* post_data);

/**
 * @brief Authorize or (De)authorize a node on a network. This operation
 * is idempotent.
 *
 * @param net_id Network ID
 * @param node_id Node ID
 * @param is_authed Boolean value for whether this node should be authorized
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_ARG` if invalid argument.
 */
ZTS_API int ZTCALL zts_central_node_auth(int* http_resp_code, uint64_t net_id, uint64_t node_id, uint8_t is_authed);

/**
 * @brief Get All Members of a Network.
 *
 * Get all members of a network for which you have at least read access.
 *
 * @return Standard HTTP response codes.
 */
ZTS_API int ZTCALL zts_central_net_get_members(int* http_resp_code, uint64_t net_id);

#endif   // ZTS_DISABLE_CENTRAL_API

//----------------------------------------------------------------------------//
// Identity Management                                                        //
//----------------------------------------------------------------------------//

/**
 * The length of a human-friendly identity key pair string
 */
#define ZTS_ID_STR_BUF_LEN 384

/**
 * @brief Generates a node identity (public/secret key-pair) and stores it in a
 *     user-provided buffer.
 *
 * @param key User-provided destination buffer
 * @param key_buf_len Length of user-provided destination buffer. Will be set
 *     to the number of bytes copied.
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_ARG` if invalid argument.
 */
ZTS_API int ZTCALL zts_id_new(char* key, unsigned int* key_buf_len);

/**
 * @brief Verifies that a key-pair is valid. Checks formatting and pairing of
 *    key to address.
 *
 * @param key Buffer containing key-pair
 * @param len Length of key-pair buffer
 * @return `1` if true, `0` if false.
 */
ZTS_API int ZTCALL zts_id_pair_is_valid(const char* key, unsigned int len);

/**
 * @brief Instruct ZeroTier to look for node identity files at the given location. This is an
 * initialization function that can only be called before `zts_node_start()`.
 *
 * Note that calling this function is not mandatory and if it is not called the node's keys will be
 * kept in memory and retrievable via `zts_node_get_id_pair()`.
 *
 * See also: `zts_init_from_memory()`
 *
 * @param port Path Null-terminated file-system path string
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument.
 */
ZTS_API int ZTCALL zts_init_from_storage(const char* path);

/**
 * @brief Instruct ZeroTier to use the identity provided in `key`. This is an initialization
 * function that can only be called before `zts_node_start()`.
 *
 * Note that calling this function is not mandatory and if it is not called the node's keys will be
 * kept in memory and retrievable via `zts_node_get_id_pair()`.
 *
 * See also: `zts_init_from_storage()`
 *
 * @param key Path Null-terminated file-system path string
 * @param len Length of `key` buffer
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument.
 */
ZTS_API int ZTCALL zts_init_from_memory(const char* key, unsigned int len);

#ifdef ZTS_ENABLE_PYTHON
#include "Python.h"

/**
 * Abstract class used as a director. Pointer to an instance of this class
 * is provided to the Python layer.
 */
class PythonDirectorCallbackClass {
  public:
    /**
     * Called by native code on event. Implemented in Python
     */
    virtual void on_zerotier_event(zts_event_msg_t* msg);
    virtual ~PythonDirectorCallbackClass() {};
};

extern PythonDirectorCallbackClass* _userEventCallback;

/**
 * @brief Set the event handler function. This is an initialization function that can only be called
 * before `zts_node_start()`.
 *
 * @param callback A function pointer to the event handler function
 * @param family `ZTS_AF_INET`, or `ZTS_AF_INET6`
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument.
 */
ZTS_API int ZTCALL zts_init_set_event_handler(PythonDirectorCallbackClass* callback);
#endif
#ifdef ZTS_ENABLE_PINVOKE
ZTS_API int ZTCALL zts_init_set_event_handler(CppCallback callback);
#endif
#ifdef ZTS_ENABLE_JAVA
#include <jni.h>
int zts_init_set_event_handler(jobject obj_ref, jmethodID id);
#endif
#ifdef ZTS_C_API_ONLY
ZTS_API int ZTCALL zts_init_set_event_handler(void (*callback)(void*));
#endif

/**
 * @brief Blacklist an interface prefix (or name). This prevents ZeroTier from
 * sending traffic over matching interfaces. This is an initialization function that can
 * only be called before `zts_node_start()`.
 *
 * @param prefix Null-terminated interface prefix string
 * @param len Length of prefix string
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument.
 */
ZTS_API int ZTCALL zts_init_blacklist_if(const char* prefix, unsigned int len);

/**
 * @brief Present a root set definition for ZeroTier to use instead of the default.
 * This is an initialization function that can only be called before `zts_node_start()`.
 *
 * @param roots_data Array of roots definition data (binary)
 * @param len Length of binary data
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument.
 */
ZTS_API int ZTCALL zts_init_set_roots(const void* roots_data, unsigned int len);

/**
 * @brief Set the port to which the node should bind. This is an initialization function that can
 * only be called before `zts_node_start()`.
 *
 * @param port Port number
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument.
 */
ZTS_API int ZTCALL zts_init_set_port(unsigned short port);

/**
 * @brief Set range that random ports will be selected from. This is an initialization function that can
 * only be called before `zts_node_start()`.
 *
 * @param start_port Start of port range
 * @param end_port End of port range
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument.
 */
ZTS_API int ZTCALL zts_init_set_random_port_range(unsigned short start_port, unsigned short end_port);

/**
 * @brief Allow or disallow ZeroTier from automatically selecting a backup port to help get through
 * buggy NAT. This is enabled by default. This port is randomly chosen and should be disabled if you
 * want to control exactly which ports ZeroTier talks on and (iff) you know with absolute certainty
 * that traffic on your chosen primary port is allowed. This is an initialization function that can
 * only be called before `zts_node_start()`.
 *
 * @param port Port number
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument.
 */
ZTS_API int ZTCALL zts_init_allow_secondary_port(unsigned int allowed);

/**
 * @brief Allow or disallow the use of port-mapping. This is enabled by default. This is an
 * initialization function that can only be called before `zts_node_start()`.
 *
 * @param port Port number
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument.
 */
ZTS_API int ZTCALL zts_init_allow_port_mapping(unsigned int allowed);

/**
 * @brief Enable or disable whether the node will cache network details
 * (enabled by default when `zts_init_from_storage()` is used.) Must be called before
 * `zts_node_start()`.
 *
 * This can potentially shorten (startup) times between node restarts. This allows the service to
 * nearly instantly inform the network stack of an address to use for this peer
 * so that it can create a transport service. This can be disabled for cases where one
 * may not want network config details to be written to storage. This is
 * especially useful for situations where address assignments do not change
 * often.
 *
 * See also: `zts_init_allow_peer_cache()`
 *
 * @param enabled Whether or not this feature is enabled
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument.
 */
ZTS_API int ZTCALL zts_init_allow_net_cache(unsigned int allowed);

/**
 * @brief Enable or disable whether the node will cache peer details (enabled
 * by default when `zts_init_from_storage()` is used.) Must be called before `zts_node_start()`.
 *
 * This can potentially shorten (connection) times between node restarts. This allows the service to
 * re-use previously discovered paths to a peer, this prevents the service from
 * having to go through the entire transport-triggered link provisioning
 * process. This is especially useful for situations where paths to peers do not
 * change often. This is enabled by default and can be disabled for cases where
 * one may not want peer details to be written to storage.
 *
 * See also: `zts_init_allow_net_cache()`
 *
 * @param enabled Whether or not this feature is enabled
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument.
 */
ZTS_API int ZTCALL zts_init_allow_peer_cache(unsigned int allowed);

/**
 * @brief Enable or disable whether the node will cache root definitions (enabled
 * by default when `zts_init_from_storage()` is used.) Must be called before `zts_node_start()`.
 *
 * @param enabled Whether or not this feature is enabled
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument.
 */
ZTS_API int ZTCALL zts_init_allow_roots_cache(unsigned int allowed);

/**
 * @brief Enable or disable whether the node will cache identities (enabled
 * by default when `zts_init_from_storage()` is used.) Must be called before `zts_node_start()`.
 *
 * @param enabled Whether or not this feature is enabled
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument.
 */
ZTS_API int ZTCALL zts_init_allow_id_cache(unsigned int allowed);

/**
 * @brief Return whether an address of the given family has been assigned by the network
 *
 * @param net_id Network ID
 * @param family `ZTS_AF_INET`, or `ZTS_AF_INET6`
 * @return `1` if true, `0` if false.
 */
ZTS_API int ZTCALL zts_addr_is_assigned(uint64_t net_id, unsigned int family);

/**
 * @brief Get the first-assigned IP on the given network.
 *
 * To get *all* assigned addresses on a given network, use `zts_addr_get_all()`.
 *
 * @param net_id Network ID
 * @param family `ZTS_AF_INET`, or `ZTS_AF_INET6`
 * @param addr Destination buffer to hold address
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument.
 */
ZTS_API int ZTCALL zts_addr_get(uint64_t net_id, unsigned int family, struct zts_sockaddr_storage* addr);

/**
 * @brief Get the first-assigned IP on the given network as a null-terminated human-readable string
 *
 * To get *all* assigned addresses on a given network, use `zts_addr_get_all()`.
 *
 * @param net_id Network ID
 * @param family `ZTS_AF_INET`, or `ZTS_AF_INET6`
 * @param dst Destination buffer
 * @param len Length of destination buffer (must be exactly `ZTS_IP_MAX_STR_LEN`)
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument.
 */
ZTS_API int ZTCALL zts_addr_get_str(uint64_t net_id, unsigned int family, char* dst, unsigned int len);

/**
 * @brief Get all IP addresses assigned to this node by the given network
 *
 * @param net_id Network ID
 * @param addr Destination buffer to hold address
 * @param count Number of addresses returned
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument.
 */
ZTS_API int ZTCALL zts_addr_get_all(uint64_t net_id, struct zts_sockaddr_storage* addr, unsigned int* count);

/**
 * @brief Compute a `6PLANE` IPv6 address for the given Network ID and Node ID
 *
 * @param net_id Network ID
 * @param node_id Node ID
 * @param addr Destination structure for address
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_ARG` if invalid argument.
 */
ZTS_API int ZTCALL
zts_addr_compute_6plane(const uint64_t net_id, const uint64_t node_id, struct zts_sockaddr_storage* addr);

/**
 * @brief Compute `RFC4193` IPv6 address for the given Network ID and Node ID
 *
 * @param net_id Network ID
 * @param node_id Node ID
 * @param addr Destination structure for address
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_ARG` if invalid argument.
 */
ZTS_API int ZTCALL
zts_addr_compute_rfc4193(const uint64_t net_id, const uint64_t node_id, struct zts_sockaddr_storage* addr);

/**
 * @brief Compute `RFC4193` IPv6 address for the given Network ID and Node ID and copy its
 * null-terminated human-readable string representation into destination buffer.
 *
 * @param net_id Network ID
 * @param node_id Node ID
 * @param dst Destination string buffer
 * @param len Length of destination string buffer (must be exactly `ZTS_IP_MAX_STR_LEN`)
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_ARG` if invalid argument.
 */
ZTS_API int ZTCALL zts_addr_compute_rfc4193_str(uint64_t net_id, uint64_t node_id, char* dst, unsigned int len);

/**
 * @brief Compute `6PLANE` IPv6 address for the given Network ID and Node ID and copy its
 * null-terminated human-readable string representation into destination buffer.
 *
 * @param net_id Network ID
 * @param node_id Node ID
 * @param dst Destination string buffer
 * @param len Length of destination string buffer (must be exactly `ZTS_IP_MAX_STR_LEN`)
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_ARG` if invalid argument.
 */
ZTS_API int ZTCALL zts_addr_compute_6plane_str(uint64_t net_id, uint64_t node_id, char* dst, unsigned int len);

/**
 * @brief Compute `RFC4193` IPv6 address for the given Network ID and Node ID
 *
 * Ad-hoc Network:
 * ```
 * ffSSSSEEEE000000
 * | |   |   |
 * | |   |   Reserved for future use, must be 0
 * | |   End of port range (hex)
 * | Start of port range (hex)
 * Reserved ZeroTier address prefix indicating a controller-less network.
 * ```
 * Ad-hoc networks are public (no access control) networks that have no network
 * controller. Instead their configuration and other credentials are generated
 * locally. Ad-hoc networks permit only IPv6 UDP and TCP unicast traffic
 * (no multicast or broadcast) using 6plane format NDP-emulated IPv6 addresses.
 * In addition an ad-hoc network ID encodes an IP port range. UDP packets and
 * TCP SYN (connection open) packets are only allowed to destination ports
 * within the encoded range.
 *
 * For example `ff00160016000000` is an ad-hoc network allowing only SSH,
 * while `ff0000ffff000000` is an ad-hoc network allowing any UDP or TCP port.
 *
 * Keep in mind that these networks are public and anyone in the entire world
 * can join them. Care must be taken to avoid exposing vulnerable services or
 * sharing unwanted files or other resources.
 *
 *
 * @param start_port Start of port allowed port range
 * @param end_port End of allowed port range
 * @return An Ad-hoc network ID
 */
ZTS_API uint64_t ZTCALL zts_net_compute_adhoc_id(uint16_t start_port, uint16_t end_port);

/**
 * @brief Join a network
 *
 * @param net_id Network ID
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument.
 */
ZTS_API int ZTCALL zts_net_join(uint64_t net_id);

/**
 * @brief Leave a network
 *
 * @param net_id Network ID
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument.
 */
ZTS_API int ZTCALL zts_net_leave(uint64_t net_id);

/**
 * @brief Return whether this network is ready to send and receive traffic.
 *
 * @return `1` if true, `0` if false.
 */
ZTS_API int ZTCALL zts_net_transport_is_ready(const uint64_t net_id);

/**
 * @brief Get the MAC Address for this node on the given network
 *
 * @param net_id Network ID
 *
 * @return MAC address in numerical format
 */
ZTS_API uint64_t ZTCALL zts_net_get_mac(uint64_t net_id);

/**
 * @brief Get the MAC Address for this node on the given network
 *
 * @param net_id Network ID
 * @param dst Destination string buffer
 * @param len Length of destination string buffer. Must be exactly `ZTS_MAC_ADDRSTRLEN`
 *
 * @return MAC address in string format
 */
ZTS_API int ZTCALL zts_net_get_mac_str(uint64_t net_id, char* dst, unsigned int len);

/**
 * @brief Return whether broadcast is enabled on this network
 *
 * @param net_id Network ID
 *
 * @return `1` if true, `0` if false.
 */
ZTS_API int ZTCALL zts_net_get_broadcast(uint64_t net_id);

/**
 * @brief Get the MTU of the given network
 *
 * @param net_id Network ID
 *
 * @return MTU
 */
ZTS_API int ZTCALL zts_net_get_mtu(uint64_t net_id);

/**
 * @brief Get the nickname of the network
 *
 * @param net_id Network ID
 * @param dst Destination string buffer
 * @param len Length of destination string buffer
 *
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument.
 */
ZTS_API int ZTCALL zts_net_get_name(uint64_t net_id, char* dst, unsigned int len);

/**
 * @brief Get the status of the network
 *
 * @param net_id Network ID
 *
 * @return Status
 */
ZTS_API int ZTCALL zts_net_get_status(uint64_t net_id);

/**
 * @brief Get the type of network (public or private.)
 *
 * @param net_id Network ID
 *
 * @return Type
 */
ZTS_API int ZTCALL zts_net_get_type(uint64_t net_id);

/**
 * @brief Return whether a managed route of the given address family has been assigned by the
 * network
 *
 * @param net_id Network ID
 * @param family `ZTS_AF_INET`, or `ZTS_AF_INET6`
 * @return `1` if true, `0` if false.
 */
ZTS_API int ZTCALL zts_route_is_assigned(uint64_t net_id, unsigned int family);

/**
 * @brief Start the ZeroTier node. Should be called after calling the relevant
 *    `zts_init_*` functions for your application. To enable storage call
 *    `zts_init_from_storage()` before this function. To enable event callbacks
 *     call `zts_init_set_event_handler()` before this function.
 *
 * Note: If neither `zts_init_from_storage()` or `zts_init_from_memory()` are
 * called a new identity will be generated and will be retrievable via
 * `zts_node_get_id_pair()` *after* the node has started.
 *
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem.
 */
ZTS_API int ZTCALL zts_node_start();

/**
 * @brief Return whether the node is online (Can reach the Internet)
 *
 * @return `1` if true, `0` if false.
 */
ZTS_API int ZTCALL zts_node_is_online();

/**
 * @brief Get the public node identity (aka `node_id`). Callable only after the node has been
 * started.
 *
 * @return Identity in numerical form
 */
ZTS_API uint64_t ZTCALL zts_node_get_id();

/**
 * @brief Copy the current node's public (and secret!) identity into a buffer.
 *
 * `WARNING`: This function exports your secret key and should be used carefully.
 *
 * @param key User-provided destination buffer
 * @param key_dst_len Length of user-provided destination buffer. Will be set to
 * number of bytes copied.
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument.
 */
ZTS_API int ZTCALL zts_node_get_id_pair(char* key, unsigned int* key_dst_len);

/**
 * @brief Get the primary port to which the node is bound. Callable only after the node has been
 * started.
 *
 * @return Port number
 */
ZTS_API int ZTCALL zts_node_get_port();

/**
 * @brief Stop the ZeroTier node and bring down all virtual network
 *     transport services. Callable only after the node has been started.
 *
 * While the ZeroTier will stop, the stack driver (with associated
 * timers) will remain active in case future traffic processing is required.
 * To stop all activity and free all resources use `zts_free()` instead.
 *
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem.
 */
ZTS_API int ZTCALL zts_node_stop();

/**
 * @brief Stop all background threads, bring down all transport services, free all
 *     resources. After calling this function an application restart will be
 *     required before the library can be used again. Callable only after the node
 *     has been started.
 *
 * This should be called at the end of your program or when you do not
 * anticipate communicating over ZeroTier again.
 *
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem.
 */
ZTS_API int ZTCALL zts_node_free();

/**
 * @brief Orbit a given moon (user-defined root server)
 *
 * @param moon_roots_id World ID
 * @param moon_seed Seed ID
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument.
 */
ZTS_API int ZTCALL zts_moon_orbit(uint64_t moon_roots_id, uint64_t moon_seed);

/**
 * @brief De-orbit a given moon (user-defined root server)
 *
 * @param moon_roots_id World ID
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument.
 */
ZTS_API int ZTCALL zts_moon_deorbit(uint64_t moon_roots_id);

//----------------------------------------------------------------------------//
// Statistics                                                                 //
//----------------------------------------------------------------------------//

/**
 * Structure containing counters for various protocol statistics
 */
typedef struct {
    /** Number of link packets transmitted */
    uint32_t link_tx;
    /** Number of link packets received */
    uint32_t link_rx;
    /** Number of link packets dropped */
    uint32_t link_drop;
    /** Aggregate number of link-level errors */
    uint32_t link_err;

    /** Number of etharp packets transmitted */
    uint32_t etharp_tx;
    /** Number of etharp packets received */
    uint32_t etharp_rx;
    /** Number of etharp packets dropped */
    uint32_t etharp_drop;
    /** Aggregate number of etharp errors */
    uint32_t etharp_err;

    /** Number of IPv4 packets transmitted */
    uint32_t ip4_tx;
    /** Number of IPv4 packets received */
    uint32_t ip4_rx;
    /** Number of IPv4 packets dropped */
    uint32_t ip4_drop;
    /** Aggregate number of IPv4 errors */
    uint32_t ip4_err;

    /** Number of IPv6 packets transmitted */
    uint32_t ip6_tx;
    /** Number of IPv6 packets received */
    uint32_t ip6_rx;
    /** Number of IPv6 packets dropped */
    uint32_t ip6_drop;
    /** Aggregate number of IPv6 errors */
    uint32_t ip6_err;

    /** Number of ICMPv4 packets transmitted */
    uint32_t icmp4_tx;
    /** Number of ICMPv4 packets received */
    uint32_t icmp4_rx;
    /** Number of ICMPv4 packets dropped */
    uint32_t icmp4_drop;
    /** Aggregate number of ICMPv4 errors */
    uint32_t icmp4_err;

    /** Number of ICMPv6 packets transmitted */
    uint32_t icmp6_tx;
    /** Number of ICMPv6 packets received */
    uint32_t icmp6_rx;
    /** Number of ICMPv6 packets dropped */
    uint32_t icmp6_drop;
    /** Aggregate number of ICMPv6 errors */
    uint32_t icmp6_err;

    /** Number of UDP packets transmitted */
    uint32_t udp_tx;
    /** Number of UDP packets received */
    uint32_t udp_rx;
    /** Number of UDP packets dropped */
    uint32_t udp_drop;
    /** Aggregate number of UDP errors */
    uint32_t udp_err;

    /** Number of TCP packets transmitted */
    uint32_t tcp_tx;
    /** Number of TCP packets received */
    uint32_t tcp_rx;
    /** Number of TCP packets dropped */
    uint32_t tcp_drop;
    /** Aggregate number of TCP errors */
    uint32_t tcp_err;

    /** Number of ND6 packets transmitted */
    uint32_t nd6_tx;
    /** Number of ND6 packets received */
    uint32_t nd6_rx;
    /** Number of ND6 packets dropped */
    uint32_t nd6_drop;
    /** Aggregate number of ND6 errors */
    uint32_t nd6_err;
} zts_stats_counter_t;

/**
 * @brief Get all statistical counters for all protocols and levels.
 * See also: lwip/stats.h.
 *
 * This function can only be used in debug builds.
 *
 * @param dst Pointer to structure that will be populated with statistics
 *
 * @return ZTS_ERR_OK on success. ZTS_ERR_ARG or ZTS_ERR_NO_RESULT on failure.
 */
ZTS_API int ZTCALL zts_stats_get_all(zts_stats_counter_t* dst);

//----------------------------------------------------------------------------//
// Socket API                                                                 //
//----------------------------------------------------------------------------//

/**
 * @brief Create a socket
 *
 * @param family `ZTS_AF_INET` or `ZTS_AF_INET6`
 * @param type `ZTS_SOCK_STREAM` or `ZTS_SOCK_DGRAM`
 * @param protocol Protocols supported on this socket
 * @return Numbered file descriptor on success, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_bsd_socket(int family, int type, int protocol);

/**
 * @brief Connect a socket to a remote host
 *
 * @param fd Socket file descriptor
 * @param addr Remote host address to connect to
 * @param addrlen Length of address
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_bsd_connect(int fd, const struct zts_sockaddr* addr, zts_socklen_t addrlen);

/**
 * @brief Bind a socket to a local address
 *
 * @param fd Socket file descriptor
 * @param addr Local interface address to bind to
 * @param addrlen Length of address
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_bsd_bind(int fd, const struct zts_sockaddr* addr, zts_socklen_t addrlen);

/**
 * @brief Listen for incoming connections on socket
 *
 * @param fd Socket file descriptor
 * @param backlog Number of backlogged connections allowed
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_bsd_listen(int fd, int backlog);

/**
 * @brief Accept an incoming connection
 *
 * @param fd Socket file descriptor
 * @param addr Address of remote host for accepted connection
 * @param addrlen Length of address
 * @return New file descriptor if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_bsd_accept(int fd, struct zts_sockaddr* addr, zts_socklen_t* addrlen);

// Socket level option number
#define ZTS_SOL_SOCKET 0x0fff
// Socket options
#define ZTS_SO_DEBUG       0x0001   // NOT YET SUPPORTED
#define ZTS_SO_ACCEPTCONN  0x0002
#define ZTS_SO_REUSEADDR   0x0004
#define ZTS_SO_KEEPALIVE   0x0008
#define ZTS_SO_DONTROUTE   0x0010   // NOT YET SUPPORTED
#define ZTS_SO_BROADCAST   0x0020
#define ZTS_SO_USELOOPBACK 0x0040   // NOT YET SUPPORTED
#define ZTS_SO_LINGER      0x0080

/*
 * Structure used for manipulating linger option.
 */
struct zts_linger {
    int l_onoff;    // option on/off
    int l_linger;   // linger time in seconds
};

#define ZTS_SO_DONTLINGER   ((int)(~ZTS_SO_LINGER))
#define ZTS_SO_OOBINLINE    0x0100   // NOT YET SUPPORTED
#define ZTS_SO_REUSEPORT    0x0200   // NOT YET SUPPORTED
#define ZTS_SO_SNDBUF       0x1001   // NOT YET SUPPORTED
#define ZTS_SO_RCVBUF       0x1002
#define ZTS_SO_SNDLOWAT     0x1003   // NOT YET SUPPORTED
#define ZTS_SO_RCVLOWAT     0x1004   // NOT YET SUPPORTED
#define ZTS_SO_SNDTIMEO     0x1005
#define ZTS_SO_RCVTIMEO     0x1006
#define ZTS_SO_ERROR        0x1007
#define ZTS_SO_TYPE         0x1008
#define ZTS_SO_CONTIMEO     0x1009
#define ZTS_SO_NO_CHECK     0x100a
#define ZTS_SO_BINDTODEVICE 0x100b
// IPPROTO_IP options
#define ZTS_IP_TOS     0x0001
#define ZTS_IP_TTL     0x0002
#define ZTS_IP_PKTINFO 0x0008
// IPPROTO_TCP options
#define ZTS_TCP_NODELAY   0x0001
#define ZTS_TCP_KEEPALIVE 0x0002
#define ZTS_TCP_KEEPIDLE  0x0003
#define ZTS_TCP_KEEPINTVL 0x0004
#define ZTS_TCP_KEEPCNT   0x0005
// IPPROTO_IPV6 options
#define ZTS_IPV6_CHECKSUM                                                                                              \
    0x0007 /* RFC3542: calculate and insert the ICMPv6 checksum for raw                                                \
              sockets. */
#define ZTS_IPV6_V6ONLY                                                                                                \
    0x001b /* RFC3493: boolean control to restrict ZTS_AF_INET6 sockets to                                             \
              IPv6 communications only. */
// UDPLITE options
#define ZTS_UDPLITE_SEND_CSCOV 0x01 /* sender checksum coverage */
#define ZTS_UDPLITE_RECV_CSCOV 0x02 /* minimal receiver checksum coverage */
// UDPLITE options
#define ZTS_IP_MULTICAST_TTL  5
#define ZTS_IP_MULTICAST_IF   6
#define ZTS_IP_MULTICAST_LOOP 7

// Multicast options
#define ZTS_IP_ADD_MEMBERSHIP  3
#define ZTS_IP_DROP_MEMBERSHIP 4

typedef struct zts_ip_mreq {
    struct zts_in_addr imr_multiaddr; /* IP multicast address of group */
    struct zts_in_addr imr_interface; /* local IP address of interface */
} zts_ip_mreq;

struct zts_in_pktinfo {
    unsigned int ipi_ifindex;    /* Interface index */
    struct zts_in_addr ipi_addr; /* Destination (from header) address */
};

#define ZTS_IPV6_JOIN_GROUP      12
#define ZTS_IPV6_ADD_MEMBERSHIP  ZTS_IPV6_JOIN_GROUP
#define ZTS_IPV6_LEAVE_GROUP     13
#define ZTS_IPV6_DROP_MEMBERSHIP ZTS_IPV6_LEAVE_GROUP

typedef struct zts_ipv6_mreq {
    struct zts_in6_addr ipv6mr_multiaddr; /*  IPv6 multicast addr */
    unsigned int ipv6mr_interface;        /*  interface index, or 0 */
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
#define ZTS_IPTOS_TOS_MASK    0x1E
#define ZTS_IPTOS_TOS(tos)    ((tos)&ZTS_IPTOS_TOS_MASK)
#define ZTS_IPTOS_LOWDELAY    0x10
#define ZTS_IPTOS_THROUGHPUT  0x08
#define ZTS_IPTOS_RELIABILITY 0x04
#define ZTS_IPTOS_LOWCOST     0x02
#define ZTS_IPTOS_MINCOST     ZTS_IPTOS_LOWCOST

/*
 * The Network Control precedence designation is intended to be used
 * within a network only.  The actual use and control of that
 * designation is up to each network. The Internetwork Control
 * designation is intended for use by gateway control originators only.
 * If the actual use of these precedence designations is of concern to
 * a particular network, it is the responsibility of that network to
 * control the access to, and use of, those precedence designations.
 */
#define ZTS_IPTOS_PREC_MASK            0xe0
#define ZTS_IPTOS_PREC(tos)            ((tos)&ZTS_IPTOS_PREC_MASK)
#define ZTS_IPTOS_PREC_NETCONTROL      0xe0
#define ZTS_IPTOS_PREC_INTERNETCONTROL 0xc0
#define ZTS_IPTOS_PREC_CRITIC_ECP      0xa0
#define ZTS_IPTOS_PREC_FLASHOVERRIDE   0x80
#define ZTS_IPTOS_PREC_FLASH           0x60
#define ZTS_IPTOS_PREC_IMMEDIATE       0x40
#define ZTS_IPTOS_PREC_PRIORITY        0x20
#define ZTS_IPTOS_PREC_ROUTINE         0x00

/**
 * @brief Set socket options.
 *
 * @param fd Socket file descriptor
 * @param level Protocol level to which option name should apply
 * @param optname Option name to set
 * @param optval Source of option value to set
 * @param optlen Length of option value
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_bsd_setsockopt(int fd, int level, int optname, const void* optval, zts_socklen_t optlen);

/**
 * @brief Get socket options.
 *
 * @param fd Socket file descriptor
 * @param level Protocol level to which option name should apply
 * @param optname Option name to get
 * @param optval Where option value will be stored
 * @param optlen Length of value
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_bsd_getsockopt(int fd, int level, int optname, void* optval, zts_socklen_t* optlen);

/**
 * @brief Get the name (address) of the local end of the socket
 *
 * @param fd Socket file descriptor
 * @param addr Name associated with this socket
 * @param addrlen Length of name
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_bsd_getsockname(int fd, struct zts_sockaddr* addr, zts_socklen_t* addrlen);

/**
 * @brief Get the name (address) of the remote end of the socket
 *
 * @param fd Socket file descriptor
 * @param addr Name associated with remote end of this socket
 * @param addrlen Length of name
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_bsd_getpeername(int fd, struct zts_sockaddr* addr, zts_socklen_t* addrlen);

/**
 * @brief Close socket.
 *
 * @param fd Socket file descriptor
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_bsd_close(int fd);

/* FD_SET used for lwip_select */

#define LWIP_SOCKET_OFFSET 0
#define MEMP_NUM_NETCONN   1024

#ifndef ZTS_FD_SET
#undef ZTS_FD_SETSIZE
// Make FD_SETSIZE match NUM_SOCKETS in socket.c
#define ZTS_FD_SETSIZE MEMP_NUM_NETCONN
#define ZTS_FDSETSAFESET(n, code)                                                                                      \
    do {                                                                                                               \
        if (((n)-LWIP_SOCKET_OFFSET < MEMP_NUM_NETCONN) && (((int)(n)-LWIP_SOCKET_OFFSET) >= 0)) {                     \
            code;                                                                                                      \
        }                                                                                                              \
    } while (0)
#define ZTS_FDSETSAFEGET(n, code)                                                                                      \
    (((n)-LWIP_SOCKET_OFFSET < MEMP_NUM_NETCONN) && (((int)(n)-LWIP_SOCKET_OFFSET) >= 0) ? (code) : 0)
#define ZTS_FD_SET(n, p)                                                                                               \
    ZTS_FDSETSAFESET(n, (p)->fd_bits[((n)-LWIP_SOCKET_OFFSET) / 8] |= (1 << (((n)-LWIP_SOCKET_OFFSET) & 7)))
#define ZTS_FD_CLR(n, p)                                                                                               \
    ZTS_FDSETSAFESET(n, (p)->fd_bits[((n)-LWIP_SOCKET_OFFSET) / 8] &= ~(1 << (((n)-LWIP_SOCKET_OFFSET) & 7)))
#define ZTS_FD_ISSET(n, p)                                                                                             \
    ZTS_FDSETSAFEGET(n, (p)->fd_bits[((n)-LWIP_SOCKET_OFFSET) / 8] & (1 << (((n)-LWIP_SOCKET_OFFSET) & 7)))
#define ZTS_FD_ZERO(p) memset((void*)(p), 0, sizeof(*(p)))

#elif LWIP_SOCKET_OFFSET
#error LWIP_SOCKET_OFFSET does not work with external FD_SET!
#elif ZTS_FD_SETSIZE < MEMP_NUM_NETCONN
#error "external ZTS_FD_SETSIZE too small for number of sockets"
#endif   // FD_SET

typedef struct zts_fd_set {
    unsigned char fd_bits[(ZTS_FD_SETSIZE + 7) / 8];
} zts_fd_set;

typedef struct zts_timeval {
    long tv_sec;  /* seconds */
    long tv_usec; /* and microseconds */
} zts_timeval;

/**
 * @brief Monitor multiple file descriptors for "readiness"
 *
 * @param nfds Set to the highest numbered file descriptor in any of the given
 * sets
 * @param readfds Set of file descriptors to monitor for READ readiness
 * @param writefds Set of file descriptors to monitor for WRITE readiness
 * @param exceptfds Set of file descriptors to monitor for exceptional
 * conditions
 * @param timeout How long this call should block
 * @return Number of ready file descriptors on success. `ZTS_ERR_SOCKET`,
 * `ZTS_ERR_SERVICE` on failure. Sets `zts_errno`
 */
ZTS_API int ZTCALL
zts_bsd_select(int nfds, zts_fd_set* readfds, zts_fd_set* writefds, zts_fd_set* exceptfds, struct zts_timeval* timeout);

// fnctl() commands
#define ZTS_F_GETFL 0x0003
#define ZTS_F_SETFL 0x0004
/* File status flags and file access modes for fnctl,
   these are bits in an int. */
#define ZTS_O_NONBLOCK 1
#define ZTS_O_NDELAY   ZTS_O_NONBLOCK
#define ZTS_O_RDONLY   2
#define ZTS_O_WRONLY   4
#define ZTS_O_RDWR     (ZTS_O_RDONLY | ZTS_O_WRONLY)

/**
 * @brief Issue file control commands on a socket
 *
 * @param fd Socket file descriptor
 * @param cmd Operation to be performed
 * @param flags Flags
 * @return
 */
ZTS_API int ZTCALL zts_bsd_fcntl(int fd, int cmd, int flags);

#define ZTS_POLLIN   0x001
#define ZTS_POLLOUT  0x002
#define ZTS_POLLERR  0x004
#define ZTS_POLLNVAL 0x008
/* Below values are unimplemented */
#define ZTS_POLLRDNORM 0x010
#define ZTS_POLLRDBAND 0x020
#define ZTS_POLLPRI    0x040
#define ZTS_POLLWRNORM 0x080
#define ZTS_POLLWRBAND 0x100
#define ZTS_POLLHUP    0x200

typedef unsigned int zts_nfds_t;

struct zts_pollfd {
    int fd;
    short events;
    short revents;
};

/**
 * @brief Wait for some event on a file descriptor.
 *
 * @param fds Set of file descriptors to monitor
 * @param nfds Number of elements in the fds array
 * @param timeout How long this call should block
 * @return Number of ready file descriptors if successful, `ZTS_ERR_SERVICE` if
 * the node experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets
 * `zts_errno`
 */
ZTS_API int ZTCALL zts_bsd_poll(struct zts_pollfd* fds, zts_nfds_t nfds, int timeout);

/**
 * @brief Control a device
 *
 * @param fd Socket file descriptor
 * @param request Selects the control function to be performed
 * @param argp Additional information
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_bsd_ioctl(int fd, unsigned long request, void* argp);

/**
 * @brief Send data to remote host
 *
 * @param fd Socket file descriptor
 * @param buf Pointer to data buffer
 * @param len Length of data to write
 * @param flags (e.g. `ZTS_MSG_DONTWAIT`, `ZTS_MSG_MORE`)
 * @return Number of bytes sent if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API ssize_t ZTCALL zts_bsd_send(int fd, const void* buf, size_t len, int flags);

/**
 * @brief Send data to remote host
 *
 * @param fd Socket file descriptor
 * @param buf Pointer to data buffer
 * @param len Length of data to write
 * @param flags Specifies type of message transmission
 * @param addr Destination address
 * @param addrlen Length of destination address
 * @return Number of bytes sent if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API ssize_t ZTCALL
zts_bsd_sendto(int fd, const void* buf, size_t len, int flags, const struct zts_sockaddr* addr, zts_socklen_t addrlen);

struct zts_iovec {
    void* iov_base;
    size_t iov_len;
};

/* */
struct zts_msghdr {
    void* msg_name;
    zts_socklen_t msg_namelen;
    struct zts_iovec* msg_iov;
    int msg_iovlen;
    void* msg_control;
    zts_socklen_t msg_controllen;
    int msg_flags;
};

/* struct msghdr->msg_flags bit field values */
#define ZTS_MSG_TRUNC  0x04
#define ZTS_MSG_CTRUNC 0x08

/**
 * @brief Send message to remote host
 *
 * @param fd Socket file descriptor
 * @param msg Message to send
 * @param flags Specifies type of message transmission
 * @return Number of bytes sent if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API ssize_t ZTCALL zts_bsd_sendmsg(int fd, const struct zts_msghdr* msg, int flags);

/**
 * @brief Receive data from remote host
 *
 * @param fd Socket file descriptor
 * @param buf Pointer to data buffer
 * @param len Length of data buffer
 * @param flags Specifies the type of message receipt
 * @return Number of bytes received if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API ssize_t ZTCALL zts_bsd_recv(int fd, void* buf, size_t len, int flags);

/**
 * @brief Receive data from remote host
 *
 * @param fd Socket file descriptor
 * @param buf Pointer to data buffer
 * @param len Length of data buffer
 * @param flags Specifies the type of message receipt
 * @param addr Destination address buffer
 * @param addrlen Length of destination address buffer
 * @return Number of bytes received if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API ssize_t ZTCALL
zts_bsd_recvfrom(int fd, void* buf, size_t len, int flags, struct zts_sockaddr* addr, zts_socklen_t* addrlen);

/**
 * @brief Receive a message from remote host
 *
 * @param fd Socket file descriptor
 * @param msg Message that was received
 * @param flags Specifies the type of message receipt
 * @return Number of bytes received if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API ssize_t ZTCALL zts_bsd_recvmsg(int fd, struct zts_msghdr* msg, int flags);

/**
 * @brief Read data from socket onto buffer
 *
 * @param fd Socket file descriptor
 * @param buf Pointer to data buffer
 * @param len Length of data buffer to receive data
 * @return Number of bytes read if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API ssize_t ZTCALL zts_bsd_read(int fd, void* buf, size_t len);

/**
 * @brief Read data from socket into multiple buffers
 *
 * @param fd Socket file descriptor
 * @param iov Array of destination buffers
 * @param iovcnt Number of buffers to read into
 * @return Number of bytes read if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API ssize_t ZTCALL zts_bsd_readv(int fd, const struct zts_iovec* iov, int iovcnt);

/**
 * @brief Write data from buffer to socket
 *
 * @param fd Socket file descriptor
 * @param buf Pointer to data buffer
 * @param len Length of buffer to write
 * @return Number of bytes written if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API ssize_t ZTCALL zts_bsd_write(int fd, const void* buf, size_t len);

/**
 * @brief Write data from multiple buffers to socket.
 *
 * @param fd Socket file descriptor
 * @param iov Array of source buffers
 * @param iovcnt Number of buffers to read from
 * @return Number of bytes written if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API ssize_t ZTCALL zts_bsd_writev(int fd, const struct zts_iovec* iov, int iovcnt);

#define ZTS_SHUT_RD   0x0
#define ZTS_SHUT_WR   0x1
#define ZTS_SHUT_RDWR 0x2

/**
 * @brief Shut down some aspect of a socket
 *
 * @param fd Socket file descriptor
 * @param how Which aspects of the socket should be shut down. Options are `ZTS_SHUT_RD`,
 * `ZTS_SHUT_WR`, or `ZTS_SHUT_RDWR`.
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_bsd_shutdown(int fd, int how);

//----------------------------------------------------------------------------//
// Simplified socket API                                                      //
//----------------------------------------------------------------------------//

/**
 * A subset (and) extension of the traditional BSD-style socket API that simplifies
 * API wrapper generation and usage in other non-C-like languages. Uses simple
 * integer types instead of bit flags, limit the number of operations each function
 * performs, prevent the user from needing to manipulate the contents of structures
 * in a non-native language.
 */

/**
 * @brief Create a socket
 *
 * @param family `ZTS_AF_INET` or `ZTS_AF_INET6`
 * @param type `ZTS_SOCK_STREAM` or `ZTS_SOCK_DGRAM`
 * @param protocol Protocols supported on this socket
 * @return Numbered file descriptor on success, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_socket(int family, int type, int protocol);

/**
 * @brief Connect a socket to a remote host
 *
 * This convenience function exists because ZeroTier uses transport-triggered
 * links. This means that links between peers do not exist until peers try to
 * talk to each other. This can be a problem during connection procedures since
 * some of the initial packets are lost. To alleviate the need to try
 * `zts_bsd_connect` many times, this function will keep re-trying for you, even if
 * no known routes exist. However, if the socket is set to `non-blocking` mode
 * it will behave identically to `zts_bsd_connect` and return immediately upon
 * failure.
 *
 * @param fd Socket file descriptor
 * @param ipstr Human-readable IP string
 * @param port Port
 * @param timeout_ms (Approximate) amount of time in milliseconds before
 *     connection attempt is aborted. Will block for `30 seconds` if timeout is
 *     set to `0`.
 *
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SOCKET` if the function times
 *     out with no connection made, `ZTS_ERR_SERVICE` if the node experiences a
 *     problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_connect(int fd, const char* ipstr, unsigned short port, int timeout_ms);

/**
 * @brief Bind a socket to a local address
 *
 * @param fd Socket file descriptor
 * @param ipstr Human-readable IP string
 * @param port Port
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_bind(int fd, const char* ipstr, unsigned short port);

/**
 * @brief Listen for incoming connections on socket
 *
 * @param fd Socket file descriptor
 * @param backlog Number of backlogged connections allowed
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_listen(int fd, int backlog);

/**
 * @brief Accept an incoming connection
 *
 * @param fd Socket file descriptor
 * @param remote_addr Buffer that will receive remote host IP string
 * @param len Size of buffer that will receive remote host IP string
 *     (must be exactly `ZTS_IP_MAX_STR_LEN`)
 * @param port Port number of the newly connected remote host (value-result)
 * @return New file descriptor if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_accept(int fd, char* remote_addr, int len, unsigned short* port);

/**
 * @brief Send data to remote host
 *
 * @param fd Socket file descriptor
 * @param buf Pointer to data buffer
 * @param len Length of data to write
 * @param flags (e.g. `ZTS_MSG_DONTWAIT`, `ZTS_MSG_MORE`)
 * @return Number of bytes sent if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API ssize_t ZTCALL zts_send(int fd, const void* buf, size_t len, int flags);

/**
 * @brief Receive data from remote host
 *
 * @param fd Socket file descriptor
 * @param buf Pointer to data buffer
 * @param len Length of data buffer
 * @param flags Specifies the type of message receipt
 * @return Number of bytes received if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API ssize_t ZTCALL zts_recv(int fd, void* buf, size_t len, int flags);

/**
 * @brief Read data from socket onto buffer
 *
 * @param fd Socket file descriptor
 * @param buf Pointer to data buffer
 * @param len Length of data buffer to receive data
 * @return Number of bytes read if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API ssize_t ZTCALL zts_read(int fd, void* buf, size_t len);

/**
 * @brief Write data from buffer to socket
 *
 * @param fd Socket file descriptor
 * @param buf Pointer to data buffer
 * @param len Length of buffer to write
 * @return Number of bytes written if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API ssize_t ZTCALL zts_write(int fd, const void* buf, size_t len);

/**
 * @brief Shut down `read` aspect of a socket
 *
 * @param fd Socket file descriptor
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_shutdown_rd(int fd);

/**
 * @brief Shut down `write` aspect of a socket
 *
 * @param fd Socket file descriptor
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_shutdown_wr(int fd);

/**
 * @brief Shut down both `read` and `write` aspect of a socket
 *
 * @param fd Socket file descriptor
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_shutdown_rdwr(int fd);

/**
 * @brief Close socket.
 *
 * @param fd Socket file descriptor
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_close(int fd);

/**
 * @brief Get the name (address) of the remote end of the socket
 *
 * @param fd Socket file descriptor
 * @param remote_addr_str Destination buffer to contain name (address) of the remote end of the socket
 * @param len Length of destination buffer
 * @param port Value-result parameter that will contain resultant port number
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_getpeername(int fd, char* remote_addr_str, int len, unsigned short* port);

/**
 * @brief Get the name (address) of the local end of the socket
 *
 * @param fd Socket file descriptor
 * @param local_addr_str Destination buffer to contain name (address) of the local end of the socket
 * @param len Length of destination buffer
 * @param port Value-result parameter that will contain resultant port number
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_getsockname(int fd, char* local_addr_str, int len, unsigned short* port);

/**
 * @brief A convenience function that takes a remote address IP string and creates
 * the appropriate type of socket, and uses it to connect to a remote host.
 *
 * @param remote_ipstr Remote address string. IPv4 or IPv6
 * @param remote_port Port to
 *
 * @return New file descriptor if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_tcp_client(const char* remote_ipstr, unsigned short remote_port);

/**
 * @brief A convenience function that takes a remote address IP string and creates
 * the appropriate type of socket, binds, listens, and then accepts on it.
 *
 * @param local_ipstr Local address to bind
 * @param local_port Local port to bind
 * @param remote_ipstr String-format IP address of newly connected remote host
 * @param len Length of `remote_ipstr`
 * @param remote_port Port of remote host
 *
 * @return New file descriptor if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_tcp_server(
    const char* local_ipstr,
    unsigned short local_port,
    char* remote_ipstr,
    int len,
    unsigned short* remote_port);

/**
 * @brief A convenience function that takes a remote address IP string and creates
 * the appropriate type of socket, and binds to it.
 *
 * @param local_ipstr Local address to bind
 * @param local_port Local port to bind
 *
 * @return New file descriptor if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_udp_server(const char* local_ipstr, unsigned short local_port);

/**
 * @brief This function doesn't really do anything other than be a namespace
 * counterpart to `zts_udp_server`. All this function does is create a
 * `ZTS_SOCK_DGRAM` socket and return its file descriptor.
 *
 * @param remote_ipstr Remote address string. IPv4 or IPv6
 *
 * @return New file descriptor if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_udp_client(const char* remote_ipstr);

/**
 * @brief Enable or disable `TCP_NODELAY`. Enabling this is equivalent to
 *     turning off Nagle's algorithm
 *
 * @param fd Socket file descriptor
 * @param enabled `[0, 1]` integer value
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_set_no_delay(int fd, int enabled);

/**
 * @brief Get the last error for the given socket
 *
 * @param fd Socket file descriptor
 * @return Error number defined in `zts_errno_t`. `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_get_last_socket_error(int fd);

/**
 * @brief Return amount of data available to read from socket
 *
 * @param fd Socket file descriptor
 * @return Number of bytes available to read. `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API size_t ZTCALL zts_get_data_available(int fd);

/**
 * @brief Return whether `TCP_NODELAY` is enabled
 *
 * @param fd Socket file descriptor
 * @return `1` if enabled, `0` if disabled, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_get_no_delay(int fd);

/**
 * @brief Enable or disable `SO_LINGER` while also setting its value
 *
 * @param fd Socket file descriptor
 * @param enabled `[0, 1]` integer value
 * @param value How long socket should linger
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_set_linger(int fd, int enabled, int value);

/**
 * @brief Return whether `SO_LINGER` is enabled
 *
 * @param fd Socket file descriptor
 * @return `1` if enabled, `0` if disabled, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_get_linger_enabled(int fd);

/**
 * @brief Return the value of `SO_LINGER`
 *
 * @param fd Socket file descriptor
 * @return Value of `SO_LINGER` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_get_linger_value(int fd);

/**
 * @brief Return the number of bytes available to read from the network buffer
 *
 * @param fd Socket file descriptor
 * @return Number of bytes to read if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_get_pending_data_size(int fd);

/**
 * @brief Enable or disable `SO_REUSEADDR`
 *
 * @param fd Socket file descriptor
 * @param enabled `[0, 1]` integer value
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_set_reuse_addr(int fd, int enabled);

/**
 * @brief Return whether `SO_REUSEADDR` is enabled
 *
 * @param fd Socket file descriptor
 * @return `1` if enabled, `0` if disabled, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_get_reuse_addr(int fd);

/**
 * @brief Set the value of `SO_RCVTIMEO`
 *
 * @param fd Socket file descriptor
 * @param seconds Number of seconds for timeout
 * @param microseconds Number of microseconds for timeout
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_set_recv_timeout(int fd, int seconds, int microseconds);

/**
 * @brief Return the value of `SO_RCVTIMEO`
 *
 * @param fd Socket file descriptor
 * @return Value of `SO_RCVTIMEO` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_get_recv_timeout(int fd);

/**
 * @brief Set the value of `SO_SNDTIMEO`
 *
 * @param fd Socket file descriptor
 * @param seconds Number of seconds for timeout
 * @param microseconds Number of microseconds for timeout
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_set_send_timeout(int fd, int seconds, int microseconds);

/**
 * @brief Return the value of `SO_SNDTIMEO`
 *
 * @param fd Socket file descriptor
 * @return Value of `SO_SNDTIMEO` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_get_send_timeout(int fd);

/**
 * @brief Set the value of `SO_SNDBUF`
 *
 * @param fd Socket file descriptor
 * @param size Size of buffer
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_set_send_buf_size(int fd, int size);

/**
 * @brief Return the value of `SO_SNDBUF`
 *
 * @param fd Socket file descriptor
 * @return Value of `SO_SNDBUF` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_get_send_buf_size(int fd);

/**
 * @brief Set the value of `SO_RCVBUF`
 *
 * @param fd Socket file descriptor
 * @param size Size of buffer
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_set_recv_buf_size(int fd, int size);

/**
 * @brief Return the value of `SO_RCVBUF`
 *
 * @param fd Socket file descriptor
 * @return Value of `SO_RCVBUF` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_get_recv_buf_size(int fd);

/**
 * @brief Set the value of `IP_TTL`
 *
 * @param fd Socket file descriptor
 * @param ttl Value of `IP_TTL`
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_set_ttl(int fd, int ttl);

/**
 * @brief Return the value of `IP_TTL`
 *
 * @param fd Socket file descriptor
 * @return Value of `IP_TTL` `[0,255]` if successful, `ZTS_ERR_SERVICE` if the
 * node experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_get_ttl(int fd);

/**
 * @brief Change blocking behavior `O_NONBLOCK`
 *
 * @param fd Socket file descriptor
 * @param enabled `[0, 1]` integer value, `1` maintains default behavior,
 *     `0` sets to non-blocking mode
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_set_blocking(int fd, int enabled);

/**
 * @brief Return whether blocking mode `O_NONBLOCK` is enabled
 *
 * @param fd Socket file descriptor
 * @return `1` if enabled, `0` if disabled, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_get_blocking(int fd);

/**
 * @brief Enable or disable `SO_KEEPALIVE`
 *
 * @param fd Socket file descriptor
 * @param enabled `[0, 1]` integer value
 * @return `ZTS_ERR_OK` if successful, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_set_keepalive(int fd, int enabled);

/**
 * @brief Return whether `SO_KEEPALIVE` is enabled
 *
 * @param fd Socket file descriptor
 * @return `1` if enabled, `0` if disabled, `ZTS_ERR_SERVICE` if the node
 *     experiences a problem, `ZTS_ERR_ARG` if invalid argument. Sets `zts_errno`
 */
ZTS_API int ZTCALL zts_get_keepalive(int fd);

//----------------------------------------------------------------------------//
// DNS                                                                        //
//----------------------------------------------------------------------------//

struct zts_hostent {
    char* h_name;             /* Official name of the host. */
    char** h_aliases;         /* A pointer to an array of pointers to alternative host
                                 names,   terminated by a null pointer. */
    int h_addrtype;           /* Address type. */
    int h_length;             /* The length, in bytes, of the address. */
    char** h_addr_list;       /* A pointer to an array of pointers to network
                                 addresses (in network byte order) for the host,
                                 terminated by a null pointer. */
#define h_addr h_addr_list[0] /* for backward compatibility */
};

/**
 * @brief Resolve a host-name
 *
 * @param name A null-terminated string representing the name of the host
 * @return Pointer to struct zts_hostent if successful, NULL otherwise
 */
struct zts_hostent* zts_bsd_gethostbyname(const char* name);

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
    uint8_t type;   // ZTS_IPADDR_TYPE_V4, ZTS_IPADDR_TYPE_V6
} zts_ip_addr;

/**
 * Initialize one of the DNS servers.
 *
 * @param index the index of the DNS server to set must be `< DNS_MAX_SERVERS`
 * @param addr IP address of the DNS server to set
 */
ZTS_API int ZTCALL zts_dns_set_server(uint8_t index, const zts_ip_addr* addr);

/**
 * Obtain one of the currently configured DNS server.
 *
 * @param index the index of the DNS server
 * @return IP address of the indexed DNS server or `ip_addr_any` if the DNS
 *         server has not been configured.
 */
ZTS_API const zts_ip_addr* ZTCALL zts_dns_get_server(uint8_t index);

//----------------------------------------------------------------------------//
// Core query sub-API (Used for simplifying high-level language wrappers)     //
//----------------------------------------------------------------------------//

/**
 * @brief Lock the core service so that queries about addresses, routes, paths, etc. can be
 * performed.
 *
 * Notice: Core locking functions are intended to be used by high-level language wrappers.
 * Only lock the core if you know *exactly* what you are doing.
 *
 * @return `ZTS_ERR_OK` if successful. `ZTS_ERR_SERVICE` if the core service is unavailable.
 */
ZTS_API int ZTCALL zts_core_lock_obtain();

/**
 * @brief Lock the core service so that queries about addresses, routes, paths, etc. can be
 * performed.
 *
 * Notice: Core locking functions are intended to be used by high-level language wrappers.
 * Only lock the core if you know *exactly* what you are doing.
 *
 * @return `ZTS_ERR_OK` if successful. `ZTS_ERR_SERVICE` if the core service is unavailable.
 */
ZTS_API int ZTCALL zts_core_lock_release();

/**
 * @brief Lock the core service so that queries about addresses, routes, paths, etc. can be
 * performed.
 *
 * Notice: Core locking functions are intended to be used by high-level language wrappers.
 * Only lock the core if you know *exactly* what you are doing. zts_core_lock_obtain() and
 * zts_core_lock_release() must be called before and after this function.
 *
 * @return `ZTS_ERR_OK` if successful. `ZTS_ERR_SERVICE` if the core service is unavailable.
 */
ZTS_API int ZTCALL zts_core_query_addr_count(uint64_t net_id);

/**
 * @brief Lock the core service so that queries about addresses, routes, paths, etc. can be
 * performed.
 *
 * Notice: Core locking functions are intended to be used by high-level language wrappers.
 * Only lock the core if you know *exactly* what you are doing. zts_core_lock_obtain() and
 * zts_core_lock_release() must be called before and after this function.
 *
 * @return `ZTS_ERR_OK` if successful. `ZTS_ERR_SERVICE` if the core service is unavailable.
 */
ZTS_API int ZTCALL zts_core_query_addr(uint64_t net_id, unsigned int idx, char* addr, unsigned int len);

/**
 * @brief Lock the core service so that queries about addresses, routes, paths, etc. can be
 * performed.
 *
 * Notice: Core locking functions are intended to be used by high-level language wrappers.
 * Only lock the core if you know *exactly* what you are doing. zts_core_lock_obtain() and
 * zts_core_lock_release() must be called before and after this function.
 *
 * @return `ZTS_ERR_OK` if successful. `ZTS_ERR_SERVICE` if the core service is unavailable.
 */
ZTS_API int ZTCALL zts_core_query_route_count(uint64_t net_id);

/**
 * @brief Lock the core service so that queries about addresses, routes, paths, etc. can be
 * performed.
 *
 * Notice: Core locking functions are intended to be used by high-level language wrappers.
 * Only lock the core if you know *exactly* what you are doing. zts_core_lock_obtain() and
 * zts_core_lock_release() must be called before and after this function.
 *
 * @return `ZTS_ERR_OK` if successful. `ZTS_ERR_SERVICE` if the core service is unavailable.
 */
ZTS_API int ZTCALL zts_core_query_route(
    uint64_t net_id,
    unsigned int idx,
    char* target,
    char* via,
    unsigned int len,
    uint16_t* flags,
    uint16_t* metric);

/**
 * @brief Lock the core service so that queries about addresses, routes, paths, etc. can be
 * performed.
 *
 * Notice: Core locking functions are intended to be used by high-level language wrappers.
 * Only lock the core if you know *exactly* what you are doing. zts_core_lock_obtain() and
 * zts_core_lock_release() must be called before and after this function.
 *
 * @return `ZTS_ERR_OK` if successful. `ZTS_ERR_SERVICE` if the core service is unavailable.
 */
ZTS_API int ZTCALL zts_core_query_path_count(uint64_t peer_id);

/**
 * @brief Lock the core service so that queries about addresses, routes, paths, etc. can be
 * performed.
 *
 * Notice: Core locking functions are intended to be used by high-level language wrappers.
 * Only lock the core if you know *exactly* what you are doing. zts_core_lock_obtain() and
 * zts_core_lock_release() must be called before and after this function.
 *
 * @return `ZTS_ERR_OK` if successful. `ZTS_ERR_SERVICE` if the core service is unavailable.
 */
ZTS_API int ZTCALL zts_core_query_path(uint64_t peer_id, unsigned int idx, char* dst, unsigned int len);

/**
 * @brief Lock the core service so that queries about addresses, routes, paths, etc. can be
 * performed.
 *
 * Notice: Core locking functions are intended to be used by high-level language wrappers.
 * Only lock the core if you know *exactly* what you are doing. zts_core_lock_obtain() and
 * zts_core_lock_release() must be called before and after this function.
 *
 * @return `ZTS_ERR_OK` if successful. `ZTS_ERR_SERVICE` if the core service is unavailable.
 */
ZTS_API int ZTCALL zts_core_query_mc_count(uint64_t net_id);

/**
 * @brief Lock the core service so that queries about addresses, routes, paths, etc. can be
 * performed.
 *
 * Notice: Core locking functions are intended to be used by high-level language wrappers.
 * Only lock the core if you know *exactly* what you are doing. zts_core_lock_obtain() and
 * zts_core_lock_release() must be called before and after this function.
 *
 * @return `ZTS_ERR_OK` if successful. `ZTS_ERR_SERVICE` if the core service is unavailable.
 */
ZTS_API int ZTCALL zts_core_query_mc(uint64_t net_id, unsigned int idx, uint64_t* mac, uint32_t* adi);

//----------------------------------------------------------------------------//
// Utilities                                                                  //
//----------------------------------------------------------------------------//

/**
 * @brief Generates a new root set definition
 *
 * @param roots_id The desired World ID (arbitrary)
 * @param ts Timestamp indicating when this generation took place
 */
ZTS_API int ZTCALL zts_util_sign_root_set(
    char* roots_out,
    unsigned int* roots_len,
    char* prev_key,
    unsigned int* prev_key_len,
    char* curr_key,
    unsigned int* curr_key_len,
    uint64_t id,
    uint64_t ts,
    zts_root_set_t* roots_spec);

/**
 * @brief Platform-agnostic delay
 *
 * @param milliseconds How long to delay
 */
ZTS_API void ZTCALL zts_util_delay(unsigned long milliseconds);

/**
 * @brief Return the family type of the IP string
 *
 * @param ipstr Either IPv4 or IPv6 string
 * @return Either `ZTS_AF_INET` or `ZTS_AF_INET6`
 */
ZTS_API int ZTCALL zts_util_get_ip_family(const char* ipstr);

/**
 * Convert human-friendly IP string to `zts_sockaddr_in` or `zts_sockaddr_in6`.
 *
 * @param src_ipstr Source IP string
 * @param port Port
 * @param dstaddr Pointer to destination structure `zts_sockaddr_in` or
 * `zts_sockaddr_in6`
 * @param addrlen Size of destination structure. Value-result: Will be set to
 * actual size of data available
 * @return return `ZTS_ERR_OK` on success, `ZTS_ERR_ARG` if invalid argument
 */
int zts_util_ipstr_to_saddr(
    const char* src_ipstr,
    unsigned short port,
    struct zts_sockaddr* dstaddr,
    zts_socklen_t* addrlen);

/**
 * @brief Similar to `inet_ntop` but determines family automatically and returns
 * port as a value result parameter.
 *
 * @param addr Pointer to address structure
 * @param addrlen Length of address structure
 * @param dst_str Destination buffer
 * @param len Length of destination buffer
 * @param port Value-result parameter that will contain resultant port number
 *
 * @return return `ZTS_ERR_OK` on success, `ZTS_ERR_ARG` if invalid argument
 */
ZTS_API int ZTCALL
zts_util_ntop(struct zts_sockaddr* addr, zts_socklen_t addrlen, char* dst_str, int len, unsigned short* port);

//----------------------------------------------------------------------------//
// Convenience functions pulled from lwIP                                     //
//----------------------------------------------------------------------------//

/**
 * Convert numeric IP address (both versions) into `ASCII` representation.
 * returns ptr to static buffer. Not reentrant.
 *
 * @param addr IP address in network order to convert
 * @return Pointer to a global static (!) buffer that holds the `ASCII`
 *         representation of addr
 */
char* zts_ipaddr_ntoa(const zts_ip_addr* addr);

/**
 * Convert IP address string (both versions) to numeric.
 * The version is auto-detected from the string.
 *
 * @param cp IP address string to convert
 * @param addr conversion result is stored here
 * @return `1` on success, `0` on error
 */
int zts_ipaddr_aton(const char* cp, zts_ip_addr* addr);

/**
 * Convert IPv4 and IPv6 address structures to human-readable text form.
 *
 * @param family Address family: `ZTS_AF_INET` or `ZTS_AF_INET6`
 * @param src Pointer to source address structure
 * @param dst Pointer to destination character array
 * @param size Size of the destination buffer
 * @return On success, returns a non-null pointer to the destination character
 * array
 */
ZTS_API const char* ZTCALL zts_inet_ntop(int family, const void* src, char* dst, zts_socklen_t size);

/**
 * Convert C-string IPv4 and IPv6 addresses to binary form.
 *
 * @param family Address family: `ZTS_AF_INET` or `ZTS_AF_INET6`
 * @param src Pointer to source character array
 * @param dst Pointer to destination address structure
 * @return return `1` on success. `0` or `-1` on failure. (Does not follow regular
 * `zts_*` conventions)
 */
ZTS_API int ZTCALL zts_inet_pton(int family, const char* src, void* dst);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif   // _H
