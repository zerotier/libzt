/*
 * ZeroTier SDK - Network Virtualization Everywhere
 * Copyright (C) 2011-2018  ZeroTier, Inc.  https://www.zerotier.com/
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

extern int errno;


#include "Mutex.hpp"
#include "MulticastGroup.hpp"
#include "InetAddress.hpp"
#include "Thread.hpp"
#include "Phy.hpp"

#include "libztDefs.h"

#include <vector>
extern std::vector<void*> vtaps;
extern ZeroTier::Mutex _vtaps_lock;

#if defined(__MINGW32__) || defined(__MINGW64__)
#include <WinSock2.h>
#include <Windows.h>
#include <IPHlpApi.h>
#include <Ifdef.h>
#endif



using namespace ZeroTier;

class VirtualSocket;

/**
 * emulates an Ethernet tap device
 */
class VirtualTap
{
	friend class Phy<VirtualTap *>;

public:
	VirtualTap(
		const char *homePath,
		const ZeroTier::MAC &mac,
		unsigned int mtu,
		unsigned int metric,
		uint64_t nwid,
		const char *friendlyName,
		void (*handler)(void *, void *, uint64_t, const ZeroTier::MAC &,
			const ZeroTier::MAC &, unsigned int, unsigned int, const void *, unsigned int),
		void *arg);

	~VirtualTap();

	void setEnabled(bool en);
	bool enabled() const;

	/**
	 * Registers a device with the given address
	 */
	bool registerIpWithStack(const ZeroTier::InetAddress &ip);

	/**
	 * Adds an address to the userspace stack interface associated with this VirtualTap
	 * - Starts VirtualTap main thread ONLY if successful
	 */
	bool addIp(const ZeroTier::InetAddress &ip);

	/**
	 * Removes an address from the userspace stack interface associated with this VirtualTap
	 */
	bool removeIp(const ZeroTier::InetAddress &ip);

	/**
	 * Presents data to the userspace stack
	 */
	void put(const ZeroTier::MAC &from,const ZeroTier::MAC &to,unsigned int etherType,const void *data,
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
	void scanMulticastGroups(std::vector<ZeroTier::MulticastGroup> &added,
		std::vector<ZeroTier::MulticastGroup> &removed);

	/**
	 * Set MTU
	 */
	void setMtu(unsigned int mtu);

	/**
	 * Calls main network stack loops
	 */
	void threadMain()
		throw();

#if defined(__MINGW32__)
	/* The following is merely to make ZeroTier's OneService happy while building on Windows.
		we won't use these in libzt */
	NET_LUID _deviceLuid;
	std::string _deviceInstanceId;

	/**
	 * Returns whether the VirtualTap interface has been initialized
	 */
	bool isInitialized() const { return _initialized; };

	inline const NET_LUID &luid() const { return _deviceLuid; }
	inline const std::string &instanceId() const { return _deviceInstanceId; }
#endif
	/**
	 * For moving data onto the ZeroTier virtual wire
	 */
	void (*_handler)(void *, void *, uint64_t, const ZeroTier::MAC &, const ZeroTier::MAC &, unsigned int, unsigned int,
		const void *, unsigned int);

	/**
	 * Signals us to close the TcpVirtualSocket associated with this PhySocket
	 */
	void phyOnUnixClose(ZeroTier::PhySocket *sock, void **uptr);

	/**
	 * Notifies us that there is data to be read from an application's socket
	 */
	void phyOnUnixData(ZeroTier::PhySocket *sock, void **uptr, void *data, ssize_t len);

	/**
	 * Notifies us that we can write to an application's socket
	 */
	void phyOnUnixWritable(ZeroTier::PhySocket *sock, void **uptr, bool stack_invoked);

	/**
	 * Adds a route to the virtual tap
	 */
	bool routeAdd(const ZeroTier::InetAddress &ip, const ZeroTier::InetAddress &nm, const ZeroTier::InetAddress &gw);

	/**
	 * Deletes a route from the virtual tap
	 */
	bool routeDelete(const ZeroTier::InetAddress &ip, const ZeroTier::InetAddress &nm);

	/**
	 * Assign a VirtualSocket to the VirtualTap
	 */
	void addVirtualSocket(VirtualSocket *vs);

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

#if defined(STACK_PICO)
		bool should_start_stack = false;
		struct pico_device *picodev = NULL;

	/****************************************************************************/
	/* Guarded RX Frame Buffer for picoTCP                                      */
	/****************************************************************************/

		unsigned char pico_frame_rxbuf[MAX_PICO_FRAME_RX_BUF_SZ];
		int pico_frame_rxbuf_tot = 0;
		Mutex _pico_frame_rxbuf_m;
#endif

	std::vector<std::pair<ZeroTier::InetAddress, ZeroTier::InetAddress> > routes;
	void *zt1ServiceRef = NULL;

	char vtap_full_name[64];
	char vtap_abbr_name[16];

	static int devno;
	size_t ifindex = 0;

	std::vector<ZeroTier::InetAddress> ips() const;
	std::vector<ZeroTier::InetAddress> _ips;

	std::string _homePath;
	void *_arg;
	volatile bool _initialized;
	volatile bool _enabled;
	volatile bool _run;
	ZeroTier::MAC _mac;
	unsigned int _mtu;
	uint64_t _nwid;
	ZeroTier::PhySocket *_unixListenSocket;
	ZeroTier::Phy<VirtualTap *> _phy;

	std::vector<VirtualSocket*> _VirtualSockets;

	Thread _thread;
	std::string _dev; // path to Unix domain socket

	std::vector<MulticastGroup> _multicastGroups;
	Mutex _multicastGroups_m;
	Mutex _ips_m, _tcpconns_m, _rx_buf_m, _close_m;

	/*
	 * Timestamp of last run of housekeeping
	 * SEE: ZT_HOUSEKEEPING_INTERVAL in libzt.h
	 */
	uint64_t last_housekeeping_ts = 0;

	/****************************************************************************/
	/* In these, we will call the stack's corresponding functions, this is      */
	/* where one would put logic to select between different stacks             */
	/****************************************************************************/

	/**
	 * Connect to a remote host via the userspace stack interface associated with this VirtualTap
	 */
	int Connect(VirtualSocket *vs, const struct sockaddr *addr, socklen_t addrlen);

	/**
	 * Bind to the userspace stack interface associated with this VirtualTap
	 */
	int Bind(VirtualSocket *vs, const struct sockaddr *addr, socklen_t addrlen);

	/**
	 * Listen for a VirtualSocket
	 */
	int Listen(VirtualSocket *vs, int backlog);

	/**
	 * Accepts an incoming VirtualSocket
	 */
	VirtualSocket* Accept(VirtualSocket *vs);

	/**
	 * Move data from RX buffer to application's "socket"
	 */
	int Read(VirtualSocket *vs, PhySocket *sock, void **uptr, bool stack_invoked);

	/**
	 * Move data from application's "socket" into network stack
	 */
	int Write(VirtualSocket *vs, void *data, ssize_t len);

	/**
	 * Send data to specified host
	 */
	int SendTo(VirtualSocket *vs, const void *buf, size_t len, int flags, const struct sockaddr *addr, socklen_t addrlen);

	/**
	 * Closes a VirtualSocket
	 */
	int Close(VirtualSocket *vs);

	/**
	 * Shuts down some aspect of a VirtualSocket
	 */
	int Shutdown(VirtualSocket *vs, int how);

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


#endif // _H
