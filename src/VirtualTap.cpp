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

// Used by raw stack driver implementation
struct ip_addr_t;
typedef unsigned short u16_t;

#include "Phy.hpp"

#include "VirtualTap.h"
#include "libzt.h"
#include "libztDebug.h"
#include "lwIP.h"
#include "picoTCP.h"
#include "ZT1Service.h"
#include "VirtualSocket.h"
#include "VirtualSocketLayer.h"
#include "SysUtils.h"

#include "Mutex.hpp"
#include "InetAddress.hpp"
#include "OneService.hpp"

//class VirtualSocket;

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
	snprintf(vtap_full_name, sizeof(vtap_full_name), "libzt%d-%lx", devno++, _nwid);
	_dev = vtap_full_name;
	DEBUG_INFO("set VirtualTap interface name to: %s", _dev.c_str());
	// set virtual tap interface name (abbreviated)
	memset(vtap_abbr_name, 0, sizeof(vtap_abbr_name));
	snprintf(vtap_abbr_name, sizeof(vtap_abbr_name), "libzt%d", devno);
#if defined(STACK_LWIP)
	// initialize network stacks
	lwip_driver_init();
#endif
	// start vtap thread and stack I/O loops
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

bool VirtualTap::registerIpWithStack(const InetAddress &ip)
{
#if defined(STACK_LWIP)
	lwip_init_interface((void*)this, this->_mac, ip);
#endif
#if defined(STACK_PICO)
	pico_register_address(this, ip);
#endif
	return true;
}

bool VirtualTap::addIp(const InetAddress &ip)
{
#if defined(NO_STACK)
	char ipbuf[INET6_ADDRSTRLEN];
	DEBUG_INFO("addIp (%s)", ip.toString(ipbuf));
	_ips.push_back(ip);
	std::sort(_ips.begin(),_ips.end());
	return true;
#endif
#if defined(STACK_PICO) || defined(STACK_LWIP)
	char ipbuf[INET6_ADDRSTRLEN];
	DEBUG_EXTRA("addr=%s", ip.toString(ipbuf));
	Mutex::Lock _l(_ips_m);
	if (registerIpWithStack(ip)) {
		if (std::find(_ips.begin(),_ips.end(),ip) == _ips.end()) {
			_ips.push_back(ip);
			std::sort(_ips.begin(),_ips.end());
		}
		return true;
	}
	return false;
#endif
}

bool VirtualTap::removeIp(const InetAddress &ip)
{
	DEBUG_EXTRA();
	Mutex::Lock _l(_ips_m);
	std::vector<InetAddress>::iterator i(std::find(_ips.begin(),_ips.end(),ip));
	//if (i == _ips.end()) {
	//	return false;
	//}
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
#if defined(STACK_LWIP)
	lwip_eth_rx(this, from, to, etherType, data, len);
#endif
#if defined(STACK_PICO)
	rd_pico_eth_rx(this,from,to,etherType,data,len);
#endif
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
		sprintf(id, "%lx",((ZeroTier::OneService *)zt1ServiceRef)->getNode()->address());
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
#if defined(STACK_LWIP) && !defined(LIBZT_RAW)
	while (true) {
		_phy.poll(ZT_PHY_POLL_INTERVAL);
		Housekeeping();
	}
#endif
#if defined(STACK_LWIP) && defined(LIBZT_RAW)
	rd_lwip_loop(this); // use driver loop
#endif
#if defined(STACK_PICO)
	pico_init_interface(this);
	if (this->should_start_stack) {
		rd_pico_loop(this); // use driver loop
	}
#endif
}

void VirtualTap::phyOnUnixClose(PhySocket *sock, void **uptr)
{
	DEBUG_EXTRA();
}

void VirtualTap::phyOnUnixData(PhySocket *sock, void **uptr, void *data, ssize_t len)
{
	DEBUG_EXTRA();
#if defined(LIBZT_RAW)
	VirtualSocket *vs = (VirtualSocket*)*uptr;
	if (vs == NULL) {
		return;
	}
	if (len > 0) {
		Write(vs, data, len);
	}
#endif
}

