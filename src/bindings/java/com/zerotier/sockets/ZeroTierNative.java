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

package com.zerotier.sockets;

/**
 * Class that exposes the low-level C socket interface provided by libzt. This
 * can be used instead of the higher-level ZeroTierSocket API.
 */
public class ZeroTierNative {
    // Only to be called by static initializer of this class
    public static native int zts_init();

    static
    {
        // Loads dynamic library at initialization time
        System.loadLibrary("zt");
        if (zts_init() != ZeroTierNative.ZTS_ERR_OK) {
            throw new ExceptionInInitializerError("JNI init() failed (see GetJavaVM())");
        }
    }

    //----------------------------------------------------------------------------//
    // Event codes                                                                //
    //----------------------------------------------------------------------------//

    /**
     * Node has been initialized
     *
     * This is the first event generated; and is always sent. It may occur
     * before node's constructor returns.
     *
     */
    public static int ZTS_EVENT_NODE_UP = 200;

    /**
     * Node is online -- at least one upstream node appears reachable
     *
     */
    public static int ZTS_EVENT_NODE_ONLINE = 201;

    /**
     * Node is offline -- network does not seem to be reachable by any available
     * strategy
     *
     */
    public static int ZTS_EVENT_NODE_OFFLINE = 202;

    /**
     * Node is shutting down
     *
     * This is generated within Node's destructor when it is being shut down.
     * It's done for convenience; since cleaning up other state in the event
     * handler may appear more idiomatic.
     *
     */
    public static int ZTS_EVENT_NODE_DOWN = 203;

    /**
     * A fatal error has occurred. One possible reason is:
     *
     * Your identity has collided with another node's ZeroTier address
     *
     * This happens if two different public keys both hash (via the algorithm
     * in Identity::generate()) to the same 40-bit ZeroTier address.
     *
     * This is something you should "never" see; where "never" is defined as
     * once per 2^39 new node initializations / identity creations. If you do
     * see it; you're going to see it very soon after a node is first
     * initialized.
     *
     * This is reported as an event rather than a return code since it's
     * detected asynchronously via error messages from authoritative nodes.
     *
     * If this occurs; you must shut down and delete the node; delete the
     * identity.secret record/file from the data store; and restart to generate
     * a new identity. If you don't do this; you will not be able to communicate
     * with other nodes.
     *
     * We'd automate this process; but we don't think silently deleting
     * private keys or changing our address without telling the calling code
     * is good form. It violates the principle of least surprise.
     *
     * You can technically get away with not handling this; but we recommend
     * doing so in a mature reliable application. Besides; handling this
     * condition is a good way to make sure it never arises. It's like how
     * umbrellas prevent rain and smoke detectors prevent fires. They do; right?
     *
     * Meta-data: none
     */
    public static int ZTS_EVENT_NODE_FATAL_ERROR = 204;

    /** Network ID does not correspond to a known network */
    public static int ZTS_EVENT_NETWORK_NOT_FOUND = 210;
    /** The version of ZeroTier inside libzt is too old */
    public static int ZTS_EVENT_NETWORK_CLIENT_TOO_OLD = 211;
    /** The configuration for a network has been requested (no action needed) */
    public static int ZTS_EVENT_NETWORK_REQ_CONFIG = 212;
    /** The node joined the network successfully (no action needed) */
    public static int ZTS_EVENT_NETWORK_OK = 213;
    /** The node is not allowed to join the network (you must authorize node) */
    public static int ZTS_EVENT_NETWORK_ACCESS_DENIED = 214;
    /** The node has received an IPv4 address from the network controller */
    public static int ZTS_EVENT_NETWORK_READY_IP4 = 215;
    /** The node has received an IPv6 address from the network controller */
    public static int ZTS_EVENT_NETWORK_READY_IP6 = 216;
    /** Deprecated */
    public static int ZTS_EVENT_NETWORK_READY_IP4_IP6 = 217;
    /** Network controller is unreachable */
    public static int ZTS_EVENT_NETWORK_DOWN = 218;
    /** Network change received from controller */
    public static int ZTS_EVENT_NETWORK_UPDATE = 219;

