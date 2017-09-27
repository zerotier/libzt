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

#include "InetAddress.hpp"
#include "Debug.hpp"

char *beautify_eth_proto_nums(int proto)
{
	if (proto == 0x0800) return (char*)"IPv4";
	if (proto == 0x0806) return (char*)"ARP";
	if (proto == 0x0842) return (char*)"Wake-on-LAN";
	if (proto == 0x22F3) return (char*)"IETF TRILL Protocol";
	if (proto == 0x22EA) return (char*)"Stream Reservation Protocol";
	if (proto == 0x6003) return (char*)"DECnet Phase IV";
	if (proto == 0x8035) return (char*)"Reverse Address Resolution Protocol";
	if (proto == 0x809B) return (char*)"AppleTalk (Ethertalk)";
	if (proto == 0x80F3) return (char*)"AppleTalk Address Resolution Protocol (AARP)";
	if (proto == 0x8100) return (char*)"VLAN-tagged frame (IEEE 802.1Q) and Shortest Path Bridging IEEE 802.1aq with NNI compatibility";
	if (proto == 0x8137) return (char*)"IPX";
	if (proto == 0x8204) return (char*)"QNX Qnet";
	if (proto == 0x86DD) return (char*)"IPv6";
	if (proto == 0x8808) return (char*)"Ethernet flow control";
	if (proto == 0x8809) return (char*)"Ethernet Slow Protocols";
	if (proto == 0x8819) return (char*)"CobraNet";
	if (proto == 0x8847) return (char*)"MPLS unicast";
	if (proto == 0x8848) return (char*)"MPLS multicast";
	if (proto == 0x8863) return (char*)"PPPoE Discovery Stage";
	if (proto == 0x8864) return (char*)"PPPoE Session Stage";
	if (proto == 0x886D) return (char*)"Intel Advanced Networking Services";
	if (proto == 0x8870) return (char*)"Jumbo Frames (Obsoleted draft-ietf-isis-ext-eth-01)";
	if (proto == 0x887B) return (char*)"HomePlug 1.0 MME";
	if (proto == 0x888E) return (char*)"EAP over LAN (IEEE 802.1X)";
	if (proto == 0x8892) return (char*)"PROFINET Protocol";
	if (proto == 0x889A) return (char*)"HyperSCSI (SCSI over Ethernet)";
	if (proto == 0x88A2) return (char*)"ATA over Ethernet";
	if (proto == 0x88A4) return (char*)"EtherCAT Protocol";
	if (proto == 0x88A8) return (char*)"Provider Bridging (IEEE 802.1ad) & Shortest Path Bridging IEEE 802.1aq";
	if (proto == 0x88AB) return (char*)"Ethernet Powerlink[citation needed]";
	if (proto == 0x88B8) return (char*)"GOOSE (Generic Object Oriented Substation event)";
	if (proto == 0x88B9) return (char*)"GSE (Generic Substation Events) Management Services";
	if (proto == 0x88BA) return (char*)"SV (Sampled Value Transmission)";
	if (proto == 0x88CC) return (char*)"Link Layer Discovery Protocol (LLDP)";
	if (proto == 0x88CD) return (char*)"SERCOS III";
	if (proto == 0x88DC) return (char*)"WSMP, WAVE Short Message Protocol";
	if (proto == 0x88E1) return (char*)"HomePlug AV MME[citation needed]";
	if (proto == 0x88E3) return (char*)"Media Redundancy Protocol (IEC62439-2)";
	if (proto == 0x88E5) return (char*)"MAC security (IEEE 802.1AE)";
	if (proto == 0x88E7) return (char*)"Provider Backbone Bridges (PBB) (IEEE 802.1ah)";
	if (proto == 0x88F7) return (char*)"Precision Time Protocol (PTP) over Ethernet (IEEE 1588)";
	if (proto == 0x88FB) return (char*)"Parallel Redundancy Protocol (PRP)";
	if (proto == 0x8902) return (char*)"IEEE 802.1ag Connectivity Fault Management (CFM) Protocol / ITU-T Recommendation Y.1731 (OAM)";
	if (proto == 0x8906) return (char*)"Fibre Channel over Ethernet (FCoE)";
	if (proto == 0x8914) return (char*)"FCoE Initialization Protocol";
	if (proto == 0x8915) return (char*)"RDMA over Converged Ethernet (RoCE)";
	if (proto == 0x891D) return (char*)"TTEthernet Protocol Control Frame (TTE)";
	if (proto == 0x892F) return (char*)"High-availability Seamless Redundancy (HSR)";
	if (proto == 0x9000) return (char*)"Ethernet Configuration Testing Protocol";
	if (proto == 0x9100) return (char*)"VLAN-tagged (IEEE 802.1Q) frame with double tagging";
	return (char*)"UNKNOWN";
}

