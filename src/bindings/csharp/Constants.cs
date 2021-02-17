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

using ZeroTier;

namespace ZeroTier
{
	public class Constants
	{
		// General error codes
		public static readonly int ERR_OK        =  0;
		public static readonly int ERR_SOCKET    = -1;
		public static readonly int ERR_SERVICE   = -2;
		public static readonly int ERR_ARG       = -3;
		public static readonly int ERR_NO_RESULT = -4;
		public static readonly int ERR_GENERAL   = -5;
	
		// Node events
		public static readonly short EVENT_NODE_UP                  = 200;
		public static readonly short EVENT_NODE_ONLINE              = 201;
		public static readonly short EVENT_NODE_OFFLINE             = 202;
		public static readonly short EVENT_NODE_DOWN                = 203;
		public static readonly short EVENT_NODE_IDENTITY_COLLISION  = 204;
		public static readonly short EVENT_NODE_UNRECOVERABLE_ERROR = 205;
		public static readonly short EVENT_NODE_NORMAL_TERMINATION  = 206;
	
		// Network events
		public static readonly short EVENT_NETWORK_NOT_FOUND        = 210;
		public static readonly short EVENT_NETWORK_CLIENT_TOO_OLD   = 211;
		public static readonly short EVENT_NETWORK_REQ_CONFIG       = 212;
		public static readonly short EVENT_NETWORK_OK               = 213;
		public static readonly short EVENT_NETWORK_ACCESS_DENIED    = 214;
		public static readonly short EVENT_NETWORK_READY_IP4        = 215;
		public static readonly short EVENT_NETWORK_READY_IP6        = 216;
		public static readonly short EVENT_NETWORK_READY_IP4_IP6    = 217;
		public static readonly short EVENT_NETWORK_DOWN             = 218;
		public static readonly short EVENT_NETWORK_UPDATE           = 219;
	
		// Network Stack events
		public static readonly short EVENT_STACK_UP                 = 220;
		public static readonly short EVENT_STACK_DOWN               = 221;
	
		// lwIP netif events
		public static readonly short EVENT_NETIF_UP                 = 230;
		public static readonly short EVENT_NETIF_DOWN               = 231;
		public static readonly short EVENT_NETIF_REMOVED            = 232;
		public static readonly short EVENT_NETIF_LINK_UP            = 233;
		public static readonly short EVENT_NETIF_LINK_DOWN          = 234;
	
		// Peer events
		public static readonly short EVENT_PEER_DIRECT              = 240;
		public static readonly short EVENT_PEER_RELAY               = 241;
		public static readonly short EVENT_PEER_UNREACHABLE         = 242;
		public static readonly short EVENT_PEER_PATH_DISCOVERED     = 243;
		public static readonly short EVENT_PEER_PATH_DEAD           = 244;
	
		// Route events
		public static readonly short EVENT_ROUTE_ADDED              = 250;
		public static readonly short EVENT_ROUTE_REMOVED            = 251;
	
		// Address events
		public static readonly short EVENT_ADDR_ADDED_IP4           = 260;
		public static readonly short EVENT_ADDR_REMOVED_IP4         = 261;
		public static readonly short EVENT_ADDR_ADDED_IP6           = 262;
		public static readonly short EVENT_ADDR_REMOVED_IP6         = 263;