    /** TCP/IP stack (lwIP) is up (for debug purposes) */
    public static int ZTS_EVENT_STACK_UP = 220;
    /** TCP/IP stack (lwIP) id down (for debug purposes) */
    public static int ZTS_EVENT_STACK_DOWN = 221;

    /** lwIP netif up (for debug purposes) */
    public static int ZTS_EVENT_NETIF_UP = 230;
    /** lwIP netif down (for debug purposes) */
    public static int ZTS_EVENT_NETIF_DOWN = 231;
    /** lwIP netif removed (for debug purposes) */
    public static int ZTS_EVENT_NETIF_REMOVED = 232;
    /** lwIP netif link up (for debug purposes) */
    public static int ZTS_EVENT_NETIF_LINK_UP = 233;
    /** lwIP netif link down (for debug purposes) */
    public static int ZTS_EVENT_NETIF_LINK_DOWN = 234;

    /** A direct P2P path to peer is known */
    public static int ZTS_EVENT_PEER_DIRECT = 240;
    /** A direct P2P path to peer is NOT known. Traffic is now relayed  */
    public static int ZTS_EVENT_PEER_RELAY = 241;
    /** A peer is unreachable. Check NAT/Firewall settings */
    public static int ZTS_EVENT_PEER_UNREACHABLE = 242;
    /** A new path to a peer was discovered */
    public static int ZTS_EVENT_PEER_PATH_DISCOVERED = 243;
    /** A known path to a peer is now considered dead */
    public static int ZTS_EVENT_PEER_PATH_DEAD = 244;

    /** A new managed network route was added */
    public static int ZTS_EVENT_ROUTE_ADDED = 250;
    /** A managed network route was removed */
    public static int ZTS_EVENT_ROUTE_REMOVED = 251;

    /** A new managed IPv4 address was assigned to this peer */
    public static int ZTS_EVENT_ADDR_ADDED_IP4 = 260;
    /** A managed IPv4 address assignment was removed from this peer  */
    public static int ZTS_EVENT_ADDR_REMOVED_IP4 = 261;
    /** A new managed IPv4 address was assigned to this peer  */
    public static int ZTS_EVENT_ADDR_ADDED_IP6 = 262;
    /** A managed IPv6 address assignment was removed from this peer  */
    public static int ZTS_EVENT_ADDR_REMOVED_IP6 = 263;

    /** The node's secret key (identity) */
    public static int ZTS_EVENT_STORE_IDENTITY_SECRET = 270;
    /** The node's public key (identity) */
    public static int ZTS_EVENT_STORE_IDENTITY_PUBLIC = 271;
    /** The node has received an updated planet config */
    public static int ZTS_EVENT_STORE_PLANET = 272;
    /** New reachability hints and peer configuration */
    public static int ZTS_EVENT_STORE_PEER = 273;
    /** New network config */
    public static int ZTS_EVENT_STORE_NETWORK = 274;

