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
#include <queue>

#include "MAC.hpp"

#include "Mutex.hpp"
#include "Constants.hpp"
#include "VirtualTap.hpp"
#include "Constants.hpp" // libzt

#include "netif/ethernet.h"
#include "lwip/netif.h"
#include "lwip/etharp.h"
#include "lwip/tcpip.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/sys.h"
#include "lwip/tcp.h"
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
#endif

namespace ZeroTier {

bool main_loop_exited = false;
bool lwip_driver_initialized = false;
bool has_already_been_initialized = false;
int hibernationDelayMultiplier = 1;

Mutex driver_m;

std::queue<struct zts_sorted_packet*> rx_queue;
ZeroTier::Mutex _rx_input_lock_m;

extern void _push_callback_event(uint64_t nwid, int eventCode);
extern void _process_callback_event_helper(uint64_t nwid, int eventCode);

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
	err_t err = ERR_OK;
	int loop_score = LWIP_FRAMES_HANDLED_PER_CORE_CALL; // max num of packets to read per polling call
	while (loop_score > 0) {
		// TODO: Swap this block out for a thread-safe container
		_rx_input_lock_m.lock();
		if (rx_queue.size() == 0) {
			_rx_input_lock_m.unlock();
			return;
		}
		struct zts_sorted_packet *sp = rx_queue.front();
		struct pbuf *p = sp->p;
		rx_queue.pop();
		_rx_input_lock_m.unlock();
		// Feed packet into appropriate lwIP netif
		if (sp->p && sp->n) {
			if ((err = sp->n->input(sp->p, sp->n)) != ERR_OK) {
				DEBUG_ERROR("packet input error (p=%p, n=%p)=%d", p, sp->n, err);
				pbuf_free(p);
			}
			sp->p = NULL;
		}
		delete sp;
		sp = NULL;
		loop_score--;
	}
}

