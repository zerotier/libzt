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
 * Virtual ethernet tap device and combined network stack driver
 */

#include "MAC.hpp"
#include "Mutex.hpp"
#include "InetAddress.hpp"
#include "MulticastGroup.hpp"

#include "lwip/netif.h"
#include "lwip/etharp.h"
#include "lwip/sys.h"
#include "lwip/ethip6.h"
#include "lwip/tcpip.h"
#include "netif/ethernet.h"

#ifdef LWIP_STATS
#include "lwip/stats.h"
#endif

#include "VirtualTap.hpp"
#include "ZeroTierSockets.h"
#include "Events.hpp"
#include "Debug.hpp"

#if defined(__WINDOWS__)
#include <time.h>
#include "Synchapi.h"
#endif

#define ZTS_TAP_THREAD_POLLING_INTERVAL 50
#define LWIP_DRIVER_LOOP_INTERVAL       250

namespace ZeroTier {

extern void _enqueueEvent(int16_t eventCode, void *arg = NULL);

/**
 * A virtual tap device. The ZeroTier core service creates one of these for each
 * virtual network joined. It will be destroyed upon leave().
 */
VirtualTap::VirtualTap(
	const char *homePath,
	const MAC &mac,
	unsigned int mtu,
	unsigned int metric,
	uint64_t nwid,
	const char *friendlyName,
	void (*handler)(void *,void*,uint64_t,const MAC &,const MAC &,
		unsigned int,unsigned int,const void *,unsigned int),
	void *arg) :
		_handler(handler),
		_homePath(homePath),
		_arg(arg),
		_initialized(false),
		_enabled(true),
		_run(true),
		_mac(mac),
		_mtu(mtu),
		_nwid(nwid),
		_unixListenSocket((PhySocket *)0),
		_phy(this,false,true)
{
	memset(vtap_full_name, 0, sizeof(vtap_full_name));
	snprintf(vtap_full_name, sizeof(vtap_full_name), "libzt%llx", (unsigned long long)_nwid);
	_dev = vtap_full_name;
#ifndef __WINDOWS__
	::pipe(_shutdownSignalPipe);
#endif
	// Start virtual tap thread and stack I/O loops
	_thread = Thread::start(this);
}

VirtualTap::~VirtualTap()
{
	struct zts_network_details *nd = new zts_network_details;
	nd->nwid = _nwid;
	_enqueueEvent(ZTS_EVENT_NETWORK_DOWN, (void*)nd);
	_run = false;
#ifndef __WINDOWS__
	::write(_shutdownSignalPipe[1],"\0",1);
#endif
	_phy.whack();
	_lwip_remove_netif(netif4);
	netif4 = NULL;
	_lwip_remove_netif(netif6);
	netif6 = NULL;
	Thread::join(_thread);
#ifndef __WINDOWS__
	::close(_shutdownSignalPipe[0]);
	::close(_shutdownSignalPipe[1]);
#endif
}

void VirtualTap::lastConfigUpdate(uint64_t lastConfigUpdateTime)
{
	_lastConfigUpdateTime = lastConfigUpdateTime;
}

void VirtualTap::setEnabled(bool en)
{
	_enabled = en;
}

bool VirtualTap::enabled() const
{
	return _enabled;
}

bool VirtualTap::hasIpv4Addr()
{
	Mutex::Lock _l(_ips_m);
	std::vector<InetAddress>::iterator it(_ips.begin());
	while (it != _ips.end()) {
		if ((*it).isV4()) { return true; }
		it++;
	}
	return false;
}

bool VirtualTap::hasIpv6Addr()
{
	Mutex::Lock _l(_ips_m);
	std::vector<InetAddress>::iterator it(_ips.begin());
	while (it != _ips.end()) {
		if ((*it).isV6()) { return true; }
		it++;
	}
	return false;
}

bool VirtualTap::addIp(const InetAddress &ip)
{
	char ipbuf[128];
	//ip.toString(ipbuf);
	//DEBUG_INFO("addr=%s", ipbuf);

	/* Limit address assignments to one per type.
	This limitation can be removed if some changes
	are made in the netif driver. */
	if (ip.isV4() && hasIpv4Addr()) {
		ip.toString(ipbuf);
		DEBUG_INFO("failed to add IP (%s), only one per type per netif allowed\n", ipbuf);
		return false;
	}
	if (ip.isV6() && hasIpv6Addr()) {
		ip.toString(ipbuf);
		DEBUG_INFO("failed to add IP (%s), only one per type per netif allowed\n", ipbuf);
		return false;
	}

	Mutex::Lock _l(_ips_m);
	if (_ips.size() >= ZT_MAX_ZT_ASSIGNED_ADDRESSES) {
		return false;
	}
	if (std::find(_ips.begin(),_ips.end(),ip) == _ips.end()) {
		_lwip_init_interface((void*)this, ip);
		// TODO: Add ZTS_EVENT_ADDR_NEW ?
		_ips.push_back(ip);
		// Send callback message
		struct zts_addr_details *ad = new zts_addr_details;
		ad->nwid = _nwid;
		if (ip.isV4()) {
			struct sockaddr_in *in4 = (struct sockaddr_in*)&(ad->addr);
			memcpy(&(in4->sin_addr.s_addr), ip.rawIpData(), 4);
			_enqueueEvent(ZTS_EVENT_ADDR_ADDED_IP4, (void*)ad);
		}
		if (ip.isV6()) {
			struct sockaddr_in6 *in6 = (struct sockaddr_in6*)&(ad->addr);
			memcpy(&(in6->sin6_addr.s6_addr), ip.rawIpData(), 16);
			_enqueueEvent(ZTS_EVENT_ADDR_ADDED_IP6, (void*)ad);
		}
		std::sort(_ips.begin(),_ips.end());
	}
	return true;
}

bool VirtualTap::removeIp(const InetAddress &ip)
{
	Mutex::Lock _l(_ips_m);
	std::vector<InetAddress>::iterator i(std::find(_ips.begin(),_ips.end(),ip));
	if (std::find(_ips.begin(),_ips.end(),ip) != _ips.end()) {
		struct zts_addr_details *ad = new zts_addr_details;
		ad->nwid = _nwid;
		if (ip.isV4()) {
			struct sockaddr_in *in4 = (struct sockaddr_in*)&(ad->addr);
			memcpy(&(in4->sin_addr.s_addr), ip.rawIpData(), 4);
			_enqueueEvent(ZTS_EVENT_ADDR_REMOVED_IP4, (void*)ad);
			// FIXME: De-register from network stack
		}
		if (ip.isV6()) {
			// FIXME: De-register from network stack
			struct sockaddr_in6 *in6 = (struct sockaddr_in6*)&(ad->addr);
			memcpy(&(in6->sin6_addr.s6_addr), ip.rawIpData(), 16);
			_enqueueEvent(ZTS_EVENT_ADDR_REMOVED_IP6, (void*)ad);
		}
		_ips.erase(i);
	}
	return true;
}

std::vector<InetAddress> VirtualTap::ips() const
{
	Mutex::Lock _l(_ips_m);
	return _ips;
}

void VirtualTap::put(const MAC &from,const MAC &to,unsigned int etherType,
	const void *data,unsigned int len)
{
	if (len && _enabled) {
		_lwip_eth_rx(this, from, to, etherType, data, len);
	}
}

std::string VirtualTap::deviceName() const
{
	return _dev;
}

void VirtualTap::setFriendlyName(const char *friendlyName)
{
	DEBUG_INFO("%s", friendlyName);
}

void VirtualTap::scanMulticastGroups(std::vector<MulticastGroup> &added,
	std::vector<MulticastGroup> &removed)
{
	std::vector<MulticastGroup> newGroups;
	Mutex::Lock _l(_multicastGroups_m);
	// TODO: get multicast subscriptions
	std::vector<InetAddress> allIps(ips());
	for (std::vector<InetAddress>::iterator ip(allIps.begin());ip!=allIps.end();++ip)
		newGroups.push_back(MulticastGroup::deriveMulticastGroupForAddressResolution(*ip));

	std::sort(newGroups.begin(),newGroups.end());
	std::unique(newGroups.begin(),newGroups.end());

	for (std::vector<MulticastGroup>::iterator m(newGroups.begin());m!=newGroups.end();++m) {
		if (!std::binary_search(_multicastGroups.begin(),_multicastGroups.end(),*m))
			added.push_back(*m);
	}
	for (std::vector<MulticastGroup>::iterator m(_multicastGroups.begin());m!=_multicastGroups.end();++m) {
		if (!std::binary_search(newGroups.begin(),newGroups.end(),*m))
			removed.push_back(*m);
	}
	_multicastGroups.swap(newGroups);
}

void VirtualTap::setMtu(unsigned int mtu)
{
	_mtu = mtu;
}

void VirtualTap::threadMain()
	throw()
{
	fd_set readfds,nullfds;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO(&readfds);
	FD_ZERO(&nullfds);
	int nfds = (int)std::max(_shutdownSignalPipe[0],0) + 1;
#if defined(__linux__)
	pthread_setname_np(pthread_self(), vtap_full_name);
#endif
#if defined(__APPLE__)
	pthread_setname_np(vtap_full_name);
#endif
	while (true) {
		FD_SET(_shutdownSignalPipe[0],&readfds);
		select(nfds,&readfds,&nullfds,&nullfds,&tv);
		// writes to shutdown pipe terminate thread
		if (FD_ISSET(_shutdownSignalPipe[0],&readfds)) {
			break;
		}
#if defined(__WINDOWS__)
		Sleep(ZTS_TAP_THREAD_POLLING_INTERVAL);
#else
		struct timespec sleepValue = {0};
		sleepValue.tv_nsec = ZTS_TAP_THREAD_POLLING_INTERVAL * 500000;
		nanosleep(&sleepValue, NULL);
#endif
	}
}

void VirtualTap::phyOnDatagram(PhySocket *sock,void **uptr,const struct sockaddr *local_address,
	const struct sockaddr *from,void *data,unsigned long len) {}
void VirtualTap::phyOnTcpConnect(PhySocket *sock,void **uptr,bool success) {}
void VirtualTap::phyOnTcpAccept(PhySocket *sockL,PhySocket *sockN,void **uptrL,void **uptrN,
	const struct sockaddr *from) {}
void VirtualTap::phyOnTcpClose(PhySocket *sock,void **uptr) {}
void VirtualTap::phyOnTcpData(PhySocket *sock,void **uptr,void *data,unsigned long len) {}
void VirtualTap::phyOnTcpWritable(PhySocket *sock,void **uptr) {}
void VirtualTap::phyOnUnixClose(PhySocket *sock,void **uptr) {}

//////////////////////////////////////////////////////////////////////////////
// Netif driver code for lwIP network stack                                 //
//////////////////////////////////////////////////////////////////////////////

bool _has_exited = false;

// Used to generate enumerated lwIP interface names
int netifCount = 0;

// Lock to guard access to network stack state changes
Mutex stackLock;

// Callback for when the TCPIP thread has been successfully started
static void _tcpip_init_done(void *arg)
{
	sys_sem_t *sem;
	sem = (sys_sem_t *)arg;
	_setState(ZTS_STATE_STACK_RUNNING);
	_enqueueEvent(ZTS_EVENT_STACK_UP);
	sys_sem_signal(sem);
}

static void _main_lwip_driver_loop(void *arg)
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
	tcpip_init(_tcpip_init_done, &sem);
	sys_sem_wait(&sem);
	// Main loop
	while(_getState(ZTS_STATE_STACK_RUNNING)) {
		zts_delay_ms(LWIP_DRIVER_LOOP_INTERVAL);
	}
	_has_exited = true;
	_enqueueEvent(ZTS_EVENT_STACK_DOWN);
}

