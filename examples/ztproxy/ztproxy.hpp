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

#ifndef ZT_ZTPROXY_HPP
#define ZT_ZTPROXY_HPP

#include "Constants.hpp"
#include "Thread.hpp"
#include "InetAddress.hpp"
#include "Mutex.hpp"
#include "Phy.hpp"
#include "OSUtils.hpp"

#include <queue>
#include <vector>
#include <stdio.h>
#include <sys/select.h>

#define BUF_SZ 1024*1024

namespace ZeroTier {

	typedef void PhySocket;
	class ZTProxy;

	class TcpConnection
	{
	public:
		int zfd;
		PhySocket *client_sock;
		RingBuffer<unsigned char> *TXbuf;
		RingBuffer<unsigned char> *RXbuf;
		Mutex tx_m, rx_m;

		TcpConnection() {
			printf("TcpConnection()\n");
			zfd = -1;			
			TXbuf = new RingBuffer<unsigned char>(BUF_SZ);
			RXbuf = new RingBuffer<unsigned char>(BUF_SZ);
		}

		~TcpConnection() {
			printf("~TcpConnection()\n");
			delete TXbuf;
			delete RXbuf;
			TXbuf = NULL;
			RXbuf = NULL;
		}
	};

	class ZTProxy
	{
		friend class Phy<ZTProxy *>;

	public:
		ZTProxy(int proxy_listen_port, std::string nwid, std::string path, std::string internal_addr, int internal_port);
		~ZTProxy();

		// Send incoming data to intended host
		void phyOnTcpData(PhySocket *sock,void **uptr,void *data,unsigned long len);
		void phyOnDatagram(PhySocket *sock,void **uptr,const struct sockaddr *localAddr,const struct sockaddr *from,void *data,unsigned long len);
		void phyOnTcpWritable(PhySocket *sock,void **uptr);
		void phyOnFileDescriptorActivity(PhySocket *sock,void **uptr,bool readable,bool writable);
		// Establish outgoing connection to intended host
		void phyOnTcpConnect(PhySocket *sock,void **uptr,bool success);
		// Accept connection 
		void phyOnTcpAccept(PhySocket *sockL,PhySocket *sockN,void **uptrL,void **uptrN,const struct sockaddr *from);
		// Handle the closure of a Unix Domain socket
		void phyOnUnixClose(PhySocket *sock,void **uptr);
		void phyOnUnixData(PhySocket *sock,void **uptr,void *data,ssize_t len);
		void phyOnUnixWritable(PhySocket *sock,void **uptr,bool lwip_invoked);
		// Handle the closure of a TCP connection
		void phyOnTcpClose(PhySocket *sock,void **uptr);

		void threadMain()
			throw();

	TcpConnection *getConnection(PhySocket *sock);

	private:	
		volatile bool _enabled;
		volatile bool _run;	

		Mutex conn_m;
		fd_set read_set, write_set;
		int nfds = 0;

		int _proxy_listen_port;
		int _internal_port;
		std::string _nwid;
		std::string _internal_addr;

		Thread _thread;
		Phy<ZTProxy*> _phy;
		PhySocket *_tcpListenSocket;
		PhySocket *_tcpListenSocket6;

		// mapping from ZeroTier VirtualSocket fd to TcpConnection pointer
		std::map<int, TcpConnection*> zmap;
		// mapping from ZeroTier PhySocket to TcpConnection pointer
		std::map<PhySocket*, TcpConnection*> cmap;

		std::vector<TcpConnection*> clist;
	};
}

#endif