void VirtualTap::phyOnUnixWritable(PhySocket *sock, void **uptr, bool stack_invoked)
{
	DEBUG_EXTRA();
#if defined(LIBZT_RAW)
	if (sock) {
		Read(sock,uptr,stack_invoked);
	} else {
		DEBUG_ERROR("!sock");
	}
#endif
}

bool VirtualTap::routeAdd(const InetAddress &ip, const InetAddress &nm, const InetAddress &gw)
{
	bool err = false;
	DEBUG_EXTRA();
#if defined(STACK_LWIP)
	// general_lwip_init_interface(this, NULL, "n1", _mac, ip, nm, gw);
	// general_turn_on_interface(NULL);
	return true;
#endif
#if defined(STACK_PICO)
	return rd_pico_route_add(this, ip, nm, gw, 0);
#endif
#if defined(NO_STACK)
	// nothing to do
#endif
	return err;
}

bool VirtualTap::routeDelete(const InetAddress &ip, const InetAddress &nm)
{
	bool err = false;
	DEBUG_EXTRA();
#if defined(STACK_LWIP)
	// general_lwip_init_interface(this, NULL, "n1", _mac, ip, nm, gw);
	// general_turn_on_interface(NULL);
	return true;
#endif
#if defined(STACK_PICO)
	return rd_pico_route_del(this, ip, nm, 0);
#endif
#if defined(NO_STACK)
	// nothing to do
#endif
	return err;
}

void VirtualTap::addVirtualSocket(VirtualSocket *vs)
{
#if defined(LIBZT_RAW)
	DEBUG_EXTRA();
	Mutex::Lock _l(_tcpconns_m);
	_VirtualSockets.push_back(vs);
#endif
}

void VirtualTap::removeVirtualSocket()
{
#if defined(LIBZT_RAW)
	DEBUG_EXTRA();
	Mutex::Lock _l(_tcpconns_m);
	for (int i=0; i<_VirtualSockets.size(); i++) {
		if (vs == _VirtualSockets[i]) {
			_VirtualSockets.erase(_VirtualSockets.begin() + i);
			break;
		}
	}
#endif
}

/****************************************************************************/
/* DNS                                                                      */
/****************************************************************************/

int VirtualTap::add_DNS_Nameserver(struct sockaddr *addr)
{
	int err = -1;
#if defined(STACK_LWIP) && defined(LIBZT_RAW)
	err = rd_lwip_add_dns_nameserver(addr);
#endif
#if defined(STACK_PICO)
	rd_pico_add_dns_nameserver(addr);
#endif
	return err;
}

int VirtualTap::del_DNS_Nameserver(struct sockaddr *addr)
{
	int err = -1;
#if defined(STACK_LWIP)  && defined(LIBZT_RAW)
	err = rd_lwip_del_dns_nameserver(addr);
#endif
#if defined(STACK_PICO)
	err = rd_pico_del_dns_nameserver(addr);
#endif
	return err;
}

/****************************************************************************/
/* SDK Socket API                                                           */
/****************************************************************************/

// Connect
int VirtualTap::Connect(VirtualSocket *vs, const struct sockaddr *addr, socklen_t addrlen)
{
#if !defined(LIBZT_RAW)
	return -1;
#endif
#if defined(NO_STACK)
	return -1;
#endif
	int err = -1;
#if defined(STACK_LWIP) && defined(LIBZT_RAW)
	err = rd_lwip_connect(vs, addr, addrlen);
#endif
#if defined(STACK_PICO)
	Mutex::Lock _l(_tcpconns_m);
	err = rd_pico_connect(vs, addr, addrlen);
#endif
	return err;
}

