/*
 * ZeroTier One - Network Virtualization Everywhere
 * Copyright (C) 2011-2015  ZeroTier, Inc.
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
 * ZeroTier may be used and distributed under the terms of the GPLv3, which
 * are available at: http://www.gnu.org/licenses/gpl-3.0.html
 *
 * If you would like to embed ZeroTier into a commercial application or
 * redistribute it in a modified binary form, please contact ZeroTier Networks
 * LLC. Start here: http://www.zerotier.com/
 */

#ifndef ZT_NETCONETHERNETTAP_HPP
#define ZT_NETCONETHERNETTAP_HPP

#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <vector>
#include <utility>
#include <stdexcept>
#include <stdint.h>

#include "Constants.hpp"
#include "MulticastGroup.hpp"
#include "Mutex.hpp"
#include "InetAddress.hpp"
#include "Thread.hpp"
#include "Phy.hpp"

#include "defs.h"
#include "rpc.h"

#if defined(SDK_LWIP)
	#include "netif/etharp.h"
 	#include "lwip.hpp"
#elif defined(SDK_PICOTCP)
    #include "picotcp.hpp"
    #include "pico_protocol.h"
#endif

// lwIP structs
struct tcp_pcb;
struct udp_pcb;

// ZT RPC structs
struct socket_st;
struct listen_st;
struct bind_st;
struct connect_st;
struct getsockname_st;
struct accept_st;

namespace ZeroTier {

	class NetconEthernetTap;
	class LWIPStack;

	extern struct pico_device picodev;
	extern NetconEthernetTap *picotap;
	
	/*
	 * TCP connection
	 */
	struct Connection
	{
	  bool listening, probation, disabled;
	  int pid, txsz, rxsz, type;
	  PhySocket *rpcSock, *sock;
	  struct tcp_pcb *TCP_pcb;
	  struct udp_pcb *UDP_pcb;
	  struct sockaddr_storage *local_addr; // Address we've bound to locally
	  struct sockaddr_storage *peer_addr; // Address of connection call to remote host
	  unsigned short port;
	  unsigned char txbuf[DEFAULT_TCP_TX_BUF_SZ];
	  unsigned char rxbuf[DEFAULT_TCP_RX_BUF_SZ];
	  // TODO: necessary still?
	  int proxy_conn_state;

	  // pico
	  struct pico_socket *picosock;
	};

	/*
	 * A helper for passing a reference to _phy to LWIP callbacks as a "state"
	 */
	struct Larg
	{
	  NetconEthernetTap *tap;
	  Connection *conn;
	  Larg(NetconEthernetTap *_tap, Connection *conn) : tap(_tap), conn(conn) {}
	};

	/*
	 * Network Containers instance -- emulates an Ethernet tap device as far as OneService knows
	 */
	class NetconEthernetTap
	{
		friend class Phy<NetconEthernetTap *>;

	public:
		NetconEthernetTap(
			const char *homePath,
			const MAC &mac,
			unsigned int mtu,
			unsigned int metric,
			uint64_t nwid,
			const char *friendlyName,
			void (*handler)(void *,uint64_t,const MAC &,const MAC &,unsigned int,unsigned int,const void *,unsigned int),
			void *arg);

		~NetconEthernetTap();

		void setEnabled(bool en);
		bool enabled() const;
		bool addIp(const InetAddress &ip);
		bool removeIp(const InetAddress &ip);
		std::vector<InetAddress> ips() const;
		std::vector<InetAddress> _ips;

		void put(const MAC &from,const MAC &to,unsigned int etherType,const void *data,unsigned int len);
		std::string deviceName() const;
		void setFriendlyName(const char *friendlyName);
		void scanMulticastGroups(std::vector<MulticastGroup> &added,std::vector<MulticastGroup> &removed);

		int sendReturnValue(int fd, int retval, int _errno);	
		void unloadRPC(void *data, pid_t &pid, pid_t &tid, char (timestamp[RPC_TIMESTAMP_SZ]), char (CANARY[sizeof(uint64_t)]), char &cmd, void* &payload);

		void threadMain()
			throw();

		std::string _homePath;
		MAC _mac;
	  	unsigned int _mtu;
	  	uint64_t _nwid;
	  	void (*_handler)(void *,uint64_t,const MAC &,const MAC &,unsigned int,unsigned int,const void *,unsigned int);
	 	void *_arg;
		Phy<NetconEthernetTap *> _phy;
		PhySocket *_unixListenSocket;
		volatile bool _enabled;
		volatile bool _run;	

	  	// --- Proxy
		struct sockaddr_storage proxyServerAddress; 
		int sockstate; // Use as flag to determine whether proxy has been started, TODO: Rename
		int proxyListenSocket;
		PhySocket *proxyListenPhySocket;
		int startProxyServer(const char *homepath, uint64_t nwid, struct sockaddr_storage *addr);
		int stopProxyServer();
		int getProxyServerAddress(struct sockaddr_storage *addr);
		int getProxyServerPort();
		void phyOnFileDescriptorActivity(PhySocket *sock,void **uptr,bool readable,bool writable);
		// --- end Proxy 


