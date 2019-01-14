/*
 * ZeroTier SDK - Network Virtualization Everywhere
 * Copyright (C) 2011-2019  ZeroTier, Inc.  https://www.zerotier.com/
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
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

#include <vector>

#include "MAC.hpp"

#include "Mutex.hpp"
#include "Constants.hpp"
#include "VirtualTap.hpp"

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

ZeroTier::Mutex _rx_input_lock_m;
struct pbuf* lwip_frame_rxbuf[LWIP_MAX_GUARDED_RX_BUF_SZ];
int lwip_frame_rxbuf_tot = 0;

bool main_loop_exited = false;
bool lwip_driver_initialized = false;
bool has_already_been_initialized = false;
int hibernationDelayMultiplier = 1;

ZeroTier::Mutex driver_m;

std::vector<struct netif *> lwip_netifs;

void lwip_hibernate_driver()
{
	hibernationDelayMultiplier = ZTS_HIBERNATION_MULTIPLIER;
}

void lwip_wake_driver()
{
	hibernationDelayMultiplier = 1;
}

// Callback for when the TCPIP thread has been successfully started
static void tcpip_init_done(void *arg)
{
	sys_sem_t *sem;
	sem = (sys_sem_t *)arg;
	lwip_driver_initialized = true;
	driver_m.unlock();
	sys_sem_signal(sem);
}

void my_tcpip_callback(void *arg)
{
	if (main_loop_exited) {
		return;
	}
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
			case PP_HTONS(ETHTYPE_IPV6): {
				iphdr = (struct ip_hdr *)((char *)p->payload + SIZEOF_ETH_HDR);
				for (size_t i=0; i<lwip_netifs.size(); i++) {
					if (lwip_netifs[i]->output_ip6 && 
						lwip_netifs[i]->output_ip6 == ethip6_output) {
						if (lwip_netifs[i]->input(p, lwip_netifs[i]) != ERR_OK) {
							DEBUG_ERROR("packet input error (ipv6, p=%p, netif=%p)", p, &lwip_netifs[i]);
							break;
						}
					}
				}	
			} break;
			case PP_HTONS(ETHTYPE_IP): {
				iphdr = (struct ip_hdr *)((char *)p->payload + SIZEOF_ETH_HDR);
				for (size_t i=0; i<lwip_netifs.size(); i++) {
					if (lwip_netifs[i]->output && 
						lwip_netifs[i]->output == etharp_output) {
						if (lwip_netifs[i]->ip_addr.u_addr.ip4.addr == iphdr->dest.addr || 
							ip4_addr_isbroadcast_u32(iphdr->dest.addr, lwip_netifs[i])) {
							if (lwip_netifs[i]->input(p, lwip_netifs[i]) != ERR_OK) {
								DEBUG_ERROR("packet input error (ipv4, p=%p, netif=%p)", p, &lwip_netifs[i]);
								break;
							}
						}
					}
				}
			} break;
			case PP_HTONS(ETHTYPE_ARP): {
				for (size_t i=0; i<lwip_netifs.size(); i++) {
					if (lwip_netifs[i]->state) {
						pbuf_ref(p);
						if (lwip_netifs[i]->input(p, lwip_netifs[i]) != ERR_OK) {
							DEBUG_ERROR("packet input error (arp, p=%p, netif=%p)", p, &lwip_netifs[i]);
						}
						break;
					}
				}
				break;
			} break;
			default:
				break;
		}
		lwip_frame_rxbuf_tot--;
		loop_score--;
	}
	int count_final = lwip_frame_rxbuf_tot;
	// Move pbuf frame pointer address buffer by the number of frames successfully fed into the stack core
	if (count_initial - count_final > 0) {
		memmove(lwip_frame_rxbuf, lwip_frame_rxbuf + count_final, count_initial - count_final);
	}
}

// main thread which starts the initialization process
static void main_lwip_driver_loop(void *arg)
{
	sys_sem_t sem;
	LWIP_UNUSED_ARG(arg);
	if (sys_sem_new(&sem, 0) != ERR_OK) {
		DEBUG_ERROR("failed to create semaphore");
	}
	tcpip_init(tcpip_init_done, &sem);
	has_already_been_initialized = true;
	sys_sem_wait(&sem);
	//DEBUG_INFO("stack thread init complete");

	while(lwip_driver_initialized) {
#if defined(_WIN32)
		ms_sleep(LWIP_GUARDED_BUF_CHECK_INTERVAL*hibernationDelayMultiplier);
#else
		usleep(LWIP_GUARDED_BUF_CHECK_INTERVAL*1000*hibernationDelayMultiplier);
#endif
		// Handle incoming packets from the core's thread context.
		// If you feed frames into the core directly you will violate the core's thread model
		tcpip_callback_with_block(my_tcpip_callback, NULL, 1);
	}
	main_loop_exited = true;
}

// Initialize the lwIP stack
void lwip_driver_init()
{
	driver_m.lock(); // Unlocked from callback indicating completion of driver init
	if (has_already_been_initialized || lwip_driver_initialized) {
		// Already initialized, skip
		driver_m.unlock();
		return;
	} if (main_loop_exited) {
		DEBUG_ERROR("stack has previously been shutdown an cannot be restarted.");
		driver_m.unlock();
		return;
	}
#if defined(_WIN32)
	sys_init(); // Required for win32 init of critical sections
#endif
	void *st = sys_thread_new("main_thread", main_lwip_driver_loop,
		NULL, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
}

void lwip_driver_shutdown()
{
	if (main_loop_exited) {
		return;
	}
	lwip_driver_initialized = false;
	// Give the stack time to call the frame feed callback one last time before shutting everything down
	int callbackInterval = LWIP_GUARDED_BUF_CHECK_INTERVAL*hibernationDelayMultiplier*1000;
	usleep(callbackInterval*3);
	while(!main_loop_exited) {
		usleep(LWIP_GUARDED_BUF_CHECK_INTERVAL*1000);
	}
	if (tcpip_shutdown() == ERR_OK) {
		sys_timeouts_free();
	}
}

void lwip_driver_set_all_interfaces_down()
{
	for (size_t i=0; i<lwip_netifs.size(); i++) {
		if (lwip_netifs[i]) {
			netif_remove(lwip_netifs[i]);
			netif_set_down(lwip_netifs[i]);
			netif_set_link_down(lwip_netifs[i]);
			delete lwip_netifs[i];
		}
	}
	lwip_netifs.clear();
}

void lwip_driver_set_tap_interfaces_down(void *tapref)
{
	int sz_i = lwip_netifs.size();
	std::vector<struct netif*>::iterator iter;
	for (iter = lwip_netifs.begin(); iter != lwip_netifs.end(); ) {
		struct netif *lp = *(iter);
		if (lp->state == tapref) {
			netif_remove(lp);
			netif_set_down(lp);
			netif_set_link_down(lp);
			iter = lwip_netifs.erase(iter);
		}
		else {
			++iter;
		}
	}
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
		char macBuf[ZTS_MAC_ADDRSTRLEN], nodeBuf[ZTS_ID_LEN];
		snprintf(macBuf, ZTS_MAC_ADDRSTRLEN, "%02x:%02x:%02x:%02x:%02x:%02x",
			ethhdr->dest.addr[0], ethhdr->dest.addr[1], ethhdr->dest.addr[2], 
			ethhdr->dest.addr[3], ethhdr->dest.addr[4], ethhdr->dest.addr[5]);
		ZeroTier::MAC mac;
		mac.setTo(ethhdr->dest.addr, 6);
		mac.toAddress(tap->_nwid).toString(nodeBuf);
		DEBUG_TRANS("len=%5d dst=%s [%s TX <-- %s] proto=0x%04x %s", totalLength, macBuf, nodeBuf, tap->nodeId().c_str(),
			ZeroTier::Utils::ntoh(ethhdr->type), flagbuf);
	}
	return ERR_OK;
}

void lwip_eth_rx(ZeroTier::VirtualTap *tap, const ZeroTier::MAC &from, const ZeroTier::MAC &to, unsigned int etherType,
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
		char macBuf[ZTS_MAC_ADDRSTRLEN], nodeBuf[ZTS_ID_LEN];
		snprintf(macBuf, ZTS_MAC_ADDRSTRLEN, "%02x:%02x:%02x:%02x:%02x:%02x",
			ethhdr.dest.addr[0], ethhdr.dest.addr[1], ethhdr.dest.addr[2], 
			ethhdr.dest.addr[3], ethhdr.dest.addr[4], ethhdr.dest.addr[5]);
		ZeroTier::MAC mac;
		mac.setTo(ethhdr.src.addr, 6);
		mac.toAddress(tap->_nwid).toString(nodeBuf);
		DEBUG_TRANS("len=%5d dst=%s [%s RX --> %s] proto=0x%04x %s", len, macBuf, nodeBuf, tap->nodeId().c_str(),
			ZeroTier::Utils::ntoh(ethhdr.type), flagbuf);
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
	if (!lwip_netifs.size()) {
		DEBUG_ERROR("there are no netifs set up to handle this packet. ignoring.");
		return;
	}
	ZeroTier::Mutex::Lock _l(_rx_input_lock_m);
	if (lwip_frame_rxbuf_tot == LWIP_MAX_GUARDED_RX_BUF_SZ) {
		DEBUG_ERROR("dropped packet -- guarded receive buffer full, adjust MAX_GUARDED_RX_BUF_SZ or LWIP_GUARDED_BUF_CHECK_INTERVAL");
		return;
	}
	//pbuf_ref(p); // Increment reference to allow user application to copy data from buffer -- Will be automatically deallocated by socket API
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

static void netif_status_callback(struct netif *netif)
{
	/*
	DEBUG_INFO("n=%p, %c%c, %d, o=%p, o6=%p, mc=%x:%x:%x:%x:%x:%x, hwln=%d, st=%p, flgs=%d\n",
		netif,
		netif->name[0],
		netif->name[1],
		netif->mtu,
		netif->output,
		netif->output_ip6,
		netif->hwaddr[0],
		netif->hwaddr[1],
		netif->hwaddr[2],
		netif->hwaddr[3],
		netif->hwaddr[4],
		netif->hwaddr[5],
		netif->hwaddr_len,
		netif->state,
		netif->flags
	);
	*/
}

