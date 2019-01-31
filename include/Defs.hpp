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

#ifndef LIBZT_DEFS_HPP
#define LIBZT_DEFS_HPP

#include "Constants.hpp"

#ifndef _USING_LWIP_DEFINITIONS_
#include <sys/socket.h>
#endif

namespace ZeroTier {

//////////////////////////////////////////////////////////////////////////////
// Subset of: ZeroTierOne.h                                                 //
// We redefine a few ZT structures here so that we don't need to drag the   //
// entire ZeroTierOne.h file into the user application                      //
//////////////////////////////////////////////////////////////////////////////

/**
 * What trust hierarchy role does this peer have?
 */
enum zts_peer_role
{
	ZTS_PEER_ROLE_LEAF = 0,       // ordinary node
	ZTS_PEER_ROLE_MOON = 1,       // moon root
	ZTS_PEER_ROLE_PLANET = 2      // planetary root
};

/**
 * A structure used to represent a virtual network route
 */
struct zts_virtual_network_route
{
	/**
	 * Target network / netmask bits (in port field) or NULL or 0.0.0.0/0 for default
	 */
	struct sockaddr_storage target;

	/**
	 * Gateway IP address (port ignored) or NULL (family == 0) for LAN-local (no gateway)
	 */
	struct sockaddr_storage via;

	/**
	 * Route flags
	 */
	uint16_t flags;

	/**
	 * Route metric (not currently used)
	 */
	uint16_t metric;
};

/**
 * A structure used to convey network-specific details to the user application
 */
struct zts_network_details
{
	/**
	 * Network ID
	 */
	uint64_t nwid;

	/**
	 * Maximum Transmission Unit size for this network
	 */
	int mtu;

	/**
	 * Number of addresses (actually) assigned to the node on this network
	 */
	short num_addresses;

	/**
	 * Array of IPv4 and IPv6 addresses assigned to the node on this network
	 */
	struct sockaddr_storage addr[ZTS_MAX_ASSIGNED_ADDRESSES];

	/**
	 * Number of routes
	 */
	unsigned int num_routes;

	/**
	 * Array of IPv4 and IPv6 addresses assigned to the node on this network
	 */
	struct zts_virtual_network_route routes[ZTS_MAX_NETWORK_ROUTES];
};

/**
 * Physical network path to a peer
 */
struct zts_physical_path
{
	/**
	 * Address of endpoint
	 */
	struct sockaddr_storage address;

	/**
	 * Time of last send in milliseconds or 0 for never
	 */
	uint64_t lastSend;

	/**
	 * Time of last receive in milliseconds or 0 for never
	 */
	uint64_t lastReceive;

	/**
	 * Is this a trusted path? If so this will be its nonzero ID.
	 */
	uint64_t trustedPathId;

	/**
	 * Is path expired?
	 */
	int expired;

	/**
	 * Is path preferred?
	 */
	int preferred;
};

/**
 * Peer status result buffer
 */
struct zts_peer_details
{
	/**
	 * ZeroTier address (40 bits)
	 */
	uint64_t address;

	/**
	 * Remote major version or -1 if not known
	 */
	int versionMajor;

	/**
	 * Remote minor version or -1 if not known
	 */
	int versionMinor;

	/**
	 * Remote revision or -1 if not known
	 */
	int versionRev;

	/**
	 * Last measured latency in milliseconds or -1 if unknown
	 */
	int latency;

	/**
	 * What trust hierarchy role does this device have?
	 */
	enum zts_peer_role role;

	/**
	 * Number of paths (size of paths[])
	 */
	unsigned int pathCount;

	/**
	 * Known network paths to peer
	 */
	zts_physical_path paths[ZT_MAX_PEER_NETWORK_PATHS];
};

/**
 * List of peers
 */
struct zts_peer_list
{
	zts_peer_details *peers;
	unsigned long peerCount;
};

} // namespace ZeroTier

#endif // _H