// Bind VirtualSocket to a network stack's interface
int VirtualTap::Bind(VirtualSocket *vs, const struct sockaddr *addr, socklen_t addrlen)
{
#if !defined(LIBZT_RAW)
	return -1;
#endif
#if defined(NO_STACK)
	return -1;
#endif
#if defined(STACK_LWIP) && defined(LIBZT_RAW)
	Mutex::Lock _l(_tcpconns_m);
	return rd_lwip_bind(this, vs, addr, addrlen);
#endif
#if defined(STACK_PICO)
	Mutex::Lock _l(_tcpconns_m);
	return rd_pico_bind(vs, addr, addrlen);
#endif
	return -1;
}

// Listen for an incoming VirtualSocket
int VirtualTap::Listen(VirtualSocket *vs, int backlog)
{
#if !defined(LIBZT_RAW)
	return -1;
#endif
#if defined(NO_STACK)
	return -1;
#endif
	int err = -1;
#if defined(STACK_LWIP) && defined(LIBZT_RAW)
	Mutex::Lock _l(_tcpconns_m);
	err = rd_lwip_listen(vs, backlog);
#endif
#if defined(STACK_PICO)
	Mutex::Lock _l(_tcpconns_m);
	return rd_pico_listen(vs, backlog);
#endif
	return err;
}

// Accept a VirtualSocket
VirtualSocket *VirtualTap::Accept(VirtualSocket *vs)
{
#if !defined(LIBZT_RAW)
	return NULL;
#endif
	VirtualSocket *new_vs = NULL;
#if defined(NO_STACK)
	new_vs = NULL;
#endif
#if defined(STACK_LWIP) && defined(LIBZT_RAW)
	Mutex::Lock _l(_tcpconns_m);
	new_vs = rd_lwip_accept(vs);
#endif
#if defined(STACK_PICO)
	// TODO: separation of church and state
	Mutex::Lock _l(_tcpconns_m);
	new_vs = rd_pico_accept(vs);
#endif
	return new_vs;
}

// Read from stack/buffers into the app's socket
int VirtualTap::Read(VirtualSocket *vs, PhySocket *sock, void **uptr, bool stack_invoked)
{
#if !defined(LIBZT_RAW)
	return -1;
#endif
#if defined(NO_STACK)
	return -1;
#endif
	int err = -1;
#if defined(STACK_LWIP) && defined(LIBZT_RAW)
	err = rd_lwip_read((VirtualSocket*)*(_phy.getuptr(sock)), stack_invoked);
#endif
#if defined(STACK_PICO)
	err = rd_pico_read(this, sock, (VirtualSocket*)uptr, stack_invoked);
#endif
	return err;
}

// Write data from app socket to the virtual wire, either raw over VL2, or via network stack
int VirtualTap::Write(VirtualSocket *vs, void *data, ssize_t len)
{
#if !defined(LIBZT_RAW)
	return -1;
#endif
#if defined(NO_STACK)
	return -1;
#endif
	DEBUG_EXTRA("vs=%p, fd=%d, data=%p, len=%d", vs, vs->app_fd, data, len);
	int err = -1;
#if defined(LIBZT_RAW)
	// VL2, SOCK_RAW, no network stack
	if (vs->socket_type == SOCK_RAW) {
		struct ether_header *eh = (struct ether_header *) data;
		MAC src_mac;
		MAC dest_mac;
		src_mac.setTo(eh->ether_shost, 6);
		dest_mac.setTo(eh->ether_dhost, 6);
		_handler(_arg,NULL,_nwid,src_mac,dest_mac, Utils::ntoh((uint16_t)eh->ether_type),0, ((char*)data) + sizeof(struct ether_header),len - sizeof(struct ether_header));
		return len;
	}
#endif
#if defined(STACK_LWIP) && defined(LIBZT_RAW)
	err = rd_lwip_write(vs, data, len);
#endif
#if defined(STACK_PICO)
	err = rd_pico_write(vs, data, len);
#endif
	return err;
}