ZeroTier::MAC _mac;

static err_t netif_init_4(struct netif *netif)
{
	netif->hwaddr_len = 6;
	netif->name[0]    = 'e';
	netif->name[1]    = '0'+lwip_netifs.size();
	netif->linkoutput = lwip_eth_tx;
	netif->output     = etharp_output;
	netif->mtu        = ZT_MAX_MTU;
	netif->flags      = NETIF_FLAG_BROADCAST
		| NETIF_FLAG_ETHARP
		| NETIF_FLAG_ETHERNET
		| NETIF_FLAG_IGMP
		| NETIF_FLAG_LINK_UP
		| NETIF_FLAG_UP;
	_mac.copyTo(netif->hwaddr, netif->hwaddr_len);
	netif->hwaddr_len = sizeof(netif->hwaddr);
	return ERR_OK;
}

static err_t netif_init_6(struct netif *netif)
{
	netif->hwaddr_len = 6;
	netif->name[0]    = 'e';
	netif->name[1]    = '0'+(char)lwip_netifs.size();
	netif->linkoutput = lwip_eth_tx;
	netif->output     = etharp_output;
	netif->output_ip6 = ethip6_output;
	netif->mtu        = ZT_MAX_MTU;
	netif->flags      = NETIF_FLAG_BROADCAST
		| NETIF_FLAG_ETHARP
		| NETIF_FLAG_ETHERNET
		| NETIF_FLAG_IGMP
		| NETIF_FLAG_MLD6;
	_mac.copyTo(netif->hwaddr, netif->hwaddr_len);
	netif->hwaddr_len = sizeof(netif->hwaddr);
	return ERR_OK;
}