bool ipv6_in_subnet(ZeroTier::InetAddress *subnet, ZeroTier::InetAddress *addr)
{
	ZeroTier::InetAddress r(addr);
	ZeroTier::InetAddress b(subnet);
	const unsigned int bits = subnet->netmaskBits();
	switch(r.ss_family) {
		case AF_INET:
			reinterpret_cast<struct sockaddr_in *>(&r)->sin_addr.s_addr &= ZeroTier::Utils::hton((uint32_t)(0xffffffff << (32 - bits)));
			break;
		case AF_INET6: {
			uint64_t nm[2];
			uint64_t nm2[2];
			memcpy(nm,reinterpret_cast<struct sockaddr_in6 *>(&r)->sin6_addr.s6_addr,16);
			memcpy(nm2,reinterpret_cast<struct sockaddr_in6 *>(&b)->sin6_addr.s6_addr,16);

			nm[0] &= ZeroTier::Utils::hton((uint64_t)((bits >= 64) ? 0xffffffffffffffffULL : (0xffffffffffffffffULL << (64 - bits))));
			nm[1] &= ZeroTier::Utils::hton((uint64_t)((bits <= 64) ? 0ULL : (0xffffffffffffffffULL << (128 - bits))));

			nm2[0] &= ZeroTier::Utils::hton((uint64_t)((bits >= 64) ? 0xffffffffffffffffULL : (0xffffffffffffffffULL << (64 - bits))));
			nm2[1] &= ZeroTier::Utils::hton((uint64_t)((bits <= 64) ? 0ULL : (0xffffffffffffffffULL << (128 - bits))));

			memcpy(reinterpret_cast<struct sockaddr_in6 *>(&r)->sin6_addr.s6_addr,nm,16);
			memcpy(reinterpret_cast<struct sockaddr_in6 *>(&b)->sin6_addr.s6_addr,nm2,16);
		}
		break;
	}
	char b0[64], b1[64];
	memset(b0, 0, 64);
	memset(b1, 0, 64);
	return !strcmp(r.toIpString(b0), b.toIpString(b1));
}

void sockaddr2inet(int socket_family, const struct sockaddr *addr, ZeroTier::InetAddress *inet)
{
	char ipstr[INET6_ADDRSTRLEN];
	memset(ipstr, 0, INET6_ADDRSTRLEN);
	if (socket_family == AF_INET) {
		inet_ntop(AF_INET,
			(const void *)&((struct sockaddr_in *)addr)->sin_addr.s_addr, ipstr, INET_ADDRSTRLEN);
		inet->fromString(ipstr);
	}
	if (socket_family == AF_INET6) {
		inet_ntop(AF_INET6,
			(const void *)&((struct sockaddr_in6 *)addr)->sin6_addr.s6_addr, ipstr, INET6_ADDRSTRLEN);
		char addrstr[64];
		sprintf(addrstr, "%s", ipstr);
		inet->fromString(addrstr);
	}
}

void mac2str(char *macbuf, int len, unsigned char* addr)
{
	snprintf(macbuf, len, "%02x:%02x:%02x:%02x:%02x:%02x",
         addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
}


/**
 * Convert from standard IPV6 address structure to an lwIP native structure
 */
/*
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
*/
