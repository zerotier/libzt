/*
 * Copyright (c)2013-2021 ZeroTier, Inc.
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file in the project's root directory.
 *
 * Change Date: 2025-01-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2.0 of the Apache License.
 */
/****/

/**
 * @file
 *
 * Header for virtual ethernet tap device and combined network stack driver
 */

#ifndef ZT_VIRTUAL_TAP_HPP
#define ZT_VIRTUAL_TAP_HPP

#include "lwip/err.h"

#define ZTS_LWIP_DRIVER_THREAD_NAME "ZTNetworkStackThread"

#include "MAC.hpp"
#include "Phy.hpp"
#include "Thread.hpp"

namespace ZeroTier {

class Mutex;
class MAC;
class MulticastGroup;
struct InetAddress;

/**
 * A virtual tap device. The ZeroTier Node Service will create one per
 * joined network. It will be destroyed upon leave().
 */
class VirtualTap {
	friend class Phy<VirtualTap*>;

  public:
	VirtualTap(
	    const char* homePath,
	    const MAC& mac,
	    unsigned int mtu,
	    unsigned int metric,
	    uint64_t nwid,
	    const char* friendlyName,
	    void (*handler)(
	        void*,
	        void*,
	        uint64_t,
	        const MAC&,
	        const MAC&,
	        unsigned int,
	        unsigned int,
	        const void*,
	        unsigned int),
	    void* arg);

	~VirtualTap();

	void setEnabled(bool en);
	bool enabled() const;

	/**
	 * Mutex for protecting IP address container for this tap.
	 */
	Mutex _ips_m;   // Public because we want it accessible by the driver layer

	/**
	 * Return whether this tap has been assigned an IPv4 address.
	 */
	bool hasIpv4Addr();

	/**
	 * Return whether this tap has been assigned an IPv6 address.
	 */
	bool hasIpv6Addr();

	/**
	 * Adds an address to the user-space stack interface associated with this VirtualTap
	 * - Starts VirtualTap main thread ONLY if successful
	 */
	bool addIp(const InetAddress& ip);

	/**
	 * Removes an address from the user-space stack interface associated with this VirtualTap
	 */
	bool removeIp(const InetAddress& ip);

	/**
	 * Presents data to the user-space stack
	 */
	void
	put(const MAC& from, const MAC& to, unsigned int etherType, const void* data, unsigned int len);

	/**
	 * Get VirtualTap device name (e.g. 'libzt17d72843bc2c5760')
	 */
	std::string deviceName() const;

	/**
	 * Set friendly name
	 */
	void setFriendlyName(const char* friendlyName);

	/**
	 * Scan multicast groups
	 */
	void
	scanMulticastGroups(std::vector<MulticastGroup>& added, std::vector<MulticastGroup>& removed);

	/**
	 * Set MTU
	 */
	void setMtu(unsigned int mtu);

	/**
	 * Calls main network stack loops
	 */
	void threadMain() throw();

	/**
	 * For moving data onto the ZeroTier virtual wire
	 */
	void (*_handler)(
	    void*,
	    void*,
	    uint64_t,
	    const MAC&,
	    const MAC&,
	    unsigned int,
	    unsigned int,
	    const void*,
	    unsigned int);

	void* netif4 = NULL;
	void* netif6 = NULL;

	/**
	 * The last time that this virtual tap received a network config update from the core
	 */
	uint64_t _lastConfigUpdateTime = 0;

	void lastConfigUpdate(uint64_t lastConfigUpdateTime);

	int _networkStatus = 0;

	char vtap_full_name[64];

	std::vector<InetAddress> ips() const;
	std::vector<InetAddress> _ips;

	std::string _homePath;
	void* _arg;
	volatile bool _initialized;
	volatile bool _enabled;
	volatile bool _run;
	MAC _mac;
	unsigned int _mtu;
	uint64_t _nwid;
	PhySocket* _unixListenSocket;
	Phy<VirtualTap*> _phy;

	Thread _thread;

	int _shutdownSignalPipe[2];

	std::string _dev;   // path to Unix domain socket

	std::vector<MulticastGroup> _multicastGroups;
	Mutex _multicastGroups_m;

	//////////////////////////////////////////////////////////////////////////////
	// Not used in this implementation                                          //
	//////////////////////////////////////////////////////////////////////////////

