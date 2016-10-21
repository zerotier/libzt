/*
 * ZeroTier One - Network Virtualization Everywhere
 * Copyright (C) 2011-2015  ZeroTier, Inc.
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
 * ZeroTier may be used and distributed under the terms of the GPLv3, which
 * are available at: http://www.gnu.org/licenses/gpl-3.0.html
 *
 * If you would like to embed ZeroTier into a commercial application or
 * redistribute it in a modified binary form, please contact ZeroTier Networks
 * LLC. Start here: http://www.zerotier.com/
 */


#if defined(SDK_LWIP) && defined(SDK_IPV6)
	#define IP6_ADDR2(ipaddr, a,b,c,d,e,f,g,h) do { (ipaddr)->addr[0] = ZeroTier::Utils::hton((u32_t)((a & 0xffff) << 16) | (b & 0xffff)); \
	                                               (ipaddr)->addr[1] = ZeroTier::Utils::hton(((c & 0xffff) << 16) | (d & 0xffff)); \
	                                               (ipaddr)->addr[2] = ZeroTier::Utils::hton(((e & 0xffff) << 16) | (f & 0xffff)); \
	                                               (ipaddr)->addr[3] = ZeroTier::Utils::hton(((g & 0xffff) << 16) | (h & 0xffff)); } while(0)


	// Convert from standard IPV6 address structure to an lwIP native structure                                               
	void in6_to_ip6(ip6_addr_t *ba, struct sockaddr_in6 *in6)
	{
		uint8_t *ip = &(in6->sin6_addr).s6_addr[0];
		uint16_t ip16;
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