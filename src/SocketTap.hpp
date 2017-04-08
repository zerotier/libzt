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

#ifndef ZT_SocketTap_HPP
#define ZT_SocketTap_HPP

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

#include "ZeroTierSDK.h"
#include "RPC.h"
#include "picoTCP.hpp"

#include "pico_protocol.h"
#include "pico_stack.h"
#include "pico_ipv4.h"
#include "pico_icmp4.h"
#include "pico_dev_tap.h"
#include "pico_protocol.h"
#include "pico_socket.h"
#include "pico_device.h"
#include "pico_ipv6.h"

// ZT RPC structs
struct socket_st;
struct listen_st;
struct bind_st;
struct connect_st;
struct getsockname_st;
struct accept_st;

struct pico_socket;

namespace ZeroTier {

	class SocketTap;

	extern SocketTap *picotap;
	
	/*
	 * Connection object
	 */
	struct Connection
	{
	  bool listening, probation, disabled;
	  int pid, txsz, rxsz;
	  PhySocket *rpcSock, *sock;
	  struct tcp_pcb *TCP_pcb;
	  struct udp_pcb *UDP_pcb;
	  struct sockaddr_storage *local_addr; // Address we've bound to locally
	  struct sockaddr_storage *peer_addr; // Address of connection call to remote host
	  unsigned short port;
	  unsigned char txbuf[DEFAULT_TCP_TX_BUF_SZ];
	  unsigned char rxbuf[DEFAULT_TCP_RX_BUF_SZ];
	  struct pico_socket *picosock;

	  int data_sock;
	  int socket_family, socket_type;
	};

	/*
	 * A helper for passing a reference to _phy to stackrpc callbacks as a "state"
	 */
	struct Larg
	{
	  SocketTap *tap;
	  Connection *conn;
	  Larg(SocketTap *_tap, Connection *conn) : tap(_tap), conn(conn) {}
	};

	/*
	 * Socket Tap -- emulates an Ethernet tap device
	 */
	class SocketTap
	{
		friend class Phy<SocketTap *>;

	public:
		SocketTap(
			const char *homePath,
			const MAC &mac,
			unsigned int mtu,
			unsigned int metric,
			uint64_t nwid,
			const char *friendlyName,
			void (*handler)(void *, void *,uint64_t,const MAC &,const MAC &,unsigned int,unsigned int,const void *,unsigned int),
			void *arg);

		~SocketTap();

		void setEnabled(bool en);
		bool enabled() const;

		/* 
		 * 
		 */
		bool addIp(const InetAddress &ip);

		/* 
		 * 
		 */
		bool removeIp(const InetAddress &ip);
		std::vector<InetAddress> ips() const;
		std::vector<InetAddress> _ips;
		
		/* 
		 * 
		 */
		void put(const MAC &from,const MAC &to,unsigned int etherType,const void *data,
			unsigned int len);

		/* 
		 * 
		 */
		std::string deviceName() const;
		
		/* 
		 * 
		 */
		void setFriendlyName(const char *friendlyName);
		
		/* 
		 * 
		 */
		void scanMulticastGroups(std::vector<MulticastGroup> &added,std::vector<MulticastGroup> &removed);
		
		/* 
		 * 
		 */
		int sendReturnValue(int fd, int retval, int _errno);	
		
		/* 
		 * 
		 */
		void unloadRPC(void *data, pid_t &pid, pid_t &tid, char (timestamp[RPC_TIMESTAMP_SZ]), 
			char (CANARY[sizeof(uint64_t)]), char &cmd, void* &payload);
		
		/* 
		 * 
		 */
		void threadMain()
			throw();

		std::string _homePath;
		MAC _mac;
	  	unsigned int _mtu;
	  	uint64_t _nwid;
		
		/* 
		 * 
		 */
	  	void (*_handler)(void *,void *,uint64_t,const MAC &,const MAC &,unsigned int,unsigned int,
	  		const void *,unsigned int);

	 	void *_arg;
		Phy<SocketTap *> _phy;
		PhySocket *_unixListenSocket;
		volatile bool _enabled;
		volatile bool _run;	

		// picoTCP
        unsigned char pico_frame_rxbuf[MAX_PICO_FRAME_RX_BUF_SZ];
        int pico_frame_rxbuf_tot;
        Mutex _pico_frame_rxbuf_m;

		void handleBind(PhySocket *sock, PhySocket *rpcsock, void **uptr, struct bind_st *bind_rpc);
		void handleListen(PhySocket *sock, PhySocket *rpcsock, void **uptr, 
			struct listen_st *listen_rpc);
		Connection * handleSocket(PhySocket *sock, void **uptr, struct socket_st* socket_rpc);
		void handleConnect(PhySocket *sock, PhySocket *rpcsock, Connection *conn, 
			struct connect_st* connect_rpc);

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
		void phyOnDatagram(PhySocket *sock,void **uptr,const struct sockaddr *local_address, 
			const struct sockaddr *from,void *data,unsigned long len);
		void phyOnTcpConnect(PhySocket *sock,void **uptr,bool success);
		void phyOnTcpAccept(PhySocket *sockL,PhySocket *sockN,void **uptrL,void **uptrN,
			const struct sockaddr *from);
		void phyOnTcpClose(PhySocket *sock,void **uptr);
		void phyOnTcpData(PhySocket *sock,void **uptr,void *data,unsigned long len);
		void phyOnTcpWritable(PhySocket *sock,void **uptr, bool stack_invoked);

		/* 
		 * 
		 */
		void handleRead(PhySocket *sock,void **uptr,bool stack_invoked);

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
	 	 * Closes a TcpConnection, associated connection strcuture, 
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
