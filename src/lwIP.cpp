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
 * lwIP network stack driver.
 *
 * Calls made in this network stack driver may never block since all packet
 * processing (input and output) as well as timer processing (TCP mainly) is done
 * in a single execution context.
 *
 */

#include "libztDefs.h"

#include "VirtualTap.h"
class VirtualTap;

#include "Mutex.hpp"
#include "MAC.hpp"
#include "ZeroTierOne.h"

#include "libzt.h"
#include "SysUtils.h"
#include "Utilities.h"
#include "libztDebug.h"

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
#include "lwip/ethip6.h"
#include "lwip/ip_addr.h"
#include "lwip/nd6.h"
#include "lwip/dns.h"
#include "lwip/netifapi.h"

#include "lwIP.h"

#if defined(_WIN32)
#include <time.h>
void ms_sleep(unsigned long ms) 
{
	Sleep(ms);
}
#endif

struct netif lwipInterfaces[10];
int lwipInterfacesCount = 0;

ZeroTier::Mutex _rx_input_lock_m;
struct pbuf* lwip_frame_rxbuf[LWIP_MAX_GUARDED_RX_BUF_SZ];
int lwip_frame_rxbuf_tot = 0;


bool lwip_driver_initialized = false;
ZeroTier::Mutex driver_m;

err_t tapif_init(struct netif *netif)
{
	// we do the actual initialization in elsewhere
	return ERR_OK;
}

