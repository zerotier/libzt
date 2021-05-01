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

using ZeroTier;

namespace ZeroTier {
public class Constants {
    public static readonly short ERR_OK = 0;
    public static readonly short ERR_SOCKET = -1;
    public static readonly short ERR_SERVICE = -2;
    public static readonly short ERR_ARG = -3;
    public static readonly short ERR_NO_RESULT = -4;
    public static readonly short ERR_GENERAL = -5;

    public static readonly short EVENT_NODE_UP = 200;
    public static readonly short EVENT_NODE_ONLINE = 201;
    public static readonly short EVENT_NODE_OFFLINE = 202;
    public static readonly short EVENT_NODE_DOWN = 203;
    public static readonly short ZTS_EVENT_NODE_FATAL_ERROR = 204;

    public static readonly short EVENT_NETWORK_NOT_FOUND = 210;
    public static readonly short EVENT_NETWORK_CLIENT_TOO_OLD = 211;
    public static readonly short EVENT_NETWORK_REQ_CONFIG = 212;
    public static readonly short EVENT_NETWORK_OK = 213;
    public static readonly short EVENT_NETWORK_ACCESS_DENIED = 214;
    public static readonly short EVENT_NETWORK_READY_IP4 = 215;
    public static readonly short EVENT_NETWORK_READY_IP6 = 216;
    public static readonly short EVENT_NETWORK_READY_IP4_IP6 = 217;
    public static readonly short EVENT_NETWORK_DOWN = 218;
    public static readonly short EVENT_NETWORK_UPDATE = 219;

    public static readonly short EVENT_STACK_UP = 220;
    public static readonly short EVENT_STACK_DOWN = 221;

    public static readonly short EVENT_NETIF_UP = 230;
    public static readonly short EVENT_NETIF_DOWN = 231;
    public static readonly short EVENT_NETIF_REMOVED = 232;
    public static readonly short EVENT_NETIF_LINK_UP = 233;
    public static readonly short EVENT_NETIF_LINK_DOWN = 234;

    public static readonly short EVENT_PEER_DIRECT = 240;
    public static readonly short EVENT_PEER_RELAY = 241;
    public static readonly short EVENT_PEER_UNREACHABLE = 242;
    public static readonly short EVENT_PEER_PATH_DISCOVERED = 243;
    public static readonly short EVENT_PEER_PATH_DEAD = 244;

    public static readonly short EVENT_ROUTE_ADDED = 250;
    public static readonly short EVENT_ROUTE_REMOVED = 251;

    public static readonly short EVENT_ADDR_ADDED_IP4 = 260;
    public static readonly short EVENT_ADDR_REMOVED_IP4 = 261;
    public static readonly short EVENT_ADDR_ADDED_IP6 = 262;
    public static readonly short EVENT_ADDR_REMOVED_IP6 = 263;

    public static readonly short EVENT_STORE_IDENTITY_SECRET = 270;
    public static readonly short EVENT_STORE_IDENTITY_PUBLIC = 271;
    public static readonly short EVENT_STORE_PLANET = 272;
    public static readonly short EVENT_STORE_PEER = 273;
    public static readonly short EVENT_STORE_NETWORK = 274;

    // Socket error codes
    public static readonly short EPERM = 1;
    public static readonly short ENOENT = 2;
    public static readonly short ESRCH = 3;
    public static readonly short EINTR = 4;
    public static readonly short EIO = 5;
    public static readonly short ENXIO = 6;
    public static readonly short EBADF = 9;
    public static readonly short EAGAIN = 11;
    public static readonly short EWOULDBLOCK = 11;
    public static readonly short ENOMEM = 12;
    public static readonly short EACCES = 13;
    public static readonly short EFAULT = 14;
    public static readonly short EBUSY = 16;
    public static readonly short EEXIST = 17;
    public static readonly short ENODEV = 19;
    public static readonly short EINVAL = 22;
    public static readonly short ENFILE = 23;
    public static readonly short EMFILE = 24;
    public static readonly short ENOSYS = 38;
    public static readonly short ENOTSOCK = 88;
    public static readonly short EDESTADDRREQ = 89;
    public static readonly short EMSGSIZE = 90;
    public static readonly short EPROTOTYPE = 91;
    public static readonly short ENOPROTOOPT = 92;
    public static readonly short EPROTONOSUPPORT = 93;
    public static readonly short ESOCKTNOSUPPORT = 94;
    public static readonly short EOPNOTSUPP = 95;
    public static readonly short EPFNOSUPPORT = 96;
    public static readonly short EAFNOSUPPORT = 97;
    public static readonly short EADDRINUSE = 98;
    public static readonly short EADDRNOTAVAIL = 99;
    public static readonly short ENETDOWN = 100;
    public static readonly short ENETUNREACH = 101;
    public static readonly short ECONNABORTED = 103;
    public static readonly short ECONNRESET = 104;
    public static readonly short ENOBUFS = 105;
    public static readonly short EISCONN = 106;
    public static readonly short ENOTCONN = 107;
    public static readonly short ETIMEDOUT = 110;
    public static readonly short EHOSTUNREACH = 113;
    public static readonly short EALREADY = 114;
    public static readonly short EINPROGRESS = 115;