		// lwIP
		#if defined(SDK_LWIP)
			netif interface, interface6;
			lwIP_stack *lwipstack;
		#endif
		// jip
		#if defined(SDK_JIP)
			jip_stack *jipstack;
		#endif
		// picoTCP
        #if defined(SDK_PICOTCP)
            unsigned char pico_frame_rxbuf[MAX_PICO_FRAME_RX_BUF_SZ];
            int pico_frame_rxbuf_tot;
            Mutex _pico_frame_rxbuf_m;
            picoTCP_stack *picostack;
        #endif

		/*
		 * Handles an RPC to bind an LWIP PCB to a given address and port
		 *
		 * @param PhySocket associated with this RPC connection
		 * @param structure containing the data and parameters for this client's RPC
		 *

		 i := should be implemented in intercept lib
		 I := is implemented in intercept lib
		 X := is implemented in service
		 ? := required treatment Unknown
		 - := Not needed

		[ ]	EACCES - The address is protected, and the user is not the superuser.
		[X]	EADDRINUSE - The given address is already in use.
		[I]	EBADF - sockfd is not a valid descriptor.
		[X]	EINVAL - The socket is already bound to an address.
		[I]	ENOTSOCK - sockfd is a descriptor for a file, not a socket.

		[X]	ENOMEM - Insufficient kernel memory was available.

		  - The following errors are specific to UNIX domain (AF_UNIX) sockets:

		[-]	EACCES - Search permission is denied on a component of the path prefix. (See also path_resolution(7).)
		[-]	EADDRNOTAVAIL - A nonexistent interface was requested or the requested address was not local.
		[-]	EFAULT - addr points outside the user's accessible address space.
		[-]	EINVAL - The addrlen is wrong, or the socket was not in the AF_UNIX family.
		[-]	ELOOP - Too many symbolic links were encountered in resolving addr.
		[-]	ENAMETOOLONG - s addr is too long.
		[-]	ENOENT - The file does not exist.
		[-]	ENOTDIR - A component of the path prefix is not a directory.
		[-]	EROFS - The socket inode would reside on a read-only file system.
		 */
		void handleBind(PhySocket *sock, PhySocket *rpcsock, void **uptr, struct bind_st *bind_rpc);
		
		/*
		 * Handles an RPC to put an LWIP PCB into LISTEN mode
		 *
		 * @param PhySocket associated with this RPC connection
		 * @param structure containing the data and parameters for this client's RPC
		 *

		 i := should be implemented in intercept lib
		 I := is implemented in intercept lib
		 X := is implemented in service
		 ? := required treatment Unknown
		 - := Not needed

		[?] EADDRINUSE - Another socket is already listening on the same port.
		[IX] EBADF - The argument sockfd is not a valid descriptor.
		[I] ENOTSOCK - The argument sockfd is not a socket.
		[I] EOPNOTSUPP - The socket is not of a type that supports the listen() operation.
		 */
		void handleListen(PhySocket *sock, PhySocket *rpcsock, void **uptr, struct listen_st *listen_rpc);
		
		/*
		 * Handles an RPC to create a socket (LWIP PCB and associated socketpair)
		 *
		 * A socketpair is created, one end is kept and wrapped into a PhySocket object
		 * for use in the main ZT I/O loop, and one end is sent to the client. The client
		 * is then required to tell the service what new file descriptor it has allocated
		 * for this connection. After the mapping is complete, the socket can be used.
		 *
		 * @param PhySocket associated with this RPC connection
		 * @param structure containing the data and parameters for this client's RPC
		 *

		 i := should be implemented in intercept lib
		 I := is implemented in intercept lib
		 X := is implemented in service
		 ? := required treatment Unknown
		 - := Not needed

		[-] EACCES - Permission to create a socket of the specified type and/or protocol is denied.
		[I] EAFNOSUPPORT - The implementation does not support the specified address family.
		[I] EINVAL - Unknown protocol, or protocol family not available.
		[I] EINVAL - Invalid flags in type.
		[I] EMFILE - Process file table overflow.
		[?] ENFILE - The system limit on the total number of open files has been reached.
		[X] ENOBUFS or ENOMEM - Insufficient memory is available.  The socket cannot be created until sufficient resources are freed.
		[?] EPROTONOSUPPORT - The protocol type or the specified protocol is not supported within this domain.
		 */
		Connection * handleSocket(PhySocket *sock, void **uptr, struct socket_st* socket_rpc);
		Connection * handleSocketProxy(PhySocket *sock, int socket_type);