		// Socket error codes
		public static readonly short EPERM            = 1;  /* Operation not permitted */
		public static readonly short ENOENT           = 2;  /* No such file or directory */
		public static readonly short ESRCH            = 3;  /* No such process */
		public static readonly short EINTR            = 4;  /* Interrupted system call */
		public static readonly short EIO              = 5;  /* I/O error */
		public static readonly short ENXIO            = 6;  /* No such device or address */
		public static readonly short E2BIG            = 7;  /* Arg list too long */
		public static readonly short ENOEXEC          = 8;  /* Exec format error */
		public static readonly short EBADF            = 9;  /* Bad file number */
		public static readonly short ECHILD          = 10;  /* No child processes */
		public static readonly short EAGAIN          = 11;  /* Try again */
		public static readonly short ENOMEM          = 12;  /* Out of memory */
		public static readonly short EACCES          = 13;  /* Permission denied */
		public static readonly short EFAULT          = 14;  /* Bad address */
		public static readonly short ENOTBLK         = 15;  /* Block device required */
		public static readonly short EBUSY           = 16;  /* Device or resource busy */
		public static readonly short EEXIST          = 17;  /* File exists */
		public static readonly short EXDEV           = 18;  /* Cross-device link */
		public static readonly short ENODEV          = 19;  /* No such device */
		public static readonly short ENOTDIR         = 20;  /* Not a directory */
		public static readonly short EISDIR          = 21;  /* Is a directory */
		public static readonly short EINVAL          = 22;  /* Invalid argument */
		public static readonly short ENFILE          = 23;  /* File table overflow */
		public static readonly short EMFILE          = 24;  /* Too many open files */
		public static readonly short ENOTTY          = 25;  /* Not a typewriter */
		public static readonly short ETXTBSY         = 26;  /* Text file busy */
		public static readonly short EFBIG           = 27;  /* File too large */
		public static readonly short ENOSPC          = 28;  /* No space left on device */
		public static readonly short ESPIPE          = 29;  /* Illegal seek */
		public static readonly short EROFS           = 30;  /* Read-only file system */
		public static readonly short EMLINK          = 31;  /* Too many links */
		public static readonly short EPIPE           = 32;  /* Broken pipe */
		public static readonly short EDOM            = 33;  /* Math argument out of domain of func */
		public static readonly short ERANGE          = 34;  /* Math result not representable */
		public static readonly short EDEADLK         = 35;  /* Resource deadlock would occur */
		public static readonly short ENAMETOOLONG    = 36;  /* File name too long */
		public static readonly short ENOLCK          = 37;  /* No record locks available */
		public static readonly short ENOSYS          = 38;  /* Function not implemented */
		public static readonly short ENOTEMPTY       = 39;  /* Directory not empty */
		public static readonly short ELOOP           = 40;  /* Too many symbolic links encountered */
		public static readonly short EWOULDBLOCK = EAGAIN;  /* Operation would block */
		public static readonly short ENOMSG          = 42;  /* No message of desired type */
		public static readonly short EIDRM           = 43;  /* Identifier removed */
		public static readonly short ECHRNG          = 44;  /* Channel number out of range */
		public static readonly short EL2NSYNC        = 45;  /* Level 2 not synchronized */
		public static readonly short EL3HLT          = 46;  /* Level 3 halted */
		public static readonly short EL3RST          = 47;  /* Level 3 reset */
		public static readonly short ELNRNG          = 48;  /* Link number out of range */
		public static readonly short EUNATCH         = 49;  /* Protocol driver not attached */
		public static readonly short ENOCSI          = 50;  /* No CSI structure available */
		public static readonly short EL2HLT          = 51;  /* Level 2 halted */
		public static readonly short EBADE           = 52;  /* Invalid exchange */
		public static readonly short EBADR           = 53;  /* Invalid request descriptor */
		public static readonly short EXFULL          = 54;  /* Exchange full */
		public static readonly short ENOANO          = 55;  /* No anode */
		public static readonly short EBADRQC         = 56;  /* Invalid request code */
		public static readonly short EBADSLT         = 57;  /* Invalid slot */

		public static readonly short EDEADLOCK  = EDEADLK;

