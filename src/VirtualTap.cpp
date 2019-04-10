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

#include "VirtualTap.hpp"
#include "Phy.hpp"
#include "Node.hpp"
//#include "OSUtils.hpp"

#include "Service.hpp"
#include "Mutex.hpp"
#include "lwipDriver.hpp"
#include "ZeroTier.h"

#ifdef _MSC_VER
#include "Synchapi.h"
#endif

namespace ZeroTier {

class VirtualTap;
extern OneService *service;
extern void postEvent(int eventCode, void *arg);

/**
 * A virtual tap device. The ZeroTier core service creates one of these for each
 * virtual network joined. It will be destroyed upon leave().
 */
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
	memset(vtap_full_name, 0, sizeof(vtap_full_name));
	snprintf(vtap_full_name, sizeof(vtap_full_name), "libzt%llx", (unsigned long long)_nwid);
	_dev = vtap_full_name;
	::pipe(_shutdownSignalPipe);
	// Start virtual tap thread and stack I/O loops
	_thread = Thread::start(this);
}

VirtualTap::~VirtualTap()
{
	struct zts_network_details *nd = new zts_network_details;
	nd->nwid = _nwid;
	postEvent(ZTS_EVENT_NETWORK_DOWN, (void*)nd);
	_run = false;
	::write(_shutdownSignalPipe[1],"\0",1);
	_phy.whack();
	lwip_remove_netif(netif);
	netif = NULL;
	Thread::join(_thread);
	::close(_shutdownSignalPipe[0]);
	::close(_shutdownSignalPipe[1]);
}

void VirtualTap::lastConfigUpdate(uint64_t lastConfigUpdateTime)
{
	_lastConfigUpdateTime = lastConfigUpdateTime;
}

void VirtualTap::setEnabled(bool en)
{
	_enabled = en;
}

bool VirtualTap::enabled() const
{
	return _enabled;
}

bool VirtualTap::hasIpv4Addr()
{
	Mutex::Lock _l(_ips_m);
	std::vector<InetAddress>::iterator it(_ips.begin());
	while (it != _ips.end()) {
		if ((*it).isV4()) { return true; }
		it++;
	}
	return false;
}

bool VirtualTap::hasIpv6Addr()
{
	Mutex::Lock _l(_ips_m);
	std::vector<InetAddress>::iterator it(_ips.begin());
	while (it != _ips.end()) {
		if ((*it).isV6()) { return true; }
		it++;
	}
	return false;
}

bool VirtualTap::addIp(const InetAddress &ip)
{
	//char ipbuf[128];
	//ip.toString(ipbuf);
	//DEBUG_INFO("addr=%s", ipbuf);

	/* Limit address assignments to one per type.
	This limitation can be removed if some changes
	are made in the netif driver. */
	if (ip.isV4() && hasIpv4Addr()) {
		return false;
	}
	if (ip.isV6() && hasIpv6Addr()) {
		return false;
	}

	Mutex::Lock _l(_ips_m);
	if (_ips.size() >= ZT_MAX_ZT_ASSIGNED_ADDRESSES) {
		return false;
	}
	if (std::find(_ips.begin(),_ips.end(),ip) == _ips.end()) {
		lwip_init_interface((void*)this, this->_mac, ip);
		// TODO: Add ZTS_EVENT_ADDR_NEW ?
		_ips.push_back(ip);
		// Send callback message
		struct zts_addr_details *ad = new zts_addr_details;
		ad->nwid = _nwid;
		if (ip.isV4()) {
			struct sockaddr_in *in4 = (struct sockaddr_in*)&(ad->addr);
			memcpy(&(in4->sin_addr.s_addr), ip.rawIpData(), 4);
			postEvent(ZTS_EVENT_ADDR_ADDED_IP4, (void*)ad);
		}
		if (ip.isV6()) {
			struct sockaddr_in6 *in6 = (struct sockaddr_in6*)&(ad->addr);
			memcpy(&(in6->sin6_addr.s6_addr), ip.rawIpData(), 16);
			postEvent(ZTS_EVENT_ADDR_ADDED_IP6, (void*)ad);
		}
		std::sort(_ips.begin(),_ips.end());
	}
	return true;
}

