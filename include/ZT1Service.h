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
#include "Defs.h"

#include <vector>

#ifndef ZT1SERVICE_H
#define ZT1SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

namespace ZeroTier
{
	extern std::vector<void*> vtaps;

	class picoTCP;
	extern ZeroTier::picoTCP *picostack;

	class lwIP;
	extern ZeroTier::lwIP *lwipstack;

	class VirtualTap;
	class VirtualSocket;
	struct InetAddress;
}

/**
 * @brief Returns a vector of network routes { target, via, metric, etc... }
 *
 * @usage
 * @param nwid 16-digit hexidecimal network identifier
 * @return
 */
std::vector<ZT_VirtualNetworkRoute> *zts_get_network_routes(char *nwid);

/**
 * @brief
 *
 * @usage For internal use only.
 * @param filepath Path to configuration files
 * @param devID buffer to which the device ID (nodeID, ztAddress) should be copied
 * @return
 */
int zts_get_device_id_from_file(const char *filepath, char *devID);

/**
 * @brief Starts a ZeroTier service in the background
 *
 * @usage For internal use only.
 * @param
 * @return
 */
void *zts_start_service(void *thread_id);

/**
 * @brief Stops all VirtualTap interfaces and associated I/O loops
 *
 * @usage For internal use only.
 * @param
 * @return
 */
void disableTaps();

/**
 * @brief Gets the VirtualTap's (interface) IPv4 address
 *
 * @usage For internal use only.
 * @param nwid
 * @param addrstr
 * @param addrlen
 * @return
 */
void zts_get_ipv4_address(const char *nwid, char *addrstr, const int addrlen);

/**
 * @brief Gets the VirtualTap's (interface) IPv6 address
 *
 * @usage For internal use only.
 * @param nwid
 * @param addrstr
 * @param addrlen
 * @return
 */
void zts_get_ipv6_address(const char *nwid, char *addrstr, const int addrlen);

/**
 * @brief Returns whether the VirtualTap has an assigned IPv4 address
 *
 * @usage For internal use only.
 * @param nwid
 * @return
 */
int zts_has_ipv4_address(const char *nwid);

/**
 * @brief Returns whether the VirtualTap has an assigned IPv6 address
 *
 * @usage For internal use only.
 * @param nwid
 * @return
 */
int zts_has_ipv6_address(const char *nwid);

/**
 * @brief Returns whether the VirtualTap has an assigned address of any protocol version
 *
 * @usage For internal use only.
 * @param nwid
 * @return
 */
int zts_has_address(const char *nwid);

/**
 * @brief Copies the 6PLANE IPv6 address for the VirtualTap into the provided buffer
 *
 * @usage
 * @param addr
 * @param nwid
 * @param devID
 * @return
 */
void zts_get_6plane_addr(char *addr, const char *nwid, const char *devID);

/**
 * @brief Copies the RFC4193 IPv6 address for the VirtualTap into the provided buffer
 *
 * @usage
 * @param addr
 * @param nwid
 * @param devID 
 * @return
 */
void zts_get_rfc4193_addr(char *addr, const char *nwid, const char *devID);

/**
 * @brief Join a network
 *
 * @usage Call this from application thread. Only after zts_start() has succeeded
 * @param nwid A 16-digit hexidecimal virtual network ID
 * @return
 */
void zts_join(const char * nwid);

/**
 * @brief Leave a network
 *
 * @usage Call this from application thread. Only after zts_start() has succeeded
 * @param nwid A 16-digit hexidecimal virtual network ID
 * @return
 */
void zts_leave(const char * nwid);

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
 * @return
 */
void zts_start(const char *path);

/**
 * @brief Alternative to zts_start(). Start an instance of libzt, wait for an address to be issues, and join
 * given network
 *
 * @usage Call this when you anticipate needing to communicate over ZeroTier virtual networks. It is recommended
 * that one call this at the beginning of your application code since it may take several seconds to fully
 * come online.
 * @param path
 * @param nwid A 16-digit hexidecimal virtual network ID
 * @return
 */
void zts_simple_start(const char *path, const char *nwid);

/**
 * @brief Stops libzt (ZeroTier core services, stack drivers, stack threads, etc)
 *
 * @usage This should be called at the end of your program or when you do not anticipate communicating over ZeroTier
 * @return
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
void zts_get_homepath(char *homePath, int len);

/**
 * @brief Copies the hexidecimal representation of this nodeID into the provided buffer
 *
 * @usage Call this after zts_start() and/or when zts_running() returns true
 * @param devID Buffer to which id string is copied
 * @return
 */
int zts_get_device_id(char *devID);

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
int zts_get_peer_address(char *peer, const char *devID);

/**
 * @brief Allow or disallow this instance of libzt to be controlled via HTTP requests
 *
 * @usage Call this after zts_start() has succeeded
 * @param allowed True or false value
 * @return
 */
void zts_allow_http_control(bool allowed);

/**
 * @brief Returns whether one can add a new socket or not. This depends on network stack in use.
 *
 * @usage Call this after zts_start() has succeeded
 * @param socket_type
 * @return
 */
bool can_provision_new_socket(int socket_type);

/**
 * @brief Returns the number of VirtualSockets either already provisioned or waiting to be
 * Some network stacks may have a limit on the number of sockets that they can
 * safely handle due to timer construction, this is a way to check that we
 * haven't passed that limit. Someday if multiple stacks are used simultaneously
 * the logic for this function should change accordingly.
 *
 * @usage Call this after zts_start() has succeeded
 * @return
 */
int zts_num_active_virt_sockets();

/**
 * @brief Return the maximum number of sockets allowable by platform/stack configuration
 *
 * @usage Call this after zts_start() has succeeded
 * @param socket_type
 * @return
 */
int zts_maxsockets(int socket_type);

/**
 * @brief Return the number of currently active picoTCP timers
 *
 * @usage Call this after zts_start() has succeeded
 * @return
 */
int pico_ntimers();

#ifdef __cplusplus
}
#endif

#endif // _H
