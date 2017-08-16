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

#include <netinet/in.h>
#include <net/if_arp.h>
#include <arpa/inet.h>

#include <algorithm>
#include <utility>
#include <sys/poll.h>
#include <stdint.h>
#include <utility>
#include <string>

#include "VirtualTap.hpp"
#include "libzt.h"

#if defined(STACK_PICO)
#include "picoTCP.hpp"
#endif
#if defined(STACK_LWIP)
#include "lwIP.hpp"
#endif

#if defined(__APPLE__)
#include <net/ethernet.h>
#endif
#if defined(__linux__)
#include <netinet/ether.h>
#endif

#include "OneService.hpp"
#include "Utils.hpp"
#include "OSUtils.hpp"
#include "Constants.hpp"
#include "Phy.hpp"

class VirtualTap;

extern std::vector<void*> vtaps;

namespace ZeroTier {

	int VirtualTap::devno = 0;

	/****************************************************************************/
	/* VirtualTap Service                                                        */
	/* - For each joined network a VirtualTap will be created to administer I/O  */
	/*   calls to the stack and the ZT virtual wire                             */
	/****************************************************************************/

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
			_enabled(true),
			_run(true),
			_mac(mac),
			_mtu(mtu),
			_nwid(nwid),
			_unixListenSocket((PhySocket *)0),
			_phy(this,false,true)
	{
		vtaps.push_back((void*)this);

		// set interface name
		char tmp3[17];
		ifindex = devno;
		sprintf(tmp3, "libzt%d", devno++);
		_dev = tmp3;
		DEBUG_INFO("set device name to: %s", _dev.c_str());

		_thread = Thread::start(this);
	}

	VirtualTap::~VirtualTap()
	{
		_run = false;
		_phy.whack();
		Thread::join(_thread);
		_phy.close(_unixListenSocket,false);
		for(int i=0; i<_VirtualSockets.size(); i++) delete _VirtualSockets[i];
	}

	void VirtualTap::setEnabled(bool en)
	{
		_enabled = en;
	}

	bool VirtualTap::enabled() const
	{
		return _enabled;
	}

	bool VirtualTap::registerIpWithStack(const InetAddress &ip)
	{
#if defined(STACK_PICO)
		if(picostack){
			picostack->pico_init_interface(this, ip);
			return true;
		}
#endif
#if defined(STACK_LWIP)
		if(lwipstack){
			lwipstack->lwip_init_interface(this, ip);
			return true;
		}
#endif
		return false;
	}

	bool VirtualTap::addIp(const InetAddress &ip)
	{
#if defined(NO_STACK)
		char ipbuf[64];
		DEBUG_INFO("addIp (%s)", ip.toString(ipbuf));
		_ips.push_back(ip);
		std::sort(_ips.begin(),_ips.end());
		return true;
#endif
		if(registerIpWithStack(ip))
		{
			// only start the stack if we successfully registered and initialized a device to 
			// the given address
			_ips.push_back(ip);
			std::sort(_ips.begin(),_ips.end());
			return true;
		}
		return false;
	}