    // Socket protocol types
    /** Stream socket */
    public static int ZTS_SOCK_STREAM = 0x00000001;
    /** Datagram socket */
    public static int ZTS_SOCK_DGRAM = 0x00000002;
    public static int ZTS_SOCK_RAW = 0x00000003;
    // Socket family types
    /** IPv4 address family */
    public static int ZTS_AF_INET = 0x00000002;
    // another test comment
    /** IPv6 address family */
    public static int ZTS_AF_INET6 = 0x0000000a;
    /* yet one more */
    public static int ZTS_PF_INET = ZTS_AF_INET;
    public static int ZTS_PF_INET6 = ZTS_AF_INET6;
    // Used as level numbers for setsockopt() and getsockopt()
    public static int ZTS_IPPROTO_IP = 0x00000000;
    public static int ZTS_IPPROTO_ICMP = 0x00000001;
    public static int ZTS_IPPROTO_TCP = 0x00000006;
    public static int ZTS_IPPROTO_UDP = 0x00000011;
    public static int ZTS_IPPROTO_IPV6 = 0x00000029;
    public static int ZTS_IPPROTO_ICMPV6 = 0x0000003a;
    public static int ZTS_IPPROTO_UDPLITE = 0x00000088;
    public static int ZTS_IPPROTO_RAW = 0x000000ff;
    // send() and recv() flags
    public static int ZTS_MSG_PEEK = 0x00000001;
    public static int ZTS_MSG_WAITALL = 0x00000002;
    public static int ZTS_MSG_OOB = 0x00000004;
    public static int ZTS_MSG_DONTWAIT = 0x00000008;
    public static int ZTS_MSG_MORE = 0x00000010;
    // fnctl() commands
    public static int ZTS_F_GETFL = 0x00000003;
    public static int ZTS_F_SETFL = 0x00000004;
    // fnctl() flags
    public static int ZTS_O_NONBLOCK = 0x00000001;
    public static int ZTS_O_NDELAY = 0x00000001;
    // Shutdown commands
    public static int ZTS_SHUT_RD = 0x00000000;
    public static int ZTS_SHUT_WR = 0x00000001;
    public static int ZTS_SHUT_RDWR = 0x00000002;
    // ioctl() commands
    public static int ZTS_FIONREAD = 0x4008667F;
    public static int ZTS_FIONBIO = 0x8008667E;
    // Socket level option number
    public static int ZTS_SOL_SOCKET = 0x00000FFF;
    // Socket options
    public static int ZTS_SO_REUSEADDR = 0x00000004;
    public static int ZTS_SO_KEEPALIVE = 0x00000008;
    public static int ZTS_SO_BROADCAST = 0x00000020;
    // Socket options
    public static int ZTS_SO_DEBUG = 0x00000001;   // NOT YET SUPPORTED
    public static int ZTS_SO_ACCEPTCONN = 0x00000002;
    public static int ZTS_SO_DONTROUTE = 0x00000010;     // NOT YET SUPPORTED
    public static int ZTS_SO_USELOOPBACK = 0x00000040;   // NOT YET SUPPORTED
    public static int ZTS_SO_LINGER = 0x00000080;
    public static int ZTS_SO_DONTLINGER = ((int)(~ZTS_SO_LINGER));
    public static int ZTS_SO_OOBINLINE = 0x00000100;   // NOT YET SUPPORTED
    public static int ZTS_SO_REUSEPORT = 0x00000200;   // NOT YET SUPPORTED
    public static int ZTS_SO_SNDBUF = 0x00001001;      // NOT YET SUPPORTED
    public static int ZTS_SO_RCVBUF = 0x00001002;
    public static int ZTS_SO_SNDLOWAT = 0x00001003;   // NOT YET SUPPORTED
    public static int ZTS_SO_RCVLOWAT = 0x00001004;   // NOT YET SUPPORTED
    public static int ZTS_SO_SNDTIMEO = 0x00001005;
    public static int ZTS_SO_RCVTIMEO = 0x00001006;
    public static int ZTS_SO_ERROR = 0x00001007;
    public static int ZTS_SO_TYPE = 0x00001008;
    public static int ZTS_SO_CONTIMEO = 0x00001009;   // NOT YET SUPPORTED
    public static int ZTS_SO_NO_CHECK = 0x0000100a;
    // IPPROTO_IP options
    public static int ZTS_IP_TOS = 0x00000001;
    public static int ZTS_IP_TTL = 0x00000002;
    // IPPROTO_TCP options
    public static int ZTS_TCP_NODELAY = 0x00000001;
    public static int ZTS_TCP_KEEPALIVE = 0x00000002;
    public static int ZTS_TCP_KEEPIDLE = 0x00000003;
    public static int ZTS_TCP_KEEPINTVL = 0x00000004;
    public static int ZTS_TCP_KEEPCNT = 0x00000005;

    //----------------------------------------------------------------------------//
    // Error codes                                                                //
    //----------------------------------------------------------------------------//

    /** (0) No error */
    public static int ZTS_ERR_OK = 0;
    /** (-1) Socket error, see zts_errno */
    public static int ZTS_ERR_SOCKET = -1;
    /** (-2) You probably did something at the wrong time */
    public static int ZTS_ERR_SERVICE = -2;
    /** (-3) Invalid argument */
    public static int ZTS_ERR_ARG = -3;
    /** (-4) No result (not necessarily an error) */
    public static int ZTS_ERR_RESULT = -4;
    /** (-5) Consider filing a bug report */
    public static int ZTS_ERR_GENERAL = -5;

    //----------------------------------------------------------------------------//
    // zts_errno Error codes                                                      //
    //----------------------------------------------------------------------------//

