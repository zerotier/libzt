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
 * Header for ZeroTier service controls
 */

#ifndef LIBZT_SERVICE_CONTROLS_HPP
#define LIBZT_SERVICE_CONTROLS_HPP

#ifdef _WIN32
	#ifdef ADD_EXPORTS
		#define ZT_SOCKET_API __declspec(dllexport)
	#else
		#define ZT_SOCKET_API __declspec(dllimport)
	#endif
	#define ZTCALL __cdecl
#else
	#define ZT_SOCKET_API
	#define ZTCALL
#endif

void api_sleep(int interval_ms);

//////////////////////////////////////////////////////////////////////////////
// ZeroTier Service Controls                                                //
//////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief (optional) Sets the port for the background libzt service. If this function is called
 * with a port number between 1-65535 it will attempt to bind to that port. If it is called with
 * a port number of 0 it will attempt to randomly search for an available port. If this function
 * is never called, the service will try to bind on LIBZT_DEFAULT_PORT which is 9994.
 *
 * @usage Should be called at the beginning of your application before `zts_startjoin()`
 * @param portno Port number
 * @return 0 if successful; or -1 if failed
 */
ZT_SOCKET_API int ZTCALL zts_set_service_port(int portno);

/**
 * @brief (optional) Returns the port number used by the ZeroTier service
 * @usage Can be called if a port number was previously assigned
 * @return the port number used by the ZeroTier service
 */
ZT_SOCKET_API int ZTCALL zts_get_service_port();

/**
 * @brief Starts the ZeroTier service
 *
 * @usage Should be called at the beginning of your application. Will blocks until all of the following conditions are met:
 * - ZeroTier core service has been initialized
 * - Cryptographic identity has been generated or loaded from directory specified by `path`
 * - Virtual network is successfully joined
 * - IP address is assigned by network controller service
 * @param path path directory where cryptographic identities and network configuration files are stored and retrieved
 *              (`identity.public`, `identity.secret`)
 * @param blocking whether or not this call will block until the entire service is up and running
 * @return 0 if successful; or 1 if failed
 */
ZT_SOCKET_API int ZTCALL zts_start(const char *path, int blocking);

/**
 * @brief Starts the ZeroTier service
 *
 * @usage Should be called at the beginning of your application. Will blocks until all of the following conditions are met:
 * - ZeroTier core service has been initialized
 * - Cryptographic identity has been generated or loaded from directory specified by `path`
 * - Virtual network is successfully joined
 * - IP address is assigned by network controller service
 * @param path path directory where cryptographic identities and network configuration files are stored and retrieved
 *              (`identity.public`, `identity.secret`)
 * @param nwid A 16-digit hexidecimal network identifier (e.g. Earth: `8056c2e21c000001`)
 * @return 0 if successful; or 1 if failed
 */
ZT_SOCKET_API int ZTCALL zts_startjoin(const char *path, const uint64_t nwid);

/**
 * @brief Stops the ZeroTier service, brings down all virtual interfaces in order to stop all traffic processing.
 *
 * @usage This should be called when the application anticipates not needing any sort of traffic processing for a
 * prolonged period of time. The stack driver (with associated timers) will remain active in case future traffic
 * processing is required. Note that the application must tolerate a multi-second startup time if zts_start()
 * zts_startjoin() is called again. To stop this background thread and free all resources use zts_free() instead.
 * @param blocking whether or not this call will block until the entire service is torn down
 * @return Returns 0 on success, -1 on failure
 */
ZT_SOCKET_API int ZTCALL zts_stop(int blocking = 1);

/**
 * @brief Stops all background services, brings down all interfaces, frees all resources. After calling this function
 * an application restart will be required before the library can be used again. This is a blocking call.
 *
 * @usage This should be called at the end of your program or when you do not anticipate communicating over ZeroTier
 * @return Returns 0 on success, -1 on failure
 */
ZT_SOCKET_API int ZTCALL zts_free();

/**
 * @brief Return whether the ZeroTier service is currently running
 *
 * @usage Call this after zts_start()
 * @return 1 if running, 0 if not running
 */
ZT_SOCKET_API int ZTCALL zts_core_running();

