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
 * lwIP network stack driver
 */

#ifndef LIBZT_LWIP_DRIVER_HPP
#define LIBZT_LWIP_DRIVER_HPP

#include "Debug.hpp"
#include "lwip/err.h"

namespace ZeroTier {

class MAC;
class Mutex;
class VirtualTap;
struct InetAddress;

/**
 * @brief Structure used to associate packets with interfaces.
 */
struct zts_sorted_packet
{
	// lwIP pbuf containing packet (originally encapsulated by ZT packet)
	struct pbuf *p;
	// ZT VirtualTap from which this packet originates
	VirtualTap *vtap;
	// lwIP netif we should accept this packet on
	struct netif *n;
};

/**
 * @brief Return whether a given netif's NETIF_FLAG_UP flag is set
 *
 * @usage This is a convenience function to encapsulate a macro
 */
bool lwip_is_netif_up(void *netif);

/**
 * @brief Increase the delay multiplier for the main driver loop
 *
 * @usage This should be called when we know the stack won't be used by any virtual taps
 */
void lwip_hibernate_driver();

/**
 * @brief Decrease the delay multiplier for the main driver loop
 *
 * @usage This should be called when at least one virtual tap is active
 */
void lwip_wake_driver();

/**
 * Returns whether the lwIP network stack is up and ready to process traffic
 */
bool lwip_is_up();

/**
 * @brief Initialize network stack semaphores, threads, and timers.
 *
 * @usage This is called during the initial setup of each VirtualTap but is only allowed to execute once
 * @return
 */
void lwip_driver_init();

/**
 * @brief Shutdown the stack as completely as possible (not officially supported by lwIP)
 *
 * @usage This is to be called after it is determined that no further network activity will take place.
 * The tcpip thread will be stopped, all interfaces will be brought down and all resources will 
 * be deallocated. A full application restart will be required to bring the stack back online.
 * @return
 */
void lwip_driver_shutdown();

/**
 * @brief Requests that a netif be brought down and removed.
 *
 * @return
 */
void lwip_remove_netif(void *netif);

/**
 * @brief Initialize and start the DNS client
 *
 * @usage Called after lwip_driver_init()
 * @return
 */
void lwip_dns_init();

/**
 * @brief Starts DHCP timers
 *
 * @usage lwip_driver_init()
 * @return
 */
void lwip_start_dhcp(void *netif);

/**
 * @brief Called when the status of a netif changes:
 *  - Interface is up/down (ZTS_EVENT_NETIF_UP, ZTS_EVENT_NETIF_DOWN)
 *  - Address changes while up (ZTS_EVENT_NETIF_NEW_ADDRESS)
 */
#if LWIP_NETIF_STATUS_CALLBACK
static void netif_status_callback(struct netif *netif);
#endif

/**
 * @brief Called when a netif is removed (ZTS_EVENT_NETIF_INTERFACE_REMOVED)
 */
#if LWIP_NETIF_REMOVE_CALLBACK
static void netif_remove_callback(struct netif *netif);
#endif

/**
 * @brief Called when a link is brought up or down (ZTS_EVENT_NETIF_LINK_UP, ZTS_EVENT_NETIF_LINK_DOWN)
 */
#if LWIP_NETIF_LINK_CALLBACK
static void netif_link_callback(struct netif *netif);
#endif

/**
 * @brief Set up an interface in the network stack for the VirtualTap.
 *
 * @param
 * @param tapref Reference to VirtualTap that will be responsible for sending and receiving data
 * @param mac Virtual hardware address for this ZeroTier VirtualTap interface
 * @param ip Virtual IP address for this ZeroTier VirtualTap interface
 * @return
 */
void lwip_init_interface(void *tapref, const MAC &mac, const InetAddress &ip);

/**
 * @brief Called from the stack, outbound ethernet frames from the network stack enter the ZeroTier virtual wire here.
 *
 * @usage This shall only be called from the stack or the stack driver. Not the application thread.
 * @param netif Transmits an outgoing Ethernet fram from the network stack onto the ZeroTier virtual wire
 * @param p A pointer to the beginning of a chain pf struct pbufs
 * @return
 */
err_t lwip_eth_tx(struct netif *netif, struct pbuf *p);

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
 * @return
 */
void lwip_eth_rx(VirtualTap *tap, const MAC &from, const MAC &to, unsigned int etherType,
	const void *data, unsigned int len);

} // namespace ZeroTier

#endif // _H
