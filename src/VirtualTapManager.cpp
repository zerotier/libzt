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
#include "VirtualTapManager.hpp"
#include "ServiceControls.hpp"
#include "OneService.hpp"

namespace ZeroTier {

extern std::map<uint64_t, VirtualTap*> vtapMap;
extern Mutex _vtaps_lock;
extern void (*_userCallbackFunc)(uint64_t, int);

class VirtualTap;

void VirtualTapManager::add_tap(VirtualTap *tap) {
    _vtaps_lock.lock();
    vtapMap[tap->_nwid] = tap;
    _vtaps_lock.unlock();
}

VirtualTap *VirtualTapManager::getTapByNWID(uint64_t nwid) {
    _vtaps_lock.lock();
    VirtualTap *s, *tap = vtapMap[nwid];
    _vtaps_lock.unlock();
    return tap;
}

size_t VirtualTapManager::get_vtaps_size() {
    size_t sz;
    _vtaps_lock.lock();
    sz = vtapMap.size();
    _vtaps_lock.unlock();
    return sz;
}

void VirtualTapManager::remove_by_nwid(uint64_t nwid) {
    _vtaps_lock.lock();
    vtapMap.erase(nwid);
    _vtaps_lock.unlock();
}

void VirtualTapManager::clear() {
    _vtaps_lock.lock();
    vtapMap.clear();
    _vtaps_lock.unlock();
}

void VirtualTapManager::get_network_details_helper(void *zt1ServiceRef, uint64_t nwid, struct zts_network_details *nd)
{
    socklen_t addrlen;
    VirtualTap *tap = vtapMap[nwid];
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
    OneService *zt1Service = (OneService*)zt1ServiceRef;
    zt1Service->getRoutes(nwid, (ZT_VirtualNetworkRoute*)&(nd->routes)[0], &(nd->num_routes));
}

void VirtualTapManager::get_network_details(void *zt1ServiceRef, uint64_t nwid, struct zts_network_details *nd) {
    _vtaps_lock.lock();
    get_network_details_helper(zt1ServiceRef, nwid, nd);
    _vtaps_lock.unlock();
}

void VirtualTapManager::get_all_network_details(void *zt1ServiceRef, struct zts_network_details *nds, int *num) {

    _vtaps_lock.lock();
    *num = vtapMap.size();
    int idx = 0;
    std::map<uint64_t, VirtualTap*>::iterator it;
    for (it = vtapMap.begin(); it != vtapMap.end(); it++) {
        get_network_details(zt1ServiceRef, it->first, &nds[idx]);
        idx++;
    }
    _vtaps_lock.unlock();
}

} // namespace ZeroTier