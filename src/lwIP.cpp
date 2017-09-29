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

#include "VirtualTap.hpp"

#include "ZeroTierOne.h"
#include "MAC.hpp"

#include "libzt.h"
#include "Utilities.h"
#include "Debug.hpp"

#include "netif/ethernet.h"
#include "lwip/netif.h"
#include "lwip/etharp.h"
#include "lwip/tcpip.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/sys.h"
#include "lwip/tcp.h"
#include "lwip/priv/tcp_priv.h" /* for tcp_debug_print_pcbs() */
#include "lwip/timeouts.h"
#include "lwip/stats.h"

#include "lwIP.hpp"

netif lwipdev, lwipdev6, n1;

struct netif netifs[10];

bool lwip_driver_initialized = false;
ZeroTier::Mutex driver_m;

err_t tapif_init(struct netif *netif)
{
	// we do the actual initialization in lwip_init_interface
	return ERR_OK;
}

/*
static void tcp_timeout(void *data)
{
	DEBUG_EXTRA();
	LWIP_UNUSED_ARG(data);
#if TCP_DEBUG && LWIP_TCP
	// tcp_debug_print_pcbs();
#endif
	sys_timeout(5000, tcp_timeout, NULL);
}
*/

// callback for when the TCPIP thread has been successfully started
static void tcpip_init_done(void *arg)
{
	sys_sem_t *sem;
	sem = (sys_sem_t *)arg;
	netif_set_up(&lwipdev);
	DEBUG_EXTRA("lwIP stack driver initialized");
	lwip_driver_initialized = true;
	driver_m.unlock();
	DEBUG_EXTRA("released lock");
	// sys_timeout(5000, tcp_timeout, NULL);
	sys_sem_signal(sem);
}

// main thread which starts the initialization process
static void main_network_stack_thread(void *arg)
{
	sys_sem_t sem;
	LWIP_UNUSED_ARG(arg);
	if (sys_sem_new(&sem, 0) != ERR_OK) {
		DEBUG_ERROR("failed to create semaphore", 0);
	}
	tcpip_init(tcpip_init_done, &sem);
	sys_sem_wait(&sem);
	DEBUG_EXTRA("tcpip thread started");
	sys_sem_wait(&sem);
}

