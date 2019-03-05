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

#include <queue>

#include "MAC.hpp"
#include "Mutex.hpp"

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
#include "lwip/netifapi.h"
#include "lwip/stats.h"

#include "VirtualTap.hpp"
#include "lwipDriver.hpp"
#include "ZeroTier.h"
#include "Controls.hpp"

extern void postEvent(uint64_t eventCode, void *arg);
extern void postEvent(uint64_t eventCode);

#if defined(_WIN32)
#include <time.h>
#endif

/**
 * Length of human-readable MAC address string
 */
#define ZTS_MAC_ADDRSTRLEN 18

#ifndef htonll
#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#endif

namespace ZeroTier {

bool _has_exited = false;
int hibernationDelayMultiplier = 1;
extern bool _run_lwip_tcpip;
Mutex lwip_driver_m;

void lwip_sleep(long ms)
{
#if defined(_WIN32)
	Sleep(ms*hibernationDelayMultiplier);
#else
	usleep(ms*1000*hibernationDelayMultiplier);
#endif
}

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
	_run_lwip_tcpip = true;
	postEvent(ZTS_EVENT_STACK_UP);
	sys_sem_signal(sem);
}

static void main_lwip_driver_loop(void *arg)
{
#if defined(__linux__)
	pthread_setname_np(pthread_self(), ZTS_LWIP_DRIVER_THREAD_NAME);
#endif
#if defined(__APPLE__)
	pthread_setname_np(ZTS_LWIP_DRIVER_THREAD_NAME);
#endif
	sys_sem_t sem;
	LWIP_UNUSED_ARG(arg);
	if (sys_sem_new(&sem, 0) != ERR_OK) {
		DEBUG_ERROR("failed to create semaphore");
	}
	tcpip_init(tcpip_init_done, &sem);
	sys_sem_wait(&sem);
	// Main loop
	while(_run_lwip_tcpip) {
		lwip_sleep(LWIP_DRIVER_LOOP_INTERVAL);
	}
	_has_exited = true;
	postEvent(ZTS_EVENT_STACK_DOWN);
}

bool lwip_is_up()
{
	Mutex::Lock _l(lwip_driver_m);
	return _run_lwip_tcpip;
}

bool lwip_has_previously_shutdown()
{
	Mutex::Lock _l(lwip_driver_m);
	return _has_exited;
}