    // Common constants
    public static readonly short MAC_ADDRSTRLEN = 18;
    public static readonly short INET_ADDRSTRLEN = 16;
    public static readonly short INET6_ADDRSTRLEN = 46;
    public static readonly short IP_MAX_STR_LEN = 46;
    public static readonly short STORE_DATA_LEN = 4096;
    public static readonly short MAX_NETWORK_SHORT_NAME_LENGTH = 127;
    public static readonly short MAX_NETWORK_ROUTES = 32;
    public static readonly short MAX_ASSIGNED_ADDRESSES = 16;
    public static readonly short MAX_PEER_NETWORK_PATHS = 16;
    public static readonly short MAX_MULTICAST_SUBSCRIPTIONS = 1024;

    // Peer roles
    public static readonly byte PEER_ROLE_LEAF = 0;
    public static readonly byte PEER_ROLE_MOON = 1;
    public static readonly byte PEER_ROLE_PLANET = 2;

    // Network status codes
    public static readonly byte NETWORK_STATUS_REQUESTING_CONFIGURATION = 0;
    public static readonly byte NETWORK_STATUS_OK = 1;
    public static readonly byte NETWORK_STATUS_ACCESS_DENIED = 2;
    public static readonly byte NETWORK_STATUS_NOT_FOUND = 3;
    public static readonly byte NETWORK_STATUS_PORT_ERROR = 4;
    public static readonly byte NETWORK_STATUS_CLIENT_TOO_OLD = 5;

    //
    public static readonly byte NETWORK_TYPE_PRIVATE = 0;
    public static readonly byte NETWORK_TYPE_PUBLIC = 1;

    // Socket protocol types
    public static readonly short SOCK_STREAM = 0x0001;
    public static readonly short SOCK_DGRAM = 0x0002;
    public static readonly short SOCK_RAW = 0x0003;
    // Socket family types
    public static readonly short AF_UNSPEC = 0x0000;
    public static readonly short AF_INET = 0x0002;
    public static readonly short AF_INET6 = 0x000a;
    public static readonly short PF_INET = AF_INET;
    public static readonly short PF_INET6 = AF_INET6;
    public static readonly short PF_UNSPEC = AF_UNSPEC;
    // Protocol command types
    public static readonly short IPPROTO_IP = 0x0000;
    public static readonly short IPPROTO_ICMP = 0x0001;
    public static readonly short IPPROTO_TCP = 0x0006;
    public static readonly short IPPROTO_UDP = 0x0011;
    public static readonly short IPPROTO_IPV6 = 0x0029;
    public static readonly short IPPROTO_ICMPV6 = 0x003a;
    public static readonly short IPPROTO_UDPLITE = 0x0088;
    public static readonly short IPPROTO_RAW = 0x00ff;
    // send() and recv() flags
    public static readonly short MSG_PEEK = 0x0001;
    public static readonly short MSG_WAITALL = 0x0002;   // NOT YET SUPPORTED
    public static readonly short MSG_OOB = 0x0004;       // NOT YET SUPPORTED
    public static readonly short MSG_DONTWAIT = 0x0008;
    public static readonly short MSG_MORE = 0x0010;