		public static readonly short EBFONT          = 59;  /* Bad font file format */
		public static readonly short ENOSTR          = 60;  /* Device not a stream */
		public static readonly short ENODATA         = 61;  /* No data available */
		public static readonly short ETIME           = 62;  /* Timer expired */
		public static readonly short ENOSR           = 63;  /* Out of streams resources */
		public static readonly short ENONET          = 64;  /* Machine is not on the network */
		public static readonly short ENOPKG          = 65;  /* Package not installed */
		public static readonly short EREMOTE         = 66;  /* Object is remote */
		public static readonly short ENOLINK         = 67;  /* Link has been severed */
		public static readonly short EADV            = 68;  /* Advertise error */
		public static readonly short ESRMNT          = 69;  /* Srmount error */
		public static readonly short ECOMM           = 70;  /* Communication error on send */
		public static readonly short EPROTO          = 71;  /* Protocol error */
		public static readonly short EMULTIHOP       = 72;  /* Multihop attempted */
		public static readonly short EDOTDOT         = 73;  /* RFS specific error */
		public static readonly short EBADMSG         = 74;  /* Not a data message */
		public static readonly short EOVERFLOW       = 75;  /* Value too large for defined data type */
		public static readonly short ENOTUNIQ        = 76;  /* Name not unique on network */
		public static readonly short EBADFD          = 77;  /* File descriptor in bad state */
		public static readonly short EREMCHG         = 78;  /* Remote address changed */
		public static readonly short ELIBACC         = 79;  /* Can not access a needed shared library */
		public static readonly short ELIBBAD         = 80;  /* Accessing a corrupted shared library */
		public static readonly short ELIBSCN         = 81;  /* .lib section in a.out corrupted */
		public static readonly short ELIBMAX         = 82;  /* Attempting to link in too many shared libraries */
		public static readonly short ELIBEXEC        = 83;  /* Cannot exec a shared library directly */
		public static readonly short EILSEQ          = 84;  /* Illegal byte sequence */
		public static readonly short ERESTART        = 85;  /* Interrupted system call should be restarted */
		public static readonly short ESTRPIPE        = 86;  /* Streams pipe error */
		public static readonly short EUSERS          = 87;  /* Too many users */
		public static readonly short ENOTSOCK        = 88;  /* Socket operation on non-socket */
		public static readonly short EDESTADDRREQ    = 89;  /* Destination address required */
		public static readonly short EMSGSIZE        = 90;  /* Message too long */
		public static readonly short EPROTOTYPE      = 91;  /* Protocol wrong type for socket */
		public static readonly short ENOPROTOOPT     = 92;  /* Protocol not available */
		public static readonly short EPROTONOSUPPORT = 93;  /* Protocol not supported */
		public static readonly short ESOCKTNOSUPPORT = 94;  /* Socket type not supported */
		public static readonly short EOPNOTSUPP      = 95;  /* Operation not supported on transport endpoint */
		public static readonly short EPFNOSUPPORT    = 96;  /* Protocol family not supported */
		public static readonly short EAFNOSUPPORT    = 97;  /* Address family not supported by protocol */
		public static readonly short EADDRINUSE      = 98;  /* Address already in use */
		public static readonly short EADDRNOTAVAIL   = 99;  /* Cannot assign requested address */
		public static readonly short ENETDOWN       = 100;  /* Network is down */
		public static readonly short ENETUNREACH    = 101;  /* Network is unreachable */
		public static readonly short ENETRESET      = 102;  /* Network dropped connection because of reset */
		public static readonly short ECONNABORTED   = 103;  /* Software caused connection abort */
		public static readonly short ECONNRESET     = 104;  /* Connection reset by peer */
		public static readonly short ENOBUFS        = 105;  /* No buffer space available */
		public static readonly short EISCONN        = 106;  /* Transport endpoint is already connected */
		public static readonly short ENOTCONN       = 107;  /* Transport endpoint is not connected */
		public static readonly short ESHUTDOWN      = 108;  /* Cannot send after transport endpoint shutdown */
		public static readonly short ETOOMANYREFS   = 109;  /* Too many references: cannot splice */
		public static readonly short ETIMEDOUT      = 110;  /* Connection timed out */
		public static readonly short ECONNREFUSED   = 111;  /* Connection refused */
		public static readonly short EHOSTDOWN      = 112;  /* Host is down */
		public static readonly short EHOSTUNREACH   = 113;  /* No route to host */
		public static readonly short EALREADY       = 114;  /* Operation already in progress */
		public static readonly short EINPROGRESS    = 115;  /* Operation now in progress */
		public static readonly short ESTALE         = 116;  /* Stale NFS file handle */
		public static readonly short EUCLEAN        = 117;  /* Structure needs cleaning */
		public static readonly short ENOTNAM        = 118;  /* Not a XENIX named type file */
		public static readonly short ENAVAIL        = 119;  /* No XENIX semaphores available */
		public static readonly short EISNAM         = 120;  /* Is a named type file */
		public static readonly short EREMOTEIO      = 121;  /* Remote I/O error */
		public static readonly short EDQUOT         = 122;  /* Quota exceeded */
		public static readonly short ENOMEDIUM      = 123;  /* No medium found */
		public static readonly short EMEDIUMTYPE    = 124;  /* Wrong medium type */

		public static readonly short INET_ADDRSTRLEN    = 16;
		public static readonly short INET6_ADDRSTRLEN   = 46;

