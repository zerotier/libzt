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

/**
 * @brief Initialize network stack semaphores, threads, and timers.
 *
 * @param 
 * @return 
 */
void lwip_driver_init();

/**
 * @brief Set up an interface in the network stack for the VirtualTap.
 *
 * @param 
 * @return 
 */
void lwip_init_interface(void *tapref, const ZeroTier::MAC &mac, const ZeroTier::InetAddress &ip);

/**
 * @brief Called from the stack, outbound ethernet frames from the network stack enter the ZeroTier virtual wire here.
 *
 * @param 
 * @return 
 */
err_t lwip_eth_tx(struct netif *netif, struct pbuf *p);

/**
 * @brief Packets from the ZeroTier virtual wire enter the stack here.
 *
 * @param 
 * @return 
 */
void lwip_eth_rx(ZeroTier::VirtualTap *tap, const ZeroTier::MAC &from, const ZeroTier::MAC &to, unsigned int etherType, 
	const void *data, unsigned int len);

#endif