void lwip_init_interface(void *tapref, const ZeroTier::MAC &mac, const ZeroTier::InetAddress &ip)
{
	char ipbuf[INET6_ADDRSTRLEN];
	char macbuf[ZTS_MAC_ADDRSTRLEN];
	struct netif *lwipdev = new struct netif;
	lwip_netifs.push_back(lwipdev);

	_mac = mac;

	if (ip.isV4()) {
		char nmbuf[INET6_ADDRSTRLEN];
		static ip4_addr_t ipaddr, netmask, gw;
		IP4_ADDR(&gw,127,0,0,1);
		ipaddr.addr = *((u32_t *)ip.rawIpData());
		netmask.addr = *((u32_t *)ip.netmask().rawIpData());
		netif_set_status_callback(lwipdev, netif_status_callback);
		netif_add(lwipdev, &ipaddr, &netmask, &gw, NULL, netif_init_4, tcpip_input);
		lwipdev->state = tapref;
		snprintf(macbuf, ZTS_MAC_ADDRSTRLEN, "%02x:%02x:%02x:%02x:%02x:%02x",
			lwipdev->hwaddr[0], lwipdev->hwaddr[1], lwipdev->hwaddr[2], 
			lwipdev->hwaddr[3], lwipdev->hwaddr[4], lwipdev->hwaddr[5]);
		DEBUG_INFO("initialized netif as [mac=%s, addr=%s, nm=%s]", 
			macbuf, ip.toString(ipbuf), ip.netmask().toString(nmbuf));
	}
	if (ip.isV6())
	{
		static ip6_addr_t ipaddr;
		memcpy(&(ipaddr.addr), ip.rawIpData(), sizeof(ipaddr.addr));
		lwipdev->ip6_autoconfig_enabled = 1;
		netif_add(lwipdev, NULL, NULL, NULL, NULL, netif_init_6, tcpip_input);
		netif_ip6_addr_set(lwipdev, 1, &ipaddr);		
		lwipdev->state = tapref;
		netif_create_ip6_linklocal_address(lwipdev, 1);
		netif_ip6_addr_set_state(lwipdev, 0, IP6_ADDR_TENTATIVE);
		netif_ip6_addr_set_state(lwipdev, 1, IP6_ADDR_TENTATIVE);
		netif_set_status_callback(lwipdev, netif_status_callback);
		netif_set_default(lwipdev);
		netif_set_up(lwipdev);
		netif_set_link_up(lwipdev);
		snprintf(macbuf, ZTS_MAC_ADDRSTRLEN, "%02x:%02x:%02x:%02x:%02x:%02x",
			lwipdev->hwaddr[0], lwipdev->hwaddr[1], lwipdev->hwaddr[2],
			lwipdev->hwaddr[3], lwipdev->hwaddr[4], lwipdev->hwaddr[5]);
		DEBUG_INFO("initialized netif as [mac=%s, addr=%s]", 
			macbuf, ip.toString(ipbuf));
	}
}