	bool VirtualTap::removeIp(const InetAddress &ip)
	{
		Mutex::Lock _l(_ips_m);
		std::vector<InetAddress>::iterator i(std::find(_ips.begin(),_ips.end(),ip));
		if (i == _ips.end())
			return false;
		_ips.erase(i);
		if (ip.isV4()) {
			// FIXME: De-register from network stacks
		}
		if (ip.isV6()) {
			// FIXME: De-register from network stacks
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
#if defined(STACK_PICO)
		if(picostack)
			picostack->pico_rx(this,from,to,etherType,data,len);
#endif
#if defined(STACK_LWIP)
		if(lwipstack)
			lwipstack->lwip_rx(this,from,to,etherType,data,len);
#endif  
	}

	std::string VirtualTap::deviceName() const
	{
		return _dev;
	}

	void VirtualTap::setFriendlyName(const char *friendlyName) 
	{
		DEBUG_INFO("%s", friendlyName);
		// Someday
	}

	void VirtualTap::scanMulticastGroups(std::vector<MulticastGroup> &added,
		std::vector<MulticastGroup> &removed)
	{
		std::vector<MulticastGroup> newGroups;
		Mutex::Lock _l(_multicastGroups_m);
		// TODO: get multicast subscriptions from network stack
		std::vector<InetAddress> allIps(ips());
		for(std::vector<InetAddress>::iterator ip(allIps.begin());ip!=allIps.end();++ip)
			newGroups.push_back(MulticastGroup::deriveMulticastGroupForAddressResolution(*ip));

		std::sort(newGroups.begin(),newGroups.end());
		std::unique(newGroups.begin(),newGroups.end());

		for(std::vector<MulticastGroup>::iterator m(newGroups.begin());m!=newGroups.end();++m) {
			if (!std::binary_search(_multicastGroups.begin(),_multicastGroups.end(),*m))
				added.push_back(*m);
		}
		for(std::vector<MulticastGroup>::iterator m(_multicastGroups.begin());m!=_multicastGroups.end();++m) {
			if (!std::binary_search(newGroups.begin(),newGroups.end(),*m))
				removed.push_back(*m);
		}
		_multicastGroups.swap(newGroups);
	}

	void VirtualTap::setMtu(unsigned int mtu)
	{
		if (_mtu != mtu) {
			_mtu = mtu;
		}
	}

	void VirtualTap::threadMain()
		throw()
	{
#if defined(STACK_PICO)
		if(picostack)
			picostack->pico_loop(this);
#endif
#if defined(STACK_LWIP)
		if(lwipstack)
			lwipstack->lwip_loop(this);
#endif
	}

	void VirtualTap::phyOnUnixClose(PhySocket *sock,void **uptr) 
	{
		if(sock) {
			VirtualSocket *vs = (VirtualSocket*)uptr;
			if(vs)
				Close(vs);
		}
	}
	
	void VirtualTap::phyOnUnixData(PhySocket *sock, void **uptr, void *data, ssize_t len)
	{
		DEBUG_ATTN("sock->fd=%d", _phy.getDescriptor(sock));
		VirtualSocket *vs = (VirtualSocket*)*uptr;
		if(!vs)
			return;
		if(len){
			Write(vs, data, len);
		}
		return;
	}

	void VirtualTap::phyOnUnixWritable(PhySocket *sock, void **uptr, bool stack_invoked)
	{
		if(sock)
			Read(sock,uptr,stack_invoked);
	}

	// Adds a route to the virtual tap
	bool VirtualTap::routeAdd(const InetAddress &addr, const InetAddress &nm, const InetAddress &gw)
	{
#if defined(STACK_PICO)
		if(picostack)
			return picostack->pico_route_add(this, addr, nm, gw, 0);
#endif
#if defined(STACK_LWIP)
		return true;
#endif
		return false;
	}

	// Deletes a route from the virtual tap
	bool VirtualTap::routeDelete(const InetAddress &addr, const InetAddress &nm)
	{
#if defined(STACK_PICO)
		if(picostack)
			return picostack->pico_route_del(this, addr, nm, 0);
#endif
#if defined(STACK_LWIP)
		return true;
#endif
		return false;
	}
	
	/****************************************************************************/
	/* SDK Socket API                                                           */
	/****************************************************************************/

	// Connect
	int VirtualTap::Connect(VirtualSocket *vs, const struct sockaddr *addr, socklen_t addrlen) {
#if defined(NO_STACK)
		return -1;
#endif
#if defined(STACK_PICO)
		if(picostack)
			Mutex::Lock _l(_tcpconns_m);
			return picostack->pico_Connect(vs, addr, addrlen);
#endif
#if defined(STACK_LWIP)
		if(lwipstack)
			return lwipstack->lwip_Connect(vs, addr, addrlen);
#endif
		return ZT_ERR_GENERAL_FAILURE;
	}

	// Bind VirtualSocket to a network stack's interface
	int VirtualTap::Bind(VirtualSocket *vs, const struct sockaddr *addr, socklen_t addrlen) {
#if defined(NO_STACK)
		return -1;
#endif
		Mutex::Lock _l(_tcpconns_m);
#if defined(STACK_PICO)
		if(picostack)
			return picostack->pico_Bind(vs, addr, addrlen);
#endif
#if defined(STACK_LWIP)
		if(lwipstack)
			return lwipstack->lwip_Bind(this, vs, addr, addrlen);
#endif	
		return ZT_ERR_GENERAL_FAILURE;
	}

	// Listen for an incoming VirtualSocket
	int VirtualTap::Listen(VirtualSocket *vs, int backlog) {
#if defined(NO_STACK)
		return -1;
#endif
		Mutex::Lock _l(_tcpconns_m);
#if defined(STACK_PICO)
		if(picostack)
			return picostack->pico_Listen(vs, backlog);
		return ZT_ERR_GENERAL_FAILURE;
#endif
#if defined(STACK_LWIP)
		if(lwipstack)
			return lwipstack->lwip_Listen(vs, backlog);
		return ZT_ERR_GENERAL_FAILURE;
#endif
		return ZT_ERR_GENERAL_FAILURE;
	}

	// Accept a VirtualSocket 
	VirtualSocket* VirtualTap::Accept(VirtualSocket *vs) {
#if defined(NO_STACK)
		return NULL;
#endif
#if defined(STACK_PICO)
		if(picostack)
			Mutex::Lock _l(_tcpconns_m);
			return picostack->pico_Accept(vs);
		return NULL;
#endif
#if defined(STACK_LWIP)
		if(lwipstack)
			return lwipstack->lwip_Accept(vs);
		return NULL;
#endif
		return NULL;
	}

	// Read from stack/buffers into the app's socket
	int VirtualTap::Read(PhySocket *sock,void **uptr,bool stack_invoked) {
#if defined(STACK_PICO)
		if(picostack)
			return picostack->pico_Read(this, sock, (VirtualSocket*)uptr, stack_invoked);
#endif
#if defined(STACK_LWIP)
		if(lwipstack)
			return lwipstack->lwip_Read((VirtualSocket*)*(_phy.getuptr(sock)), stack_invoked);
#endif
		return -1;
	}

	// Write data from app socket to the virtual wire, either raw over VL2, or via network stack
	int VirtualTap::Write(VirtualSocket *vs, void *data, ssize_t len) {
		// VL2, SOCK_RAW, no network stack
		if(vs->socket_type == SOCK_RAW) {
			struct ether_header *eh = (struct ether_header *) data;
			MAC src_mac;
			MAC dest_mac;
			src_mac.setTo(eh->ether_shost, 6);
			dest_mac.setTo(eh->ether_dhost, 6);
			_handler(_arg,NULL,_nwid,src_mac,dest_mac, Utils::ntoh((uint16_t)eh->ether_type),0, ((char*)data) + sizeof(struct ether_header),len - sizeof(struct ether_header));
			return len;
		}
#if defined(STACK_PICO)
		if(picostack)
			return picostack->pico_Write(vs, data, len);
#endif
#if defined(STACK_LWIP)
		if(lwipstack)
			return lwipstack->lwip_Write(vs, data, len);
#endif
		return -1;
	}

	// Send data to a specified host
	int VirtualTap::SendTo(VirtualSocket *vs, const void *buf, size_t len, int flags, const struct sockaddr *addr, socklen_t addrlen)
	{
		// TODO: flags
		int err = 0;
		DEBUG_INFO();
#if defined(STACK_PICO)
		if(picostack) {
			err = picostack->pico_Connect(vs, addr, addrlen); // implicit
			err = picostack->pico_Write(vs, (void*)buf, len);
		}
#endif
#if defined(STACK_LWIP)
		if(lwipstack)
			err = lwipstack->lwip_Connect(vs, addr, addrlen); // implicit
			err = lwipstack->lwip_Write(vs, (void*)buf, len);
#endif
		return err;
	}

	int VirtualTap::Close(VirtualSocket *vs) {
#if defined(STACK_PICO)
		if(!vs) {
			DEBUG_ERROR("invalid VirtualSocket");
			return -1;
		}
		picostack->pico_Close(vs);
		if(!vs->sock) {
			// DEBUG_EXTRA("invalid PhySocket");
			return -1;
		}
		// Here we assume _tcpconns_m is already locked by caller
		// FIXME: is this assumption still valid
		if(vs->state==ZT_SOCK_STATE_LISTENING)
		{
			// since we never wrapped this socket
			DEBUG_INFO("in LISTENING state, no need to close in PhyIO");
			return -1;
		}
		else
		{
			if(vs->sock)
				_phy.close(vs->sock, false);
		}
		close(_phy.getDescriptor(vs->sock));
		for(size_t i=0;i<_VirtualSockets.size();++i) {
			if(_VirtualSockets[i] == vs){
				// FIXME: double free issue exists here (potentially)
				// _VirtualSockets.erase(_VirtualSockets.begin() + i);
				//delete conn;
				break;
			}
		}
#endif
#if defined(STACK_LWIP)
		if(lwipstack)
			lwipstack->lwip_Close(vs);
#endif
		return 0; // TODO
	}

	void VirtualTap::Housekeeping()
	{
		Mutex::Lock _l(_tcpconns_m);
		std::time_t current_ts = std::time(nullptr);
		if(current_ts > last_housekeeping_ts + ZT_HOUSEKEEPING_INTERVAL) {
			// update managed routes (add/del from network stacks)		
			if(zt1ServiceRef) {
				std::vector<ZT_VirtualNetworkRoute> *managed_routes = ((ZeroTier::OneService *)zt1ServiceRef)->getRoutes(this->_nwid);
				ZeroTier::InetAddress target_addr;
				ZeroTier::InetAddress via_addr;
				ZeroTier::InetAddress null_addr;
				ZeroTier::InetAddress nm;
				null_addr.fromString("");
				bool found;
				char ipbuf[64], ipbuf2[64], ipbuf3[64];
				// TODO: Rework this when we have time
				// check if pushed route exists in tap (add)
				for(int i=0; i<ZT_MAX_NETWORK_ROUTES; i++) {
					found = false;
					target_addr = managed_routes->at(i).target;
					via_addr = managed_routes->at(i).via;
					nm = target_addr.netmask();
					for(int j=0; j<routes.size(); j++) {
						if(via_addr.ipsEqual(null_addr) || target_addr.ipsEqual(null_addr)) {
							found=true;
							continue;
						}
						if(routes[j].first.ipsEqual(target_addr) && routes[j].second.ipsEqual(nm)) {
							found=true;
						}
					}
					if(!found) {
						if(!via_addr.ipsEqual(null_addr)) {
							DEBUG_INFO("adding route <target=%s, nm=%s, via=%s>", target_addr.toString(ipbuf), nm.toString(ipbuf2), via_addr.toString(ipbuf3));
							routes.push_back(std::pair<ZeroTier::InetAddress,ZeroTier::InetAddress>(target_addr, nm));							
							routeAdd(target_addr, nm, via_addr);						
						}
					}
				}
				// check if route exists in tap but not in pushed routes (remove)
				for(int i=0; i<routes.size(); i++) {
					found = false;
					for(int j=0; j<ZT_MAX_NETWORK_ROUTES; j++) {
						target_addr = managed_routes->at(j).target;
						via_addr = managed_routes->at(j).via;
						nm = target_addr.netmask();
						if(routes[i].first.ipsEqual(target_addr) && routes[i].second.ipsEqual(nm)) {
							found=true;
						}
					}
					if(!found) {
						DEBUG_INFO("removing route to <target=%s>", routes[i].first.toString(ipbuf), routes[i].second.toString(ipbuf2));
						routes.erase(routes.begin() + i);
						routeDelete(routes[i].first, routes[i].second);
					}
				}
			}

			// TODO: Clean up VirtualSocket objects

			last_housekeeping_ts = std::time(nullptr);
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

} // namespace ZeroTier