/**
 * @brief Return whether libzt is ready to handle socket API calls. Alternatively you could 
 * have just called zts_startjoin(path, nwid)
 *
 * @usage Call this after zts_start()
 * @return 1 if running, 0 if not running
 */
ZT_SOCKET_API int ZTCALL zts_ready();

/**
 * @brief Return the number of networks currently joined by this node
 *
 * @usage Call this after zts_start(), zts_startjoin() and/or zts_join()
 * @return Number of networks joined by this node
 */
ZT_SOCKET_API zts_err_t ZTCALL zts_get_num_joined_networks();

/**
 * @brief Populates a structure with details for a given network
 *
 * @usage Call this from the application thread any time after the node has joined a network
 * @param nwid A 16-digit hexidecimal virtual network ID
 * @param nd Pointer to a zts_network_details structure to populate
 * @return ZTS_ERR_SERVICE if failed, 0 if otherwise
 */
ZT_SOCKET_API zts_err_t ZTCALL zts_get_network_details(uint64_t nwid, struct zts_network_details *nd);

/**
 * @brief Populates an array of structures with details for any given number of networks
 *
 * @usage Call this from the application thread any time after the node has joined a network
 * @param nds Pointer to an array of zts_network_details structures to populate
 * @param num Number of zts_network_details structures available to copy data into, will be updated
 * to reflect number of structures that were actually populated
 * @return ZTS_ERR_SERVICE if failed, 0 if otherwise
 */
ZT_SOCKET_API zts_err_t ZTCALL zts_get_all_network_details(struct zts_network_details *nds, int *num);

/**
 * @brief Join a network
 *
 * @usage Call this from application thread. Only after zts_start() has succeeded
 * @param nwid A 16-digit hexidecimal virtual network ID
 * @return 0 if successful, -1 for any failure
 */
ZT_SOCKET_API zts_err_t ZTCALL zts_join(const uint64_t nwid, int blocking = 1);

/**
 * @brief Leave a network
 *
 * @usage Call this from application thread. Only after zts_start() has succeeded
 * @param nwid A 16-digit hexidecimal virtual network ID
 * @return 0 if successful, -1 for any failure
 */
ZT_SOCKET_API zts_err_t ZTCALL zts_leave(const uint64_t nwid, int blocking = 1);


/**
 * @brief Leaves all networks
 *
 * @usage Call this from application thread. Only after zts_start() has succeeded
 * @param nwid A 16-digit hexidecimal virtual network ID
 * @return 0 if successful, -1 for any failure
 */
ZT_SOCKET_API zts_err_t ZTCALL zts_leave_all(int blocking = 1);

/**
 * @brief Orbits a given moon (user-defined root server)
 *
 * @usage Call this from application thread. Only after zts_start() has succeeded
 * @param moonWorldId A 16-digit hexidecimal world ID
 * @param moonSeed A 16-digit hexidecimal seed ID
 * @return ZTS_ERR_OK if successful, ZTS_ERR_SERVICE, ZTS_ERR_INVALID_ARG, ZTS_ERR_INVALID_OP if otherwise
 */
ZT_SOCKET_API zts_err_t ZTCALL zts_orbit(uint64_t moonWorldId, uint64_t moonSeed);

/**
 * @brief De-orbits a given moon (user-defined root server)
 *
 * @usage Call this from application thread. Only after zts_start() has succeeded
 * @param moonWorldId A 16-digit hexidecimal world ID
 * @return ZTS_ERR_OK if successful, ZTS_ERR_SERVICE, ZTS_ERR_INVALID_ARG, ZTS_ERR_INVALID_OP if otherwise
 */
ZT_SOCKET_API zts_err_t ZTCALL zts_deorbit(uint64_t moonWorldId);

/**
 * @brief Copies the configuration path used by ZeroTier into the provided buffer
 *
 * @usage Use this to determine where ZeroTier is storing identity files
 * @param homePath Path to ZeroTier configuration files
 * @param len Length of destination buffer
 * @return 0 if no error, -1 if invalid argument was supplied
 */
ZT_SOCKET_API zts_err_t ZTCALL zts_get_path(char *homePath, size_t *len);

/**
 * @brief Returns the node ID of this instance
 *
 * @usage Call this after zts_start() and/or when zts_running() returns true
 * @return
 */
ZT_SOCKET_API uint64_t ZTCALL zts_get_node_id();