    /** Operation not permitted (`zts_errno` value) */
    public static int ZTS_EPERM = 1;
    /** No such file or directory */
    public static int ZTS_ENOENT = 2;
    /** No such process */
    public static int ZTS_ESRCH = 3;
    /** Interrupted system call */
    public static int ZTS_EINTR = 4;
    /** I/O error */
    public static int ZTS_EIO = 5;
    /** No such device or address */
    public static int ZTS_ENXIO = 6;
    /** Bad file number */
    public static int ZTS_EBADF = 9;
    /** Try again */
    public static int ZTS_EAGAIN = 11;
    /** Operation would block */
    public static int ZTS_EWOULDBLOCK = 11;
    /** Out of memory */
    public static int ZTS_ENOMEM = 12;
    /** Permission denied */
    public static int ZTS_EACCES = 13;
    /** Bad address */
    public static int ZTS_EFAULT = 14;
    /** Device or resource busy */
    public static int ZTS_EBUSY = 16;
    /** File exists */
    public static int ZTS_EEXIST = 17;
    /** No such device */
    public static int ZTS_ENODEV = 19;
    /** Invalid argument */
    public static int ZTS_EINVAL = 22;
    /** File table overflow */
    public static int ZTS_ENFILE = 23;
    /** Too many open files */
    public static int ZTS_EMFILE = 24;
    /** Function not implemented */
    public static int ZTS_ENOSYS = 38;
    /** Socket operation on non-socket */
    public static int ZTS_ENOTSOCK = 88;
    /** Destination address required */
    public static int ZTS_EDESTADDRREQ = 89;
    /** Message too long */
    public static int ZTS_EMSGSIZE = 90;
    /** Protocol wrong type for socket */
    public static int ZTS_EPROTOTYPE = 91;
    /** Protocol not available */
    public static int ZTS_ENOPROTOOPT = 92;
    /** Protocol not supported */
    public static int ZTS_EPROTONOSUPPORT = 93;
    /** Socket type not supported */
    public static int ZTS_ESOCKTNOSUPPORT = 94;
    /** Operation not supported on transport endpoint */
    public static int ZTS_EOPNOTSUPP = 95;
    /** Protocol family not supported */
    public static int ZTS_EPFNOSUPPORT = 96;
    /** Address family not supported by protocol */
    public static int ZTS_EAFNOSUPPORT = 97;
    /** Address already in use */
    public static int ZTS_EADDRINUSE = 98;
    /** Cannot assign requested address */
    public static int ZTS_EADDRNOTAVAIL = 99;
    /** Network is down */
    public static int ZTS_ENETDOWN = 100;
    /** Network is unreachable */
    public static int ZTS_ENETUNREACH = 101;
    /** Software caused connection abort */
    public static int ZTS_ECONNABORTED = 103;
    /** Connection reset by peer */
    public static int ZTS_ECONNRESET = 104;
    /** No buffer space available */
    public static int ZTS_ENOBUFS = 105;
    /** Transport endpoint is already connected */
    public static int ZTS_EISCONN = 106;
    /** Transport endpoint is not connected */
    public static int ZTS_ENOTCONN = 107;
    /** Connection timed out */
    public static int ZTS_ETIMEDOUT = 110;
    /** No route to host */
    public static int ZTS_EHOSTUNREACH = 113;
    /** Operation already in progress */
    public static int ZTS_EALREADY = 114;
    /** Operation now in progress */
    public static int ZTS_EINPROGRESS = 115;

    //----------------------------------------------------------------------------//
    // Misc definitions                                                           //
    //----------------------------------------------------------------------------//

    /**
     * Length of human-readable MAC address string
     */
    public static int ZTS_MAC_ADDRSTRLEN = 18;

    /**
     * Max length of human-readable IPv4 string
     */
    public static int ZTS_INET_ADDRSTRLEN = 16;

    /**
     * Max length of human-readable IPv6 string
     */
    public static int ZTS_INET6_ADDRSTRLEN = 46;

    /**
     * Maximum (and required) length of string buffers used to receive
     * string-format IP addresses from the API. This is set to `ZTS_INET6_ADDRSTRLEN`
     * to handle all cases: `ZTS_AF_INET` and `ZTS_AF_INET6`
     */
    public static int ZTS_IP_MAX_STR_LEN = 46;

