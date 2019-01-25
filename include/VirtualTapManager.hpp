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

#ifndef LIBZT_VIRTUAL_TAP_MANAGER_H
#define LIBZT_VIRTUAL_TAP_MANAGER_H

#include "VirtualTap.hpp"
#include "OneService.hpp"

namespace ZeroTier {

class VirtualTap;

/**
 * @brief Static utility class for safely handling VirtualTap(s)
 */
class VirtualTapManager
{
public:

	static void add_tap(VirtualTap *tap);
	static VirtualTap *getTapByNWID(uint64_t nwid);
	static size_t get_vtaps_size();
	static void remove_by_nwid(uint64_t nwid);
	static void clear();
	static void get_network_details_helper(void *zt1ServiceRef, uint64_t nwid, struct zts_network_details *nd);
	static void get_network_details(void *zt1ServiceRef, uint64_t nwid, struct zts_network_details *nd);
	static void get_all_network_details(void *zt1ServiceRef, struct zts_network_details *nds, int *num);
};

} // namespace ZeroTier

#endif // _H