// initialize the lwIP stack
void lwip_driver_init()
{
	DEBUG_EXTRA("getting lock..");
	driver_m.lock(); // unlocked from callback indicating completion of init
	DEBUG_EXTRA("got lock");
	if (lwip_driver_initialized == true) {
		return;
	}
	sys_thread_new("main_network_stack_thread", main_network_stack_thread,
		NULL, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
}

err_t lwip_eth_tx(struct netif *netif, struct pbuf *p)
{
	struct pbuf *q;
	char buf[ZT_MAX_MTU+32];
	char *bufptr;
	int totalLength = 0;

	ZeroTier::VirtualTap *tap = (ZeroTier::VirtualTap*)netif->state;
	bufptr = buf;
	for (q = p; q != NULL; q = q->next) {
		memcpy(bufptr, q->payload, q->len);
		bufptr += q->len;
		totalLength += q->len;
	}
	struct eth_hdr *ethhdr;
	ethhdr = (struct eth_hdr *)buf;

	ZeroTier::MAC src_mac;
	ZeroTier::MAC dest_mac;
	src_mac.setTo(ethhdr->src.addr, 6);
	dest_mac.setTo(ethhdr->dest.addr, 6);

	char *data = buf + sizeof(struct eth_hdr);
	int len = totalLength - sizeof(struct eth_hdr);
	int proto = ZeroTier::Utils::ntoh((uint16_t)ethhdr->type);
	tap->_handler(tap->_arg, NULL, tap->_nwid, src_mac, dest_mac, proto, 0, data, len);

	if (ZT_MSG_TRANSFER == true) {
		char flagbuf[32];
		memset(&flagbuf, 0, 32);
		char macBuf[ZT_MAC_ADDRSTRLEN], nodeBuf[ZTO_ID_LEN];
		mac2str(macBuf, ZT_MAC_ADDRSTRLEN, ethhdr->dest.addr);
		ZeroTier::MAC mac;
		mac.setTo(ethhdr->dest.addr, 6);
		mac.toAddress(tap->_nwid).toString(nodeBuf);
		DEBUG_TRANS("len=%5d dst=%s [%s TX <-- %s] proto=0x%04x %s %s", totalLength, macBuf, nodeBuf, tap->nodeId().c_str(),
			ZeroTier::Utils::ntoh(ethhdr->type), beautify_eth_proto_nums(ZeroTier::Utils::ntoh(ethhdr->type)), flagbuf);
	}
	return ERR_OK;
}

void general_lwip_init_interface(void *tapref, struct netif *interface, const char *name, const ZeroTier::MAC &mac, const ZeroTier::InetAddress &addr, const ZeroTier::InetAddress &nm, const ZeroTier::InetAddress &gw)
{
	char ipbuf[INET6_ADDRSTRLEN], nmbuf[INET6_ADDRSTRLEN], gwbuf[INET6_ADDRSTRLEN];
	static ip_addr_t _addr, _nm, _gw;
	IP4_ADDR(&_gw,127,0,0,1);
	IP4_ADDR(&_addr,10,6,6,86);
	//_addr.addr = *((u32_t *)addr.rawIpData());
	_nm.addr = *((u32_t *)addr.netmask().rawIpData());
	netif_add(&(n1),&_addr, &_nm, &_gw, NULL, tapif_init, tcpip_input);
	n1.state = tapref;
	n1.output = etharp_output;
	n1.mtu = ZT_MAX_MTU;
	n1.name[0] = name[0];
	n1.name[1] = name[1];
	n1.linkoutput = lwip_eth_tx;
	n1.hwaddr_len = 6;
	mac.copyTo(n1.hwaddr, n1.hwaddr_len);
	n1.flags = NETIF_FLAG_BROADCAST
		| NETIF_FLAG_ETHARP
		| NETIF_FLAG_IGMP
		| NETIF_FLAG_LINK_UP
		| NETIF_FLAG_UP;
	netif_set_link_up(&n1);
	char macbuf[ZT_MAC_ADDRSTRLEN];
	mac2str(macbuf, ZT_MAC_ADDRSTRLEN, n1.hwaddr);
	DEBUG_INFO("initialized netif as [mac=%s, addr=%s, nm=%s, gw=%s]", macbuf, addr.toString(ipbuf), addr.netmask().toString(nmbuf), gw.toString(gwbuf));
}

void general_turn_on_interface(struct netif *interface)
{
	//netif_set_up(&n1);
	//netif_set_default(&n1);
	//lwipdev.linkoutput = NULL;
	//sleep(2);
	//netif_set_down(&lwipdev);
	//netif_set_link_down(&lwipdev);
}

void lwip_init_interface(void *tapref, const ZeroTier::MAC &mac, const ZeroTier::InetAddress &ip)
{
	/* NOTE: It is a known issue that when assigned more than one IP address via
	Central, this interface will be unable to transmit (including ARP). */
	char ipbuf[INET6_ADDRSTRLEN], nmbuf[INET6_ADDRSTRLEN];
	if (ip.isV4()) {
		static ip_addr_t ipaddr, netmask, gw;
		IP4_ADDR(&gw,127,0,0,1);
		ipaddr.addr = *((u32_t *)ip.rawIpData());
		netmask.addr = *((u32_t *)ip.netmask().rawIpData());
		netif_add(&(lwipdev),&ipaddr, &netmask, &gw, NULL, tapif_init, tcpip_input);
		lwipdev.state = tapref;
		lwipdev.output = etharp_output;
		lwipdev.mtu = ZT_MAX_MTU;
		lwipdev.name[0] = 'l';
		lwipdev.name[1] = '4';
		lwipdev.linkoutput = lwip_eth_tx;
		lwipdev.hwaddr_len = 6;
		mac.copyTo(lwipdev.hwaddr, lwipdev.hwaddr_len);
		lwipdev.flags = NETIF_FLAG_BROADCAST
			| NETIF_FLAG_ETHARP
			| NETIF_FLAG_IGMP
			| NETIF_FLAG_LINK_UP
			| NETIF_FLAG_UP;
		netif_set_default(&(lwipdev));
		netif_set_link_up(&(lwipdev));
		netif_set_up(&(lwipdev));
		char macbuf[ZT_MAC_ADDRSTRLEN];
		mac2str(macbuf, ZT_MAC_ADDRSTRLEN, lwipdev.hwaddr);
		DEBUG_INFO("initialized netif as [mac=%s, addr=%s, nm=%s]", macbuf, ip.toString(ipbuf), ip.netmask().toString(nmbuf));
	}
}

void lwip_eth_rx(ZeroTier::VirtualTap *tap, const ZeroTier::MAC &from, const ZeroTier::MAC &to, unsigned int etherType,
	const void *data, unsigned int len)
{
	struct pbuf *p,*q;
	struct eth_hdr ethhdr;
	from.copyTo(ethhdr.src.addr, 6);
	to.copyTo(ethhdr.dest.addr, 6);
	ethhdr.type = ZeroTier::Utils::hton((uint16_t)etherType);

	p = pbuf_alloc(PBUF_RAW, len+sizeof(struct eth_hdr), PBUF_POOL);
	if (p != NULL) {
		const char *dataptr = reinterpret_cast<const char *>(data);
		// First pbuf gets ethernet header at start
		q = p;
		if (q->len < sizeof(ethhdr)) {
			DEBUG_ERROR("dropped packet: first pbuf smaller than ethernet header");
			return;
		}
		memcpy(q->payload,&ethhdr,sizeof(ethhdr));
		memcpy((char*)q->payload + sizeof(ethhdr),dataptr,q->len - sizeof(ethhdr));
		dataptr += q->len - sizeof(ethhdr);
		// Remaining pbufs (if any) get rest of data
		while ((q = q->next)) {
			memcpy(q->payload,dataptr,q->len);
			dataptr += q->len;
		}
	}
	else {
		DEBUG_ERROR("dropped packet: no pbufs available");
		return;
	}
	if (ZT_MSG_TRANSFER == true) {
		char flagbuf[32];
		memset(&flagbuf, 0, 32);
		char macBuf[ZT_MAC_ADDRSTRLEN], nodeBuf[ZTO_ID_LEN];
		mac2str(macBuf, ZT_MAC_ADDRSTRLEN, ethhdr.dest.addr);
		ZeroTier::MAC mac;
		mac.setTo(ethhdr.src.addr, 6);
		mac.toAddress(tap->_nwid).toString(nodeBuf);
		DEBUG_TRANS("len=%5d dst=%s [%s RX --> %s] proto=0x%04x %s %s", len, macBuf, nodeBuf, tap->nodeId().c_str(),
			ZeroTier::Utils::ntoh(ethhdr.type), beautify_eth_proto_nums(ZeroTier::Utils::ntoh(ethhdr.type)), flagbuf);
	}
	{

		// Here we select which interface shall receive the Ethernet frames coming in off the ZeroTier virtual wire

		// ROUTING CODE SHALL GO HERE
/*
		for (int i=0; i<num_netif; i++) {
			if (netifs[i].hwaddr == ethhdr.dest.addr) {
				// we will use this interface
				this->hwaddr;

			}
		}
*/
#if defined(LIBZT_IPV4)
		if (lwipdev.input == NULL)
		{
			return;
		}
			if (lwipdev.input(p, &(lwipdev)) != ERR_OK) {
				DEBUG_ERROR("error while feeding frame into stack interface (ipv4)");
			}
#endif
	}
}

