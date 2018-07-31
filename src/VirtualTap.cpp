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

// Used by raw stack driver implementation
struct ip_addr_t;
typedef unsigned short u16_t;

#include "Phy.hpp"

#include "VirtualTap.h"
#include "libzt.h"
#include "libztDebug.h"
#include "lwIP.h"
#include "ZT1Service.h"
#include "SysUtils.h"

#include "Mutex.hpp"
#include "InetAddress.hpp"
#include "OneService.hpp"

#ifdef _MSC_VER
#include "Synchapi.h"
#endif

int VirtualTap::devno = 0;

VirtualTap::VirtualTap(
	const char *homePath,
	const MAC &mac,
	unsigned int mtu,
	unsigned int metric,
	uint64_t nwid,
	const char *friendlyName,
	void (*handler)(void *,void*,uint64_t,const MAC &,const MAC &,
		unsigned int,unsigned int,const void *,unsigned int),
	void *arg) :
		_handler(handler),
		_homePath(homePath),
		_arg(arg),
		_initialized(false),
		_enabled(true),
		_run(true),
		_mac(mac),
		_mtu(mtu),
		_nwid(nwid),
		_unixListenSocket((PhySocket *)0),
		_phy(this,false,true)
{
	vtaps.push_back((void*)this);

	// set virtual tap interface name (full)
	memset(vtap_full_name, 0, sizeof(vtap_full_name));
	ifindex = devno;
	snprintf(vtap_full_name, sizeof(vtap_full_name), "libzt%d-%llx", devno++, (unsigned long long)_nwid);
	_dev = vtap_full_name;
	DEBUG_INFO("set VirtualTap interface name to: %s", _dev.c_str());
	// set virtual tap interface name (abbreviated)
	memset(vtap_abbr_name, 0, sizeof(vtap_abbr_name));
	snprintf(vtap_abbr_name, sizeof(vtap_abbr_name), "libzt%d", devno);
	lwip_driver_init();
	// start virtual tap thread and stack I/O loops
	_thread = Thread::start(this);
}

VirtualTap::~VirtualTap()
{
	_run = false;
	_phy.whack();
	Thread::join(_thread);
	_phy.close(_unixListenSocket,false);
}

void VirtualTap::setEnabled(bool en)
{
	_enabled = en;
}

bool VirtualTap::enabled() const
{
	return _enabled;
}

void VirtualTap::registerIpWithStack(const InetAddress &ip)
{
	lwip_init_interface((void*)this, this->_mac, ip);
}

bool VirtualTap::addIp(const InetAddress &ip)
{
	char ipbuf[INET6_ADDRSTRLEN];
	DEBUG_EXTRA("addr=%s, nwid=%llx", ip.toString(ipbuf), (unsigned long long)_nwid);
	Mutex::Lock _l(_ips_m);
	registerIpWithStack(ip);
	if (std::find(_ips.begin(),_ips.end(),ip) == _ips.end()) {
		_ips.push_back(ip);
		std::sort(_ips.begin(),_ips.end());
	}
	return true;
}

bool VirtualTap::removeIp(const InetAddress &ip)
{
	Mutex::Lock _l(_ips_m);
	std::vector<InetAddress>::iterator i(std::find(_ips.begin(),_ips.end(),ip));
	//if (i == _ips.end()) {
	//	return false;
	//}
	_ips.erase(i);
	if (ip.isV4()) {
		// FIXME: De-register from network stack
	}
	if (ip.isV6()) {
		// FIXME: De-register from network stack
	}
	return true;
}

std::vector<InetAddress> VirtualTap::ips() const
{
	Mutex::Lock _l(_ips_m);
	return _ips;
}

void VirtualTap::put(const MAC &from,const MAC &to,unsigned int etherType,
	const void *data,unsigned int len)
{
	lwip_eth_rx(this, from, to, etherType, data, len);
}

std::string VirtualTap::deviceName() const
{
	return _dev;
}

std::string VirtualTap::nodeId() const
{
	if (zt1ServiceRef) {
		char id[ZTO_ID_LEN];
		memset(id, 0, sizeof(id));
		sprintf(id, "%llx", (unsigned long long)((ZeroTier::OneService *)zt1ServiceRef)->getNode()->address());
		return std::string(id);
	}
	else {
		return std::string("----------");
	}
}

void VirtualTap::setFriendlyName(const char *friendlyName)
{
	DEBUG_EXTRA("%s", friendlyName);
}

