
// Socket protocol types
/** Stream socket */
export const ZTS_SOCK_STREAM = 0x00000001;
/** Datagram socket */
export const ZTS_SOCK_DGRAM = 0x00000002;
export const ZTS_SOCK_RAW = 0x00000003;
// Socket family types
/** IPv4 address family */
export const ZTS_AF_INET = 0x00000002;
// another test comment
/** IPv6 address family */
export const ZTS_AF_INET6 = 0x0000000a;
/* yet one more */
export const ZTS_PF_INET = ZTS_AF_INET;
export const ZTS_PF_INET6 = ZTS_AF_INET6;
// Used as level numbers for setsockopt() and getsockopt()
export const ZTS_IPPROTO_IP = 0x00000000;
export const ZTS_IPPROTO_ICMP = 0x00000001;
export const ZTS_IPPROTO_TCP = 0x00000006;
export const ZTS_IPPROTO_UDP = 0x00000011;
export const ZTS_IPPROTO_IPV6 = 0x00000029;
export const ZTS_IPPROTO_ICMPV6 = 0x0000003a;
export const ZTS_IPPROTO_UDPLITE = 0x00000088;
export const ZTS_IPPROTO_RAW = 0x000000ff;
// send() and recv() flags
export const ZTS_MSG_PEEK = 0x00000001;
export const ZTS_MSG_WAITALL = 0x00000002;
export const ZTS_MSG_OOB = 0x00000004;
export const ZTS_MSG_DONTWAIT = 0x00000008;
export const ZTS_MSG_MORE = 0x00000010;
// fnctl() commands
export const ZTS_F_GETFL = 0x00000003;
export const ZTS_F_SETFL = 0x00000004;
// fnctl() flags
export const ZTS_O_NONBLOCK = 0x00000001;
export const ZTS_O_NDELAY = 0x00000001;
// Shutdown commands
export const ZTS_SHUT_RD = 0x00000000;
export const ZTS_SHUT_WR = 0x00000001;
export const ZTS_SHUT_RDWR = 0x00000002;
// ioctl() commands
export const ZTS_FIONREAD = 0x4008667F;
export const ZTS_FIONBIO = 0x8008667E;
// Socket level option number
export const ZTS_SOL_SOCKET = 0x00000FFF;
// Socket options
export const ZTS_SO_REUSEADDR = 0x00000004;
export const ZTS_SO_KEEPALIVE = 0x00000008;
export const ZTS_SO_BROADCAST = 0x00000020;

// // Socket options
// export const ZTS_SO_DEBUG = 0x00000001;   // NOT YET SUPPORTED
// export const ZTS_SO_ACCEPTCONN = 0x00000002;
// export const ZTS_SO_DONTROUTE = 0x00000010;     // NOT YET SUPPORTED
// export const ZTS_SO_USELOOPBACK = 0x00000040;   // NOT YET SUPPORTED
// export const ZTS_SO_LINGER = 0x00000080;
// export const ZTS_SO_DONTLINGER = ((int)(~ZTS_SO_LINGER));
// export const ZTS_SO_OOBINLINE = 0x00000100;   // NOT YET SUPPORTED
// export const ZTS_SO_REUSEPORT = 0x00000200;   // NOT YET SUPPORTED
// export const ZTS_SO_SNDBUF = 0x00001001;      // NOT YET SUPPORTED
// export const ZTS_SO_RCVBUF = 0x00001002;
// export const ZTS_SO_SNDLOWAT = 0x00001003;   // NOT YET SUPPORTED
// export const ZTS_SO_RCVLOWAT = 0x00001004;   // NOT YET SUPPORTED
// export const ZTS_SO_SNDTIMEO = 0x00001005;
// export const ZTS_SO_RCVTIMEO = 0x00001006;
// export const ZTS_SO_ERROR = 0x00001007;
// export const ZTS_SO_TYPE = 0x00001008;
// export const ZTS_SO_CONTIMEO = 0x00001009;   // NOT YET SUPPORTED
// export const ZTS_SO_NO_CHECK = 0x0000100a;

// IPPROTO_IP options
export const ZTS_IP_TOS = 0x00000001;
export const ZTS_IP_TTL = 0x00000002;
// IPPROTO_TCP options
export const ZTS_TCP_NODELAY = 0x00000001;
export const ZTS_TCP_KEEPALIVE = 0x00000002;
export const ZTS_TCP_KEEPIDLE = 0x00000003;
export const ZTS_TCP_KEEPINTVL = 0x00000004;
export const ZTS_TCP_KEEPCNT = 0x00000005;


//----------------------------------------------------------------------------//
// Misc definitions                                                           //
//----------------------------------------------------------------------------//

/**
     * Length of human-readable MAC address string
     */
export const ZTS_MAC_ADDRSTRLEN = 18;

/**
     * Max length of human-readable IPv4 string
     */
export const ZTS_INET_ADDRSTRLEN = 16;

/**
     * Max length of human-readable IPv6 string
     */
export const ZTS_INET6_ADDRSTRLEN = 46;

/**
     * Maximum (and required) length of string buffers used to receive
     * string-format IP addresses from the API. This is set to `ZTS_INET6_ADDRSTRLEN`
     * to handle all cases: `ZTS_AF_INET` and `ZTS_AF_INET6`
     */
export const ZTS_IP_MAX_STR_LEN = 46;

/**
     * Required buffer length to safely receive data store items
     */
export const ZTS_STORE_DATA_LEN = 4096;

/**
     * Maximum length of network short name
     */
export const ZTS_MAX_NETWORK_SHORT_NAME_LENGTH = 127;

/**
     * Maximum number of pushed routes on a network
     */
export const ZTS_MAX_NETWORK_ROUTES = 32;

/**
     * Maximum number of statically assigned IP addresses per network endpoint
     * using ZT address management (not DHCP)
     */
export const ZTS_MAX_ASSIGNED_ADDRESSES = 16;

/**
     * Maximum number of direct network paths to a given peer
     */
export const ZTS_MAX_PEER_NETWORK_PATHS = 64;

/**
     * Maximum number of multicast groups a device / network interface can be
     * subscribed to at once
     */
export const ZTS_MAX_MULTICAST_SUBSCRIPTIONS = 1024;

/**
     * The length of a human-friendly identity key pair string
     */
export const ZTS_ID_STR_BUF_LEN = 384;