    // Socket level option number
    public static readonly short SOL_SOCKET = 0x0fff;
    // Socket options
    public static readonly short SO_DEBUG = 0x0001;   // NOT YET SUPPORTED
    public static readonly short SO_ACCEPTCONN = 0x0002;
    public static readonly short SO_REUSEADDR = 0x0004;
    public static readonly short SO_KEEPALIVE = 0x0008;
    public static readonly short SO_DONTROUTE = 0x0010;   // NOT YET SUPPORTED
    public static readonly short SO_BROADCAST = 0x0020;
    public static readonly short SO_USELOOPBACK = 0x0040;   // NOT YET SUPPORTED
    public static readonly short SO_LINGER = 0x0080;
    public static readonly short SO_DONTLINGER = ((short)(~SO_LINGER));
    public static readonly short SO_OOBINLINE = 0x0100;   // NOT YET SUPPORTED
    public static readonly short SO_REUSEPORT = 0x0200;   // NOT YET SUPPORTED
    public static readonly short SO_SNDBUF = 0x1001;      // NOT YET SUPPORTED
    public static readonly short SO_RCVBUF = 0x1002;
    public static readonly short SO_SNDLOWAT = 0x1003;   // NOT YET SUPPORTED
    public static readonly short SO_RCVLOWAT = 0x1004;   // NOT YET SUPPORTED
    public static readonly short SO_SNDTIMEO = 0x1005;
    public static readonly short SO_RCVTIMEO = 0x1006;
    public static readonly short SO_ERROR = 0x1007;
    public static readonly short SO_TYPE = 0x1008;
    public static readonly short SO_CONTIMEO = 0x1009;
    public static readonly short SO_NO_CHECK = 0x100a;
    public static readonly short SO_BINDTODEVICE = 0x100b;
    // IPPROTO_IP options
    public static readonly short IP_TOS = 0x0001;
    public static readonly short IP_TTL = 0x0002;
    public static readonly short IP_PKTINFO = 0x0008;
    // IPPROTO_TCP options
    public static readonly short TCP_NODELAY = 0x0001;
    public static readonly short TCP_KEEPALIVE = 0x0002;
    public static readonly short TCP_KEEPIDLE = 0x0003;
    public static readonly short TCP_KEEPINTVL = 0x0004;
    public static readonly short TCP_KEEPCNT = 0x0005;
    // IPPROTO_IPV6 options
    public static readonly short IPV6_CHECKSUM =
        0x0007;   // RFC3542: calculate and insert the ICMPv6 checksum for raw sockets.
    public static readonly short IPV6_V6ONLY =
        0x001b;   // RFC3493: boolean control to restrict AF_INET6 sockets to IPv6 communications only.
    // UDPLITE options
    public static readonly short UDPLITE_SEND_CSCOV = 0x01;   // sender checksum coverage
    public static readonly short UDPLITE_RECV_CSCOV = 0x02;   // minimal receiver checksum coverage
    // UDPLITE options
    public static readonly short IP_MULTICAST_TTL = 5;
    public static readonly short IP_MULTICAST_IF = 6;
    public static readonly short IP_MULTICAST_LOOP = 7;

    // Multicast options
    public static readonly short IP_ADD_MEMBERSHIP = 3;
    public static readonly short IP_DROP_MEMBERSHIP = 4;

    public static readonly short IPV6_JOIN_GROUP = 12;
    public static readonly short IPV6_ADD_MEMBERSHIP = IPV6_JOIN_GROUP;
    public static readonly short IPV6_LEAVE_GROUP = 13;
    public static readonly short IPV6_DROP_MEMBERSHIP = IPV6_LEAVE_GROUP;

    // Polling options
    public static readonly short POLLIN = 0x001;
    public static readonly short POLLOUT = 0x002;
    public static readonly short POLLERR = 0x004;
    public static readonly short POLLNVAL = 0x008;
    // Below values are unimplemented
    public static readonly short POLLRDNORM = 0x010;
    public static readonly short POLLRDBAND = 0x020;
    public static readonly short POLLPRI = 0x040;
    public static readonly short POLLWRNORM = 0x080;
    public static readonly short POLLWRBAND = 0x100;
    public static readonly short POLLHUP = 0x200;

    public static readonly short F_GETFL = 0x0003;
    public static readonly short F_SETFL = 0x0004;

    // File status flags and file access modes for fnctl, these are bits in an int.
    public static readonly short O_NONBLOCK = 1;
    public static readonly short O_NDELAY = O_NONBLOCK;
    public static readonly short O_RDONLY = 2;
    public static readonly short O_WRONLY = 4;
    public static readonly short O_RDWR = (short)(O_RDONLY | O_WRONLY);

    public static readonly short MSG_TRUNC = 0x04;
    public static readonly short MSG_CTRUNC = 0x08;

    public static readonly short SHUT_RD = 0x0;
    public static readonly short SHUT_WR = 0x1;
    public static readonly short SHUT_RDWR = 0x2;
}
}