	void phyOnDatagram(
	    PhySocket* sock,
	    void** uptr,
	    const struct sockaddr* local_address,
	    const struct sockaddr* from,
	    void* data,
	    unsigned long len);
	void phyOnTcpConnect(PhySocket* sock, void** uptr, bool success);
	void phyOnTcpAccept(
	    PhySocket* sockL,
	    PhySocket* sockN,
	    void** uptrL,
	    void** uptrN,
	    const struct sockaddr* from);
	void phyOnTcpClose(PhySocket* sock, void** uptr);
	void phyOnTcpData(PhySocket* sock, void** uptr, void* data, unsigned long len);
	void phyOnTcpWritable(PhySocket* sock, void** uptr);
	void phyOnUnixClose(PhySocket* sock, void** uptr);
};

/**
 * @brief Return whether a given netif's NETIF_FLAG_UP flag is set
 *
 * @usage This is a convenience function to encapsulate a macro
 */
bool _lwip_is_netif_up(void* netif);

/**
 * @brief Increase the delay multiplier for the main driver loop
 *
 * @usage This should be called when we know the stack won't be used by any virtual taps
 */
void _lwip_hibernate_driver();

/**
 * @brief Decrease the delay multiplier for the main driver loop
 *
 * @usage This should be called when at least one virtual tap is active
 */
void _lwip_wake_driver();

/**
 * Returns whether the lwIP network stack is up and ready to process traffic
 */
bool _lwip_is_up();

/**
 * @brief Initialize network stack semaphores, threads, and timers.
 *
 * @usage This is called during the initial setup of each VirtualTap but is only allowed to execute
 * once
 */
void _lwip_driver_init();

/**
 * @brief Shutdown the stack as completely as possible (not officially supported by lwIP)
 *
 * @usage This is to be called after it is determined that no further network activity will take
 * place. The tcpip thread will be stopped, all interfaces will be brought down and all resources
 * will be deallocated. A full application restart will be required to bring the stack back online.
 */
void _lwip_driver_shutdown();

/**
 * @brief Requests that a netif be brought down and removed.
 */
void _lwip_remove_netif(void* netif);

/**
 * @brief Starts DHCP timers
 */
void _lwip_start_dhcp(void* netif);

/**
 * @brief Called when the status of a netif changes:
 *  - Interface is up/down (ZTS_EVENT_NETIF_UP, ZTS_EVENT_NETIF_DOWN)
 *  - Address changes while up (ZTS_EVENT_NETIF_NEW_ADDRESS)
 */
#if LWIP_NETIF_STATUS_CALLBACK
static void _netif_status_callback(struct netif* netif);
#endif

/**
 * @brief Called when a netif is removed (ZTS_EVENT_NETIF_INTERFACE_REMOVED)
 */
#if LWIP_NETIF_REMOVE_CALLBACK
static void _netif_remove_callback(struct netif* netif);
#endif

/**
 * @brief Called when a link is brought up or down (ZTS_EVENT_NETIF_LINK_UP,
 * ZTS_EVENT_NETIF_LINK_DOWN)
 */
#if LWIP_NETIF_LINK_CALLBACK
static void _netif_link_callback(struct netif* netif);
#endif

/**
 * @brief Set up an interface in the network stack for the VirtualTap.
 *
 * @param tapref Reference to VirtualTap that will be responsible for sending and receiving data
 * @param ip Virtual IP address for this ZeroTier VirtualTap interface
 */
void _lwip_init_interface(void* tapref, const InetAddress& ip);

/**
 * @brief Remove an assigned address from an lwIP netif
 *
 * @param tapref Reference to VirtualTap
 * @param ip Virtual IP address to remove from this interface
 */
void _lwip_remove_address_from_netif(void* tapref, const InetAddress& ip);

/**
 * @brief Called from the stack, outbound ethernet frames from the network stack enter the ZeroTier
 * virtual wire here.
 *
 * @usage This shall only be called from the stack or the stack driver. Not the application thread.
 * @param netif Transmits an outgoing Ethernet fram from the network stack onto the ZeroTier virtual
 * wire
 * @param p A pointer to the beginning of a chain pf struct pbufs
 * @return
 */
err_t _lwip_eth_tx(struct netif* netif, struct pbuf* p);

/**
 * @brief Receives incoming Ethernet frames from the ZeroTier virtual wire
 *
 * @usage This shall be called from the VirtualTap's I/O thread (via VirtualTap::put())
 * @param tap Pointer to VirtualTap from which this data comes
 * @param from Origin address (virtual ZeroTier hardware address)
 * @param to Intended destination address (virtual ZeroTier hardware address)
 * @param etherType Protocol type
 * @param data Pointer to Ethernet frame
 * @param len Length of Ethernet frame
 */
void _lwip_eth_rx(
    VirtualTap* tap,
    const MAC& from,
    const MAC& to,
    unsigned int etherType,
    const void* data,
    unsigned int len);

}   // namespace ZeroTier

#endif   // _H
