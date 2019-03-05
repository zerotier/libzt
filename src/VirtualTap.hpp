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

/**
 * @file
 *
 * Virtual Ethernet tap device
 */

#ifndef LIBZT_VIRTUALTAP_HPP
#define LIBZT_VIRTUALTAP_HPP

#ifndef _MSC_VER
extern int errno;
#endif

#include "Phy.hpp"
#include "Thread.hpp"
#include "InetAddress.hpp"
#include "MulticastGroup.hpp"
#include "Mutex.hpp"

#include "Options.h"

#if defined(_WIN32)
#include <WinSock2.h>
#include <Windows.h>
#include <IPHlpApi.h>
#include <Ifdef.h>
#endif

namespace ZeroTier {

class Mutex;

/**
 * A virtual tap device. The ZeroTier core service creates one of these for each
 * virtual network joined. It will be destroyed upon leave().
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
		void (*handler)(void *, void *, uint64_t, const MAC &,
			const MAC &, unsigned int, unsigned int, const void *, unsigned int),
		void *arg);

	~VirtualTap();

	void setEnabled(bool en);
	bool enabled() const;

	/**
	 * Mutex for protecting IP address container for this tap.
	 */
	Mutex _ips_m; // Public because we want it accessible by the driver layer

	/**
	 * Return whether this tap has been assigned an IPv4 address.
	 */
	bool hasIpv4Addr();

	/**
	 * Return whether this tap has been assigned an IPv6 address.
	 */
	bool hasIpv6Addr();

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
	 * Get VirtualTap device name (e.g. 'libzt17d72843bc2c5760')
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
	void scanMulticastGroups(std::vector<MulticastGroup> &added,
		std::vector<MulticastGroup> &removed);

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
	void (*_handler)(void *, void *, uint64_t, const MAC &, const MAC &, unsigned int, unsigned int,
		const void *, unsigned int);

	void phyOnUnixClose(PhySocket *sock, void **uptr);
	void phyOnUnixData(PhySocket *sock, void **uptr, void *data, ssize_t len);
	void phyOnUnixWritable(PhySocket *sock, void **uptr, bool stack_invoked);

	//////////////////////////////////////////////////////////////////////////////
	// Lower-level lwIP netif handling and traffic handling readiness           //
	//////////////////////////////////////////////////////////////////////////////

	void *netif = NULL;

	/**
	 * The last time that this virtual tap received a network config update from the core
	 */
	uint64_t _lastConfigUpdateTime = 0;

	/**
	 * The last time that a callback notification was sent to the user application signalling
	 * that this interface is ready to process traffic.
	 */
	uint64_t _lastReadyReportTime = 0;

	void lastConfigUpdate(uint64_t lastConfigUpdateTime);

	int _networkStatus = 0;

	//////////////////////////////////////////////////////////////////////////////
	// Vars                                                                     //
	//////////////////////////////////////////////////////////////////////////////

	std::vector<std::pair<InetAddress, InetAddress> > routes;

	char vtap_full_name[64];

	std::vector<InetAddress> ips() const;
	std::vector<InetAddress> _ips;

	std::string _homePath;
	void *_arg;
	volatile bool _initialized;
	volatile bool _enabled;
	volatile bool _run;
	MAC _mac;
	unsigned int _mtu;
	uint64_t _nwid;
	PhySocket *_unixListenSocket;
	Phy<VirtualTap *> _phy;

	Thread _thread;

	int _shutdownSignalPipe[2];

	std::string _dev; // path to Unix domain socket

	std::vector<MulticastGroup> _multicastGroups;
	Mutex _multicastGroups_m;

	/*
	 * Timestamp of last run of housekeeping
	 * SEE: ZT_HOUSEKEEPING_INTERVAL in ZeroTier.h
	 */
	uint64_t last_housekeeping_ts = 0;

	/**
	 * Performs miscellaneous background tasks
	 */
	void Housekeeping();

	//////////////////////////////////////////////////////////////////////////////
	// Not used in this implementation                                          //
	//////////////////////////////////////////////////////////////////////////////

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

#endif // _H
