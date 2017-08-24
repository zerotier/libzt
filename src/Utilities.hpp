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

/*
 * Returns masked address for subnet comparisons
 */
//ZeroTier::InetAddress *ztipv6_mask(ZeroTier::InetAddress *addr, unsigned int bits);
bool ipv6_in_subnet(ZeroTier::InetAddress *subnet, ZeroTier::InetAddress *addr);

/*
 * Convert protocol numbers to human-readable strings
 */
char *beautify_eth_proto_nums(int proto);

/*
 * Convert a struct sockaddr to a ZeroTier::InetAddress
 */
void sockaddr2inet(int socket_family, const struct sockaddr *addr, ZeroTier::InetAddress *inet);

/*
 * Convert a raw MAC address byte array into a human-readable string
 */
void mac2str(char *macbuf, int len, unsigned char* addr);

#if defined(STACK_LWIP) && defined(LIBZT_IPV6)
#define IP6_ADDR2(ipaddr, a,b,c,d,e,f,g,h) do { (ipaddr)->addr[0] = ZeroTier::Utils::hton((u32_t)((a & 0xffff) << 16) | (b & 0xffff)); \
                                               (ipaddr)->addr[1] = ZeroTier::Utils::hton(((c & 0xffff) << 16) | (d & 0xffff)); \
                                               (ipaddr)->addr[2] = ZeroTier::Utils::hton(((e & 0xffff) << 16) | (f & 0xffff)); \
                                               (ipaddr)->addr[3] = ZeroTier::Utils::hton(((g & 0xffff) << 16) | (h & 0xffff)); } while(0)

/*
 * Convert from standard IPV6 address structure to an lwIP native structure                                               
 */
inline void in6_to_ip6(ip6_addr_t *ba, struct sockaddr_in6 *in6)
{
	uint8_t *ip = &(in6->sin6_addr).s6_addr[0];
	IP6_ADDR2(ba,
		(((ip[ 0] & 0xffff) << 8) | ((ip[ 1]) & 0xffff)),
		(((ip[ 2] & 0xffff) << 8) | ((ip[ 3]) & 0xffff)),
		(((ip[ 4] & 0xffff) << 8) | ((ip[ 5]) & 0xffff)),
		(((ip[ 6] & 0xffff) << 8) | ((ip[ 7]) & 0xffff)),
		(((ip[ 8] & 0xffff) << 8) | ((ip[ 9]) & 0xffff)),
		(((ip[10] & 0xffff) << 8) | ((ip[11]) & 0xffff)),
		(((ip[12] & 0xffff) << 8) | ((ip[13]) & 0xffff)),
		(((ip[14] & 0xffff) << 8) | ((ip[15]) & 0xffff))
	);
}
#endif


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