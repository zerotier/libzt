/*
 * ZeroTier SDK - Network Virtualization Everywhere
 * Copyright (C) 2011-2017  ZeroTier, Inc.  https://www.zerotier.com/
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
 * You can be released from the requirements of the license by purchasing
 * a commercial license. Buying such a license is mandatory as soon as you
 * develop commercial closed-source software that incorporates or links
 * directly against ZeroTier software without disclosing the source code
 * of your own application.
 */

#ifndef ZT_SOCKETTAP_HPP
#define ZT_SOCKETTAP_HPP

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <queue>
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
#include "picoTCP.hpp"
#include "Connection.hpp"

#include "pico_protocol.h"
#include "pico_stack.h"
#include "pico_ipv4.h"
#include "pico_icmp4.h"
#include "pico_dev_tap.h"
#include "pico_protocol.h"
#include "pico_device.h"
#include "pico_ipv6.h"

namespace ZeroTier {
	
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
		 * Registers a device with the given address
		 */
		bool registerIpWithStack(const InetAddress &ip);

		/* 
		 * Adds an address to the userspace stack interface associated with this SocketTap
		 * - Starts SocketTap main thread ONLY if successful
		 */
		bool addIp(const InetAddress &ip);

		/* 
		 * Removes an address from the userspace stack interface associated with this SocketTap
		 */
		bool removeIp(const InetAddress &ip);
		
		/* 
		 * Presents data to the userspace stack
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
		void setMtu(unsigned int mtu);

		/* 
		 * 
		 */
		void threadMain()
			throw();

		/* 
		 * For moving data onto the ZeroTier virtual wire
		 */
	  	void (*_handler)(void *,void *,uint64_t,const MAC &,const MAC &,unsigned int,unsigned int,
	  		const void *,unsigned int);

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

		/****************************************************************************/
		/* Vars                                                                     */
		/****************************************************************************/

		struct pico_device *picodev;
		struct pico_device *picodev6;

		std::vector<InetAddress> ips() const;
		std::vector<InetAddress> _ips;

		std::string _homePath;
	 	void *_arg;
		volatile bool _enabled;
		volatile bool _run;	
		MAC _mac;
	  	unsigned int _mtu;
	  	uint64_t _nwid;
	  	PhySocket *_unixListenSocket;
		Phy<SocketTap *> _phy;

		std::vector<Connection*> _Connections;

		Thread _thread;
		std::string _dev; // path to Unix domain socket

		std::vector<MulticastGroup> _multicastGroups;
		Mutex _multicastGroups_m;
		Mutex _ips_m, _tcpconns_m, _rx_buf_m, _close_m;


		/****************************************************************************/
		/* Guarded RX Frame Buffer for picoTCP                                      */
		/****************************************************************************/

        unsigned char pico_frame_rxbuf[MAX_PICO_FRAME_RX_BUF_SZ];
        int pico_frame_rxbuf_tot;
        Mutex _pico_frame_rxbuf_m;

		/****************************************************************************/
		/* In these, we will call the stack's corresponding functions, this is      */
		/* where one would put logic to select between different stacks             */
		/****************************************************************************/

		/* 
		 * Connect to a remote host via the userspace stack interface associated with this SocketTap
		 */
		int Connect(Connection *conn, int fd, const struct sockaddr *addr, socklen_t addrlen);
	
		/* 
		 * Bind to the userspace stack interface associated with this SocketTap
		 */
		int Bind(Connection *conn, int fd, const struct sockaddr *addr, socklen_t addrlen);
		
		/* 
		 * Listen for a Connection
		 */
		int Listen(Connection *conn, int fd, int backlog);
		
		/* 
		 * Accepts an incoming Connection
		 */
		Connection* Accept(Connection *conn);
		
		/* 
		 * Move data from RX buffer to application's "socket"
		 */
		void Read(PhySocket *sock,void **uptr,bool stack_invoked);

		/* 
	 	 * Move data from application's "socket" into network stack
	 	 */
		void Write(Connection *conn, void *data, ssize_t len);

		/*
		 * Closes a Connection
		 */
        void Close(Connection *conn);







		/* 
		 * Return the address that the socket is bound to 
		 */
		void handleGetsockname(PhySocket *sock, PhySocket *rpcsock, void **uptr, struct getsockname_st *getsockname_rpc);
		
		/* 
		 * Return the address of the peer connected to this socket
		 */
		void handleGetpeername(PhySocket *sock, PhySocket *rpcsock, void **uptr, struct getsockname_st *getsockname_rpc);
		
		/****************************************************************************/
		/* Not used in this implementation                                          */
		/****************************************************************************/

		void phyOnDatagram(PhySocket *sock,void **uptr,const struct sockaddr *local_address, 
			const struct sockaddr *from,void *data,unsigned long len);
		void phyOnTcpConnect(PhySocket *sock,void **uptr,bool success);
		void phyOnTcpAccept(PhySocket *sockL,PhySocket *sockN,void **uptrL,void **uptrN,
			const struct sockaddr *from);
		void phyOnTcpClose(PhySocket *sock,void **uptr);
		void phyOnTcpData(PhySocket *sock,void **uptr,void *data,unsigned long len);
		void phyOnTcpWritable(PhySocket *sock,void **uptr);
	};

} // namespace ZeroTier

#endif