// Main thread which starts the initialization process
static void main_lwip_driver_loop(void *arg)
{
#if defined(__linux__)
	pthread_setname_np(pthread_self(), "lwipDriver");
#endif
#if defined(__APPLE__)
	pthread_setname_np("lwipDriver");
#endif
	sys_sem_t sem;
	LWIP_UNUSED_ARG(arg);
	if (sys_sem_new(&sem, 0) != ERR_OK) {
		DEBUG_ERROR("failed to create semaphore");
	}
	tcpip_init(tcpip_init_done, &sem);
	has_already_been_initialized = true;
	sys_sem_wait(&sem);
	while(lwip_driver_initialized) {
#if defined(_WIN32)
		Sleep(LWIP_GUARDED_BUF_CHECK_INTERVAL*hibernationDelayMultiplier);
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

void lwip_dispose_of_netifs(void *tapref)
{
	ZeroTier::VirtualTap *vtap = (ZeroTier::VirtualTap*)tapref;
	if (vtap->netif4) {
		netif_remove((struct netif*)(vtap->netif4));
		netif_set_down((struct netif*)(vtap->netif4));
		netif_set_link_down((struct netif*)(vtap->netif4));
		delete vtap->netif4;
		vtap->netif4 = NULL;
	}
	if (vtap->netif6) {
		netif_remove((struct netif*)(vtap->netif6));
		netif_set_down((struct netif*)(vtap->netif6));
		netif_set_link_down((struct netif*)(vtap->netif6));
		delete vtap->netif6;
		vtap->netif6 = NULL;
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
/*
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
*/
	
	
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
/*
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
*/
	if (etherType == 0x0800 || etherType == 0x0806) { // ip4 or ARP
		if (!tap->netif4) {
			DEBUG_ERROR("dropped packet: no netif to accept this packet (etherType=%x) on this vtap (%p)", etherType, tap);
			return;
		}
	}
	if (etherType == 0x86DD) { // ip6
		if (!tap->netif6) {
			DEBUG_ERROR("dropped packet: no netif to accept this packet (etherType=%x) on this vtap (%p)", etherType, tap);
			return;
		}
	}

	p = pbuf_alloc(PBUF_RAW, len+sizeof(struct eth_hdr), PBUF_RAM);
	if (!p) {
		DEBUG_ERROR("dropped packet: unable to allocate memory for pbuf");
		return;
	}
	// First pbuf gets ethernet header at start
	q = p;
	if (q->len < sizeof(ethhdr)) {
		pbuf_free(p);
		p = NULL;
		DEBUG_ERROR("dropped packet: first pbuf smaller than ethernet header");
		return;
	}
	const char *dataptr = reinterpret_cast<const char *>(data);
	memcpy(q->payload,&ethhdr,sizeof(ethhdr));
	int remainingPayloadSpace = q->len - sizeof(ethhdr);
	memcpy((char*)q->payload + sizeof(ethhdr),dataptr,remainingPayloadSpace);
	dataptr += remainingPayloadSpace;
	// Remaining pbufs (if any) get rest of data
	while ((q = q->next)) {
		memcpy(q->payload,dataptr,q->len);
		dataptr += q->len;
	}

	_rx_input_lock_m.lock();
	if (rx_queue.size() >= LWIP_MAX_GUARDED_RX_BUF_SZ) {
		DEBUG_INFO("dropped packet: rx_queue is full (>= %d)", LWIP_MAX_GUARDED_RX_BUF_SZ);
		// TODO: Test performance scenarios: dropping this packet, dropping oldest front packet
		pbuf_free(p);
		p = NULL;
		return;
	}

	// Construct a pre-sorted packet for lwIP packet feeder timeout
	struct zts_sorted_packet *sp = new struct zts_sorted_packet;
	sp->p = p;
	sp->vtap=tap;

	switch (etherType)
	{
		case 0x0800: // ip4
		case 0x0806: // ARP
			sp->n = (struct netif *)tap->netif4;
			break;
		case 0x86DD: // ip6
			sp->n = (struct netif *)tap->netif6;
			break;
		default:
			DEBUG_ERROR("dropped packet: unhandled (etherType=%x)", etherType);
			break;
	}
	rx_queue.push(sp);
	_rx_input_lock_m.unlock();
}

static void print_netif_info(struct netif *netif) {
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
}

bool lwip_is_netif_up(void *netif)
{
	return netif_is_up((struct netif*)netif);
}

/**
 * Called when the status of a netif changes:
 *  - Interface is up/down (ZTS_EVENT_NETIF_UP, ZTS_EVENT_NETIF_DOWN)
 *  - Address changes while up (ZTS_EVENT_NETIF_NEW_ADDRESS)
 */
static void netif_status_callback(struct netif *netif)
{
	// TODO: It appears that there may be a bug in lwIP's handling of callbacks for netifs
	// configured to handle ipv4 traffic. For this reason a temporary measure of checking
	// the status of the interfaces ourselves from the service is used.
	if (!netif->state) {
		return;
	}
	ZeroTier::VirtualTap *tap = (ZeroTier::VirtualTap *)netif->state;
	if (netif->flags & NETIF_FLAG_UP) {
		ZeroTier::VirtualTap *vtap = (ZeroTier::VirtualTap*)netif->state;
		if (netif == vtap->netif6) {
			// DEBUG_INFO("netif=%p, vtap->netif6=%p", netif, vtap->netif6);
			_push_callback_event(tap->_nwid, ZTS_EVENT_NETIF_UP_IP6);
		}
		if (netif == vtap->netif4) {
			// DEBUG_INFO("netif=%p, vtap->netif4=%p", netif, vtap->netif4);
			_push_callback_event(tap->_nwid, ZTS_EVENT_NETIF_UP_IP4);
		}
	}
	if (!(netif->flags & NETIF_FLAG_UP)) {
		if (netif->flags & NETIF_FLAG_MLD6) {
			_push_callback_event(tap->_nwid, ZTS_EVENT_NETIF_DOWN_IP6);
		} else {
			_push_callback_event(tap->_nwid, ZTS_EVENT_NETIF_DOWN_IP4);
		}
	}
	// TODO: ZTS_EVENT_NETIF_NEW_ADDRESS
	//print_netif_info(netif);
}

/**
 * Called when a netif is removed (ZTS_EVENT_NETIF_INTERFACE_REMOVED)
 */
static void netif_remove_callback(struct netif *netif)
{
	if (!netif->state) {
		return;
	}
	ZeroTier::VirtualTap *tap = (ZeroTier::VirtualTap *)netif->state;
	_push_callback_event(tap->_nwid, ZTS_EVENT_NETIF_REMOVED);
	//print_netif_info(netif);
}

/**
 * Called when a link is brought up or down (ZTS_EVENT_NETIF_LINK_UP, ZTS_EVENT_NETIF_LINK_DOWN)
 */
static void netif_link_callback(struct netif *netif)
{
	if (!netif->state) {
		return;
	}
	ZeroTier::VirtualTap *tap = (ZeroTier::VirtualTap *)netif->state;
	if (netif->flags & NETIF_FLAG_LINK_UP) {
		_push_callback_event(tap->_nwid, ZTS_EVENT_NETIF_LINK_UP);
	}
	if (netif->flags & NETIF_FLAG_LINK_UP) {
		_push_callback_event(tap->_nwid, ZTS_EVENT_NETIF_LINK_DOWN);
	}
	//print_netif_info(netif);
}

static err_t netif_init_4(struct netif *netif)
{
	netif->hwaddr_len = 6;
	netif->name[0]    = 'z';
	netif->name[1]    = '4';
	netif->linkoutput = lwip_eth_tx;
	netif->output     = etharp_output;
	netif->mtu        = ZT_MAX_MTU;
	netif->flags      = NETIF_FLAG_BROADCAST
		| NETIF_FLAG_ETHARP
		| NETIF_FLAG_ETHERNET
		| NETIF_FLAG_IGMP
		| NETIF_FLAG_LINK_UP
		| NETIF_FLAG_UP;
	netif->hwaddr_len = sizeof(netif->hwaddr);
	return ERR_OK;
}

static err_t netif_init_6(struct netif *netif)
{
	netif->hwaddr_len = 6;
	netif->name[0]    = 'z';
	netif->name[1]    = '6';
	netif->linkoutput = lwip_eth_tx;
	netif->output     = etharp_output;
	netif->output_ip6 = ethip6_output;
	netif->mtu        = ZT_MAX_MTU;
	netif->flags      = NETIF_FLAG_BROADCAST
		| NETIF_FLAG_ETHARP
		| NETIF_FLAG_ETHERNET
		| NETIF_FLAG_IGMP
		| NETIF_FLAG_MLD6;
	netif->hwaddr_len = sizeof(netif->hwaddr);
	return ERR_OK;
}

void lwip_init_interface(void *tapref, const ZeroTier::MAC &mac, const ZeroTier::InetAddress &ip)
{
	char ipbuf[INET6_ADDRSTRLEN];
	char macbuf[ZTS_MAC_ADDRSTRLEN];
	struct netif *n = new struct netif;

	if (ip.isV4()) {
		char nmbuf[INET6_ADDRSTRLEN];
		static ip4_addr_t ipaddr, netmask, gw;
		IP4_ADDR(&gw,127,0,0,1);
		ipaddr.addr = *((u32_t *)ip.rawIpData());
		netmask.addr = *((u32_t *)ip.netmask().rawIpData());
		netif_add(n, &ipaddr, &netmask, &gw, NULL, netif_init_4, tcpip_input);
		n->state = tapref;
		mac.copyTo(n->hwaddr, n->hwaddr_len);
		snprintf(macbuf, ZTS_MAC_ADDRSTRLEN, "%02x:%02x:%02x:%02x:%02x:%02x",
			n->hwaddr[0], n->hwaddr[1], n->hwaddr[2],
			n->hwaddr[3], n->hwaddr[4], n->hwaddr[5]);
		DEBUG_INFO("initialized netif as [mac=%s, addr=%s, nm=%s]", 
			macbuf, ip.toString(ipbuf), ip.netmask().toString(nmbuf));
		netif_set_up(n);
		netif_set_link_up(n);
		ZeroTier::VirtualTap *vtap = (ZeroTier::VirtualTap*)tapref;
		vtap->netif4 = (void*)n;
	}
	if (ip.isV6())
	{
		static ip6_addr_t ipaddr;
		memcpy(&(ipaddr.addr), ip.rawIpData(), sizeof(ipaddr.addr));
		n->ip6_autoconfig_enabled = 1;
		netif_add(n, NULL, NULL, NULL, NULL, netif_init_6, tcpip_input);
		netif_ip6_addr_set(n, 1, &ipaddr);
		n->state = tapref;
		mac.copyTo(n->hwaddr, n->hwaddr_len);
		netif_create_ip6_linklocal_address(n, 1);
		netif_ip6_addr_set_state(n, 0, IP6_ADDR_TENTATIVE);
		netif_ip6_addr_set_state(n, 1, IP6_ADDR_TENTATIVE);
		netif_set_default(n);
		netif_set_up(n);
		netif_set_link_up(n);
		snprintf(macbuf, ZTS_MAC_ADDRSTRLEN, "%02x:%02x:%02x:%02x:%02x:%02x",
			n->hwaddr[0], n->hwaddr[1], n->hwaddr[2],
			n->hwaddr[3], n->hwaddr[4], n->hwaddr[5]);
		DEBUG_INFO("initialized netif as [mac=%s, addr=%s]", 
			macbuf, ip.toString(ipbuf));
		ZeroTier::VirtualTap *vtap = (ZeroTier::VirtualTap*)tapref;
		vtap->netif6 = (void*)n;
	}
	// Set netif callbacks, these will be used to inform decisions made
	// by the higher level callback monitor thread
	netif_set_status_callback(n, netif_status_callback);
	netif_set_remove_callback(n, netif_remove_callback);
	netif_set_link_callback(n, netif_link_callback);
}

} // namespace ZeroTier