void VirtualTap::scanMulticastGroups(std::vector<MulticastGroup> &added,
	std::vector<MulticastGroup> &removed)
{
	std::vector<MulticastGroup> newGroups;
	Mutex::Lock _l(_multicastGroups_m);
	// TODO: get multicast subscriptions
	std::vector<InetAddress> allIps(ips());
	for (std::vector<InetAddress>::iterator ip(allIps.begin());ip!=allIps.end();++ip)
		newGroups.push_back(MulticastGroup::deriveMulticastGroupForAddressResolution(*ip));

	std::sort(newGroups.begin(),newGroups.end());
	std::unique(newGroups.begin(),newGroups.end());

	for (std::vector<MulticastGroup>::iterator m(newGroups.begin());m!=newGroups.end();++m) {
		if (!std::binary_search(_multicastGroups.begin(),_multicastGroups.end(),*m))
			added.push_back(*m);
	}
	for (std::vector<MulticastGroup>::iterator m(_multicastGroups.begin());m!=_multicastGroups.end();++m) {
		if (!std::binary_search(newGroups.begin(),newGroups.end(),*m))
			removed.push_back(*m);
	}
	_multicastGroups.swap(newGroups);
}

void VirtualTap::setMtu(unsigned int mtu)
{
	_mtu = mtu;
}

void VirtualTap::threadMain()
	throw()
{
	while (true) {
#ifdef _MSC_VER
		Sleep(ZT_PHY_POLL_INTERVAL);
		_phy.poll(0);
#else
		_phy.poll(ZT_PHY_POLL_INTERVAL);
#endif
		Housekeeping();
	}
}

void VirtualTap::Housekeeping()
{
	Mutex::Lock _l(_tcpconns_m);
	uint64_t current_ts = time_now();
	if (current_ts > last_housekeeping_ts + ZT_HOUSEKEEPING_INTERVAL) {
		// update managed routes (add/del from network stacks)
		ZeroTier::OneService *service = ((ZeroTier::OneService *)zt1ServiceRef);
		if (service) {
			std::unique_ptr<std::vector<ZT_VirtualNetworkRoute>> managed_routes(service->getRoutes(this->_nwid));
			ZeroTier::InetAddress target_addr;
			ZeroTier::InetAddress via_addr;
			ZeroTier::InetAddress null_addr;
			ZeroTier::InetAddress nm;
			null_addr.fromString("");
			bool found;
			char ipbuf[INET6_ADDRSTRLEN], ipbuf2[INET6_ADDRSTRLEN], ipbuf3[INET6_ADDRSTRLEN];
			// TODO: Rework this when we have time
			// check if pushed route exists in tap (add)
			/*
			for (int i=0; i<ZT_MAX_NETWORK_ROUTES; i++) {
				found = false;
				target_addr = managed_routes->at(i).target;
				via_addr = managed_routes->at(i).via;
				nm = target_addr.netmask();
				for (size_t j=0; j<routes.size(); j++) {
					if (via_addr.ipsEqual(null_addr) || target_addr.ipsEqual(null_addr)) {
						found=true;
						continue;
					}
					if (routes[j].first.ipsEqual(target_addr) && routes[j].second.ipsEqual(nm)) {
						found=true;
					}
				}
				if (found == false) {
					if (via_addr.ipsEqual(null_addr) == false) {
						DEBUG_INFO("adding route <target=%s, nm=%s, via=%s>", target_addr.toString(ipbuf), nm.toString(ipbuf2), via_addr.toString(ipbuf3));
						routes.push_back(std::pair<ZeroTier::InetAddress,ZeroTier::InetAddress>(target_addr, nm));
						routeAdd(target_addr, nm, via_addr);
					}
				}
			}
			// check if route exists in tap but not in pushed routes (remove)
			for (size_t i=0; i<routes.size(); i++) {
				found = false;
				for (int j=0; j<ZT_MAX_NETWORK_ROUTES; j++) {
					target_addr = managed_routes->at(j).target;
					via_addr = managed_routes->at(j).via;
					nm = target_addr.netmask();
					if (routes[i].first.ipsEqual(target_addr) && routes[i].second.ipsEqual(nm)) {
						found=true;
					}
				}
				if (found == false) {
					DEBUG_INFO("removing route to <%s,%s>", routes[i].first.toString(ipbuf), routes[i].second.toString(ipbuf2));
					routes.erase(routes.begin() + i);
					routeDelete(routes[i].first, routes[i].second);
				}
			}
			*/
		}
		// TODO: Clean up VirtualSocket objects
		last_housekeeping_ts = time_now();
	}
}

/****************************************************************************/
/* Not used in this implementation                                          */
/****************************************************************************/

void VirtualTap::phyOnDatagram(PhySocket *sock,void **uptr,const struct sockaddr *local_address,
	const struct sockaddr *from,void *data,unsigned long len) {}
void VirtualTap::phyOnTcpConnect(PhySocket *sock,void **uptr,bool success) {}
void VirtualTap::phyOnTcpAccept(PhySocket *sockL,PhySocket *sockN,void **uptrL,void **uptrN,
	const struct sockaddr *from) {}
void VirtualTap::phyOnTcpClose(PhySocket *sock,void **uptr) {}
void VirtualTap::phyOnTcpData(PhySocket *sock,void **uptr,void *data,unsigned long len) {}
void VirtualTap::phyOnTcpWritable(PhySocket *sock,void **uptr) {}

