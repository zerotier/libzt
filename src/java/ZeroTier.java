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

package com.zerotier.libzt;

import java.net.*;

public class ZeroTier
{
	//////////////////////////////////////////////////////////////////////////////
	// Control API error codes                                                  //
	//////////////////////////////////////////////////////////////////////////////

	// Everything is ok
	public static int ZTS_ERR_OK                       =  0;
	// A argument provided by the user application is invalid (e.g. out of range, NULL, etc)
	public static int ZTS_ERR_INVALID_ARG              = -1;
	// The service isn't initialized or is for some reason currently unavailable. Try again.
	public static int ZTS_ERR_SERVICE                  = -2;
	// For some reason this API operation is not permitted or doesn't make sense at this time.
	public static int ZTS_ERR_INVALID_OP               = -3;
	// The call succeeded, but no object or relevant result was available
	public static int ZTS_ERR_NO_RESULT                = -4;
	// General internal failure
	public static int ZTS_ERR_GENERAL                  = -5;

	//////////////////////////////////////////////////////////////////////////////
	// Static initialization                                                    //
	//////////////////////////////////////////////////////////////////////////////

	static
	{
		// loads libzt.so or libzt.dylib
		System.loadLibrary("zt");
		// Give the native code a reference to this VM (for callbacks)
		if (init() != ZTS_ERR_OK) {
			throw new ExceptionInInitializerError("JNI init() failed (see GetJavaVM())");
		}
	}

	//////////////////////////////////////////////////////////////////////////////
	// Control API event codes                                                  //
	//////////////////////////////////////////////////////////////////////////////

	public static int  EVENT_NONE                      = -1;
	// Node-specific events
	public static int  EVENT_NODE_UP                   = 0;
	public static int  EVENT_NODE_OFFLINE              = 1;
	public static int  EVENT_NODE_ONLINE               = 2;
	public static int  EVENT_NODE_DOWN                 = 3;
	public static int  EVENT_NODE_IDENTITY_COLLISION   = 4;
	// libzt node events
	public static int  EVENT_NODE_UNRECOVERABLE_ERROR  = 16;
	public static int  EVENT_NODE_NORMAL_TERMINATION   = 17;
	// Network-specific events
	public static int  EVENT_NETWORK_NOT_FOUND         = 32;
	public static int  EVENT_NETWORK_CLIENT_TOO_OLD    = 33;
	public static int  EVENT_NETWORK_REQUESTING_CONFIG = 34;
	public static int  EVENT_NETWORK_OK                = 35;
	public static int  EVENT_NETWORK_ACCESS_DENIED     = 36;
	public static int  EVENT_NETWORK_READY_IP4         = 37;
	public static int  EVENT_NETWORK_READY_IP6         = 38;
	public static int  EVENT_NETWORK_READY_IP4_IP6     = 39;
	public static int  EVENT_NETWORK_DOWN              = 40;
	// lwIP netif events
	public static int  EVENT_NETIF_UP_IP4              = 64;
	public static int  EVENT_NETIF_UP_IP6              = 65;
	public static int  EVENT_NETIF_DOWN_IP4            = 66;
	public static int  EVENT_NETIF_DOWN_IP6            = 67;
	public static int  EVENT_NETIF_REMOVED             = 68;
	public static int  EVENT_NETIF_LINK_UP             = 69;
	public static int  EVENT_NETIF_LINK_DOWN           = 70;
	public static int  EVENT_NETIF_NEW_ADDRESS         = 71;
	// Peer events
	public static int  EVENT_PEER_P2P                  = 96;
	public static int  EVENT_PEER_RELAY                = 97;
	public static int  EVENT_PEER_UNREACHABLE          = 98;

	//////////////////////////////////////////////////////////////////////////////
	// ZeroTier Constants                                                       //
	//////////////////////////////////////////////////////////////////////////////

	public static int  ZT_MAX_PEER_NETWORK_PATHS       = 16;

	//////////////////////////////////////////////////////////////////////////////
	// Socket API Constants                                                     //
	//////////////////////////////////////////////////////////////////////////////

