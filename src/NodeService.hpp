/*
 * Copyright (c)2013-2020 ZeroTier, Inc.
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file in the project's root directory.
 *
 * Change Date: 2024-01-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2.0 of the Apache License.
 */
/****/

/**
 * @file
 *
 * Header for ZeroTier Node Service (a distant relative of OneService)
 */

#ifndef ZT_NODE_SERVICE_HPP
#define ZT_NODE_SERVICE_HPP

#include <string>
#include <vector>

#include "Node.hpp"
#include "InetAddress.hpp"
#include "Mutex.hpp"
#include "ZeroTierSockets.h"

#define ZTS_SERVICE_THREAD_NAME           "ZTServiceThread"
#define ZTS_EVENT_CALLBACK_THREAD_NAME    "ZTEventCallbackThread"
// Interface metric for ZeroTier taps -- this ensures that if we are on WiFi and also
// bridged via ZeroTier to the same LAN traffic will (if the OS is sane) prefer WiFi.
#define ZT_IF_METRIC                      5000
// How often to check for new multicast subscriptions on a tap device
#define ZT_TAP_CHECK_MULTICAST_INTERVAL   5000
// How often to check for local interface addresses
#define ZT_LOCAL_INTERFACE_CHECK_INTERVAL 60000

#ifdef __WINDOWS__
#include <Windows.h>
#endif

namespace ZeroTier {

/**
 * Local service for ZeroTier One as system VPN/NFV provider
 */
class NodeService
{
public:

	uint16_t _userProvidedPort;
	std::string _userProvidedPath;

	/**
	 * Returned by node main if/when it terminates
	 */
	enum ReasonForTermination
	{
		/**
		 * Instance is still running
		 */
		ONE_STILL_RUNNING = 0,

		/**
		 * Normal shutdown
		 */
		ONE_NORMAL_TERMINATION = 1,

		/**
		 * A serious unrecoverable error has occurred
		 */
		ONE_UNRECOVERABLE_ERROR = 2,

		/**
		 * Your identity has collided with another
		 */
		ONE_IDENTITY_COLLISION = 3
	};

	/**
	 * Local settings for each network
	 */
	struct NetworkSettings
	{
		/**
		 * Allow this network to configure IP addresses and routes?
		 */
		bool allowManaged;

		/**
		 * Whitelist of addresses that can be configured by this network.
		 * If empty and allowManaged is true, allow all private/pseudoprivate addresses.
		 */
		std::vector<InetAddress> allowManagedWhitelist;

		/**
		 * Allow configuration of IPs and routes within global (Internet) IP space?
		 */
		bool allowGlobal;

		/**
		 * Allow overriding of system default routes for "full tunnel" operation?
		 */
		bool allowDefault;
	};

	/**
	 * @return Platform default home path or empty string if this platform doesn't have one
	 */
	static std::string platformDefaultHomePath();

	/**
	 * Create a new instance of the service
	 *
	 * Once created, you must call the run() method to actually start
	 * processing.
	 *
	 * The port is saved to a file in the home path called zerotier-one.port,
	 * which is used by the CLI and can be used to see which port was chosen if
	 * 0 (random port) is picked.
	 *
	 * @param hp Home path
	 * @param port TCP and UDP port for packets and HTTP control (if 0, pick random port)
	 */
	static NodeService *newInstance(const char *hp,unsigned int port);

	virtual ~NodeService();

	/**
	 * Execute the service main I/O loop until terminated
	 *
	 * The terminate() method may be called from a signal handler or another
	 * thread to terminate execution. Otherwise this will not return unless
	 * another condition terminates execution such as a fatal error.
	 */
	virtual ReasonForTermination run() = 0;

	/**
	 * @return Reason for terminating or ONE_STILL_RUNNING if running
	 */
	virtual ReasonForTermination reasonForTermination() const = 0;

	/**
	 * @return Fatal error message or empty string if none
	 */
	virtual std::string fatalErrorMessage() const = 0;

	/**
	 * @return System device name corresponding with a given ZeroTier network ID or empty string if not opened yet or network ID not found
	 */
	virtual std::string portDeviceName(uint64_t nwid) const = 0;

	/**
	 * Whether we allow access to the service via local HTTP requests (disabled by default in libzt)
	 */
	bool allowHttpBackplaneManagement = false;

	/**
	 * @return Reference to the Node
	 */
	virtual Node * getNode() = 0;

	/**
	 * Fills out a structure with network-specific route information
	 */
	virtual void getRoutes(uint64_t nwid, void *routeArray, unsigned int *numRoutes) = 0;

	virtual size_t networkCount() = 0;
	virtual void leaveAll() = 0;
	virtual void join(uint64_t nwid) = 0;
	virtual void leave(uint64_t nwid) = 0;
	virtual int getPeerStatus(uint64_t id) = 0;
	
	/**
	 * Terminate background service (can be called from other threads)
	 */
	virtual void terminate() = 0;

	/**
	 * Get local settings for a network
	 *
	 * @param nwid Network ID
	 * @param settings Buffer to fill with local network settings
	 * @return True if network was found and settings is filled
	 */
	virtual bool getNetworkSettings(const uint64_t nwid,NetworkSettings &settings) const = 0;

	/**
	 * @return True if service is still running
	 */
	inline bool isRunning() const { return (this->reasonForTermination() == ONE_STILL_RUNNING); }

protected:
	NodeService() {}

private:
	NodeService(const NodeService &one) {}
	inline NodeService &operator=(const NodeService &one) { return *this; }
};

struct serviceParameters
{
	int port;
	std::string path;
};

#ifdef __WINDOWS__
DWORD WINAPI _runNodeService(LPVOID arg);
#else
/**
 * NodeService thread
 */
void *_runNodeService(void *arg);
#endif

} // namespace ZeroTier

#endif
