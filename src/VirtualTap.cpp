/*
 * Copyright (c)2013-2021 ZeroTier, Inc.
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file in the project's root directory.
 *
 * Change Date: 2026-01-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2.0 of the Apache License.
 */
/****/

/**
 * @file
 *
 * Virtual Ethernet tap device and combined network stack driver
 */

#include "InetAddress.hpp"
#include "MAC.hpp"
#include "MulticastGroup.hpp"
#include "Mutex.hpp"
#include "OSUtils.hpp"
#include "lwip/etharp.h"
#include "lwip/ethip6.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#include "lwip/tcpip.h"
#include "netif/ethernet.h"

#ifdef LWIP_STATS
#include "lwip/stats.h"
#endif

#include "Events.hpp"
#include "VirtualTap.hpp"

#if defined(__WINDOWS__)
#include "Synchapi.h"

#include <time.h>
#endif

#define ZTS_TAP_THREAD_POLLING_INTERVAL 50
#define LWIP_DRIVER_LOOP_INTERVAL       100

namespace ZeroTier {

extern Events* zts_events;

/**
 * Virtual tap device. ZeroTier will create one per joined network. It will
 * then be destroyed upon leaving the network.
 */
VirtualTap::VirtualTap(
    const char* homePath,
    const MAC& mac,
    unsigned int mtu,
    unsigned int metric,
    uint64_t net_id,
    void (*handler)(
        void*,
        void*,
        uint64_t,
        const MAC&,
        const MAC&,
        unsigned int,
        unsigned int,
        const void*,
        unsigned int),
    void* arg)
    : _handler(handler)
    , _homePath(homePath)
    , _arg(arg)
    , _initialized(false)
    , _enabled(true)
    , _run(true)
    , _mac(mac)
    , _mtu(mtu)
    , _net_id(net_id)
    , _phy(this, false, true)
{
    OSUtils::ztsnprintf(vtap_full_name, VTAP_NAME_LEN, "libzt-vtap-%llx", _net_id);
#ifndef __WINDOWS__
    ::pipe(_shutdownSignalPipe);
#endif
    // Start virtual tap thread and stack I/O loops
    _thread = Thread::start(this);
}

VirtualTap::~VirtualTap()
{
    _run = false;
#ifndef __WINDOWS__
    ::write(_shutdownSignalPipe[1], "\0", 1);
#endif
    _phy.whack();
    zts_lwip_remove_netif(netif4);
    netif4 = NULL;
    zts_lwip_remove_netif(netif6);
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

void VirtualTap::setUserEventSystem(Events* events)
{
    _events = events;
}

bool VirtualTap::hasIpv4Addr()
{
    Mutex::Lock _l(_ips_m);
    std::vector<InetAddress>::iterator it(_ips.begin());
    while (it != _ips.end()) {
        if ((*it).isV4()) {
            return true;
        }
        ++it;
    }
    return false;
}

bool VirtualTap::hasIpv6Addr()
{
    Mutex::Lock _l(_ips_m);
    std::vector<InetAddress>::iterator it(_ips.begin());
    while (it != _ips.end()) {
        if ((*it).isV6()) {
            return true;
        }
        ++it;
    }
    return false;
}

bool VirtualTap::addIp(const InetAddress& ip)
{
    // TODO: Rewrite to allow for more addresses
    char ipbuf[128] = { 0 };
    /* Limit address assignments to one per type.
    This limitation can be removed if some changes
    are made in the netif driver. */
    if (ip.isV4() && hasIpv4Addr()) {
        ip.toString(ipbuf);
        // DEBUG_INFO("failed to add IP (%s), only one per type per netif
        // allowed\n", ipbuf);
        return false;
    }
    if (ip.isV6() && hasIpv6Addr()) {
        ip.toString(ipbuf);
        // DEBUG_INFO("failed to add IP (%s), only one per type per netif
        // allowed\n", ipbuf);
        return false;
    }

    Mutex::Lock _l(_ips_m);
    if (_ips.size() >= ZT_MAX_ZT_ASSIGNED_ADDRESSES) {
        return false;
    }
    if (std::find(_ips.begin(), _ips.end(), ip) == _ips.end()) {
        zts_lwip_init_interface((void*)this, ip);
        _ips.push_back(ip);
        std::sort(_ips.begin(), _ips.end());
    }
    return true;
}

bool VirtualTap::removeIp(const InetAddress& ip)
{
    Mutex::Lock _l(_ips_m);
    if (std::find(_ips.begin(), _ips.end(), ip) != _ips.end()) {
        std::vector<InetAddress>::iterator i(std::find(_ips.begin(), _ips.end(), ip));
        zts_lwip_remove_address_from_netif((void*)this, ip);
        _ips.erase(i);
    }
    return true;
}

std::vector<InetAddress> VirtualTap::ips() const
{
    Mutex::Lock _l(_ips_m);
    return _ips;
}

void VirtualTap::put(const MAC& from, const MAC& to, unsigned int etherType, const void* data, unsigned int len)
{
    if (len && _enabled) {
        zts_lwip_eth_rx(this, from, to, etherType, data, len);
    }
}

void VirtualTap::scanMulticastGroups(std::vector<MulticastGroup>& added, std::vector<MulticastGroup>& removed)
{
    std::vector<MulticastGroup> newGroups;
    Mutex::Lock _l(_multicastGroups_m);
    // TODO: get multicast subscriptions
    std::vector<InetAddress> allIps(ips());
    for (std::vector<InetAddress>::iterator ip(allIps.begin()); ip != allIps.end(); ++ip)
        newGroups.push_back(MulticastGroup::deriveMulticastGroupForAddressResolution(*ip));

    std::sort(newGroups.begin(), newGroups.end());

    for (std::vector<MulticastGroup>::iterator m(newGroups.begin()); m != newGroups.end(); ++m) {
        if (! std::binary_search(_multicastGroups.begin(), _multicastGroups.end(), *m))
            added.push_back(*m);
    }
    for (std::vector<MulticastGroup>::iterator m(_multicastGroups.begin()); m != _multicastGroups.end(); ++m) {
        if (! std::binary_search(newGroups.begin(), newGroups.end(), *m))
            removed.push_back(*m);
    }
    _multicastGroups.swap(newGroups);
}

void VirtualTap::setMtu(unsigned int mtu)
{
    _mtu = mtu;
}

void VirtualTap::threadMain() throw()
{
    fd_set readfds, nullfds;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&readfds);
    FD_ZERO(&nullfds);
    int nfds = (int)std::max(_shutdownSignalPipe[0], 0) + 1;
#if defined(__linux__)
    // pthread_setname_np(pthread_self(), vtap_full_name);
#endif
#if defined(__APPLE__)
    // pthread_setname_np(vtap_full_name);
#endif
    while (true) {
        FD_SET(_shutdownSignalPipe[0], &readfds);
        select(nfds, &readfds, &nullfds, &nullfds, &tv);
        // writes to shutdown pipe terminate thread
        if (FD_ISSET(_shutdownSignalPipe[0], &readfds)) {
            break;
        }
#if defined(__WINDOWS__)
        Sleep(ZTS_TAP_THREAD_POLLING_INTERVAL);
#else
        struct timespec sleepValue = { 0, 0 };
        sleepValue.tv_nsec = ZTS_TAP_THREAD_POLLING_INTERVAL * 500000;
        nanosleep(&sleepValue, NULL);
#endif
    }
}

//----------------------------------------------------------------------------//
// Netif driver code for lwIP network stack                                   //
//----------------------------------------------------------------------------//

bool _has_exited = false;
bool _has_started = false;

// Used to generate enumerated lwIP interface names
int netifCount = 0;

// Lock to guard access to network stack state changes
Mutex lwip_state_m;

// Callback for when the TCPIP thread has been successfully started
static void zts_tcpip_init_done(void* arg)
{
    sys_sem_t* sem;
    sem = (sys_sem_t*)arg;
    zts_events->setState(ZTS_STATE_STACK_RUNNING);
    _has_started = true;
    // zts_events->enqueue(ZTS_EVENT_STACK_UP, NULL);
    sys_sem_signal(sem);
}

static void zts_main_lwip_driver_loop(void* arg)
{
#if defined(__linux__)
    // pthread_setname_np(pthread_self(), ZTS_LWIP_THREAD_NAME);
#endif
#if defined(__APPLE__)
    // pthread_setname_np(ZTS_LWIP_THREAD_NAME);
#endif
    sys_sem_t sem;
    LWIP_UNUSED_ARG(arg);
    if (sys_sem_new(&sem, 0) != ERR_OK) {
        // DEBUG_ERROR("failed to create semaphore");
    }
    tcpip_init(zts_tcpip_init_done, &sem);
    sys_sem_wait(&sem);
    // Main loop
    while (zts_events->getState(ZTS_STATE_STACK_RUNNING)) {
        zts_util_delay(LWIP_DRIVER_LOOP_INTERVAL);
    }
    _has_exited = true;
    zts_events->enqueue(ZTS_EVENT_STACK_DOWN, NULL);
}

bool zts_lwip_is_up()
{
    Mutex::Lock _l(lwip_state_m);
    return zts_events->getState(ZTS_STATE_STACK_RUNNING);
}

void zts_lwip_driver_init()
{
    if (zts_lwip_is_up()) {
        return;
    }
    if (_has_exited) {
        return;
    }
    Mutex::Lock _l(lwip_state_m);
#if defined(__WINDOWS__)
    sys_init();   // Required for win32 init of critical sections
#endif
    sys_thread_new(
        ZTS_LWIP_THREAD_NAME,
        zts_main_lwip_driver_loop,
        NULL,
        DEFAULT_THREAD_STACKSIZE,
        DEFAULT_THREAD_PRIO);
}

void zts_lwip_driver_shutdown()
{
    if (_has_exited) {
        return;
    }
    Mutex::Lock _l(lwip_state_m);
    // Set flag to stop sending frames into the core
    zts_events->clrState(ZTS_STATE_STACK_RUNNING);
    // Wait until the main lwIP thread has exited
    if (_has_started) {
        while (! _has_exited) {
            zts_util_delay(LWIP_DRIVER_LOOP_INTERVAL);
        }
    }
}

void zts_lwip_remove_netif(void* netif)
{
    if (! netif) {
        return;
    }
    struct netif* n = (struct netif*)netif;
    LOCK_TCPIP_CORE();
    netif_remove(n);
    netif_set_down(n);
    netif_set_link_down(n);
    UNLOCK_TCPIP_CORE();
}

signed char zts_lwip_eth_tx(struct netif* n, struct pbuf* p)
{
    if (! n) {
        return ERR_IF;
    }
    struct pbuf* q;
    char buf[ZT_MAX_MTU + 32] = { 0 };
    char* bufptr;
    int totalLength = 0;

    VirtualTap* tap = (VirtualTap*)n->state;
    bufptr = buf;
    for (q = p; q != NULL; q = q->next) {
        memcpy(bufptr, q->payload, q->len);
        bufptr += q->len;
        totalLength += q->len;
    }
    struct eth_hdr* ethhdr;
    ethhdr = (struct eth_hdr*)buf;

    MAC src_mac;
    MAC dest_mac;
    src_mac.setTo(ethhdr->src.addr, 6);
    dest_mac.setTo(ethhdr->dest.addr, 6);

    char* data = buf + sizeof(struct eth_hdr);
    int len = totalLength - sizeof(struct eth_hdr);
    int proto = Utils::ntoh((uint16_t)ethhdr->type);
    tap->_handler(tap->_arg, NULL, tap->_net_id, src_mac, dest_mac, proto, 0, data, len);

    return ERR_OK;
}

void zts_lwip_eth_rx(
    VirtualTap* tap,
    const MAC& from,
    const MAC& to,
    unsigned int etherType,
    const void* data,
    unsigned int len)
{
#ifdef LWIP_STATS
    stats_display();
#endif
    if (! zts_events->getState(ZTS_STATE_STACK_RUNNING)) {
        return;
    }
    struct pbuf *p, *q;
    struct eth_hdr ethhdr;
    from.copyTo(ethhdr.src.addr, 6);
    to.copyTo(ethhdr.dest.addr, 6);
    ethhdr.type = Utils::hton((uint16_t)etherType);

    p = pbuf_alloc(PBUF_RAW, (uint16_t)len + sizeof(struct eth_hdr), PBUF_RAM);
    if (! p) {
        // DEBUG_ERROR("dropped packet: unable to allocate memory for
        // pbuf");
        return;
    }
    // First pbuf gets Ethernet header at start
    q = p;
    if (q->len < sizeof(ethhdr)) {
        pbuf_free(p);
        p = NULL;
        // DEBUG_ERROR("dropped packet: first pbuf smaller than Ethernet
        // header");
        return;
    }
    // Copy frame data into pbuf
    const char* dataptr = reinterpret_cast<const char*>(data);
    memcpy(q->payload, &ethhdr, sizeof(ethhdr));
    int remainingPayloadSpace = q->len - sizeof(ethhdr);
    memcpy((char*)q->payload + sizeof(ethhdr), dataptr, remainingPayloadSpace);
    dataptr += remainingPayloadSpace;
    // Remaining pbufs (if any) get rest of data
    while ((q = q->next)) {
        memcpy(q->payload, dataptr, q->len);
        dataptr += q->len;
    }
    // Feed packet into stack
    int err;

    if (Utils::ntoh(ethhdr.type) == 0x800 || Utils::ntoh(ethhdr.type) == 0x806) {
        if (tap->netif4) {
            if ((err = ((struct netif*)tap->netif4)->input(p, (struct netif*)tap->netif4)) != ERR_OK) {
                // DEBUG_ERROR("packet input error (%d)", err);
                pbuf_free(p);
            }
        }
    }
    if (Utils::ntoh(ethhdr.type) == 0x86DD) {
        if (tap->netif6) {
            if ((err = ((struct netif*)tap->netif6)->input(p, (struct netif*)tap->netif6)) != ERR_OK) {
                // DEBUG_ERROR("packet input error (%d)", err);
                pbuf_free(p);
            }
        }
    }
}

bool zts_lwip_is_netif_up(void* n)
{
    if (! n) {
        return false;
    }
    LOCK_TCPIP_CORE();
    bool result = netif_is_up((struct netif*)n);
    UNLOCK_TCPIP_CORE();
    return result;
}

static err_t zts_netif_init4(struct netif* n)
{
    if (! n || ! n->state) {
        return ERR_IF;
    }
    // Called from core, no need to lock
    VirtualTap* tap = (VirtualTap*)(n->state);
    n->hwaddr_len = 6;
    n->name[0] = '4';
    n->name[1] = 'a' + netifCount;
    n->linkoutput = zts_lwip_eth_tx;
    n->output = etharp_output;
    n->mtu = std::min(LWIP_MTU, (int)tap->_mtu);
    n->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_IGMP | NETIF_FLAG_MLD6
               | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
    n->hwaddr_len = sizeof(n->hwaddr);
    tap->_mac.copyTo(n->hwaddr, n->hwaddr_len);
    return ERR_OK;
}

static err_t zts_netif_init6(struct netif* n)
{
    if (! n || ! n->state) {
        return ERR_IF;
    }
    n->hwaddr_len = sizeof(n->hwaddr);
    VirtualTap* tap = (VirtualTap*)(n->state);
    tap->_mac.copyTo(n->hwaddr, n->hwaddr_len);
    // Called from core, no need to lock
    n->hwaddr_len = 6;
    n->name[0] = '6';
    n->name[1] = 'a' + netifCount;
    n->linkoutput = zts_lwip_eth_tx;
    n->output_ip6 = ethip6_output;
    n->mtu = std::min(LWIP_MTU, (int)tap->_mtu);
    n->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_IGMP | NETIF_FLAG_MLD6
               | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
    return ERR_OK;
}

void zts_lwip_init_interface(void* tapref, const InetAddress& ip)
{
    char macbuf[ZTS_MAC_ADDRSTRLEN] = { 0 };

    VirtualTap* vtap = (VirtualTap*)tapref;
    struct netif* n = NULL;
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

        static ip4_addr_t ip4, netmask, gw;
        IP4_ADDR(&gw, 127, 0, 0, 1);
        ip4.addr = *((u32_t*)ip.rawIpData());
        netmask.addr = *((u32_t*)ip.netmask().rawIpData());
        LOCK_TCPIP_CORE();
        netif_add(n, &ip4, &netmask, &gw, (void*)vtap, zts_netif_init4, tcpip_input);
        vtap->netif4 = (void*)n;
        UNLOCK_TCPIP_CORE();
        snprintf(
            macbuf,
            ZTS_MAC_ADDRSTRLEN,
            "%02x:%02x:%02x:%02x:%02x:%02x",
            n->hwaddr[0],
            n->hwaddr[1],
            n->hwaddr[2],
            n->hwaddr[3],
            n->hwaddr[4],
            n->hwaddr[5]);
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
            netif_add(n, NULL, NULL, NULL, (void*)vtap, zts_netif_init6, ethernet_input);
            n->ip6_autoconfig_enabled = 1;
            vtap->_mac.copyTo(n->hwaddr, n->hwaddr_len);
            netif_create_ip6_linklocal_address(n, 1);
            netif_set_link_up(n);
            netif_set_up(n);
            netif_set_default(n);
        }
        netif_add_ip6_address(n, &ip6, NULL);
        n->output_ip6 = ethip6_output;
        UNLOCK_TCPIP_CORE();
        snprintf(
            macbuf,
            ZTS_MAC_ADDRSTRLEN,
            "%02x:%02x:%02x:%02x:%02x:%02x",
            n->hwaddr[0],
            n->hwaddr[1],
            n->hwaddr[2],
            n->hwaddr[3],
            n->hwaddr[4],
            n->hwaddr[5]);
    }
}

void zts_lwip_remove_address_from_netif(void* tapref, const InetAddress& ip)
{
    if (! tapref) {
        return;
    }
    VirtualTap* vtap = (VirtualTap*)tapref;
    struct netif* n = NULL;
    /* When true multi-homing is implemented this will need to
    be a bit more sophisticated */
    if (ip.isV4()) {
        if (vtap->netif4) {
            n = (struct netif*)vtap->netif4;
        }
    }
    if (ip.isV6()) {
        if (vtap->netif6) {
            n = (struct netif*)vtap->netif6;
        }
    }
    if (! n) {
        return;
    }
    zts_lwip_remove_netif(n);
}

}   // namespace ZeroTier
