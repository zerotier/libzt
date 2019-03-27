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

#ifndef ZEROTIER_H
#define ZEROTIER_H

#include "ZeroTierConstants.h"

#include <stdint.h>

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

#if !defined(_WIN32) && !defined(__ANDROID__)
typedef unsigned int socklen_t;
#else
typedef int socklen_t;
//#include <sys/socket.h>
#endif

#if defined(__APPLE__)
    #include "TargetConditionals.h"
    //#if TARGET_OS_IPHONE
		//#ifndef sockaddr_storage
    		#include <sys/socket.h>
		//#endif
	//#endif
    // TARGET_OS_MAC
#endif

#if defined(_WIN32)
#include <WinSock2.h>
#include <stdint.h>
#include <WS2tcpip.h>
#include <Windows.h>
#endif

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

//namespace ZeroTier {

#ifdef __cplusplus
extern "C" {
#endif

// Custom errno to prevent conflicts with platform's own errno
extern int zts_errno;

typedef uint32_t zts_in_addr_t;
typedef uint16_t zts_in_port_t;
typedef uint8_t zts_sa_family_t;

struct zts_in_addr {
  zts_in_addr_t s_addr;
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
  zts_sa_family_t     sin_family;
  zts_in_port_t       sin_port;
  struct zts_in_addr  sin_addr;
#define SIN_ZERO_LEN 8
  char            sin_zero[SIN_ZERO_LEN];
};

struct zts_sockaddr_in6 {
  uint8_t            sin6_len;      /* length of this structure    */
  zts_sa_family_t     sin6_family;   /* AF_INET6                    */
  zts_in_port_t       sin6_port;     /* Transport layer port #      */
  uint32_t           sin6_flowinfo; /* IPv6 flow information       */
  struct zts_in6_addr sin6_addr;     /* IPv6 address                */
  uint32_t           sin6_scope_id; /* Set of interfaces for scope */
};

struct zts_sockaddr {
  uint8_t        sa_len;
  zts_sa_family_t sa_family;
  char        sa_data[14];
};

struct zts_sockaddr_storage {
  uint8_t        s2_len;
  zts_sa_family_t ss_family;
  char        s2_data1[2];
  uint32_t       s2_data2[3];
  uint32_t       s2_data3[3];
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

//////////////////////////////////////////////////////////////////////////////
// Subset of: ZeroTierOne.h                                                 //
// We redefine a few ZT structures here so that we don't need to drag the   //
// entire ZeroTierOne.h file into the user application                      //
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
#define ZT_MAX_PEER_NETWORK_PATHS 16

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
	 * The current clock value accord to the node
	 */ 
	uint64_t clock;

	/**
	 * Whether or not this node is online
	 */
	bool online;

	/**
	 * Whether port mapping is enabled
	 */
	bool portMappingEnabled;

	/**
	 * Whether multipath support is enabled. If true, this node will
	 * be capable of utilizing multiple physical links simultaneosly
	 * to create higher quality or more robust aggregate links.
	 *
	 * See: https://www.zerotier.com/manual.shtml#2_1_5
	 */
	bool multipathEnabled;

	/**
	 * The port used by the service to send and receive
	 * all encapsulated traffic
	 */
	uint16_t primaryPort;

	/**
	 * Planet ID
	 */
	uint64_t planetWorldId;
	uint64_t planetWorldTimestamp;
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
	int eventCode;

	struct zts_node_details *node;
	struct zts_network_details *network;
	struct zts_netif_details *netif;
	struct zts_virtual_network_route *route;
	struct zts_physical_path *path;
	struct zts_peer_details *peer;
	struct zts_addr_details *addr;
};

struct zts_addr_details
{
	uint64_t nwid;
	struct sockaddr_storage addr;
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

	/**
	 * The IPv4 address assigned to this interface.
	 */
	//struct sockaddr_in ip4_addr;

	/**
	 * The IPv6 addresses assigned to this interface.
	 */
	//struct sockaddr_in6 ip6_addr[LWIP_IPV6_NUM_ADDRESSES];

	/**
	 * Number of IPv4 addresses assigned to this interface
	 */
	//int num_ip4_addr;

	/**
	 * Number of IPv6 addresses assigned to this interface
	 */
	//int num_ip6_addr;
};

/**
 * A structure used to represent a virtual network route
 */
struct zts_virtual_network_route
{
	/**
	 * Target network / netmask bits (in port field) or NULL or 0.0.0.0/0 for default
	 */
	struct sockaddr_storage target;

	/**
	 * Gateway IP address (port ignored) or NULL (family == 0) for LAN-local (no gateway)
	 */
	struct sockaddr_storage via;

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
 * A structure used to convey network-specific details to the user application
 */
struct zts_network_details
{
	/**
	 * Network ID
	 */
	uint64_t nwid;

	/**
	 * Maximum Transmission Unit size for this network
	 */
	int mtu;

	/**
	 * Number of addresses (actually) assigned to the node on this network
	 */
	short num_addresses;

	/**
	 * Array of IPv4 and IPv6 addresses assigned to the node on this network
	 */
	struct sockaddr_storage addr[ZTS_MAX_ASSIGNED_ADDRESSES];

	/**
	 * Number of routes
	 */
	unsigned int num_routes;

	/**
	 * Array of IPv4 and IPv6 addresses assigned to the node on this network
	 */
	struct zts_virtual_network_route routes[ZTS_MAX_NETWORK_ROUTES];
};

/**
 * Physical network path to a peer
 */
struct zts_physical_path
{
	/**
	 * Address of endpoint
	 */
	struct sockaddr_storage address;

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
	struct zts_physical_path paths[ZT_MAX_PEER_NETWORK_PATHS];
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
// ZeroTier Service Controls                                                //
//////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Starts the ZeroTier service and notifies user application of events via callback
 *
 * @usage Should be called at the beginning of your application. Will blocks until all of the following conditions are met:
 * - ZeroTier core service has been initialized
 * - Cryptographic identity has been generated or loaded from directory specified by `path`
 * - Virtual network is successfully joined
 * - IP address is assigned by network controller service
 * @param path path directory where cryptographic identities and network configuration files are stored and retrieved
 *              (`identity.public`, `identity.secret`)
 * @param userCallbackFunc User-specified callback for ZeroTier events
 * @return 0 if successful; or 1 if failed
 */
ZT_SOCKET_API int ZTCALL zts_start(const char *path, void (*userCallbackFunc)(struct zts_callback_msg*), int port);

/**
 * @brief Stops the ZeroTier service, brings down all virtual interfaces in order to stop all traffic processing.
 *
 * @usage This should be called when the application anticipates not needing any sort of traffic processing for a
 * prolonged period of time. The stack driver (with associated timers) will remain active in case future traffic
 * processing is required. Note that the application must tolerate a multi-second startup time if zts_start()
 * zts_startjoin() is called again. To stop this background thread and free all resources use zts_free() instead.
 * @return Returns 0 on success, -1 on failure
 */
ZT_SOCKET_API int ZTCALL zts_stop();

/**
 * @brief Stops and re-starts the ZeroTier service.
 *
 * @usage This call will block until the service has been brought offline. Then
 * it will return and the user application can then watch for the appropriate
 * startup callback events.
 * @return Returns ZTS_ERR_OK on success, -1 on failure
 */
ZT_SOCKET_API int ZTCALL zts_restart();

/**
 * @brief Stops all background services, brings down all interfaces, frees all resources. After calling this function
 * an application restart will be required before the library can be used again. This is a blocking call.
 *
 * @usage This should be called at the end of your program or when you do not anticipate communicating over ZeroTier
 * @return Returns 0 on success, -1 on failure
 */
ZT_SOCKET_API int ZTCALL zts_free();

/**
 * @brief Return whether the ZeroTier service is currently running
 *
 * @usage Call this after zts_start()
 * @return 1 if running, 0 if not running
 */
ZT_SOCKET_API int ZTCALL zts_core_running();

/**
 * @brief Return the number of networks currently joined by this node
 *
 * @usage Call this after zts_start(), zts_startjoin() and/or zts_join()
 * @return Number of networks joined by this node
 */
ZT_SOCKET_API int ZTCALL zts_get_num_joined_networks();

/**
 * @brief Populates a structure with details for a given network
 *
 * @usage Call this from the application thread any time after the node has joined a network
 * @param nwid A 16-digit hexidecimal virtual network ID
 * @param nd Pointer to a zts_network_details structure to populate
 * @return ZTS_ERR_SERVICE if failed, 0 if otherwise
 */
ZT_SOCKET_API int ZTCALL zts_get_network_details(uint64_t nwid, struct zts_network_details *nd);

/**
 * @brief Populates an array of structures with details for any given number of networks
 *
 * @usage Call this from the application thread any time after the node has joined a network
 * @param nds Pointer to an array of zts_network_details structures to populate
 * @param num Number of zts_network_details structures available to copy data into, will be updated
 * to reflect number of structures that were actually populated
 * @return ZTS_ERR_SERVICE if failed, 0 if otherwise
 */
ZT_SOCKET_API int ZTCALL zts_get_all_network_details(struct zts_network_details *nds, int *num);

/**
 * @brief Join a network
 *
 * @usage Call this from application thread. Only after zts_start() has succeeded
 * @param nwid A 16-digit hexidecimal virtual network ID
 * @return 0 if successful, -1 for any failure
 */
ZT_SOCKET_API int ZTCALL zts_join(const uint64_t nwid);

/**
 * @brief Leave a network
 *
 * @usage Call this from application thread. Only after zts_start() has succeeded
 * @param nwid A 16-digit hexidecimal virtual network ID
 * @return 0 if successful, -1 for any failure
 */
ZT_SOCKET_API int ZTCALL zts_leave(const uint64_t nwid);

/**
 * @brief Leaves all networks
 *
 * @usage Call this from application thread. Only after zts_start() has succeeded
 * @return 0 if successful, -1 for any failure
 */
ZT_SOCKET_API int ZTCALL zts_leave_all();

/**
 * @brief Orbits a given moon (user-defined root server)
 *
 * @usage Call this from application thread. Only after zts_start() has succeeded
 * @param moonWorldId A 16-digit hexidecimal world ID
 * @param moonSeed A 16-digit hexidecimal seed ID
 * @return ZTS_ERR_OK if successful, ZTS_ERR_SERVICE, ZTS_ERR_INVALID_ARG, ZTS_ERR_INVALID_OP if otherwise
 */
ZT_SOCKET_API int ZTCALL zts_orbit(uint64_t moonWorldId, uint64_t moonSeed);

/**
 * @brief De-orbits a given moon (user-defined root server)
 *
 * @usage Call this from application thread. Only after zts_start() has succeeded
 * @param moonWorldId A 16-digit hexidecimal world ID
 * @return ZTS_ERR_OK if successful, ZTS_ERR_SERVICE, ZTS_ERR_INVALID_ARG, ZTS_ERR_INVALID_OP if otherwise
 */
ZT_SOCKET_API int ZTCALL zts_deorbit(uint64_t moonWorldId);

/**
 * @brief Copies the configuration path used by ZeroTier into the provided buffer
 *
 * @usage Use this to determine where ZeroTier is storing identity files
 * @param homePath Path to ZeroTier configuration files
 * @param len Length of destination buffer
 * @return 0 if no error, -1 if invalid argument was supplied
 */
ZT_SOCKET_API int ZTCALL zts_get_path(char *homePath, size_t *len);

/**
 * @brief Returns the node ID of this instance
 *
 * @usage Call this after zts_start() and/or when zts_running() returns true
 * @return
 */
ZT_SOCKET_API uint64_t ZTCALL zts_get_node_id();

/**
 * @brief Returns whether any address has been assigned to the SockTap for this network
 *
 * @usage This is used as an indicator of readiness for service for the ZeroTier core and stack
 * @param nwid Network ID
 * @return
 */
ZT_SOCKET_API int ZTCALL zts_has_address(const uint64_t nwid);


/**
 * @brief Returns the number of addresses assigned to this node for the given nwid
 *
 * @param nwid Network ID
 * @return The number of addresses assigned
 */
ZT_SOCKET_API int ZTCALL zts_get_num_assigned_addresses(const uint64_t nwid);

/**
 * @brief Returns the assigned address located at the given index
 *
 * @usage The indices of each assigned address are not guaranteed and should only
 * be used for iterative purposes.
 * @param nwid Network ID
 * @param index location of assigned address
 * @return The number of addresses assigned
 */
ZT_SOCKET_API int ZTCALL zts_get_address_at_index(
	const uint64_t nwid, const int index, struct sockaddr *addr, socklen_t *addrlen);

/**
 * @brief Get IP address for this device on a given network
 *
 * @usage FIXME: Only returns first address found, good enough for most cases
 * @param nwid Network ID
 * @param addr Destination structure for address
 * @param addrlen size of destination address buffer, will be changed to size of returned address
 * @return 0 if an address was successfully found, -1 if failure
 */
ZT_SOCKET_API int ZTCALL zts_get_address(
	const uint64_t nwid, struct sockaddr_storage *addr, const int address_family);

/**
 * @brief Computes a 6PLANE IPv6 address for the given Network ID and Node ID
 *
 * @usage Can call any time
 * @param addr Destination structure for address
 * @param nwid Network ID 
 * @param nodeId Node ID
 * @return
 */
ZT_SOCKET_API void ZTCALL zts_get_6plane_addr(
	struct sockaddr_storage *addr, const uint64_t nwid, const uint64_t nodeId);

/**
 * @brief Computes a RFC4193 IPv6 address for the given Network ID and Node ID
 *
 * @usage Can call any time
 * @param addr Destination structure for address
 * @param nwid Network ID 
 * @param nodeId Node ID
 * @return
 */
ZT_SOCKET_API void ZTCALL zts_get_rfc4193_addr(
	struct sockaddr_storage *addr, const uint64_t nwid, const uint64_t nodeId);

/**
 * @brief Return the number of peers
 *
 * @usage Call this after zts_start() has succeeded
 * @return
 */
ZT_SOCKET_API int zts_get_peer_count();

/**
 * @brief Return details of all peers
 *
 * @param pds Pointer to array of zts_peer_details structs to be filled out
 * @param num Length of destination array, will be filled out with actual number
 * of peers that details were available for.
 * @usage Call this after zts_start() has succeeded
 * @return
 */
ZT_SOCKET_API int zts_get_peers(struct zts_peer_details *pds, int *num);

/**
 * @brief Return details of a given peer.
 *
 * @param pds Pointer to zts_peer_details struct to be filled out
 * @param peerId ID of peer that the caller wants details of
 * @usage Call this after zts_start() has succeeded
 * @return
 */
ZT_SOCKET_API int zts_get_peer(struct zts_peer_details *pds, uint64_t peerId);

/**
 * @brief Starts a ZeroTier service in the background
 *
 * @usage For internal use only.
 * @param
 * @return
 */
#if defined(_WIN32)
DWORD WINAPI _zts_start_service(LPVOID thread_id);
#else
void *_zts_start_service(void *thread_id);
#endif

/**
 * @brief [Should not be called from user application] This function must be surrounded by 
 * ZT service locks. It will determine if it is currently safe and allowed to operate on 
 * the service.
 * @usage Can be called at any time
 * @return 1 or 0
 */
int _zts_can_perform_service_operation();

//////////////////////////////////////////////////////////////////////////////
// Statistics                                                               //
//////////////////////////////////////////////////////////////////////////////

#ifndef LWIP_STATS

/** Protocol related stats */
struct zts_stats_proto {
  uint32_t xmit;             /* Transmitted packets. */
  uint32_t recv;             /* Received packets. */
  uint32_t fw;               /* Forwarded packets. */
  uint32_t drop;             /* Dropped packets. */
  uint32_t chkerr;           /* Checksum error. */
  uint32_t lenerr;           /* Invalid length error. */
  uint32_t memerr;           /* Out of memory error. */
  uint32_t rterr;            /* Routing error. */
  uint32_t proterr;          /* Protocol error. */
  uint32_t opterr;           /* Error in options. */
  uint32_t err;              /* Misc error. */
  uint32_t cachehit;
};

/** IGMP stats */
struct zts_stats_igmp {
  uint32_t xmit;             /* Transmitted packets. */
  uint32_t recv;             /* Received packets. */
  uint32_t drop;             /* Dropped packets. */
  uint32_t chkerr;           /* Checksum error. */
  uint32_t lenerr;           /* Invalid length error. */
  uint32_t memerr;           /* Out of memory error. */
  uint32_t proterr;          /* Protocol error. */
  uint32_t rx_v1;            /* Received v1 frames. */
  uint32_t rx_group;         /* Received group-specific queries. */
  uint32_t rx_general;       /* Received general queries. */
  uint32_t rx_report;        /* Received reports. */
  uint32_t tx_join;          /* Sent joins. */
  uint32_t tx_leave;         /* Sent leaves. */
  uint32_t tx_report;        /* Sent reports. */
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

#endif

/**
 * @brief Returns all statistical counters for all protocols (inefficient)
 *
 * @usage This function can only be used in debug builds. It can be called at
 * any time after the node has come online
 * @return ZTS_ERR_OK if successful, ZTS_ERR_* otherwise
 */
int zts_get_all_stats(struct zts_stats *statsDest);


/**
 * @brief Populates the given structure with the requested protocol's
 * statistical counters (from lwIP)
 *
 * @usage This function can only be used in debug builds. It can be called at
 * any time after the node has come online
 * @return ZTS_ERR_OK if successful, ZTS_ERR_* otherwise
 */
int zts_get_protocol_stats(int protocolType, void *protoStatsDest);

//////////////////////////////////////////////////////////////////////////////
// Status getters                                                           //
//////////////////////////////////////////////////////////////////////////////

/**
 * @brief Queries a the status of the core node/service
 *
 * @usage Can be called at any time
 * @return ZTS_EVENT_NODE_ONLINE, ZTS_EVENT_NODE_OFFLINE, or standard ZTS_ errors
 */
ZT_SOCKET_API int zts_get_node_status();

/**
 * @brief Queries a the status of a network
 *
 * @usage Can be called at any time
 * @return ZTS_NETWORK_ values, or standard ZTS_ errors
 */
ZT_SOCKET_API int zts_get_network_status(uint64_t nwid);

/**
 * @brief Determines whether a peer is reachable via a P2P connection
 * or is being relayed via roots.
 *
 * @usage
 * @param peerId The ID of the peer to check
 * @return ZTS_PEER_ values, or standard ZTS_ errors
 */
ZT_SOCKET_API int zts_get_peer_status(uint64_t peerId);

//////////////////////////////////////////////////////////////////////////////
// Socket API                                                               //
//////////////////////////////////////////////////////////////////////////////

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
ZT_SOCKET_API int ZTCALL zts_setsockopt(
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
ZT_SOCKET_API int ZTCALL zts_getsockopt(
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
ZT_SOCKET_API int ZTCALL zts_select(
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

//} // namespace ZeroTier

#endif // _H