		/** 255.255.255.255 */
		//public static readonly uint IPADDR_NONE         =((uint32_t)0xffffffffUL);
		/** 127.0.0.1 */
		//public static readonly uint IPADDR_LOOPBACK     =((uint32_t)0x7f000001UL);
		/** 0.0.0.0 */
		//public static readonly uint IPADDR_ANY          =((uint32_t)0x00000000UL);
		/** 255.255.255.255 */
		//public static readonly uint IPADDR_BROADCAST    =((uint32_t)0xffffffffUL);

		/** 255.255.255.255 */
		//public static readonly uint INADDR_NONE         =IPADDR_NONE;
		/** 127.0.0.1 */
		//public static readonly uint INADDR_LOOPBACK     =IPADDR_LOOPBACK;
		/** 0.0.0.0 */
		//public static readonly uint INADDR_ANY          =IPADDR_ANY;
		/** 255.255.255.255 */
		//public static readonly uint INADDR_BROADCAST    =IPADDR_BROADCAST;

		// Socket protocol types
		public static readonly short SOCK_STREAM     = 0x0001;
		public static readonly short SOCK_DGRAM      = 0x0002;
		public static readonly short SOCK_RAW        = 0x0003;
		// Socket family types
		public static readonly short AF_UNSPEC       = 0x0000;
		public static readonly short AF_INET         = 0x0002;
		public static readonly short AF_INET6        = 0x000a;
		public static readonly short PF_INET         = AF_INET;
		public static readonly short PF_INET6        = AF_INET6;
		public static readonly short PF_UNSPEC       = AF_UNSPEC;
		// Protocol command types
		public static readonly short IPPROTO_IP      = 0x0000;
		public static readonly short IPPROTO_ICMP    = 0x0001;
		public static readonly short IPPROTO_TCP     = 0x0006;
		public static readonly short IPPROTO_UDP     = 0x0011;
		public static readonly short IPPROTO_IPV6    = 0x0029;
		public static readonly short IPPROTO_ICMPV6  = 0x003a;
		public static readonly short IPPROTO_UDPLITE = 0x0088;
		public static readonly short IPPROTO_RAW     = 0x00ff;
		// send() and recv() flags
		public static readonly short MSG_PEEK        = 0x0001;
		public static readonly short MSG_WAITALL     = 0x0002; // NOT YET SUPPORTED
		public static readonly short MSG_OOB         = 0x0004; // NOT YET SUPPORTED
		public static readonly short MSG_DONTWAIT    = 0x0008;
		public static readonly short MSG_MORE        = 0x0010;

		// Macros for defining ioctl() command values
		/*
		public static readonly ulong IOCPARM_MASK    = 0x7fU;
		public static readonly ulong IOC_VOID        = 0x20000000UL;
		public static readonly ulong IOC_OUT         = 0x40000000UL;
		public static readonly ulong IOC_IN          = 0x80000000UL;
		public static readonly ulong IOC_INOUT       = (IOC_IN   | IOC_OUT);
		public static readonly ulong IO(x,y)         = (IOC_VOID | ((x)<<8)|(y));
		public static readonly ulong IOR(x,y,t)      = (IOC_OUT  | (((long)sizeof(t) & IOCPARM_MASK)<<16) | ((x)<<8) | (y));
		public static readonly ulong IOW(x,y,t)      = (IOC_IN   | (((long)sizeof(t) & IOCPARM_MASK)<<16) | ((x)<<8) | (y));
		// ioctl() commands
		public static readonly ulong FIONREAD        = IOR('f', 127, unsigned long);
		public static readonly ulong FIONBIO         = IOW('f', 126, unsigned long);
		*/