bool VirtualTap::removeIp(const InetAddress &ip)
{
	Mutex::Lock _l(_ips_m);
	std::vector<InetAddress>::iterator i(std::find(_ips.begin(),_ips.end(),ip));
	if (std::find(_ips.begin(),_ips.end(),ip) != _ips.end()) {
		struct zts_addr_details *ad = new zts_addr_details;
		ad->nwid = _nwid;
		if (ip.isV4()) {
			struct sockaddr_in *in4 = (struct sockaddr_in*)&(ad->addr);
			memcpy(&(in4->sin_addr.s_addr), ip.rawIpData(), 4);
			postEvent(ZTS_EVENT_ADDR_REMOVED_IP4, (void*)ad);
			// FIXME: De-register from network stack
		}
		if (ip.isV6()) {
			// FIXME: De-register from network stack
			struct sockaddr_in6 *in6 = (struct sockaddr_in6*)&(ad->addr);
			memcpy(&(in6->sin6_addr.s6_addr), ip.rawIpData(), 16);
			postEvent(ZTS_EVENT_ADDR_REMOVED_IP6, (void*)ad);
		}
		_ips.erase(i);
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
	if (len <= _mtu && _enabled) {
		lwip_eth_rx(this, from, to, etherType, data, len);
	}
}

std::string VirtualTap::deviceName() const
{
	return _dev;
}

std::string VirtualTap::nodeId() const
{
	if (service) {
		char id[16];
		memset(id, 0, sizeof(id));
		sprintf(id, "%llx", (unsigned long long)((OneService *)service)->getNode()->address());
		return std::string(id);
	}
	else {
		return std::string("----------");
	}
}

void VirtualTap::setFriendlyName(const char *friendlyName)
{
	DEBUG_INFO("%s", friendlyName);
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
	fd_set readfds,nullfds;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO(&readfds);
	FD_ZERO(&nullfds);
	int nfds = (int)std::max(_shutdownSignalPipe[0],0) + 1;
#if defined(__linux__)
	pthread_setname_np(pthread_self(), vtap_full_name);
#endif
#if defined(__APPLE__)
	pthread_setname_np(vtap_full_name);
#endif
	while (true) {
		FD_SET(_shutdownSignalPipe[0],&readfds);
		select(nfds,&readfds,&nullfds,&nullfds,&tv);
		// writes to shutdown pipe terminate thread
		if (FD_ISSET(_shutdownSignalPipe[0],&readfds)) {
			break;
		}
#if defined(_WIN32)
		Sleep(ZTS_TAP_THREAD_POLLING_INTERVAL);
#else
		struct timespec sleepValue = {0};
		sleepValue.tv_nsec = ZTS_TAP_THREAD_POLLING_INTERVAL * 500000;
		nanosleep(&sleepValue, NULL);
#endif
	}
}

void VirtualTap::Housekeeping()
{
    //
}

void VirtualTap::phyOnDatagram(PhySocket *sock,void **uptr,const struct sockaddr *local_address,
	const struct sockaddr *from,void *data,unsigned long len) {}
void VirtualTap::phyOnTcpConnect(PhySocket *sock,void **uptr,bool success) {}
void VirtualTap::phyOnTcpAccept(PhySocket *sockL,PhySocket *sockN,void **uptrL,void **uptrN,
	const struct sockaddr *from) {}
void VirtualTap::phyOnTcpClose(PhySocket *sock,void **uptr) {}
void VirtualTap::phyOnTcpData(PhySocket *sock,void **uptr,void *data,unsigned long len) {}
void VirtualTap::phyOnTcpWritable(PhySocket *sock,void **uptr) {}

} // namespace ZeroTier