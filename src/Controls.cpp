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
 * Node / Network control interface
 */

#include "Events.hpp"
#include "NodeService.hpp"
#include "Signals.hpp"
#include "VirtualTap.hpp"

#include <string.h>

using namespace ZeroTier;

#ifdef __WINDOWS__
#include <Windows.h>
WSADATA wsaData;
#endif

#ifdef ZTS_ENABLE_PYTHON
#define ZTS_ENABLE_CUSTOM_SIGNAL_HANDLERS 1
#endif

namespace ZeroTier {

#ifdef ZTS_ENABLE_PYTHON
#endif
#ifdef ZTS_ENABLE_PINVOKE
extern void (*_userEventCallback)(void*);
#endif
#ifdef ZTS_C_API_ONLY
extern void (*_userEventCallback)(void*);
#endif
extern uint8_t allowNetworkCaching;
extern uint8_t allowPeerCaching;

NodeService* zts_service;
Events* zts_events;

extern Mutex events_m;
Mutex service_m;

int init_subsystems()
{
    /** Set up service and callback threads and tell them about one another.
     * A separate thread is used for callbacks so that if the user fails to
     * return control it won't affect the core service's operations. */
    if (! zts_events) {
        zts_events = new Events();
    }
    if (zts_events->getState(ZTS_STATE_FREE_CALLED)) {
        return ZTS_ERR_SERVICE;
    }
#ifdef ZTS_ENABLE_CUSTOM_SIGNAL_HANDLERS
    zts_install_signal_handlers();
#endif   // ZTS_ENABLE_CUSTOM_SIGNAL_HANDLERS
    if (! zts_service) {
#if defined(__WINDOWS__)
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
        zts_service = new NodeService();
        zts_service->setUserEventSystem(zts_events);
    }
    return ZTS_ERR_OK;
}

#ifdef __cplusplus
extern "C" {
#endif

int zts_init_from_storage(const char* path)
{
    ACQUIRE_SERVICE_OFFLINE();
    zts_service->setHomePath(path);
    return ZTS_ERR_OK;
}

int zts_init_from_memory(const char* keypair, unsigned int len)
{
    ACQUIRE_SERVICE_OFFLINE();
    return zts_service->setIdentity(keypair, len);
}

int zts_init_from_secret(const char* secret, unsigned int len)
{
    ACQUIRE_SERVICE_OFFLINE();
    return zts_service->setIdentityFromSecret(secret, len);
}

#ifdef ZTS_ENABLE_PYTHON
int zts_init_set_event_handler(PythonDirectorCallbackClass* callback)
#endif
#ifdef ZTS_ENABLE_PINVOKE
    int zts_init_set_event_handler(CppCallback callback)
#endif
#ifdef ZTS_ENABLE_JAVA
        int zts_init_set_event_handler(jobject obj_ref, jmethodID id)
#endif
#ifdef ZTS_C_API_ONLY
            int zts_init_set_event_handler(void (*callback)(void*))
#endif
{
    ACQUIRE_SERVICE_OFFLINE();
#ifdef ZTS_ENABLE_JAVA
    zts_events->setJavaCallback(obj_ref, id);
#else
    if (! callback) {
        return ZTS_ERR_ARG;
    }
    _userEventCallback = callback;
#endif
    zts_service->enableEvents();
    return ZTS_ERR_OK;
}

int zts_init_blacklist_if(const char* prefix, unsigned int len)
{
    ACQUIRE_SERVICE_OFFLINE();
    return zts_service->addInterfacePrefixToBlacklist(prefix, len);
}

int zts_init_set_roots(const void* roots_data, unsigned int len)
{
    ACQUIRE_SERVICE_OFFLINE();
    return zts_service->setRoots(roots_data, len);
}

int zts_init_set_port(unsigned short port)
{
    ACQUIRE_SERVICE_OFFLINE();
    zts_service->setPrimaryPort(port);
    return ZTS_ERR_OK;
}

int zts_init_set_random_port_range(unsigned short start_port, unsigned short end_port)
{
    ACQUIRE_SERVICE_OFFLINE();
    zts_service->setRandomPortRange(start_port, end_port);
    return ZTS_ERR_OK;
}

int zts_init_allow_secondary_port(unsigned int allowed)
{
    ACQUIRE_SERVICE_OFFLINE();
    return zts_service->allowSecondaryPort(allowed);
}

int zts_init_allow_port_mapping(unsigned int allowed)
{
    ACQUIRE_SERVICE_OFFLINE();
    return zts_service->allowPortMapping(allowed);
}

int zts_init_allow_peer_cache(unsigned int allowed)
{
    ACQUIRE_SERVICE_OFFLINE();
    return zts_service->allowPeerCaching(allowed);
}

int zts_init_allow_net_cache(unsigned int allowed)
{
    ACQUIRE_SERVICE_OFFLINE();
    return zts_service->allowNetworkCaching(allowed);
}

int zts_init_allow_roots_cache(unsigned int allowed)
{
    ACQUIRE_SERVICE_OFFLINE();
    return zts_service->allowRootSetCaching(allowed);
}

int zts_init_allow_id_cache(unsigned int allowed)
{
    ACQUIRE_SERVICE_OFFLINE();
    return zts_service->allowIdentityCaching(allowed);
}

int zts_addr_compute_6plane(const uint64_t net_id, const uint64_t node_id, struct zts_sockaddr_storage* addr)
{
    if (! addr || ! net_id || ! node_id) {
        return ZTS_ERR_ARG;
    }
    InetAddress _6planeAddr = InetAddress::makeIpv66plane(net_id, node_id);
    struct sockaddr_in6* in6 = (struct sockaddr_in6*)addr;
    memcpy(in6->sin6_addr.s6_addr, _6planeAddr.rawIpData(), sizeof(struct in6_addr));
    return ZTS_ERR_OK;
}

int zts_addr_compute_rfc4193(const uint64_t net_id, const uint64_t node_id, struct zts_sockaddr_storage* addr)
{
    if (! addr || ! net_id || ! node_id) {
        return ZTS_ERR_ARG;
    }
    InetAddress _rfc4193Addr = InetAddress::makeIpv6rfc4193(net_id, node_id);
    struct sockaddr_in6* in6 = (struct sockaddr_in6*)addr;
    memcpy(in6->sin6_addr.s6_addr, _rfc4193Addr.rawIpData(), sizeof(struct in6_addr));
    return ZTS_ERR_OK;
}

int zts_addr_compute_rfc4193_str(uint64_t net_id, uint64_t node_id, char* dst, unsigned int len)
{
    if (! net_id || ! node_id || ! dst || len != ZTS_IP_MAX_STR_LEN) {
        return ZTS_ERR_ARG;
    }
    struct zts_sockaddr_storage ss;
    int err = ZTS_ERR_OK;
    if ((err = zts_addr_compute_rfc4193(net_id, node_id, &ss)) != ZTS_ERR_OK) {
        return err;
    }
    struct zts_sockaddr_in6* in6 = (struct zts_sockaddr_in6*)&ss;
    zts_inet_ntop(ZTS_AF_INET6, &(in6->sin6_addr), dst, ZTS_IP_MAX_STR_LEN);
    return ZTS_ERR_OK;
}

int zts_addr_compute_6plane_str(uint64_t net_id, uint64_t node_id, char* dst, unsigned int len)
{
    if (! net_id || ! node_id || ! dst || len != ZTS_IP_MAX_STR_LEN) {
        return ZTS_ERR_ARG;
    }
    struct zts_sockaddr_storage ss;
    int err = ZTS_ERR_OK;
    if ((err = zts_addr_compute_6plane(net_id, node_id, &ss)) != ZTS_ERR_OK) {
        return err;
    }
    struct zts_sockaddr_in6* in6 = (struct zts_sockaddr_in6*)&ss;
    zts_inet_ntop(ZTS_AF_INET6, &(in6->sin6_addr), dst, ZTS_IP_MAX_STR_LEN);
    return ZTS_ERR_OK;
}

uint64_t zts_net_compute_adhoc_id(unsigned short start_port, unsigned short end_port)
{
    char net_id_str[ZTS_INET6_ADDRSTRLEN] = { 0 };
    OSUtils::ztsnprintf(net_id_str, ZTS_INET6_ADDRSTRLEN, "ff%04x%04x000000", start_port, end_port);
    return strtoull(net_id_str, NULL, 16);
}

int zts_id_new(char* key, unsigned int* dst_len)
{
    if (key == NULL || *dst_len != ZT_IDENTITY_STRING_BUFFER_LENGTH) {
        return ZTS_ERR_ARG;
    }
    Identity id;
    id.generate();
    char idtmp[1024] = { 0 };
    std::string idser = id.toString(true, idtmp);
    unsigned int key_pair_len = idser.length();
    if (key_pair_len > *dst_len) {
        return ZTS_ERR_ARG;
    }
    memcpy(key, idser.c_str(), key_pair_len);
    *dst_len = key_pair_len;
    return ZTS_ERR_OK;
}

int zts_id_pair_is_valid(const char* key, unsigned int len)
{
    if (key == NULL || len != ZT_IDENTITY_STRING_BUFFER_LENGTH) {
        return false;
    }
    Identity id;
    if ((strnlen(key, len) > 32) && (key[10] == ':')) {
        if (id.fromString(key)) {
            return id.locallyValidate();
        }
    }
    return false;
}

int zts_node_get_id_pair(char* key, unsigned int* key_dst_len)
{
    ACQUIRE_SERVICE(ZTS_ERR_SERVICE);
    int err = ZTS_ERR_OK;
    if ((err = zts_service->getIdentity(key, key_dst_len)) != ZTS_ERR_OK) {
        return err;
    }
    return *key_dst_len > 0 ? ZTS_ERR_OK : ZTS_ERR_GENERAL;
}

#if defined(__WINDOWS__)
DWORD WINAPI cbRun(LPVOID arg)
#else
void* cbRun(void* arg)
#endif
{
    ZTS_UNUSED_ARG(arg);
#if defined(__APPLE__)
    // pthread_setname_np(ZTS_EVENT_CALLBACK_THREAD_NAME);
#endif
    zts_events->run();
    //#if ZTS_ENABLE_JAVA
    //    _java_detach_from_thread();
    // pthread_exit(0);
    //#endif
    return NULL;
}

int zts_addr_is_assigned(uint64_t net_id, unsigned int family)
{
    ACQUIRE_SERVICE(0);
    return zts_service->addrIsAssigned(net_id, family);
}

int zts_addr_get(uint64_t net_id, unsigned int family, struct zts_sockaddr_storage* addr)
{
    ACQUIRE_SERVICE(ZTS_ERR_SERVICE);
    return zts_service->getFirstAssignedAddr(net_id, family, addr);
}

int zts_addr_get_str(uint64_t net_id, unsigned int family, char* dst, unsigned int len)
{
    // No service lock required since zts_addr_get will lock it
    if (net_id == 0) {
        return ZTS_ERR_ARG;
    }
    if (len < ZTS_INET6_ADDRSTRLEN) {
        return ZTS_ERR_ARG;
    }

    if (family == ZTS_AF_INET) {
        struct zts_sockaddr_storage ss;
        int err = ZTS_ERR_OK;
        if ((err = zts_addr_get(net_id, family, &ss)) != ZTS_ERR_OK) {
            return err;
        }
        struct zts_sockaddr_in* in4 = (struct zts_sockaddr_in*)&ss;
        zts_inet_ntop(family, &(in4->sin_addr), dst, ZTS_INET6_ADDRSTRLEN);
    }
    if (family == ZTS_AF_INET6) {
        struct zts_sockaddr_storage ss;
        int err = ZTS_ERR_OK;
        if ((err = zts_addr_get(net_id, family, &ss)) != ZTS_ERR_OK) {
            return err;
        }
        struct zts_sockaddr_in6* in6 = (struct zts_sockaddr_in6*)&ss;
        zts_inet_ntop(family, &(in6->sin6_addr), dst, ZTS_INET6_ADDRSTRLEN);
    }
    return ZTS_ERR_OK;
}

int zts_addr_get_all(uint64_t net_id, struct zts_sockaddr_storage* addr, unsigned int* count)
{
    ACQUIRE_SERVICE(ZTS_ERR_SERVICE);
    return zts_service->getAllAssignedAddr(net_id, addr, count);
}

int zts_core_lock_obtain()
{
    ACQUIRE_SERVICE(ZTS_ERR_SERVICE);
    zts_service->obtainLock();
    return ZTS_ERR_OK;
}

int zts_core_lock_release()
{
    ACQUIRE_SERVICE(ZTS_ERR_SERVICE);
    zts_service->releaseLock();
    return ZTS_ERR_OK;
}

int zts_core_query_addr_count(uint64_t net_id)
{
    ACQUIRE_SERVICE(ZTS_ERR_SERVICE);
    return zts_service->addressCount(net_id);
}

int zts_core_query_addr(uint64_t net_id, unsigned int idx, char* addr, unsigned int len)
{
    ACQUIRE_SERVICE(ZTS_ERR_SERVICE);
    return zts_service->getAddrAtIdx(net_id, idx, addr, len);
}

int zts_core_query_route_count(uint64_t net_id)
{
    ACQUIRE_SERVICE(ZTS_ERR_SERVICE);
    return zts_service->routeCount(net_id);
}

int zts_core_query_route(
    uint64_t net_id,
    unsigned int idx,
    char* target,
    char* via,
    unsigned int len,
    uint16_t* flags,
    uint16_t* metric)
{
    ACQUIRE_SERVICE(ZTS_ERR_SERVICE);
    return zts_service->getRouteAtIdx(net_id, idx, target, via, len, flags, metric);
}

int zts_core_query_path_count(uint64_t peer_id)
{
    ACQUIRE_SERVICE(ZTS_ERR_SERVICE);
    return zts_service->pathCount(peer_id);
}
int zts_core_query_path(uint64_t peer_id, unsigned int idx, char* path, unsigned int len)
{
    ACQUIRE_SERVICE(ZTS_ERR_SERVICE);
    return zts_service->getPathAtIdx(peer_id, idx, path, len);
}

int zts_core_query_mc_count(uint64_t net_id)
{
    ACQUIRE_SERVICE(ZTS_ERR_SERVICE);
    return zts_service->multicastSubCount(net_id);
}
int zts_core_query_mc(uint64_t net_id, unsigned int idx, uint64_t* mac, uint32_t* adi)
{
    ACQUIRE_SERVICE(ZTS_ERR_SERVICE);
    return zts_service->getMulticastSubAtIdx(net_id, idx, mac, adi);
}

int zts_net_join(const uint64_t net_id)
{
    ACQUIRE_SERVICE(ZTS_ERR_SERVICE);
    return zts_service->join(net_id);
}

int zts_net_leave(const uint64_t net_id)
{
    ACQUIRE_SERVICE(ZTS_ERR_SERVICE);
    return zts_service->leave(net_id);
}

int zts_net_transport_is_ready(const uint64_t net_id)
{
    ACQUIRE_SERVICE(ZTS_ERR_SERVICE);
    return zts_service->networkIsReady(net_id);
}

uint64_t zts_net_get_mac(uint64_t net_id)
{
    ACQUIRE_SERVICE(ZTS_ERR_SERVICE);
    return zts_service->getMACAddress(net_id);
}

ZTS_API int ZTCALL zts_net_get_mac_str(uint64_t net_id, char* dst, unsigned int len)
{
    ACQUIRE_SERVICE(ZTS_ERR_SERVICE);
    if (! dst || len < ZTS_MAC_ADDRSTRLEN) {
        return ZTS_ERR_ARG;
    }
    uint64_t mac = zts_service->getMACAddress(net_id);
    OSUtils::ztsnprintf(
        dst,
        ZTS_MAC_ADDRSTRLEN,
        "%x:%x:%x:%x:%x:%x",
        (mac >> 40) & 0xFF,
        (mac >> 32) & 0xFF,
        (mac >> 24) & 0xFF,
        (mac >> 16) & 0xFF,
        (mac >> 8) & 0xFF,
        (mac >> 0) & 0xFF);
    return ZTS_ERR_OK;
}

int zts_net_get_broadcast(uint64_t net_id)
{
    ACQUIRE_SERVICE(ZTS_ERR_SERVICE);
    return zts_service->getNetworkBroadcast(net_id);
}

int zts_net_get_mtu(uint64_t net_id)
{
    ACQUIRE_SERVICE(ZTS_ERR_SERVICE);
    return zts_service->getNetworkMTU(net_id);
}

int zts_net_get_name(uint64_t net_id, char* dst, unsigned int len)
{
    ACQUIRE_SERVICE(ZTS_ERR_SERVICE);
    return zts_service->getNetworkName(net_id, dst, len);
}

int zts_net_get_status(uint64_t net_id)
{
    ACQUIRE_SERVICE(ZTS_ERR_SERVICE);
    return zts_service->getNetworkStatus(net_id);
}

int zts_net_get_type(uint64_t net_id)
{
    ACQUIRE_SERVICE(ZTS_ERR_SERVICE);
    return zts_service->getNetworkType(net_id);
}

int zts_route_is_assigned(uint64_t net_id, unsigned int family)
{
    ACQUIRE_SERVICE(ZTS_ERR_SERVICE);
    return zts_service->networkHasRoute(net_id, family);
}

// Start a ZeroTier NodeService background thread
#if defined(__WINDOWS__)
DWORD WINAPI _runNodeService(LPVOID arg)
#else
void* _runNodeService(void* arg)
#endif
{
    ZTS_UNUSED_ARG(arg);
#if defined(__APPLE__)
    // pthread_setname_np(ZTS_SERVICE_THREAD_NAME);
#endif
    try {
        zts_service->run();
        // Begin shutdown
        service_m.lock();
        zts_events->clrState(ZTS_STATE_NODE_RUNNING);
        delete zts_service;
        zts_service = (NodeService*)0;
        service_m.unlock();
        events_m.lock();
        zts_util_delay(ZTS_CALLBACK_PROCESSING_INTERVAL * 2);
        if (zts_events) {
            zts_events->disable();
        }
        events_m.unlock();
    }
    catch (...) {
    }
#ifndef __WINDOWS__
    pthread_exit(0);
#endif
    return NULL;
}

int zts_node_start()
{
    ACQUIRE_SERVICE_OFFLINE();
    // Start TCP/IP stack
    zts_lwip_driver_init();
    // Start callback thread
    int res = ZTS_ERR_OK;
    if (zts_events->hasCallback()) {
#if defined(__WINDOWS__)
        HANDLE callbackThread = CreateThread(NULL, 0, cbRun, NULL, 0, NULL);
        // TODO: Check success
#else
        pthread_t cbThread;
        if ((res = pthread_create(&cbThread, NULL, cbRun, NULL)) != 0) {}
#endif
#if defined(__linux__)
        // pthread_setname_np(cbThread, ZTS_EVENT_CALLBACK_THREAD_NAME);
#endif
        if (res != ZTS_ERR_OK) {
            zts_events->clrState(ZTS_STATE_CALLBACKS_RUNNING);
            zts_events->clrCallback();
        }
        zts_events->setState(ZTS_STATE_CALLBACKS_RUNNING);
    }
    // Start ZeroTier service
#if defined(__WINDOWS__)
    HANDLE serviceThread = CreateThread(NULL, 0, _runNodeService, (void*)NULL, 0, NULL);
    // TODO: Check success
#else
    pthread_t service_thread;
    if ((res = pthread_create(&service_thread, NULL, _runNodeService, (void*)NULL)) != 0) {}
#endif
#if defined(__linux__)
    // pthread_setname_np(service_thread, ZTS_SERVICE_THREAD_NAME);
#endif
    if (res != ZTS_ERR_OK) {
        zts_events->clrState(ZTS_STATE_NODE_RUNNING);
    }
    zts_events->setState(ZTS_STATE_NODE_RUNNING);
    return ZTS_ERR_OK;
}

int zts_node_is_online()
{
    ACQUIRE_SERVICE(0);
    return zts_service->nodeIsOnline();
}

uint64_t zts_node_get_id()
{
    ACQUIRE_SERVICE(ZTS_ERR_SERVICE);
    return zts_service->getNodeId();
}

int zts_node_get_port()
{
    ACQUIRE_SERVICE(ZTS_ERR_SERVICE);
    return zts_service->getPrimaryPort();
}

int zts_node_stop()
{
    ACQUIRE_SERVICE(ZTS_ERR_SERVICE);
    zts_events->clrState(ZTS_STATE_NODE_RUNNING);
    zts_service->terminate();
#if defined(__WINDOWS__)
    WSACleanup();
#endif
    return ZTS_ERR_OK;
}

int zts_node_free()
{
    ACQUIRE_SERVICE(ZTS_ERR_SERVICE);
    zts_events->setState(ZTS_STATE_FREE_CALLED);
    zts_events->clrState(ZTS_STATE_NODE_RUNNING);
    zts_service->terminate();
#if defined(__WINDOWS__)
    WSACleanup();
#endif
    zts_lwip_driver_shutdown();
    delete zts_events;
    zts_events = (Events*)0;
    return ZTS_ERR_OK;
}

int zts_moon_orbit(uint64_t moon_roots_id, uint64_t moon_seed)
{
    ACQUIRE_SERVICE(ZTS_ERR_SERVICE);
    zts_service->orbit(moon_roots_id, moon_seed);
    return ZTS_ERR_OK;
}

int zts_moon_deorbit(uint64_t moon_roots_id)
{
    ACQUIRE_SERVICE(ZTS_ERR_SERVICE);
    zts_service->deorbit(moon_roots_id);
    return ZTS_ERR_OK;
}

int zts_stats_get_all(zts_stats_counter_t* dst)
{
    if (! dst) {
        return ZTS_ERR_ARG;
    }
    if (! transport_ok()) {
        return ZTS_ERR_SERVICE;
    }
#if LWIP_STATS

    extern struct stats_ lwip_stats;

#define lws lwip_stats

    /* Summarize lwIP's statistics for simplicity at the expense of specificity */

    // link
    dst->link_tx = lws.link.xmit;
    dst->link_rx = lws.link.recv;
    dst->link_drop = lws.link.drop;
    dst->link_err = lws.link.chkerr + lws.link.lenerr + lws.link.memerr + lws.link.rterr + lws.link.proterr
                    + lws.link.opterr + lws.link.err;
    // etharp
    dst->etharp_tx = lws.etharp.xmit;
    dst->etharp_rx = lws.etharp.recv;
    dst->etharp_drop = lws.etharp.drop;
    dst->etharp_err = lws.etharp.chkerr + lws.etharp.lenerr + lws.etharp.memerr + lws.etharp.rterr + lws.etharp.proterr
                      + lws.etharp.opterr + lws.etharp.err;
    // ip4
    dst->ip4_tx = lws.ip.xmit;
    dst->ip4_rx = lws.ip.recv;
    dst->ip4_drop = lws.ip.drop;
    dst->ip4_err = lws.ip.chkerr + lws.ip.lenerr + lws.ip.memerr + lws.ip.rterr + lws.ip.proterr + lws.ip.opterr
                   + lws.ip.err + lws.ip_frag.chkerr + lws.ip_frag.lenerr + lws.ip_frag.memerr + lws.ip_frag.rterr
                   + lws.ip_frag.proterr + lws.ip_frag.opterr + lws.ip_frag.err;
    // ip6
    dst->ip6_tx = lws.ip6.xmit;
    dst->ip6_rx = lws.ip6.recv;
    dst->ip6_drop = lws.ip6.drop;
    dst->ip6_err = lws.ip6.chkerr + lws.ip6.lenerr + lws.ip6.memerr + lws.ip6.rterr + lws.ip6.proterr + lws.ip6.opterr
                   + lws.ip6.err + lws.ip6_frag.chkerr + lws.ip6_frag.lenerr + lws.ip6_frag.memerr + lws.ip6_frag.rterr
                   + lws.ip6_frag.proterr + lws.ip6_frag.opterr + lws.ip6_frag.err;

    // icmp4
    dst->icmp4_tx = lws.icmp.xmit;
    dst->icmp4_rx = lws.icmp.recv;
    dst->icmp4_drop = lws.icmp.drop;
    dst->icmp4_err = lws.icmp.chkerr + lws.icmp.lenerr + lws.icmp.memerr + lws.icmp.rterr + lws.icmp.proterr
                     + lws.icmp.opterr + lws.icmp.err;
    // icmp6
    dst->icmp6_tx = lws.icmp6.xmit;
    dst->icmp6_rx = lws.icmp6.recv;
    dst->icmp6_drop = lws.icmp6.drop;
    dst->icmp6_err = lws.icmp6.chkerr + lws.icmp6.lenerr + lws.icmp6.memerr + lws.icmp6.rterr + lws.icmp6.proterr
                     + lws.icmp6.opterr + lws.icmp6.err;
    // udp
    dst->udp_tx = lws.udp.xmit;
    dst->udp_rx = lws.udp.recv;
    dst->udp_drop = lws.udp.drop;
    dst->udp_err = lws.udp.chkerr + lws.udp.lenerr + lws.udp.memerr + lws.udp.rterr + lws.udp.proterr + lws.udp.opterr
                   + lws.udp.err;
    // tcp
    dst->tcp_tx = lws.tcp.xmit;
    dst->tcp_rx = lws.tcp.recv;
    dst->tcp_drop = lws.tcp.drop;
    dst->tcp_err = lws.tcp.chkerr + lws.tcp.lenerr + lws.tcp.memerr + lws.tcp.rterr + lws.tcp.proterr + lws.tcp.opterr
                   + lws.tcp.err;
    // nd6
    dst->nd6_tx = lws.nd6.xmit;
    dst->nd6_rx = lws.nd6.recv;
    dst->nd6_drop = lws.nd6.drop;
    dst->nd6_err = lws.nd6.chkerr + lws.nd6.lenerr + lws.nd6.memerr + lws.nd6.rterr + lws.nd6.proterr + lws.nd6.opterr
                   + lws.nd6.err;

    // TODO: Add mem and sys stats

    return ZTS_ERR_OK;
#else
    return ZTS_ERR_NO_RESULT;
#endif
#undef lws
}

#ifdef __cplusplus
}
#endif

}   // namespace ZeroTier
