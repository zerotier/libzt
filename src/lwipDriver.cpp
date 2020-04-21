/*
 * Copyright (c)2013-2020 ZeroTier, Inc.
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file in the project's root directory.
 *
 * Change Date: 2024-01-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2.0 of the Apache License.
 */
/****/

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

#ifdef LWIP_STATS
#include "lwip/stats.h"
#endif

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
int netifCount = 0;
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
	sys_thread_new(ZTS_LWIP_DRIVER_THREAD_NAME, main_lwip_driver_loop,
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
	if (!netif) {
		return;
	}
	struct netif *n = (struct netif*)netif;
	LOCK_TCPIP_CORE();
	netif_remove(n);
	netif_set_down(n);
	netif_set_link_down(n);
	UNLOCK_TCPIP_CORE();
}

err_t lwip_eth_tx(struct netif *n, struct pbuf *p)
{
	if (!n) {
		return ERR_IF;
	}
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
		/*
		DEBUG_TRANS("len=%5d dst=%s [%s TX <-- %s] ethertype=0x%04x %s", totalLength, macBuf, nodeBuf, tap->nodeId().c_str(),
			Utils::ntoh(ethhdr->type), flagbuf);
		*/
	}
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
		/*
		DEBUG_TRANS("len=%5d dst=%s [%s RX --> %s] ethertype=0x%04x %s", len, macBuf, nodeBuf, tap->nodeId().c_str(),
			Utils::ntoh(ethhdr.type), flagbuf);
		*/
	}

	p = pbuf_alloc(PBUF_RAW, (uint16_t)len+sizeof(struct eth_hdr), PBUF_RAM);
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

	if (Utils::ntoh(ethhdr.type) == 0x800 || Utils::ntoh(ethhdr.type) == 0x806) {
		if ((err = ((struct netif *)tap->netif4)->input(p, (struct netif *)tap->netif4)) != ERR_OK) {
			DEBUG_ERROR("packet input error (%d)", err);
			pbuf_free(p);
		}
	}
	if (Utils::ntoh(ethhdr.type) == 0x86DD) {
		if ((err = ((struct netif *)tap->netif6)->input(p, (struct netif *)tap->netif6)) != ERR_OK) {
			DEBUG_ERROR("packet input error (%d)", err);
			pbuf_free(p);
		}
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
	if (!n) {
		return false;
	}
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
	if (!n || !n->state) {
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
	if (!n || !n->state) {
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
	if (!n) {
		return;
	}
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
	if (!n || !n->state) {
		return NULL;
	}
	VirtualTap *tap = (VirtualTap*)(n->state);
	struct zts_netif_details *ifd = new zts_netif_details;
	ifd->nwid = tap->_nwid;
	ifd->mtu = n->mtu;
	memcpy(&(ifd->mac), n->hwaddr, n->hwaddr_len);
	ifd->mac = htonll(ifd->mac) >> 16;
	return ifd;
}

static err_t netif_init(struct netif *n)
{
	if (!n || !n->state) {
		return ERR_IF;
	}
	// Called from netif code, no need to lock
	n->hwaddr_len = 6;
	n->name[0]    = '4';
	n->name[1]    = 'a'+netifCount;
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

static err_t netif_init6(struct netif *n)
{
	if (!n || !n->state) {
		return ERR_IF;
	}
	n->hwaddr_len = sizeof(n->hwaddr);
	VirtualTap *tap = (VirtualTap*)(n->state);
	tap->_mac.copyTo(n->hwaddr, n->hwaddr_len);
	// Called from netif code, no need to lock
	n->hwaddr_len = 6;
	n->name[0]    = '6';
	n->name[1]    = 'a'+netifCount;
	n->linkoutput = lwip_eth_tx;
	n->output_ip6 = ethip6_output;
	n->mtu        = LWIP_MTU < ZT_MAX_MTU ? LWIP_MTU : ZT_MAX_MTU;
	n->flags      = NETIF_FLAG_BROADCAST
	    | NETIF_FLAG_ETHARP
		| NETIF_FLAG_ETHERNET
		| NETIF_FLAG_IGMP
		| NETIF_FLAG_MLD6
		| NETIF_FLAG_LINK_UP
		| NETIF_FLAG_UP;
	return ERR_OK;
}

void lwip_init_interface(void *tapref, const InetAddress &ip)
{
	char ipbuf[INET6_ADDRSTRLEN];
	char macbuf[ZTS_MAC_ADDRSTRLEN];

	VirtualTap *vtap = (VirtualTap*)tapref;
	struct netif *n = NULL;
	bool isNewNetif = false;

	if (ip.isV4()) {
		if (vtap->netif4) {
			n = (struct netif*)vtap->netif4;
		}
		else {
			n = new struct netif;
			isNewNetif = true;
			netifCount++;
		}
		char nmbuf[INET6_ADDRSTRLEN];
		static ip4_addr_t ip4, netmask, gw;
		IP4_ADDR(&gw,127,0,0,1);
		ip4.addr = *((u32_t *)ip.rawIpData());
		netmask.addr = *((u32_t *)ip.netmask().rawIpData());
		LOCK_TCPIP_CORE();
		netif_add(n, &ip4, &netmask, &gw, (void*)vtap, netif_init, tcpip_input);
		vtap->netif4 = (void*)n;
		postEvent(ZTS_EVENT_NETIF_UP, (void*)lwip_prepare_netif_status_msg(n));
		UNLOCK_TCPIP_CORE();
		snprintf(macbuf, ZTS_MAC_ADDRSTRLEN, "%02x:%02x:%02x:%02x:%02x:%02x",
			n->hwaddr[0], n->hwaddr[1], n->hwaddr[2],
			n->hwaddr[3], n->hwaddr[4], n->hwaddr[5]);
		DEBUG_INFO("initialized netif=%p as [mac=%s, addr=%s, nm=%s, tap=%p]",n,
			macbuf, ip.toString(ipbuf), ip.netmask().toString(nmbuf), vtap);
	}
	if (ip.isV6()) {
		if (vtap->netif6) {
			n = (struct netif*)vtap->netif6;
		}
		else {
			n = new struct netif;
			isNewNetif = true;
			netifCount++;
		}
		static ip6_addr_t ip6;
		memcpy(&(ip6.addr), ip.rawIpData(), sizeof(ip6.addr));
		LOCK_TCPIP_CORE();
		if (isNewNetif) {
			vtap->netif6 = (void*)n;
			netif_add(n, NULL, NULL, NULL, (void*)vtap, netif_init6, ethernet_input);
			n->ip6_autoconfig_enabled = 1;
			vtap->_mac.copyTo(n->hwaddr, n->hwaddr_len);
			netif_create_ip6_linklocal_address(n, 1);
			netif_set_link_up(n);
			netif_set_up(n);
			netif_set_default(n);
		}
		netif_add_ip6_address(n,&ip6,NULL);
		n->output_ip6 = ethip6_output;
		UNLOCK_TCPIP_CORE();
		postEvent(ZTS_EVENT_NETIF_UP, (void*)lwip_prepare_netif_status_msg(n));
		snprintf(macbuf, ZTS_MAC_ADDRSTRLEN, "%02x:%02x:%02x:%02x:%02x:%02x",
			n->hwaddr[0], n->hwaddr[1], n->hwaddr[2],
			n->hwaddr[3], n->hwaddr[4], n->hwaddr[5]);
		DEBUG_INFO("initialized netif=%p as [mac=%s, addr=%s, tap=%p]", n,
			macbuf, ip.toString(ipbuf), vtap);
	}
}

} // namespace ZeroTier