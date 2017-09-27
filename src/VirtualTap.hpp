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

/**
 * @file
 *
 * Virtual Ethernet tap device
 */

#ifndef ZT_VIRTUALTAP_HPP
#define ZT_VIRTUALTAP_HPP

#include <ctime>

#include "Mutex.hpp"
#include "MulticastGroup.hpp"
#include "InetAddress.hpp"
#include "Thread.hpp"
#include "Phy.hpp"

namespace ZeroTier {

	/**
	 * emulates an Ethernet tap device
	 */
	class VirtualTap
	{
		friend class Phy<VirtualTap *>;

	public:
		VirtualTap(
			const char *homePath,
			const MAC &mac,
			unsigned int mtu,
			unsigned int metric,
			uint64_t nwid,
			const char *friendlyName,
			void (*handler)(void *, void *,uint64_t,const MAC &,const MAC &,unsigned int,unsigned int,const void *,unsigned int),
			void *arg);

		~VirtualTap();

		void setEnabled(bool en);
		bool enabled() const;

		/**
		 * Registers a device with the given address
		 */
		bool registerIpWithStack(const InetAddress &ip);

		/**
		 * Adds an address to the userspace stack interface associated with this VirtualTap
		 * - Starts VirtualTap main thread ONLY if successful
		 */
		bool addIp(const InetAddress &ip);

		/**
		 * Removes an address from the userspace stack interface associated with this VirtualTap
		 */
		bool removeIp(const InetAddress &ip);

		/**
		 * Presents data to the userspace stack
		 */
		void put(const MAC &from,const MAC &to,unsigned int etherType,const void *data,
			unsigned int len);

		/**
		 * Get VirtualTap device name (e.g. 'libzt4-17d72843bc2c5760')
		 */
		std::string deviceName() const;

		/**
		 * Get Node ID (ZT address)
		 */
		std::string nodeId() const;

		/**
		 * Set friendly name
		 */
		void setFriendlyName(const char *friendlyName);

		/**
		 * Scan multicast groups
		 */
		void scanMulticastGroups(std::vector<MulticastGroup> &added,std::vector<MulticastGroup> &removed);

		/**
		 * Set MTU
		 */
		void setMtu(unsigned int mtu);

		/**
		 * Calls main network stack loops
		 */
		void threadMain()
			throw();

		/**
		 * For moving data onto the ZeroTier virtual wire
		 */
		void (*_handler)(void *, void *, uint64_t, const MAC &, const MAC &, unsigned int, unsigned int,
			const void *, unsigned int);

		/**
		 * Signals us to close the TcpVirtualSocket associated with this PhySocket
		 */
		void phyOnUnixClose(PhySocket *sock, void **uptr);

		/**
		 * Notifies us that there is data to be read from an application's socket
		 */
		void phyOnUnixData(PhySocket *sock, void **uptr, void *data, ssize_t len);

		/**
		 * Notifies us that we can write to an application's socket
		 */
		void phyOnUnixWritable(PhySocket *sock, void **uptr, bool stack_invoked);

		/**
		 * Adds a route to the virtual tap
		 */
		bool routeAdd(const InetAddress &addr, const InetAddress &nm, const InetAddress &gw);

		/**
		 * Deletes a route from the virtual tap
		 */
		bool routeDelete(const InetAddress &addr, const InetAddress &nm);

		/**
		 * Assign a VirtualSocket to the VirtualTap
		 */
		void addVirtualSocket();

		/**
		 * Remove a VirtualSocket from the VirtualTap
		 */
		void removeVirtualSocket();

		/****************************************************************************/
		/* DNS                                                                      */
		/****************************************************************************/

		/**
		 * Registers a DNS nameserver with the network stack
		 */
		int add_DNS_Nameserver(struct sockaddr *addr);

		/**
		 * Un-registers a DNS nameserver from the network stack
		 */
		int del_DNS_Nameserver(struct sockaddr *addr);

		/****************************************************************************/
		/* Vars                                                                     */
		/****************************************************************************/

		std::vector<std::pair<ZeroTier::InetAddress, ZeroTier::InetAddress>> routes;
		void *zt1ServiceRef = NULL;

		char vtap_full_name[64];
		char vtap_abbr_name[16];

		static int devno;
		int ifindex = 0;

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
		Phy<VirtualTap *> _phy;

		//std::vector<VirtualSocket*> _VirtualSockets;

		Thread _thread;
		std::string _dev; // path to Unix domain socket

		std::vector<MulticastGroup> _multicastGroups;
		Mutex _multicastGroups_m;
		Mutex _ips_m, _tcpconns_m, _rx_buf_m, _close_m;

		/*
		 * Timestamp of last run of housekeeping
		 * SEE: ZT_HOUSEKEEPING_INTERVAL in libzt.h
		 */
		std::time_t last_housekeeping_ts = 0;

		/****************************************************************************/
		/* In these, we will call the stack's corresponding functions, this is      */
		/* where one would put logic to select between different stacks             */
		/****************************************************************************/

		/**
		 * Connect to a remote host via the userspace stack interface associated with this VirtualTap
		 */
		int Connect( const struct sockaddr *addr, socklen_t addrlen);

		/**
		 * Bind to the userspace stack interface associated with this VirtualTap
		 */
		int Bind(const struct sockaddr *addr, socklen_t addrlen);

		/**
		 * Listen for a VirtualSocket
		 */
		int Listen(int backlog);

		/**
		 * Accepts an incoming VirtualSocket
		 */
		void Accept();

		/**
		 * Move data from RX buffer to application's "socket"
		 */
		int Read(PhySocket *sock,void **uptr,bool stack_invoked);

		/**
		 * Move data from application's "socket" into network stack
		 */
		int Write(void *data, ssize_t len);

		/**
		 * Send data to specified host
		 */
		int SendTo(const void *buf, size_t len, int flags, const struct sockaddr *addr, socklen_t addrlen);

		/**
		 * Closes a VirtualSocket
		 */
		int Close();

		/**
		 * Shuts down some aspect of a VirtualSocket
		 */
		int Shutdown(int how);

		/**
		 * Disposes of previously-closed VirtualSockets
		 */
		void Housekeeping();

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