void lwip_driver_init()
{
	if (lwip_is_up()) {
		return;
	}
	if (lwip_has_previously_shutdown()) {
		return;
	}
	Mutex::Lock _l(lwip_driver_m);
#if defined(_WIN32)
	sys_init(); // Required for win32 init of critical sections
#endif
	void *st = sys_thread_new(ZTS_LWIP_DRIVER_THREAD_NAME, main_lwip_driver_loop,
		NULL, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
}

void lwip_driver_shutdown()
{
	if (lwip_has_previously_shutdown()) {
		return;
	}
	Mutex::Lock _l(lwip_driver_m);
	// Set flag to stop sending frames into the core
	_run_lwip_tcpip = false;
	// Wait until the main lwIP thread has exited
	while (!_has_exited) { lwip_sleep(LWIP_DRIVER_LOOP_INTERVAL); }
	/*
	if (tcpip_shutdown() == ERR_OK) {
		sys_timeouts_free();
	}
	*/
}

void lwip_remove_netif(void *netif)
{
	struct netif *n = (struct netif*)netif;
	LOCK_TCPIP_CORE();
	netif_remove(n);
	netif_set_down(n);
	netif_set_link_down(n);
	UNLOCK_TCPIP_CORE();
}

err_t lwip_eth_tx(struct netif *n, struct pbuf *p)
{
	struct pbuf *q;
	char buf[ZT_MAX_MTU+32];
	char *bufptr;
	int totalLength = 0;

	VirtualTap *tap = (VirtualTap*)n->state;
	bufptr = buf;
	for (q = p; q != NULL; q = q->next) {
		memcpy(bufptr, q->payload, q->len);
		bufptr += q->len;
		totalLength += q->len;
	}
	struct eth_hdr *ethhdr;
	ethhdr = (struct eth_hdr *)buf;

	MAC src_mac;
	MAC dest_mac;
	src_mac.setTo(ethhdr->src.addr, 6);
	dest_mac.setTo(ethhdr->dest.addr, 6);

	char *data = buf + sizeof(struct eth_hdr);
	int len = totalLength - sizeof(struct eth_hdr);
	int proto = Utils::ntoh((uint16_t)ethhdr->type);
	tap->_handler(tap->_arg, NULL, tap->_nwid, src_mac, dest_mac, proto, 0, data, len);
/*
	if (ZT_MSG_TRANSFER == true) {
		char flagbuf[32];
		memset(&flagbuf, 0, 32);
		char macBuf[ZTS_MAC_ADDRSTRLEN], nodeBuf[16];
		snprintf(macBuf, ZTS_MAC_ADDRSTRLEN, "%02x:%02x:%02x:%02x:%02x:%02x",
			ethhdr->dest.addr[0], ethhdr->dest.addr[1], ethhdr->dest.addr[2], 
			ethhdr->dest.addr[3], ethhdr->dest.addr[4], ethhdr->dest.addr[5]);
		MAC mac;
		mac.setTo(ethhdr->dest.addr, 6);
		mac.toAddress(tap->_nwid).toString(nodeBuf);
		DEBUG_TRANS("len=%5d dst=%s [%s TX <-- %s] proto=0x%04x %s", totalLength, macBuf, nodeBuf, tap->nodeId().c_str(),
			Utils::ntoh(ethhdr->type), flagbuf);
	}
*/
	return ERR_OK;
}

void lwip_eth_rx(VirtualTap *tap, const MAC &from, const MAC &to, unsigned int etherType,
	const void *data, unsigned int len)
{
#ifdef LWIP_STATS
	stats_display();
#endif
	if (!_run_lwip_tcpip) {
		return;
	}
	struct pbuf *p,*q;
	struct eth_hdr ethhdr;
	from.copyTo(ethhdr.src.addr, 6);
	to.copyTo(ethhdr.dest.addr, 6);
	ethhdr.type = Utils::hton((uint16_t)etherType);
/*
	if (ZT_MSG_TRANSFER == true) {
		char flagbuf[32];
		memset(&flagbuf, 0, 32);
		char macBuf[ZTS_MAC_ADDRSTRLEN], nodeBuf[16];
		snprintf(macBuf, ZTS_MAC_ADDRSTRLEN, "%02x:%02x:%02x:%02x:%02x:%02x",
			ethhdr.dest.addr[0], ethhdr.dest.addr[1], ethhdr.dest.addr[2],
			ethhdr.dest.addr[3], ethhdr.dest.addr[4], ethhdr.dest.addr[5]);
		MAC mac;
		mac.setTo(ethhdr.src.addr, 6);
		mac.toAddress(tap->_nwid).toString(nodeBuf);
		DEBUG_TRANS("len=%5d dst=%s [%s RX --> %s] proto=0x%04x %s", len, macBuf, nodeBuf, tap->nodeId().c_str(),
			Utils::ntoh(ethhdr.type), flagbuf);
	}
*/
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
	// Copy frame data into pbuf
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
	// Feed packet into stack
	int err;
	if ((err = ((struct netif *)tap->netif)->input(p, (struct netif *)tap->netif)) != ERR_OK) {
		DEBUG_ERROR("packet input error (%d)", err);
		pbuf_free(p);
	}
}

/*
static void print_netif_info(struct netif *n) {
	DEBUG_INFO("n=%p, %c%c, %d, o=%p, o6=%p, mc=%x:%x:%x:%x:%x:%x, hwln=%d, st=%p, flgs=%d\n",
		n,
		n->name[0],
		n->name[1],
		n->mtu,
		n->output,
		n->output_ip6,
		n->hwaddr[0],
		n->hwaddr[1],
		n->hwaddr[2],
		n->hwaddr[3],
		n->hwaddr[4],
		n->hwaddr[5],
		n->hwaddr_len,
		n->state,
		n->flags
	);
}
*/

bool lwip_is_netif_up(void *n)
{
	LOCK_TCPIP_CORE();
	bool result = netif_is_up((struct netif*)n);
	UNLOCK_TCPIP_CORE();
	return result;
}

/**
 * Called when a netif is removed (ZTS_EVENT_NETIF_INTERFACE_REMOVED)
 */
#if LWIP_NETIF_REMOVE_CALLBACK
static void netif_remove_callback(struct netif *n)
{
	// Called from core, no need to lock
	if (!n->state) {
		return;
	}
	VirtualTap *tap = (VirtualTap *)n->state;
	uint64_t mac = 0;
	memcpy(&mac, n->hwaddr, n->hwaddr_len);
	struct zts_netif_details *ifd = new zts_netif_details;
	ifd->nwid = tap->_nwid;
	memcpy(&(ifd->mac), n->hwaddr, n->hwaddr_len);
	ifd->mac = lwip_htonl(ifd->mac) >> 16;
	postEvent(ZTS_EVENT_NETIF_REMOVED, (void*)ifd);
}
#endif

/**
 * Called when a link is brought up or down (ZTS_EVENT_NETIF_LINK_UP, ZTS_EVENT_NETIF_LINK_DOWN)
 */
#if LWIP_NETIF_LINK_CALLBACK
static void netif_link_callback(struct netif *n)
{
	// Called from core, no need to lock
	if (!n->state) {
		return;
	}
	VirtualTap *tap = (VirtualTap *)n->state;
	uint64_t mac = 0;
	memcpy(&mac, n->hwaddr, n->hwaddr_len);
	if (n->flags & NETIF_FLAG_LINK_UP) {
		struct zts_netif_details *ifd = new zts_netif_details;
		ifd->nwid = tap->_nwid;
		memcpy(&(ifd->mac), n->hwaddr, n->hwaddr_len);
		ifd->mac = lwip_htonl(ifd->mac) >> 16;
		postEvent(ZTS_EVENT_NETIF_LINK_UP, (void*)ifd);
	}
	if (n->flags & NETIF_FLAG_LINK_UP) {
		struct zts_netif_details *ifd = new zts_netif_details;
		ifd->nwid = tap->_nwid;
		memcpy(&(ifd->mac), n->hwaddr, n->hwaddr_len);
		ifd->mac = lwip_htonl(ifd->mac) >> 16;
		postEvent(ZTS_EVENT_NETIF_LINK_DOWN, (void*)ifd);
	}
}
#endif

void lwip_set_callbacks(struct netif *n)
{
#if LWIP_NETIF_STATUS_CALLBACK
	// Not currently used
	netif_set_status_callback(n, netif_status_callback);
#endif
#if LWIP_NETIF_REMOVE_CALLBACK
	netif_set_remove_callback(n, netif_remove_callback);
#endif
#if LWIP_NETIF_LINK_CALLBACK
	netif_set_link_callback(n, netif_link_callback);
#endif
}

static struct zts_netif_details *lwip_prepare_netif_status_msg(struct netif *n)
{
	VirtualTap *tap = (VirtualTap*)(n->state);
	struct zts_netif_details *ifd = new zts_netif_details;
	// nwid
	ifd->nwid = tap->_nwid;
	// mtu
	ifd->mtu = n->mtu;
	// MAC
	memcpy(&(ifd->mac), n->hwaddr, n->hwaddr_len);
	ifd->mac = htonll(ifd->mac) >> 16;
	return ifd;
}

static err_t netif_init(struct netif *n)
{
	// Called from netif code, no need to lock
	n->hwaddr_len = 6;
	n->name[0]    = 'z';
	n->name[1]    = '4';
	n->linkoutput = lwip_eth_tx;
	n->output     = etharp_output;
	n->mtu        = LWIP_MTU < ZT_MAX_MTU ? LWIP_MTU : ZT_MAX_MTU;
	n->flags      = NETIF_FLAG_BROADCAST
		| NETIF_FLAG_ETHARP
		| NETIF_FLAG_ETHERNET
		| NETIF_FLAG_IGMP
		| NETIF_FLAG_MLD6
		| NETIF_FLAG_LINK_UP
		| NETIF_FLAG_UP;
	n->hwaddr_len = sizeof(n->hwaddr);
	VirtualTap *tap = (VirtualTap*)(n->state);
	tap->_mac.copyTo(n->hwaddr, n->hwaddr_len);
	return ERR_OK;
}

void lwip_init_interface(void *tapref, const MAC &mac, const InetAddress &ip)
{
	char ipbuf[INET6_ADDRSTRLEN];
	char macbuf[ZTS_MAC_ADDRSTRLEN];

	VirtualTap *vtap = (VirtualTap*)tapref;
	struct netif *n = NULL;
	if (vtap->netif) {
		n = (struct netif*)vtap->netif;
	}
	else {
		n = new struct netif;
	}

	if (ip.isV4()) {
		char nmbuf[INET6_ADDRSTRLEN];
		static ip4_addr_t ipaddr, netmask, gw;
		IP4_ADDR(&gw,127,0,0,1);
		ipaddr.addr = *((u32_t *)ip.rawIpData());
		netmask.addr = *((u32_t *)ip.netmask().rawIpData());
		LOCK_TCPIP_CORE();
		netif_add(n, &ipaddr, &netmask, &gw, tapref, netif_init, tcpip_input);
		postEvent(ZTS_EVENT_NETIF_UP, (void*)lwip_prepare_netif_status_msg(n));
		UNLOCK_TCPIP_CORE();
/*
		snprintf(macbuf, ZTS_MAC_ADDRSTRLEN, "%02x:%02x:%02x:%02x:%02x:%02x",
			n->hwaddr[0], n->hwaddr[1], n->hwaddr[2],
			n->hwaddr[3], n->hwaddr[4], n->hwaddr[5]);
		DEBUG_INFO("initialized netif=%p as [mac=%s, addr=%s, nm=%s]",n, 
			macbuf, ip.toString(ipbuf), ip.netmask().toString(nmbuf));
*/
		vtap->netif = (void*)n;
	}
	if (ip.isV6()) {
		static ip6_addr_t ipaddr;
		memcpy(&(ipaddr.addr), ip.rawIpData(), sizeof(ipaddr.addr));
		n->ip6_autoconfig_enabled = 1;

		LOCK_TCPIP_CORE();
		netif_ip6_addr_set(n, 1, &ipaddr);
		netif_create_ip6_linklocal_address(n, 1);
		netif_ip6_addr_set_state(n, 0, IP6_ADDR_TENTATIVE);
		netif_ip6_addr_set_state(n, 1, IP6_ADDR_TENTATIVE);
		n->output_ip6 = ethip6_output;
		postEvent(ZTS_EVENT_NETIF_UP, (void*)lwip_prepare_netif_status_msg(n));
		UNLOCK_TCPIP_CORE();
/*
		snprintf(macbuf, ZTS_MAC_ADDRSTRLEN, "%02x:%02x:%02x:%02x:%02x:%02x",
			n->hwaddr[0], n->hwaddr[1], n->hwaddr[2],
			n->hwaddr[3], n->hwaddr[4], n->hwaddr[5]);
		DEBUG_INFO("initialized netif=%p as [mac=%s, addr=%s]", n,
			macbuf, ip.toString(ipbuf));
*/
	}
}

} // namespace ZeroTier