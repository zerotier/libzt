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
 * ZeroTier One service control wrapper header file
 */

#include "ZeroTierOne.h"
#include "InetAddress.hpp"
#include "libztDefs.h"

#include <vector>

#ifndef ZT1SERVICE_H
#define ZT1SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

class VirtualTap;
class VirtualSocket;

VirtualTap *getTapByAddr(ZeroTier::InetAddress *addr);
VirtualTap *getTapByName(char *ifname);
VirtualTap *getTapByIndex(size_t index);
VirtualTap *getAnyTap();

/**
 * @brief Returns a vector of network routes { target, via, metric, etc... }
 *
 * @usage
 * @param nwid 16-digit hexidecimal network identifier
 * @return
 */
std::vector<ZT_VirtualNetworkRoute> *zts_get_network_routes(const uint64_t nwid);

/**
 * @brief Starts a ZeroTier service in the background
 *
 * @usage For internal use only.
 * @param
 * @return
 */
void *zts_start_service(void *thread_id);

/**
 * @brief Returns masked address for subnet comparisons
 *
 * @usage For internal use only.
 * @param socket_type
 * @return
 */
bool _ipv6_in_subnet(ZeroTier::InetAddress *subnet, ZeroTier::InetAddress *addr);

#ifdef __cplusplus
}
#endif

#endif // _H
