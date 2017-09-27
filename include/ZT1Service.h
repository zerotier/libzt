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
 * @param nwid
 * @return 
 */
std::vector<ZT_VirtualNetworkRoute> *zts_get_network_routes(char *nwid);

/**
 * @brief 
 * 
 * @usage For internal use only.
 * @param 
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
 * @brief 
 * 
 * @usage For internal use only.
 * @param 
 * @return 
 */
void disableTaps();

/**
 * @brief 
 * 
 * @usage For internal use only.
 * @param 
 * @return 
 */
void zts_get_ipv4_address(const char *nwid, char *addrstr, const int addrlen);

/**
 * @brief 
 * 
 * @usage For internal use only.
 * @param 
 * @return 
 */
void zts_get_ipv6_address(const char *nwid, char *addrstr, const int addrlen);

/**
 * @brief 
 * 
 * @usage For internal use only.
 * @param 
 * @return 
 */
int zts_has_ipv4_address(const char *nwid);

/**
 * @brief 
 * 
 * @usage For internal use only.
 * @param 
 * @return 
 */
int zts_has_ipv6_address(const char *nwid);

/**
 * @brief 
 * 
 * @usage For internal use only.
 * @param 
 * @return 
 */
int zts_has_address(const char *nwid);

/**
 * @brief 
 * 
 * @usage 
 * @param 
 * @return 
 */
void zts_get_6plane_addr(char *addr, const char *nwid, const char *devID);

/**
 * @brief 
 * 
 * @usage 
 * @param 
 * @return 
 */
void zts_get_rfc4193_addr(char *addr, const char *nwid, const char *devID);

/**
 * @brief 
 * 
 * @usage 
 * @param 
 * @return 
 */
void zts_join(const char * nwid);

/**
 * @brief 
 * 
 * @usage For internal use only.
 * @param 
 * @return 
 */
void zts_join_soft(const char * filepath, const char * nwid);

/**
 * @brief 
 * 
 * @usage 
 * @param 
 * @return 
 */
void zts_leave(const char * nwid); 

/**
 * @brief 
 * 
 * @usage For internal use only.
 * @param 
 * @return 
 */
void zts_leave_soft(const char * filepath, const char * nwid); 

/**
 * @brief 
 * 
 * @usage 
 * @param 
 * @return 
 */
int zts_running();

/**
 * @brief 
 * 
 * @usage 
 * @param 
 * @return 
 */
void zts_start(const char *path);

/**
 * @brief 
 * 
 * @usage 
 * @param 
 * @return 
 */
void zts_simple_start(const char *path, const char *nwid);

/**
 * @brief 
 * 
 * @usage 
 * @param 
 * @return 
 */
void zts_stop(); 

/**
 * @brief 
 * 
 * @usage 
 * @param 
 * @return 
 */void zts_get_homepath(char *homePath, int len); 

/**
 * @brief 
 * 
 * @usage 
 * @param 
 * @return 
 */
void zts_core_version(char *ver); 

/**
 * @brief 
 * 
 * @usage 
 * @param 
 * @return 
 */
void zts_lib_version(char *ver); 

/**
 * @brief 
 * 
 * @usage 
 * @param 
 * @return 
 */
int zts_get_device_id(char *devID); 

/**
 * @brief 
 * 
 * @usage 
 * @param 
 * @return 
 */
unsigned long zts_get_peer_count();

/**
 * @brief 
 * 
 * @usage 
 * @param 
 * @return 
 */
int zts_get_peer_address(char *peer, const char *devID);

/**
 * @brief 
 * 
 * @usage 
 * @param 
 * @return 
 */
void zts_enable_http_control_plane();

/**
 * @brief 
 * 
 * @usage 
 * @param 
 * @return 
 */
void zts_disable_http_control_plane();

/**
 * @brief Whether we can add a new socket or not. Depends on stack in use
 * 
 * @usage 
 * @param socket_type
 * @return 
 */
bool can_provision_new_socket(int socket_type);

/**
 * @brief Returns the number of sockets either already provisioned or waiting to be
 * Some network stacks may have a limit on the number of sockets that they can
 * safely handle due to timer construction, this is a way to check that we
 * haven't passed that limit. Someday if multiple stacks are used simultaneously
 * the logic for this function should change accordingly.
 * 
 * @usage 
 * @param 
 * @return 
 */
int zts_num_active_virt_sockets();

/**
 * @brief Returns maximum number of sockets allowed by network stack
 * 
 * @usage 
 * @param socket_type
 * @return 
 */
int zts_maxsockets(int socket_type);

/**
 * @brief 
 * 
 * @usage 
 * @param 
 * @return 
 */
int pico_ntimers();

#endif // ZT1SERVICE_H

#ifdef __cplusplus
}
#endif