	// Socket protocol types
	public static int SOCK_STREAM     = 0x00000001;
	public static int SOCK_DGRAM      = 0x00000002;
	public static int SOCK_RAW        = 0x00000003;
	// Socket family types
	public static int AF_INET         = 0x00000002;
	public static int AF_INET6        = 0x0000000a;
	public static int PF_INET         = AF_INET;
	public static int PF_INET6        = AF_INET6;
	// Used as level numbers for setsockopt() and getsockopt()
	public static int IPPROTO_IP      = 0x00000000;
	public static int IPPROTO_ICMP    = 0x00000001;
	public static int IPPROTO_TCP     = 0x00000006;
	public static int IPPROTO_UDP     = 0x00000011;
	public static int IPPROTO_IPV6    = 0x00000029;
	public static int IPPROTO_ICMPV6  = 0x0000003a;
	public static int IPPROTO_UDPLITE = 0x00000088;
	public static int IPPROTO_RAW     = 0x000000ff;
	// send() and recv() flags
	public static int MSG_PEEK        = 0x00000001;
	public static int MSG_WAITALL     = 0x00000002;
	public static int MSG_OOB         = 0x00000004;
	public static int MSG_DONTWAIT    = 0x00000008;
	public static int MSG_MORE        = 0x00000010;
	// fnctl() commands
	public static int F_GETFL         = 0x00000003;
	public static int F_SETFL         = 0x00000004;
	// fnctl() flags
	public static int O_NONBLOCK      = 0x00000001;
	public static int O_NDELAY        = 0x00000001;
	// Shutdown commands
	public static int SHUT_RD         = 0x00000000;
	public static int SHUT_WR         = 0x00000001;
	public static int SHUT_RDWR       = 0x00000002;
	// ioctl() commands
	public static int FIONREAD        = 0x4008667F;
	public static int FIONBIO         = 0x8008667E;
	// Socket level option number
	public static int SOL_SOCKET      = 0x00000FFF;
	// Socket options
	public static int SO_REUSEADDR    = 0x00000004;
	public static int SO_KEEPALIVE    = 0x00000008;
	public static int SO_BROADCAST    = 0x00000020;
	// Socket options
	public static int SO_DEBUG        = 0x00000001; // NOT YET SUPPORTED
	public static int SO_ACCEPTCONN   = 0x00000002;
	public static int SO_DONTROUTE    = 0x00000010; // NOT YET SUPPORTED
	public static int SO_USELOOPBACK  = 0x00000040; // NOT YET SUPPORTED
	public static int SO_LINGER       = 0x00000080;
	public static int SO_DONTLINGER   = ((int)(~SO_LINGER));
	public static int SO_OOBINLINE    = 0x00000100; // NOT YET SUPPORTED
	public static int SO_REUSEPORT    = 0x00000200; // NOT YET SUPPORTED
	public static int SO_SNDBUF       = 0x00001001; // NOT YET SUPPORTED
	public static int SO_RCVBUF       = 0x00001002;
	public static int SO_SNDLOWAT     = 0x00001003; // NOT YET SUPPORTED
	public static int SO_RCVLOWAT     = 0x00001004; // NOT YET SUPPORTED
	public static int SO_SNDTIMEO     = 0x00001005;
	public static int SO_RCVTIMEO     = 0x00001006;
	public static int SO_ERROR        = 0x00001007;
	public static int SO_TYPE         = 0x00001008;
	public static int SO_CONTIMEO     = 0x00001009; // NOT YET SUPPORTED
	public static int SO_NO_CHECK     = 0x0000100a;
	// IPPROTO_IP options
	public static int IP_TOS          = 0x00000001;
	public static int IP_TTL          = 0x00000002;
	// IPPROTO_TCP options
	public static int TCP_NODELAY     = 0x00000001;
	public static int TCP_KEEPALIVE   = 0x00000002;
	public static int TCP_KEEPIDLE    = 0x00000003;
	public static int TCP_KEEPINTVL   = 0x00000004;
	public static int TCP_KEEPCNT     = 0x00000005;