		// Socket level option number
		public static readonly short SOL_SOCKET      = 0x0fff;
		// Socket options
		public static readonly short SO_DEBUG        = 0x0001; // NOT YET SUPPORTED
		public static readonly short SO_ACCEPTCONN   = 0x0002;
		public static readonly short SO_REUSEADDR    = 0x0004;
		public static readonly short SO_KEEPALIVE    = 0x0008;
		public static readonly short SO_DONTROUTE    = 0x0010; // NOT YET SUPPORTED
		public static readonly short SO_BROADCAST    = 0x0020;
		public static readonly short SO_USELOOPBACK  = 0x0040; // NOT YET SUPPORTED
		public static readonly short SO_LINGER       = 0x0080;
		public static readonly short SO_DONTLINGER   = ((short)(~SO_LINGER));
		public static readonly short SO_OOBINLINE    = 0x0100; // NOT YET SUPPORTED
		public static readonly short SO_REUSEPORT    = 0x0200; // NOT YET SUPPORTED
		public static readonly short SO_SNDBUF       = 0x1001; // NOT YET SUPPORTED
		public static readonly short SO_RCVBUF       = 0x1002;
		public static readonly short SO_SNDLOWAT     = 0x1003; // NOT YET SUPPORTED
		public static readonly short SO_RCVLOWAT     = 0x1004; // NOT YET SUPPORTED
		public static readonly short SO_SNDTIMEO     = 0x1005;
		public static readonly short SO_RCVTIMEO     = 0x1006;
		public static readonly short SO_ERROR        = 0x1007;
		public static readonly short SO_TYPE         = 0x1008;
		public static readonly short SO_CONTIMEO     = 0x1009;
		public static readonly short SO_NO_CHECK     = 0x100a;
		public static readonly short SO_BINDTODEVICE = 0x100b;
		// IPPROTO_IP options
		public static readonly short IP_TOS          = 0x0001;
		public static readonly short IP_TTL          = 0x0002;
		public static readonly short IP_PKTINFO      = 0x0008;
		// IPPROTO_TCP options
		public static readonly short TCP_NODELAY     = 0x0001;
		public static readonly short TCP_KEEPALIVE   = 0x0002;
		public static readonly short TCP_KEEPIDLE    = 0x0003;
		public static readonly short TCP_KEEPINTVL   = 0x0004;
		public static readonly short TCP_KEEPCNT     = 0x0005;
		// IPPROTO_IPV6 options
		public static readonly short IPV6_CHECKSUM   = 0x0007;  // RFC3542: calculate and insert the ICMPv6 checksum for raw sockets. 
		public static readonly short IPV6_V6ONLY     = 0x001b;  // RFC3493: boolean control to restrict AF_INET6 sockets to IPv6 communications only. 
		// UDPLITE options
		public static readonly short UDPLITE_SEND_CSCOV = 0x01; // sender checksum coverage 
		public static readonly short UDPLITE_RECV_CSCOV = 0x02; // minimal receiver checksum coverage 
		// UDPLITE options
		public static readonly short IP_MULTICAST_TTL   = 5;
		public static readonly short IP_MULTICAST_IF    = 6;
		public static readonly short IP_MULTICAST_LOOP  = 7;

		// Multicast options
		public static readonly short IP_ADD_MEMBERSHIP  = 3;
		public static readonly short IP_DROP_MEMBERSHIP = 4;

		public static readonly short IPV6_JOIN_GROUP      = 12;
		public static readonly short IPV6_ADD_MEMBERSHIP  = IPV6_JOIN_GROUP;
		public static readonly short IPV6_LEAVE_GROUP     = 13;
		public static readonly short IPV6_DROP_MEMBERSHIP = IPV6_LEAVE_GROUP;

		// Polling options
		public static readonly short POLLIN     = 0x001;
		public static readonly short POLLOUT    = 0x002;
		public static readonly short POLLERR    = 0x004;
		public static readonly short POLLNVAL   = 0x008;
		// Below values are unimplemented 
		public static readonly short POLLRDNORM = 0x010;
		public static readonly short POLLRDBAND = 0x020;
		public static readonly short POLLPRI    = 0x040;
		public static readonly short POLLWRNORM = 0x080;
		public static readonly short POLLWRBAND = 0x100;
		public static readonly short POLLHUP    = 0x200;

		public static readonly short F_GETFL    = 0x0003;
		public static readonly short F_SETFL    = 0x0004;

		// File status flags and file access modes for fnctl, these are bits in an int. 
		public static readonly short  O_NONBLOCK  = 1;
		public static readonly short  O_NDELAY    = O_NONBLOCK;
		public static readonly short  O_RDONLY    = 2;
		public static readonly short  O_WRONLY    = 4;
		public static readonly short  O_RDWR      = (short)(O_RDONLY|O_WRONLY);

		public static readonly short  MSG_TRUNC   = 0x04;
		public static readonly short  MSG_CTRUNC  = 0x08;

		public static readonly short  SHUT_RD     = 0x0;
		public static readonly short  SHUT_WR     = 0x1;
		public static readonly short  SHUT_RDWR   = 0x2;
	}
}