// Send data to a specified host
int VirtualTap::SendTo(VirtualSocket *vs, const void *buf, size_t len, int flags, const struct sockaddr *addr, socklen_t addrlen)
{
#if !defined(ZT_VIRTUAL_SOCKET)
	return -1;
#endif
	int err = -1;
#if defined(STACK_LWIP) && defined(ZT_VIRTUAL_SOCKET)
	if ((err = rd_lwip_connect(vs, addr, addrlen)) < 0) { // implicit
		return err;
	}
	if ((err = rd_lwip_write(vs, (void*)buf, len)) < 0) {
		return err;
	}
#endif
#if defined(STACK_PICO) && defined(ZT_VIRTUAL_SOCKET)
	if ((err = rd_pico_connect(vs, addr, addrlen)) < 0) { // implicit
		errno = ENOTCONN;
		return err;
	}
	if ((err = rd_pico_write(vs, (void*)buf, len)) < 0) {
		errno = ENOBUFS; // TODO: translate pico err to something more useful
		return err;
	}
#endif
	return err;
}

// Remove VritualSocket from VirtualTap, and instruct network stacks to dismantle their
// respective protocol control structures
int VirtualTap::Close(VirtualSocket *vs)
{
#if !defined(LIBZT_RAW)
	return -1;
#endif
	int err = 0;
#if defined(LIBZT_RAW)
	if (vs == NULL) {
		DEBUG_ERROR("invalid VirtualSocket");
		return -1;
	}
	if (vs->sock) {
		DEBUG_EXTRA("calling _phy.close()");
		_phy.close(vs->sock, true);
	}
	removeVirtualSocket(vs);
#endif
#if defined(STACK_LWIP) && defined(LIBZT_RAW)
	err = rd_lwip_close(vs);
#endif
#if defined(STACK_PICO)
	/*
		if (vs->get_state() != VS_STATE_CLOSED && vs->get_state() != VS_STATE_LISTENING) {
			DEBUG_EXTRA("vs=%p, vs->get_state()=%d, vs->picosock->state=%d", vs, vs->get_state(), vs->picosock->state);
			// doesn't make sense to shut down a listening socket, just close it
			if ((err = vs->tap->Shutdown(vs, SHUT_RDWR)) < 0) {
				DEBUG_ERROR("error while shutting down socket");
				return - 1;
			}
		}
		rd_pico_Close(vs);
		removeVirtualSocket(vs);
		if (vs->socket_type == SOCK_STREAM) {
			while (!(vs->picosock->state & PICO_SOCKET_STATE_CLOSED)) {
				nanosleep((const struct timespec[]) {{0, (ZT_ACCEPT_RECHECK_DELAY * 1000000)}}, NULL);
				DEBUG_EXTRA("virtual lingering on socket, ps=%p, buf remaining=%d",vs->picosock, vs->TXbuf->count());
			}
		}
	*/
#endif
	return err;
}

// Shuts down some aspect of a connection
int VirtualTap::Shutdown(VirtualSocket *vs, int how)
{
#if !defined(LIBZT_RAW)
	return -1;
#endif
	int err = 0;
#if defined(STACK_LWIP) && defined(LIBZT_RAW)
	err = rd_lwip_shutdown(vs, how);
#endif
#if defined(STACK_PICO)
	err = rd_pico_shutdown(vs, how);
#endif
	return err;
}

void VirtualTap::Housekeeping()
{
	Mutex::Lock _l(_tcpconns_m);
	uint64_t current_ts = time_now();
	if (current_ts > last_housekeeping_ts + ZT_HOUSEKEEPING_INTERVAL) {
		DEBUG_EXTRA();
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
					DEBUG_INFO("removing route to <target=%s>", routes[i].first.toString(ipbuf), routes[i].second.toString(ipbuf2));
					routes.erase(routes.begin() + i);
					routeDelete(routes[i].first, routes[i].second);
				}
			}
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