		/*
		 * Handles an RPC to connect to a given address and port
		 *
		 * @param PhySocket associated with this RPC connection
		 * @param structure containing the data and parameters for this client's RPC

		--- Error handling in this method will only catch problems which are immedately
		    apprent. Some errors will need to be caught in the nc_connected(0 callback

		 i := should be implemented in intercept lib
	 	 I := is implemented in intercept lib
	 	 X := is implemented in service
	 	 ? := required treatment Unknown
	 	 - := Not needed

		[-] EACCES - For UNIX domain sockets, which are identified by pathname: Write permission is denied ...
		[?] EACCES, EPERM - The user tried to connect to a broadcast address without having the socket broadcast flag enabled ...
		[X] EADDRINUSE - Local address is already in use.
		[I] EAFNOSUPPORT - The passed address didn't have the correct address family in its sa_family field.
		[X] EAGAIN - No more free local ports or insufficient entries in the routing cache.
		[ ] EALREADY - The socket is nonblocking and a previous connection attempt has not yet been completed.
		[IX] EBADF - The file descriptor is not a valid index in the descriptor table.
		[ ] ECONNREFUSED - No-one listening on the remote address.
		[i] EFAULT - The socket structure address is outside the user's address space.
		[ ] EINPROGRESS - The socket is nonblocking and the connection cannot be completed immediately.
		[-] EINTR - The system call was interrupted by a signal that was caught.
		[X] EISCONN - The socket is already connected.
		[X] ENETUNREACH - Network is unreachable.
		[I] ENOTSOCK - The file descriptor is not associated with a socket.
		[X] ETIMEDOUT - Timeout while attempting connection.

		[X] EINVAL - Invalid argument, SVr4, generally makes sense to set this
		 */
		void handleConnect(PhySocket *sock, PhySocket *rpcsock, Connection *conn, struct connect_st* connect_rpc);
		int handleConnectProxy(PhySocket *sock, struct sockaddr_in *rawAddr);

		// void handleIsConnected();

		/* 
		 * Return the address that the socket is bound to 
		 */
		void handleGetsockname(PhySocket *sock, PhySocket *rpcsock, void **uptr, struct getsockname_st *getsockname_rpc);
		
		/* 
		 * Return the address of the peer connected to this socket
		 */
		void handleGetpeername(PhySocket *sock, PhySocket *rpcsock, void **uptr, struct getsockname_st *getsockname_rpc);
		
		/* 
	 	 * Writes data from the application's socket to the LWIP connection
	 	 */
		void handleWrite(Connection *conn);

		// Unused -- no UDP or TCP from this thread/Phy<>
		void phyOnDatagram(PhySocket *sock,void **uptr,const struct sockaddr *local_address, const struct sockaddr *from,void *data,unsigned long len);
		void phyOnTcpConnect(PhySocket *sock,void **uptr,bool success);
		void phyOnTcpAccept(PhySocket *sockL,PhySocket *sockN,void **uptrL,void **uptrN,const struct sockaddr *from);
		void phyOnTcpClose(PhySocket *sock,void **uptr);
		void phyOnTcpData(PhySocket *sock,void **uptr,void *data,unsigned long len);

		void handleRead(PhySocket *sock,void **uptr,bool lwip_invoked);
		void phyOnTcpWritable(PhySocket *sock,void **uptr, bool lwip_invoked);

		/*
	 	 * Signals us to close the TcpConnection associated with this PhySocket
	 	 */
		void phyOnUnixClose(PhySocket *sock,void **uptr);
		
	    /* 
	 	 * Notifies us that there is data to be read from an application's socket
	 	 */
		void phyOnUnixData(PhySocket *sock,void **uptr,void *data,ssize_t len);
		
		/* 
	 	 * Notifies us that we can write to an application's socket
	 	 */
		void phyOnUnixWritable(PhySocket *sock,void **uptr,bool lwip_invoked);

		/*
	 	 * Returns a pointer to a TcpConnection associated with a given PhySocket
	 	 */
		Connection *getConnection(PhySocket *sock);

		/*
	 	 * Returns a pointer to a TcpConnection associated with a given pico_socket
	 	 */
		Connection *getConnection(struct pico_socket *socket);

		/*
	 	 * Closes a TcpConnection, associated LWIP PCB strcuture, 
	 	 * PhySocket, and underlying file descriptor
	 	 */
		void closeConnection(PhySocket *sock);

		std::vector<Connection*> _Connections;

		std::map<uint64_t, std::pair<PhySocket*, void*> > jobmap;
		pid_t rpcCounter;

		Thread _thread;
		std::string _dev; // path to Unix domain socket

		std::vector<MulticastGroup> _multicastGroups;
		Mutex _multicastGroups_m;

		Mutex _ips_m, _tcpconns_m, _rx_buf_m, _close_m;
	};

} // namespace ZeroTier

#endif