    /**
     * Required buffer length to safely receive data store items
     */
    public static int ZTS_STORE_DATA_LEN = 4096;

    /**
     * Maximum length of network short name
     */
    public static int ZTS_MAX_NETWORK_SHORT_NAME_LENGTH = 127;

    /**
     * Maximum number of pushed routes on a network
     */
    public static int ZTS_MAX_NETWORK_ROUTES = 32;

    /**
     * Maximum number of statically assigned IP addresses per network endpoint
     * using ZT address management (not DHCP)
     */
    public static int ZTS_MAX_ASSIGNED_ADDRESSES = 16;

    /**
     * Maximum number of direct network paths to a given peer
     */
    public static int ZTS_MAX_PEER_NETWORK_PATHS = 16;

    /**
     * Maximum number of multicast groups a device / network interface can be
     * subscribed to at once
     */
    public static int ZTS_MAX_MULTICAST_SUBSCRIPTIONS = 1024;

    /**
     * The length of a human-friendly identity key pair string
     */
    public static int ZTS_ID_STR_BUF_LEN = 384;

    // public static native int zts_id_new(char* key,  int* key_buf_len);
    // public static native int zts_id_pair_is_valid(/*const*/ char* key,  int len);
    public static native int zts_init_from_storage(String path);
    public static native int zts_init_set_event_handler(ZeroTierEventListener callbackClass);
    public static native int zts_init_set_port(short port);
    // public static native int zts_init_from_memory(/*const*/ char* key,  int len);
    public static native int zts_init_blacklist_if(/*const*/ String prefix, int len);
    // public static native int zts_init_set_roots(/*const*/ void* roots_data,  int len);
    public static native int zts_init_allow_net_cache(int allowed);
    public static native int zts_init_allow_peer_cache(int allowed);
    public static native int zts_init_allow_roots_cache(int allowed);
    public static native int zts_init_allow_id_cache(int allowed);
    public static native int zts_addr_is_assigned(long net_id, int family);
    public static native String zts_addr_get_str(long net_id, int family);
    // public static native int zts_addr_get_all(long net_id, struct sockaddr_storage* addr,  int* count);
    // public static native int zts_addr_compute_6plane(/*const*/ long net_id, /*const*/ long node_id, struct
    // sockaddr_storage* addr); public static native int zts_addr_compute_rfc4193(/*const*/ long net_id, /*const*/ long
    // node_id, struct sockaddr_storage* addr);
    public static native int zts_addr_compute_rfc4193_str(long net_id, long node_id, String dst, int len);
    public static native int zts_addr_compute_6plane_str(long net_id, long node_id, String dst, int len);
    public static native long zts_net_compute_adhoc_id(short start_port, short end_port);
    public static native int zts_net_join(long net_id);
    public static native int zts_net_leave(long net_id);
    public static native int zts_net_transport_is_ready(/*const*/ long net_id);
    public static native long zts_net_get_mac(long net_id);
    public static native String zts_net_get_mac_str(long net_id);
    public static native int zts_net_get_broadcast(long net_id);
    public static native int zts_net_get_mtu(long net_id);
    public static native int zts_net_get_name(long net_id, String dst, int len);
    public static native int zts_net_get_status(long net_id);
    public static native int zts_net_get_type(long net_id);
    public static native int zts_route_is_assigned(long net_id, int family);
    public static native int zts_node_start();
    public static native int zts_node_is_online();
    public static native long zts_node_get_id();
    // public static native int zts_node_get_id_pair(char* key, int* key_buf_len);
    public static native int zts_node_get_port();
    public static native int zts_node_stop();
    public static native int zts_node_free();
    public static native int zts_moon_orbit(long moon_roots_id, long moon_seed);
    public static native int zts_moon_deorbit(long moon_roots_id);
    public static native int zts_connect(int fd, String ipstr, int port, int timeout_ms);
    public static native int zts_bind(int fd, String ipstr, int port);
    // public static native int zts_accept(int fd, String remote_addr, int len, int* port);
    public static native int zts_set_no_delay(int fd, int enabled);
    public static native int zts_get_no_delay(int fd);
    public static native int zts_set_linger(int fd, int enabled, int value);
    public static native int zts_get_linger_enabled(int fd);
    public static native int zts_get_linger_value(int fd);
    public static native int zts_get_pending_data_size(int fd);
    public static native int zts_set_reuse_addr(int fd, int enabled);
    public static native int zts_get_reuse_addr(int fd);
    public static native int zts_set_recv_timeout(int fd, int seconds, int microseconds);
    public static native int zts_get_recv_timeout(int fd);
    public static native int zts_set_send_timeout(int fd, int seconds, int microseconds);
    public static native int zts_get_send_timeout(int fd);
    public static native int zts_set_send_buf_size(int fd, int size);
    public static native int zts_get_send_buf_size(int fd);
    public static native int zts_set_recv_buf_size(int fd, int size);
    public static native int zts_get_recv_buf_size(int fd);
    public static native int zts_set_ttl(int fd, int ttl);
    public static native int zts_get_ttl(int fd);
    public static native int zts_set_blocking(int fd, int enabled);
    public static native int zts_get_blocking(int fd);
    public static native int zts_set_keepalive(int fd, int enabled);
    public static native int zts_get_keepalive(int fd);
    // struct hostent* gethostbyname(/*const*/ String name);
    // public static native int zts_dns_set_server(uint8_t index, /*const*/ ip_addr* addr);
    // ip_addr* dns_get_server(uint8_t index);
    public static native int zts_core_lock_obtain();
    public static native int zts_core_lock_release();
    public static native int zts_core_query_addr_count(long net_id);
    public static native int zts_core_query_addr(long net_id, int idx, String addr, int len);
    public static native int zts_core_query_route_count(long net_id);
    /*
    public static native int zts_core_query_route(
        long net_id,
         int idx,
        String target,
        String via,
         int len,
        short* flags,
        short* metric);
    */
    public static native int zts_core_query_path_count(long peer_id);
    public static native int zts_core_query_path(long peer_id, int idx, String dst, int len);
    public static native int zts_core_query_mc_count(long net_id);
    // public static native int zts_core_query_mc(long net_id,  int idx, long* mac, uint32_t* adi);
    /*
    public static native int zts_util_roots_new(
        char* roots_out,
         int* roots_len,
        char* prev_key,
         int* prev_key_len,
        char* curr_key,
         int* curr_key_len,
        long id,
        long ts,
        roots_t* roots_spec);
    */
    public static native void zts_util_delay(long milliseconds);
    public static native int zts_bsd_socket(int family, int type, int protocol);
    public static native int zts_bsd_listen(int fd, int backlog);
    public static native int zts_bsd_accept(int fd, ZeroTierSocketAddress addr);
    public static native int zts_bsd_read(int fd, byte[] buf);
    public static native int zts_bsd_read_offset(int fd, byte[] buf, int offset, int len);
    public static native int zts_bsd_read_length(int fd, byte[] buf, int len);
    public static native int zts_bsd_recv(int fd, byte[] buf, int flags);
    public static native int zts_bsd_recvfrom(int fd, byte[] buf, int flags, ZeroTierSocketAddress addr);
    public static native int zts_bsd_write(int fd, byte[] buf);
    public static native int zts_bsd_write_byte(int fd, byte b);
    public static native int zts_bsd_write_offset(int fd, byte[] buf, int offset, int len);
    public static native int zts_bsd_sendto(int fd, byte[] buf, int flags, ZeroTierSocketAddress addr);
    public static native int zts_bsd_send(int fd, byte[] buf, int flags);
    public static native int zts_bsd_shutdown(int fd, int how);
    public static native int zts_bsd_close(int fd);
    public static native boolean zts_bsd_getsockname(int fd, ZeroTierSocketAddress addr);
    public static native int zts_bsd_getpeername(int fd, ZeroTierSocketAddress addr);
    public static native int zts_bsd_fcntl(int sock, int cmd, int flag);
    // public static native int zts_bsd_ioctl(int fd, long request, ZeroTierIoctlArg arg);
    public static native int zts_bsd_select(
        int nfds,
        ZeroTierFileDescriptorSet readfds,
        ZeroTierFileDescriptorSet writefds,
        ZeroTierFileDescriptorSet exceptfds,
        int timeout_sec,
        int timeout_usec);
}