bool _lwip_is_up()
{
	Mutex::Lock _l(stackLock);
	return _getState(ZTS_STATE_STACK_RUNNING);
}

bool _lwip_has_previously_shutdown()
{
	Mutex::Lock _l(stackLock);
	return _has_exited;
}

void _lwip_driver_init()
{
	if (_lwip_is_up()) {
		return;
	}
	if (_lwip_has_previously_shutdown()) {
		return;
	}
	Mutex::Lock _l(stackLock);
#if defined(__WINDOWS__)
	sys_init(); // Required for win32 init of critical sections
#endif
	sys_thread_new(ZTS_LWIP_DRIVER_THREAD_NAME, _main_lwip_driver_loop,
		NULL, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
}

void _lwip_driver_shutdown()
{
	if (_lwip_has_previously_shutdown()) {
		return;
	}
	Mutex::Lock _l(stackLock);
	// Set flag to stop sending frames into the core
	_clrState(ZTS_STATE_STACK_RUNNING);
	// Wait until the main lwIP thread has exited
	while (!_has_exited) { zts_delay_ms(LWIP_DRIVER_LOOP_INTERVAL); }
	/*
	if (tcpip_shutdown() == ERR_OK) {
		sys_timeouts_free();
	}
	*/
}

void _lwip_remove_netif(void *netif)
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

err_t _lwip_eth_tx(struct netif *n, struct pbuf *p)
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

void _lwip_eth_rx(VirtualTap *tap, const MAC &from, const MAC &to, unsigned int etherType,
	const void *data, unsigned int len)
{
#ifdef LWIP_STATS
	stats_display();
#endif
	if (!_getState(ZTS_STATE_STACK_RUNNING)) {
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

bool _lwip_is_netif_up(void *n)
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
static void _netif_remove_callback(struct netif *n)
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
	_enqueueEvent(ZTS_EVENT_NETIF_REMOVED, (void*)ifd);
}
#endif

/**
 * Called when a link is brought up or down (ZTS_EVENT_NETIF_LINK_UP, ZTS_EVENT_NETIF_LINK_DOWN)
 */
#if LWIP_NETIF_LINK_CALLBACK
static void _netif_link_callback(struct netif *n)
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
		_enqueueEvent(ZTS_EVENT_NETIF_LINK_UP, (void*)ifd);
	}
	if (n->flags & NETIF_FLAG_LINK_UP) {
		struct zts_netif_details *ifd = new zts_netif_details;
		ifd->nwid = tap->_nwid;
		memcpy(&(ifd->mac), n->hwaddr, n->hwaddr_len);
		ifd->mac = lwip_htonl(ifd->mac) >> 16;
		_enqueueEvent(ZTS_EVENT_NETIF_LINK_DOWN, (void*)ifd);
	}
}
#endif

