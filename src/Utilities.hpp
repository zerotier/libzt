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

#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#if defined(STACK_LWIP) && defined(LIBZT_IPV4)

#include "lwip/ip_addr.h"
#include <netinet/in.h>

#define ip4_addr1b(ipaddr) (((u8_t*)(ipaddr))[0])
#define ip4_addr2b(ipaddr) (((u8_t*)(ipaddr))[1])
#define ip4_addr3b(ipaddr) (((u8_t*)(ipaddr))[2])
#define ip4_addr4b(ipaddr) (((u8_t*)(ipaddr))[3])
inline ip_addr_t convert_ip(struct sockaddr_in * addr)
{
  ip_addr_t conn_addr;
  struct sockaddr_in *ipv4 = addr;
  short a = ip4_addr1b(&(ipv4->sin_addr));
  short b = ip4_addr2b(&(ipv4->sin_addr));
  short c = ip4_addr3b(&(ipv4->sin_addr));
  short d = ip4_addr4b(&(ipv4->sin_addr));
  IP4_ADDR(&conn_addr, a,b,c,d);
  return conn_addr;
}

#endif // STACK_LWIP && LIBZT_IPV4

#endif // UTILITIES_HPP