/*
static void tcp_timeout(void *data)
{
	DEBUG_EXTRA("");
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
	//netif_set_up(&lwipdev);
	lwip_driver_initialized = true;
	driver_m.unlock();
	// sys_timeout(5000, tcp_timeout, NULL);
	sys_sem_signal(sem);
}

void my_tcpip_callback(void *arg)
{
	ZeroTier::Mutex::Lock _l(_rx_input_lock_m);
	int loop_score = LWIP_FRAMES_HANDLED_PER_CORE_CALL; // max num of packets to read per polling call
	// TODO: Optimize (use Ringbuffer)
	int pkt_num = 0;
	int count_initial = lwip_frame_rxbuf_tot;
	while (lwip_frame_rxbuf_tot > 0 && loop_score > 0) {
		struct pbuf *p = lwip_frame_rxbuf[pkt_num];
		pkt_num++;
		// Packet routing logic. Inputs packet into correct lwip netif interface depending on protocol type
		struct ip_hdr *iphdr;
		switch (((struct eth_hdr *)p->payload)->type)
		{
#ifdef LIBZT_IPV6
			case PP_HTONS(ETHTYPE_IPV6): {
				iphdr = (struct ip_hdr *)((char *)p->payload + SIZEOF_ETH_HDR);
				for (int i=0; i<lwipInterfacesCount; i++) {
					if (lwipInterfaces[i].output_ip6 && lwipInterfaces[i].output_ip6 == ethip6_output) {
						if (lwipInterfaces[i].input(p, &lwipInterfaces[i]) != ERR_OK) {
							DEBUG_ERROR("packet input error (ipv6, p=%p, netif=%p)", p, &lwipInterfaces[i]);
							break;
						}

					}
				}	
			} break;
#endif
#ifdef LIBZT_IPV4
			case PP_HTONS(ETHTYPE_IP): {
				iphdr = (struct ip_hdr *)((char *)p->payload + SIZEOF_ETH_HDR);
				for (int i=0; i<lwipInterfacesCount; i++) {
					if (lwipInterfaces[i].output &&  lwipInterfaces[i].output == etharp_output) {
						//if (lwipInterfaces[i].ip_addr.addr == iphdr->dest.addr || ip4_addr_isbroadcast_u32(iphdr->dest.addr, &lwipInterfaces[i])) {
						if (lwipInterfaces[i].ip_addr.u_addr.ip4.addr == iphdr->dest.addr || ip4_addr_isbroadcast_u32(iphdr->dest.addr, &lwipInterfaces[i])) {
							if (lwipInterfaces[i].input(p, &lwipInterfaces[i]) != ERR_OK) {
								DEBUG_ERROR("packet input error (ipv4, p=%p, netif=%p)", p, &lwipInterfaces[i]);
								break;
							}
						}
					}
				}
			} break;
#endif
			case PP_HTONS(ETHTYPE_ARP): {
				for (int i=0; i<lwipInterfacesCount; i++) {
					if (lwipInterfaces[i].state) {
						pbuf_ref(p);
						if (lwipInterfaces[i].input(p, &lwipInterfaces[i]) != ERR_OK) {
							DEBUG_ERROR("packet input error (arp, p=%p, netif=%p)", p, &lwipInterfaces[i]);
						}
						break;
					}
				}
				break;
			} break;
			default:
				break;
		}
		lwip_frame_rxbuf_tot--;;
		loop_score--;
	}
	int count_final = count_initial - lwip_frame_rxbuf_tot;
	// move pbuf frame pointer address buffer by the number of frames successfully fed into the stack core
	memmove(lwip_frame_rxbuf, lwip_frame_rxbuf + count_final, sizeof(lwip_frame_rxbuf) - count_final);
}

// main thread which starts the initialization process
static void main_thread(void *arg)
{
	sys_sem_t sem;
	LWIP_UNUSED_ARG(arg);
	if (sys_sem_new(&sem, 0) != ERR_OK) {
		DEBUG_ERROR("failed to create semaphore");
	}
	
	tcpip_init(tcpip_init_done, &sem);
	sys_sem_wait(&sem);
	DEBUG_EXTRA("stack thread init complete");

	while(1) {
#if defined(_WIN32)
		ms_sleep(LWIP_GUARDED_BUF_CHECK_INTERVAL);
#else
		usleep(LWIP_GUARDED_BUF_CHECK_INTERVAL*1000);
#endif
		// Handle incoming packets from the core's thread context.
		// If you feed frames into the core directly you will violate the core's thread model
		tcpip_callback_with_block(my_tcpip_callback, NULL, 1);
	}
	sys_sem_wait(&sem); // block forever
}

// initialize the lwIP stack
void lwip_driver_init()
{
	driver_m.lock(); // unlocked from callback indicating completion of driver init
	if (lwip_driver_initialized == true) {
		return;
	}
#if defined(_WIN32)
	sys_init(); // required for win32 initializtion of critical sections
#endif
	sys_thread_new("main_thread", main_thread,
		NULL, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
}

err_t lwip_eth_tx(struct netif *netif, struct pbuf *p)
{
	struct pbuf *q;
	char buf[ZT_MAX_MTU+32];
	char *bufptr;
	int totalLength = 0;

	VirtualTap *tap = (VirtualTap*)netif->state;
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

void lwip_eth_rx(VirtualTap *tap, const ZeroTier::MAC &from, const ZeroTier::MAC &to, unsigned int etherType,
	const void *data, unsigned int len)
{
	struct pbuf *p,*q;
	struct eth_hdr ethhdr;
	from.copyTo(ethhdr.src.addr, 6);
	to.copyTo(ethhdr.dest.addr, 6);
	ethhdr.type = ZeroTier::Utils::hton((uint16_t)etherType);

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

	if (lwipInterfacesCount <= 0) {
		DEBUG_ERROR("there are no netifs set up to handle this packet. ignoring.");
		return;
	}
	Mutex::Lock _l(_rx_input_lock_m);
	if (lwip_frame_rxbuf_tot == LWIP_MAX_GUARDED_RX_BUF_SZ) {
		DEBUG_ERROR("guarded receive buffer full, adjust MAX_GUARDED_RX_BUF_SZ or LWIP_GUARDED_BUF_CHECK_INTERVAL");
	}
	lwip_frame_rxbuf[lwip_frame_rxbuf_tot] = p;
	lwip_frame_rxbuf_tot += 1;
}

/*
void lwip_dns_init()
{
	dns_init();
}
*/

