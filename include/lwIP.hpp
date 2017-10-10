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
 * lwIP network stack driver
 */

#ifndef ZT_LWIP_HPP
#define ZT_LWIP_HPP

#include "MAC.hpp"
#include "InetAddress.hpp"
#include "Defs.h"
#include "Mutex.hpp"

/**
 * @brief Initialize network stack semaphores, threads, and timers.
 *
 * @usage This is called during the initial setup of each VirtualTap but is only allowed to execute once
 * @return
 */
void lwip_driver_init();

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

void general_lwip_init_interface(void *tapref, void *netif, const char *name, const ZeroTier::MAC &mac,
	const ZeroTier::InetAddress &addr, const ZeroTier::InetAddress &nm, const ZeroTier::InetAddress &gw);

void general_turn_on_interface(void *netif);

/**
 * @brief Set up an interface in the network stack for the VirtualTap.
 *
 * @param
 * @param tapref Reference to VirtualTap that will be responsible for sending and receiving data
 * @param mac Virtual hardware address for this ZeroTier VirtualTap interface
 * @param ip Virtual IP address for this ZeroTier VirtualTap interface
 * @return
 */
void lwip_init_interface(void *tapref, const ZeroTier::MAC &mac, const ZeroTier::InetAddress &ip);

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
void lwip_eth_rx(ZeroTier::VirtualTap *tap, const ZeroTier::MAC &from, const ZeroTier::MAC &to, unsigned int etherType,
	const void *data, unsigned int len);

#endif
