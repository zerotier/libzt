/*
 * ZeroTier SDK - Network Virtualization Everywhere
 * Copyright (C) 2011-2016  ZeroTier, Inc.  https://www.zerotier.com/
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
 */

#include <algorithm>
#include <utility>
#include <dlfcn.h>
#include <sys/poll.h>
#include <stdint.h>
#include <utility>
#include <string>
#include <sys/resource.h>
#include <sys/syscall.h>

#include "SocketTap.hpp"
#include "ZeroTierSDK.h"
#include "picoTCP.hpp"

#include "Utils.hpp"
#include "OSUtils.hpp"
#include "Constants.hpp"
#include "Phy.hpp"

namespace ZeroTier {

	/****************************************************************************/
	/* SocketTap Service                                                        */
	/* - For each joined network a SocketTap will be created to administer I/O  */
	/*   calls to the stack and the ZT virtual wire                             */
	/****************************************************************************/

	SocketTap::SocketTap(
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
	    _thread = Thread::start(this);
	}

	SocketTap::~SocketTap()
	{
		// TODO: Verify deletion of all objects
		_run = false;
		_phy.whack();
		Thread::join(_thread);
		_phy.close(_unixListenSocket,false);
		for(int i=0; i<_Connections.size(); i++) delete _Connections[i];
	}

	void SocketTap::setEnabled(bool en)
	{
		_enabled = en;
	}

	bool SocketTap::enabled() const
	{
		return _enabled;
	}

	bool SocketTap::addIp(const InetAddress &ip)
	{
		DEBUG_INFO();
		picostack->pico_init_interface(this, ip);
		_ips.push_back(ip);
		return true;
	}

	bool SocketTap::removeIp(const InetAddress &ip)
	{
		DEBUG_INFO();
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

	std::vector<InetAddress> SocketTap::ips() const
	{
		Mutex::Lock _l(_ips_m);
		return _ips;
	}

	void SocketTap::put(const MAC &from,const MAC &to,unsigned int etherType,
		const void *data,unsigned int len)
	{
	    // RX packet
	   	picostack->pico_rx(this, from,to,etherType,data,len);
	}

	std::string SocketTap::deviceName() const
	{
		return _dev;
	}

	void SocketTap::setFriendlyName(const char *friendlyName) {
		DEBUG_INFO();
	}

	void SocketTap::scanMulticastGroups(std::vector<MulticastGroup> &added,
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

	void SocketTap::threadMain()
		throw()
	{
		picostack->pico_loop(this);
	}

	void SocketTap::phyOnUnixClose(PhySocket *sock,void **uptr) 
	{
		Close((Connection*)uptr);
	}
	
	void SocketTap::phyOnUnixData(PhySocket *sock, void **uptr, void *data, ssize_t len)
	{
		Connection *conn = (Connection*)*uptr;
		if(!conn)
			return;
	    if(len) {
	    	unsigned char *buf = (unsigned char*)data;
	    	memcpy(conn->txbuf + conn->txsz, buf, len);
	        conn->txsz += len;
	        Write(conn);
	    }
	    return;
	}

	void SocketTap::phyOnUnixWritable(PhySocket *sock,void **uptr,bool stack_invoked)
	{
		Read(sock,uptr,stack_invoked);
	}

	/****************************************************************************/
	/* SDK Socket API                                                           */
	/****************************************************************************/

	int SocketTap::Connect(Connection *conn, int fd, const struct sockaddr *addr, socklen_t addrlen) {
		Mutex::Lock _l(_tcpconns_m);
		return picostack->pico_Connect(conn, fd, addr, addrlen);
	}

	int SocketTap::Bind(Connection *conn, int fd, const struct sockaddr *addr, socklen_t addrlen) {
		Mutex::Lock _l(_tcpconns_m);
		return picostack->pico_Bind(conn, fd, addr, addrlen);
	}

	void SocketTap::Listen(Connection *conn, int fd, int backlog) {
		Mutex::Lock _l(_tcpconns_m);
		picostack->pico_Listen(conn, fd, backlog);
	}

	int SocketTap::Accept(Connection *conn) {
		Mutex::Lock _l(_tcpconns_m);
		return picostack->pico_Accept(conn);
	}

	void SocketTap::Read(PhySocket *sock,void **uptr,bool stack_invoked) {
		picostack->pico_Read(this, sock, uptr, stack_invoked);
	}

	void SocketTap::Write(Connection *conn) {
		picostack->pico_Write(conn);
	}

	void SocketTap::Close(Connection *conn) {
		Mutex::Lock _l(_close_m);
		// Here we assume _tcpconns_m is already locked by caller
		if(!conn->sock) {
			DEBUG_EXTRA("invalid PhySocket");
			return;
		}
	    if(!conn)
	        return;
		for(size_t i=0;i<_Connections.size();++i) {
			if(_Connections[i] == conn){
				_Connections.erase(_Connections.begin() + i);
				delete conn;
				break;
			}
		}
		if(!conn->sock)
			return;
		close(_phy.getDescriptor(conn->sock));
		_phy.close(conn->sock, false);
	}

	/****************************************************************************/
	/* Not used in this implementation                                          */
	/****************************************************************************/

	void SocketTap::phyOnDatagram(PhySocket *sock,void **uptr,const struct sockaddr *local_address, 
		const struct sockaddr *from,void *data,unsigned long len) {}
	void SocketTap::phyOnTcpConnect(PhySocket *sock,void **uptr,bool success) {}
	void SocketTap::phyOnTcpAccept(PhySocket *sockL,PhySocket *sockN,void **uptrL,void **uptrN,
		const struct sockaddr *from) {}
	void SocketTap::phyOnTcpClose(PhySocket *sock,void **uptr) {}
	void SocketTap::phyOnTcpData(PhySocket *sock,void **uptr,void *data,unsigned long len) {}
	void SocketTap::phyOnTcpWritable(PhySocket *sock,void **uptr, bool stack_invoked) {}

} // namespace ZeroTier

