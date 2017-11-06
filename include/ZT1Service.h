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
 * @brief
 *
 * @usage For internal use only.
 * @param filepath Path to configuration files
 * @param devID buffer to which the device ID (nodeID, ztAddress) should be copied
 * @return
 */
int zts_getid_from_file(const char *filepath, uint64_t nodeId);

/**
 * @brief Starts a ZeroTier service in the background
 *
 * @usage For internal use only.
 * @param
 * @return
 */
void *zts_start_service(void *thread_id);

/**
 * @brief Gets the VirtualTap's (interface) IP address
 *
 * @usage For internal use only.
 * @param nwid
 * @param addr
 * @param addrlen
 * @return
 */
void zts_get_address(const uint64_t nwid, struct sockaddr_storage *addr, const size_t addrlen);

/**
 * @brief Returns whether the VirtualTap has an assigned address (IPv4 or IPv6)
 *
 * @usage For internal use only.
 * @param nwid
 * @return
 */
int zts_has_address(const uint64_t nwid);

/**
 * @brief Copies the 6PLANE IPv6 address for the VirtualTap into the provided buffer
 *
 * @usage
 * @param addr
 * @param nwid
 * @param devID
 * @return
 */
void zts_get_6plane_addr(struct sockaddr_storage *addr, const uint64_t nwid, const uint64_t nodeId);

/**
 * @brief Copies the RFC4193 IPv6 address for the VirtualTap into the provided buffer
 *
 * @usage
 * @param addr
 * @param nwid
 * @param devID
 * @return
 */
void zts_get_rfc4193_addr(struct sockaddr_storage *addr, const uint64_t nwid, const uint64_t nodeId);

/**
 * @brief Join a network
 *
 * @usage Call this from application thread. Only after zts_start() has succeeded
 * @param nwid A 16-digit hexidecimal virtual network ID
 * @return
 */
void zts_join(const uint64_t nwid);

/**
 * @brief Leave a network
 *
 * @usage Call this from application thread. Only after zts_start() has succeeded
 * @param nwid A 16-digit hexidecimal virtual network ID
 * @return
 */
void zts_leave(const uint64_t nwid);

/**
 * @brief Return whether libzt (specifically the ZeroTier core service) is currently running
 *
 * @usage Call this before, during, or after zts_start()
 * @return
 */
int zts_running();

/**
 * @brief Start an instance of libzt (ZeroTier core service, network stack drivers, network stack threads, etc)
 *
 * @usage Call this when you anticipate needing to communicate over ZeroTier virtual networks. It is recommended
 * that one call this at the beginning of your application code since it may take several seconds to fully
 * come online.
 * @param path Where this instance of ZeroTier will store its identity and configuration files
 * @return Returns 1 if ZeroTier is currently running, and 0 if it is not
 */
int zts_start(const char *path, bool blocking);

/**
 * @brief Alternative to zts_start(). Start an instance of libzt, wait for an address to be issues, and join
 * given network
 *
 * @usage Call this when you anticipate needing to communicate over ZeroTier virtual networks. It is recommended
 * that one call this at the beginning of your application code since it may take several seconds to fully
 * come online.
 * @param path
 * @param nwid A 16-digit hexidecimal virtual network ID
 * @return Returns 0 on success, -1 on failure
 */
int zts_startjoin(const char *path, const uint64_t nwid);

/**
 * @brief Stops libzt (ZeroTier core services, stack drivers, stack threads, etc)
 *
 * @usage This should be called at the end of your program or when you do not anticipate communicating over ZeroTier
 * @return Returns 0 on success, -1 on failure
 */
void zts_stop();

/**
 * @brief Copies the configuration path used by ZeroTier into the provided buffer
 *
 * @usage
 * @param homePath
 * @param len
 * @return
 */
void zts_get_homepath(char *homePath, size_t len);

/**
 * @brief Returns the ztaddress/nodeId/device ID of this instance
 *
 * @usage Call this after zts_start() and/or when zts_running() returns true
 * @return
 */
uint64_t zts_get_node_id();

/**
 * @brief Return the number of peers
 *
 * @usage Call this after zts_start() has succeeded
 * @param
 * @return
 */
unsigned long zts_get_peer_count();

/**
 * @brief Get the virtual address of a perr given it's ztAddress/nodeID
 *
 * @usage Call this after zts_start() has succeeded
 * @param
 * @return
 */
int zts_get_peer_address(char *peer, const uint64_t nodeId);

/**
 * @brief Allow or disallow this instance of libzt to be controlled via HTTP requests
 *
 * @usage Call this after zts_start() has succeeded
 * @param allowed True or false value
 * @return
 */
void zts_allow_http_control(bool allowed);

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
