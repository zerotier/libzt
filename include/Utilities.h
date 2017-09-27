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
 * Misc utilities
 */

#ifndef LIBZT_UTILITIES_H
#define LIBZT_UTILITIES_H

#include "InetAddress.hpp"

/**
 * @brief Returns masked address for subnet comparisons
 *
 * @usage For internal use only.
 * @param socket_type
 * @return
 */
bool ipv6_in_subnet(ZeroTier::InetAddress *subnet, ZeroTier::InetAddress *addr);

/**
 * @brief Convert protocol numbers to human-readable strings
 *
 * @usage For internal use only.
 * @param proto
 * @return
 */
char *beautify_eth_proto_nums(int proto);

/**
 * @brief Convert a struct sockaddr to a ZeroTier::InetAddress
 *
 * @usage For internal use only.
 * @param socket_family
 * @param addr
 * @param inet
 * @return
 */
void sockaddr2inet(int socket_family, const struct sockaddr *addr, ZeroTier::InetAddress *inet);

/**
 * @brief Convert a raw MAC address byte array into a human-readable string
 *
 * @usage For internal use only.
 * @param macbuf
 * @param len
 * @param addr
 * @return
 */
void mac2str(char *macbuf, int len, unsigned char* addr);

#if defined(STACK_LWIP) && defined(LIBZT_IPV6)
#define IP6_ADDR2(ipaddr, a,b,c,d,e,f,g,h) do { (ipaddr)->addr[0] = ZeroTier::Utils::hton((u32_t)((a & 0xffff) << 16) | (b & 0xffff)); \
                                               (ipaddr)->addr[1] = ZeroTier::Utils::hton(((c & 0xffff) << 16) | (d & 0xffff)); \
                                               (ipaddr)->addr[2] = ZeroTier::Utils::hton(((e & 0xffff) << 16) | (f & 0xffff)); \
                                               (ipaddr)->addr[3] = ZeroTier::Utils::hton(((g & 0xffff) << 16) | (h & 0xffff)); } while(0)

#endif

#endif // _H