/**
 * @brief Returns whether any address has been assigned to the SockTap for this network
 *
 * @usage This is used as an indicator of readiness for service for the ZeroTier core and stack
 * @param nwid Network ID
 * @return
 */
ZT_SOCKET_API int ZTCALL zts_has_address(const uint64_t nwid);


/**
 * @brief Returns the number of addresses assigned to this node for the given nwid
 *
 * @param nwid Network ID
 * @return The number of addresses assigned
 */
ZT_SOCKET_API int ZTCALL zts_get_num_assigned_addresses(const uint64_t nwid);

/**
 * @brief Returns the assigned address located at the given index
 *
 * @usage The indices of each assigned address are not guaranteed and should only
 * be used for iterative purposes.
 * @param nwid Network ID
 * @param index location of assigned address
 * @return The number of addresses assigned
 */
ZT_SOCKET_API int ZTCALL zts_get_address_at_index(
	const uint64_t nwid, const int index, struct sockaddr *addr, socklen_t *addrlen);

/**
 * @brief Get IP address for this device on a given network
 *
 * @usage FIXME: Only returns first address found, good enough for most cases
 * @param nwid Network ID
 * @param addr Destination structure for address
 * @param addrlen size of destination address buffer, will be changed to size of returned address
 * @return 0 if an address was successfully found, -1 if failure
 */
ZT_SOCKET_API int ZTCALL zts_get_address(
	const uint64_t nwid, struct sockaddr_storage *addr, const int address_family);

/**
 * @brief Computes a 6PLANE IPv6 address for the given Network ID and Node ID
 *
 * @usage Can call any time
 * @param addr Destination structure for address
 * @param nwid Network ID 
 * @param nodeId Node ID
 * @return
 */
ZT_SOCKET_API void ZTCALL zts_get_6plane_addr(
	struct sockaddr_storage *addr, const uint64_t nwid, const uint64_t nodeId);

/**
 * @brief Computes a RFC4193 IPv6 address for the given Network ID and Node ID
 *
 * @usage Can call any time
 * @param addr Destination structure for address
 * @param nwid Network ID 
 * @param nodeId Node ID
 * @return
 */
ZT_SOCKET_API void ZTCALL zts_get_rfc4193_addr(
	struct sockaddr_storage *addr, const uint64_t nwid, const uint64_t nodeId);

/**
 * @brief Return the number of peers
 *
 * @usage Call this after zts_start() has succeeded
 * @return
 */
ZT_SOCKET_API zts_err_t zts_get_peer_count();

ZT_SOCKET_API zts_err_t zts_get_peers(struct zts_peer_details *pds, int *num);

/**
 * @brief Enables the HTTP backplane management system
 *
 * @usage Call this after zts_start() has succeeded
 * @return ZTS_ERR_OK if successful, ZTS_ERR_SERVICE if otherwise
 */
ZT_SOCKET_API zts_err_t zts_enable_http_backplane_mgmt();

/**
 * @brief Disables the HTTP backplane management system
 *
 * @usage Call this after zts_start() has succeeded
 * @return ZTS_ERR_OK if successful, ZTS_ERR_SERVICE, ZTS_ERR_INVALID_OP if otherwise
 */
ZT_SOCKET_API zts_err_t zts_disable_http_backplane_mgmt();


/**
 * @brief Starts a ZeroTier service in the background
 *
 * @usage For internal use only.
 * @param
 * @return
 */
#if defined(_WIN32)
DWORD WINAPI _zts_start_service(LPVOID thread_id);
#else
void *_zts_start_service(void *thread_id);
#endif

/**
 * @brief [Should not be called from user application] This function must be surrounded by 
 * ZT service locks. It will determine if it is currently safe and allowed to operate on 
 * the service.
 * @usage Can be called at any time
 * @return 1 or 0
 */
int _zts_can_perform_service_operation();

/**
 * @brief [Should not be called from user application] Returns whether or not the node is 
 * online.
 * @usage Can be called at any time
 * @return 1 or 0
 */
int _zts_node_online();

/**
 * @brief [Should not be called from user application] Adjusts the delay multiplier for the
 * network stack driver thread.
 * @usage Can be called at any time
 */
void _hibernate_if_needed();

#ifdef __cplusplus
}
#endif

#endif // _H