	//////////////////////////////////////////////////////////////////////////////
	// Statistics                                                               //
	//////////////////////////////////////////////////////////////////////////////

	public static int STATS_PROTOCOL_LINK      = 0;
	public static int STATS_PROTOCOL_ETHARP    = 1;
	public static int STATS_PROTOCOL_IP        = 2;
	public static int STATS_PROTOCOL_UDP       = 3;
	public static int STATS_PROTOCOL_TCP       = 4;
	public static int STATS_PROTOCOL_ICMP      = 5;
	public static int STATS_PROTOCOL_IP_FRAG   = 6;
	public static int STATS_PROTOCOL_IP6       = 7;
	public static int STATS_PROTOCOL_ICMP6     = 8;
	public static int STATS_PROTOCOL_IP6_FRAG  = 9;

	public static native int get_protocol_stats(int protocolNum, ZeroTierProtoStats stats);

	//////////////////////////////////////////////////////////////////////////////
	// ZeroTier Service Controls                                                //
	//////////////////////////////////////////////////////////////////////////////

	public static native int start(String path, ZeroTierEventListener callbackClass, int port);
	public static native int stop();
	public static native int restart();
	public static native int join(long nwid);
	public static native int leave(long nwid);
	public static native long get_node_id();
	public static native int get_num_assigned_addresses(long nwid);
	public static native void get_6plane_addr(long nwid, long nodeId, ZeroTierSocketAddress addr);
	public static native void get_rfc4193_addr(long nwid, long nodeId, ZeroTierSocketAddress addr);
	public static native int get_node_status();
	public static native int get_network_status(long networkId);
	public static native int get_peer_status(long peerId);
	public static native int get_peer(long peerId, ZeroTierPeerDetails details);

	//////////////////////////////////////////////////////////////////////////////
	// Socket API                                                               //
	//////////////////////////////////////////////////////////////////////////////

	public static native int socket(int family, int type, int protocol);
	public static native int connect(int fd, ZeroTierSocketAddress addr);
	public static native int bind(int fd, ZeroTierSocketAddress addr);
	public static native int listen(int fd, int backlog);
	public static native int accept(int fd, ZeroTierSocketAddress addr);
	public static native int accept4(int fd, String addr, int port);

	public static native int setsockopt(int fd, int level, int optname, ZeroTierSocketOptionValue optval);
	public static native int getsockopt(int fd, int level, int optname, ZeroTierSocketOptionValue optval);

	public static native int read(int fd, byte[] buf);
	public static native int read_offset(int fd, byte[] buf, int offset, int len);
	public static native int read_length(int fd, byte[] buf, int len);
	public static native int recv(int fd, byte[] buf, int flags);
	public static native int recvfrom(int fd, byte[] buf, int flags, ZeroTierSocketAddress addr);

	public static native int write(int fd, byte[] buf);
	public static native int write_byte(int fd, byte b);
	public static native int write_offset(int fd, byte[] buf, int offset, int len);
	public static native int sendto(int fd, byte[] buf, int flags, ZeroTierSocketAddress addr);
	public static native int send(int fd, byte[] buf, int flags);

	public static native int shutdown(int fd, int how);
	public static native int close(int fd);

	public static native boolean getsockname(int fd, ZeroTierSocketAddress addr);
	public static native int getpeername(int fd, ZeroTierSocketAddress addr);
	public static native int fcntl(int sock, int cmd, int flag);
	public static native int ioctl(int fd, long request, ZeroTierIoctlArg arg);
	public static native int select(int nfds, ZeroTierFileDescriptorSet readfds, ZeroTierFileDescriptorSet writefds, ZeroTierFileDescriptorSet exceptfds, int timeout_sec, int timeout_usec);

	//////////////////////////////////////////////////////////////////////////////
	// Internal - Do not call                                                   //
	//////////////////////////////////////////////////////////////////////////////

	public static native int init(); // Only to be called by static initializer of this class
}