void _lwip_set_callbacks(struct netif *n)
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

static struct zts_netif_details *_lwip_prepare_netif_status_msg(struct netif *n)
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

static err_t _netif_init4(struct netif *n)
{
	if (!n || !n->state) {
		return ERR_IF;
	}
	// Called from netif code, no need to lock
	VirtualTap *tap = (VirtualTap*)(n->state);
	n->hwaddr_len = 6;
	n->name[0]    = '4';
	n->name[1]    = 'a'+netifCount;
	n->linkoutput = _lwip_eth_tx;
	n->output     = etharp_output;
	n->mtu        = std::min(LWIP_MTU,(int)tap->_mtu);
	n->flags      = NETIF_FLAG_BROADCAST
		| NETIF_FLAG_ETHARP
		| NETIF_FLAG_ETHERNET
		| NETIF_FLAG_IGMP
		| NETIF_FLAG_MLD6
		| NETIF_FLAG_LINK_UP
		| NETIF_FLAG_UP;
	n->hwaddr_len = sizeof(n->hwaddr);
	tap->_mac.copyTo(n->hwaddr, n->hwaddr_len);
	return ERR_OK;
}

static err_t _netif_init6(struct netif *n)
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
	n->linkoutput = _lwip_eth_tx;
	n->output_ip6 = ethip6_output;
	n->mtu        = std::min(LWIP_MTU,(int)tap->_mtu);
	n->flags      = NETIF_FLAG_BROADCAST
	    | NETIF_FLAG_ETHARP
		| NETIF_FLAG_ETHERNET
		| NETIF_FLAG_IGMP
		| NETIF_FLAG_MLD6
		| NETIF_FLAG_LINK_UP
		| NETIF_FLAG_UP;
	return ERR_OK;
}

void _lwip_init_interface(void *tapref, const InetAddress &ip)
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
		netif_add(n, &ip4, &netmask, &gw, (void*)vtap, _netif_init4, tcpip_input);
		vtap->netif4 = (void*)n;
		_enqueueEvent(ZTS_EVENT_NETIF_UP, (void*)_lwip_prepare_netif_status_msg(n));
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
			netif_add(n, NULL, NULL, NULL, (void*)vtap, _netif_init6, ethernet_input);
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
		_enqueueEvent(ZTS_EVENT_NETIF_UP, (void*)_lwip_prepare_netif_status_msg(n));
		snprintf(macbuf, ZTS_MAC_ADDRSTRLEN, "%02x:%02x:%02x:%02x:%02x:%02x",
			n->hwaddr[0], n->hwaddr[1], n->hwaddr[2],
			n->hwaddr[3], n->hwaddr[4], n->hwaddr[5]);
		DEBUG_INFO("initialized netif=%p as [mac=%s, addr=%s, tap=%p]", n,
			macbuf, ip.toString(ipbuf), vtap);
	}
}

} // namespace ZeroTier
