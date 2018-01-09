/*
 * ZeroTier SDK - Network Virtualization Everywhere
 * Copyright (C) 2011-2018  ZeroTier, Inc.  https://www.zerotier.com/
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

#include "Utilities.h"

#if defined(__MINGW32__) || defined(__MINGW64__)

#include <WinSock2.h>
#include <stdint.h>
#include <string.h>

int inet_pton4(const char *src, void *dst)
{
    uint8_t tmp[NS_INADDRSZ], *tp;

    int saw_digit = 0;
    int octets = 0;
    *(tp = tmp) = 0;

    int ch;
    while ((ch = *src++) != '\0')
    {
        if (ch >= '0' && ch <= '9')
        {
            uint32_t n = *tp * 10 + (ch - '0');

            if (saw_digit && *tp == 0)
                return 0;

            if (n > 255)
                return 0;

            *tp = n;
            if (!saw_digit)
            {
                if (++octets > 4)
                    return 0;
                saw_digit = 1;
            }
        }
        else if (ch == '.' && saw_digit)
        {
            if (octets == 4)
                return 0;
            *++tp = 0;
            saw_digit = 0;
        }
        else
            return 0;
    }
    if (octets < 4)
        return 0;

    memcpy(dst, tmp, NS_INADDRSZ);

    return 1;
}

int inet_pton6(const char *src, void *dst)
{
    static const char xdigits[] = "0123456789abcdef";
    uint8_t tmp[NS_IN6ADDRSZ];

    uint8_t *tp = (uint8_t*) memset(tmp, '\0', NS_IN6ADDRSZ);
    uint8_t *endp = tp + NS_IN6ADDRSZ;
    uint8_t *colonp = NULL;

    /* Leading :: requires some special handling. */
    if (*src == ':')
    {
        if (*++src != ':')
            return 0;
    }

    const char *curtok = src;
    int saw_xdigit = 0;
    uint32_t val = 0;
    int ch;
    while ((ch = tolower(*src++)) != '\0')
    {
        const char *pch = strchr(xdigits, ch);
        if (pch != NULL)
        {
            val <<= 4;
            val |= (pch - xdigits);
            if (val > 0xffff)
                return 0;
            saw_xdigit = 1;
            continue;
        }
        if (ch == ':')
        {
            curtok = src;
            if (!saw_xdigit)
            {
                if (colonp)
                    return 0;
                colonp = tp;
                continue;
            }
            else if (*src == '\0')
            {
                return 0;
            }
            if (tp + NS_INT16SZ > endp)
                return 0;
            *tp++ = (uint8_t) (val >> 8) & 0xff;
            *tp++ = (uint8_t) val & 0xff;
            saw_xdigit = 0;
            val = 0;
            continue;
        }
        if (ch == '.' && ((tp + NS_INADDRSZ) <= endp) &&
                inet_pton4(curtok, (char*) tp) > 0)
        {
            tp += NS_INADDRSZ;
            saw_xdigit = 0;
            break; /* '\0' was seen by inet_pton4(). */
        }
        return 0;
    }
    if (saw_xdigit)
    {
        if (tp + NS_INT16SZ > endp)
            return 0;
        *tp++ = (uint8_t) (val >> 8) & 0xff;
        *tp++ = (uint8_t) val & 0xff;
    }
    if (colonp != NULL)
    {
        /*
         * Since some memmove()'s erroneously fail to handle
         * overlapping regions, we'll do the shift by hand.
         */
        const int n = tp - colonp;

        if (tp == endp)
            return 0;

        for (int i = 1; i <= n; i++)
        {
            endp[-i] = colonp[n - i];
            colonp[n - i] = 0;
        }
        tp = endp;
    }
    if (tp != endp)
        return 0;

    memcpy(dst, tmp, NS_IN6ADDRSZ);

    return 1;
}

int inet_pton(int af, const char *src, void *dst)
{
    switch (af)
    {
    case AF_INET:
        return inet_pton4(src, dst);
    case AF_INET6:
        return inet_pton6(src, dst);
    default:
        return -1;
    }
}

#endif

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

void mac2str(char *macbuf, int len, unsigned char* addr)
{
	snprintf(macbuf, len, "%02x:%02x:%02x:%02x:%02x:%02x",
         addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
}