void lwip_start_dhcp(void *netif)
{
#if defined(LIBZT_IPV4)
	//netifapi_dhcp_start((struct netif *)netif);
#endif
}


void lwip_init_interface(void *tapref, const ZeroTier::MAC &mac, const ZeroTier::InetAddress &ip)
{
	char ipbuf[INET6_ADDRSTRLEN], nmbuf[INET6_ADDRSTRLEN];
	char macbuf[ZT_MAC_ADDRSTRLEN];
	DEBUG_EXTRA("lwipInterfacesCount=%d", lwipInterfacesCount);
	struct netif *lwipdev = &lwipInterfaces[lwipInterfacesCount];
	DEBUG_EXTRA("netif=%p", lwipdev);

	if (ip.isV4()) {
		static ip4_addr_t ipaddr, netmask, gw;
		IP4_ADDR(&gw,127,0,0,1);
		ipaddr.addr = *((u32_t *)ip.rawIpData());
		netmask.addr = *((u32_t *)ip.netmask().rawIpData());
		netif_add(lwipdev, &ipaddr, &netmask, &gw, NULL, tapif_init, tcpip_input);
		lwipdev->state = tapref;
		lwipdev->output = etharp_output;
		lwipdev->mtu = ZT_MAX_MTU;
		lwipdev->name[0] = 'l';
		lwipdev->name[1] = '0'+lwipInterfacesCount;
		lwipdev->linkoutput = lwip_eth_tx;
		lwipdev->hwaddr_len = 6;
		mac.copyTo(lwipdev->hwaddr, lwipdev->hwaddr_len);
		lwipdev->flags = 0;
		lwipdev->flags = NETIF_FLAG_BROADCAST
			| NETIF_FLAG_ETHARP
			| NETIF_FLAG_IGMP
			| NETIF_FLAG_LINK_UP
			| NETIF_FLAG_UP;
		netif_set_default(lwipdev);
		netif_set_link_up(lwipdev);
		netif_set_up(lwipdev);
		mac2str(macbuf, ZT_MAC_ADDRSTRLEN, lwipdev->hwaddr);
		DEBUG_INFO("initialized netif as [mac=%s, addr=%s, nm=%s]", macbuf, ip.toString(ipbuf), ip.netmask().toString(nmbuf));
	}
	if (ip.isV6()) {
		static ip6_addr_t ipaddr;
		memcpy(&(ipaddr.addr), ip.rawIpData(), sizeof(ipaddr.addr));
		lwipdev->mtu = ZT_MAX_MTU;
		lwipdev->name[0] = 'l';
		lwipdev->name[1] = '0'+lwipInterfacesCount;
		lwipdev->hwaddr_len = 6;
		lwipdev->linkoutput = lwip_eth_tx;
		lwipdev->ip6_autoconfig_enabled = 1;
		mac.copyTo(lwipdev->hwaddr, lwipdev->hwaddr_len);
		netif_add(lwipdev, NULL, NULL, NULL, NULL, tapif_init, ethernet_input);
		lwipdev->output_ip6 = ethip6_output;
		lwipdev->state = tapref;
		netif_create_ip6_linklocal_address(lwipdev, 1);
		s8_t idx = 1;
		netif_add_ip6_address(lwipdev, &ipaddr, &idx);
		netif_set_default(lwipdev);
		netif_set_up(lwipdev);
		netif_set_link_up(lwipdev);
		netif_ip6_addr_set_state(lwipdev, 1, IP6_ADDR_TENTATIVE);
		mac2str(macbuf, ZT_MAC_ADDRSTRLEN, lwipdev->hwaddr);
		DEBUG_INFO("initialized netif as [mac=%s, addr=%s]", macbuf, ip.toString(ipbuf));
	}
	lwipInterfacesCount++;
}
