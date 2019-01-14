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
 * Management of virtual tap interfaces
 */

#include "VirtualTap.hpp"
#include "OneService.hpp"

namespace ZeroTier {

extern std::vector<void*> vtaps;
extern Mutex _vtaps_lock;

class VirtualTap;

/**
 * @brief Static utility class for safely handling VirtualTap(s)
 */
class VirtualTapManager
{
public:

	static void add_tap(VirtualTap *tap) {
		_vtaps_lock.lock();
		vtaps.push_back((void*)tap);
		_vtaps_lock.unlock();
	}

	static VirtualTap *getTapByNWID(uint64_t nwid)
	{
		_vtaps_lock.lock();
		VirtualTap *s, *tap = nullptr;
		for (size_t i=0; i<vtaps.size(); i++) {
			s = (VirtualTap*)vtaps[i];
			if (s->_nwid == nwid) { tap = s; }
		}
		_vtaps_lock.unlock();
		return tap;
	}

	static size_t get_vtaps_size() {
		size_t sz;
		_vtaps_lock.lock();
		sz = vtaps.size();
		_vtaps_lock.unlock();
		return sz;
	}

	// TODO: We shouldn't re-apply the reference to everything all the time
	static void update_service_references(void *serviceRef) {
		_vtaps_lock.lock();
		for (size_t i=0;i<vtaps.size(); i++) {
			VirtualTap *s = (VirtualTap*)vtaps[i];
			s->zt1ServiceRef=serviceRef;
		}
		_vtaps_lock.unlock();
	}

	static void remove_by_nwid(uint64_t nwid) {
		_vtaps_lock.lock();
		for (size_t i=0;i<vtaps.size(); i++) {
			VirtualTap *s = (VirtualTap*)vtaps[i];
			if (s->_nwid == nwid) {
				vtaps.erase(vtaps.begin() + i);
			}
		}
		_vtaps_lock.unlock();
	}

	static void clear() {
		_vtaps_lock.lock();
		vtaps.clear();
		_vtaps_lock.unlock();
	}

	static void get_network_details(uint64_t nwid, struct zts_network_details *nd)
	{
		VirtualTap *tap;
		socklen_t addrlen;
		_vtaps_lock.lock();
		for (size_t i=0; i<vtaps.size(); i++) {
			tap = (VirtualTap*)vtaps[i];
			if (tap->_nwid == nwid) {
				nd->nwid = tap->_nwid;
				nd->mtu = tap->_mtu;
				// assigned addresses
				nd->num_addresses = tap->_ips.size() < ZTS_MAX_ASSIGNED_ADDRESSES ? tap->_ips.size() : ZTS_MAX_ASSIGNED_ADDRESSES;
				for (int j=0; j<nd->num_addresses; j++) {
					addrlen = tap->_ips[j].isV4() ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
					memcpy(&(nd->addr[j]), &(tap->_ips[j]), addrlen);
				}
				// routes
				nd->num_routes = ZTS_MAX_NETWORK_ROUTES;
				OneService *zt1Service = (OneService*)tap->zt1ServiceRef;
				zt1Service->getRoutes(nwid, (ZT_VirtualNetworkRoute*)&(nd->routes)[0], &(nd->num_routes));
				break;
			}
		}
		_vtaps_lock.unlock();
	}

	static void get_all_network_details(struct zts_network_details *nds, int *num)
	{
		VirtualTap *tap;
		*num = get_vtaps_size();
		for (size_t i=0; i<vtaps.size(); i++) {
			tap = (VirtualTap*)vtaps[i];
			get_network_details(tap->_nwid, &nds[i]);
		}
	}
};

} // namespace ZeroTier