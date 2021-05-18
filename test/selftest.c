/**
 * Selftest. To be run for every commit.
 */

#include <ZeroTierSockets.h>
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <time.h>

#define LIBZT_DEBUG 1

#include "../src/Debug.hpp"

#pragma GCC diagnostic ignored "-Wunused-value"

int random32()
{
    const int BITS_PER_RAND = (int)(log2(RAND_MAX / 2 + 1) + 1.0);
    int ret = 0;
    for (int i = 0; i < sizeof(int) * CHAR_BIT; i += BITS_PER_RAND) {
        ret <<= BITS_PER_RAND;
        ret |= rand();
    }
    return ret;
}

uint64_t random64()
{
    return ((uint64_t)random32() << 32) | random32();
}

//----------------------------------------------------------------------------//
// Test parameters and variables                                              //
//----------------------------------------------------------------------------//

int verbosity = 2;
int callback_was_called_flag = 0;

// Used throughout the selftest
char keypair_i[ZTS_ID_STR_BUF_LEN];

//----------------------------------------------------------------------------//
// Event handler                                                              //
//----------------------------------------------------------------------------//

void print_node_details(const char* msg, zts_node_info_t* d)
{
    DEBUG_INFO("  %s", msg);
    if (verbosity < 2) {
        return;
    }
    DEBUG_INFO("    msg->node->node_id        : %10llx", d->node_id);
    DEBUG_INFO("    msg->node->port_primary   : %10d", d->port_primary);
    DEBUG_INFO("    msg->node->port_secondary : %10d", d->port_secondary);
    DEBUG_INFO("    msg->node->port_tertiary  : %10d", d->port_tertiary);
    DEBUG_INFO("    msg->node->ver_major      : %10d", d->ver_major);
    DEBUG_INFO("    msg->node->ver_minor      : %10d", d->ver_minor);
    DEBUG_INFO("    msg->node->ver_rev        : %10d", d->ver_rev);
}

void print_net_details(const char* msg, zts_net_info_t* d)
{
    DEBUG_INFO("  %s", msg);
    if (verbosity < 2) {
        return;
    }
    DEBUG_INFO("    net_id              : %16llx", d->net_id);
    DEBUG_INFO("    mac                 : %llx", d->mac);
    DEBUG_INFO("    name                : %s", d->name);
    DEBUG_INFO("    type                : %d", d->type);
    DEBUG_INFO("    mtu                 : %d", d->mtu);
    DEBUG_INFO("    dhcp                : %d", d->dhcp);
    DEBUG_INFO("    bridge              : %d", d->bridge);
    DEBUG_INFO("    broadcast_enabled   : %d", d->broadcast_enabled);
    DEBUG_INFO("    port_error          : %d", d->port_error);
    DEBUG_INFO("    netconf_rev         : %lu", d->netconf_rev);
    DEBUG_INFO("    route_count         : %d", d->route_count);
    DEBUG_INFO("    multicast_sub_count : %d", d->multicast_sub_count);

    for (int i = 0; i < d->multicast_sub_count; i++) {
        DEBUG_INFO("\t  - mac=%llx, adi=%x", d->multicast_subs[i].mac, d->multicast_subs[i].adi);
    }

    DEBUG_INFO("\t- addresses:");

    for (int i = 0; i < d->assigned_addr_count; i++) {
        if (d->assigned_addrs[i].ss_family == ZTS_AF_INET) {
            char ipstr[ZTS_INET_ADDRSTRLEN] = { 0 };
            struct zts_sockaddr_in* in4 = (struct zts_sockaddr_in*)&(d->assigned_addrs[i]);
            zts_inet_ntop(ZTS_AF_INET, &(in4->sin_addr), ipstr, ZTS_INET_ADDRSTRLEN);
            DEBUG_INFO("\t  - %s", ipstr);
        }
        if (d->assigned_addrs[i].ss_family == ZTS_AF_INET6) {
            char ipstr[ZTS_INET6_ADDRSTRLEN] = { 0 };
            struct zts_sockaddr_in6* in6 = (struct zts_sockaddr_in6*)&(d->assigned_addrs[i]);
            zts_inet_ntop(ZTS_AF_INET6, &(in6->sin6_addr), ipstr, ZTS_INET6_ADDRSTRLEN);
            DEBUG_INFO("\t  - %s", ipstr);
        }
    }

    char target[ZTS_INET6_ADDRSTRLEN] = { 0 };
    char via[ZTS_INET6_ADDRSTRLEN] = { 0 };

    DEBUG_INFO("\t- routes:");
    for (int i = 0; i < d->route_count; i++) {
        if (d->routes[i].target.ss_family == ZTS_AF_INET) {
            struct zts_sockaddr_in* in4 = (struct zts_sockaddr_in*)&(d->routes[i].target);
            zts_inet_ntop(ZTS_AF_INET, &(in4->sin_addr), target, ZTS_INET6_ADDRSTRLEN);
            in4 = (struct zts_sockaddr_in*)&(d->routes[i].via);
            zts_inet_ntop(ZTS_AF_INET, &(in4->sin_addr), via, ZTS_INET6_ADDRSTRLEN);
        }
        if (d->routes[i].target.ss_family == ZTS_AF_INET6) {
            struct zts_sockaddr_in6* in6 = (struct zts_sockaddr_in6*)&(d->routes[i].target);
            zts_inet_ntop(ZTS_AF_INET6, &(in6->sin6_addr), target, ZTS_INET6_ADDRSTRLEN);
            in6 = (struct zts_sockaddr_in6*)&(d->routes[i].via);
            zts_inet_ntop(ZTS_AF_INET6, &(in6->sin6_addr), via, ZTS_INET6_ADDRSTRLEN);
        }
        DEBUG_INFO("\t  - target : %s", target);
        DEBUG_INFO("\t  - via    : %s", via);
        DEBUG_INFO("\t  - flags  : %d", d->routes[i].flags);
        DEBUG_INFO("\t  - metric : %d", d->routes[i].metric);
    }
}

void print_peer_details(const char* msg, zts_peer_info_t* d)
{
    DEBUG_INFO("  %s", msg);
    if (verbosity < 2) {
        return;
    }
    DEBUG_INFO("\t- peer                       : %llx", d->peer_id);
    DEBUG_INFO("\t- role                       : %d", d->role);
    DEBUG_INFO("\t- latency                    : %d", d->latency);
    DEBUG_INFO("\t- version                    : %d.%d.%d", d->ver_major, d->ver_minor, d->ver_rev);
    DEBUG_INFO("\t- path_count                 : %d", d->path_count);
    DEBUG_INFO("\t- paths:");

    // Print all known paths for each peer
    for (unsigned int j = 0; j < d->path_count; j++) {
        char ipstr[ZTS_INET6_ADDRSTRLEN] = { 0 };
        int port = 0;
        struct zts_sockaddr* sa = (struct zts_sockaddr*)&(d->paths[j].address);
        if (sa->sa_family == ZTS_AF_INET) {
            struct zts_sockaddr_in* in4 = (struct zts_sockaddr_in*)sa;
            zts_inet_ntop(ZTS_AF_INET, &(in4->sin_addr), ipstr, ZTS_INET_ADDRSTRLEN);
            port = ntohs(in4->sin_port);
        }
        if (sa->sa_family == ZTS_AF_INET6) {
            struct zts_sockaddr_in6* in6 = (struct zts_sockaddr_in6*)sa;
            zts_inet_ntop(ZTS_AF_INET6, &(in6->sin6_addr), ipstr, ZTS_INET6_ADDRSTRLEN);
        }
        DEBUG_INFO("\t  - %15s : %6d", ipstr, port);
    }
    DEBUG_INFO("");
}

void print_netif_details(const char* msg, zts_netif_info_t* d)
{
    DEBUG_INFO("  %s", msg);
    if (verbosity < 2) {
        return;
    }
    DEBUG_INFO("\t- net_id : %llx", d->net_id);
    DEBUG_INFO("\t- mac  : %llx", d->mac);
    DEBUG_INFO("\t- mtu  : %d", d->mtu);
}

void print_route_details(const char* msg, zts_route_info_t* d)
{
    DEBUG_INFO("%s", msg);
    if (verbosity < 2) {
        return;
    }
}

void print_address_details(const char* msg, zts_addr_info_t* d)
{
    DEBUG_INFO("  %s", msg);
    if (verbosity < 2) {
        return;
    }
    char ipstr[ZTS_INET6_ADDRSTRLEN] = { 0 };
    struct zts_sockaddr* sa = (struct zts_sockaddr*)&(d->addr);
    if (sa->sa_family == ZTS_AF_INET) {
        struct zts_sockaddr_in* in4 = (struct zts_sockaddr_in*)&(d->addr);
        zts_inet_ntop(ZTS_AF_INET, &(in4->sin_addr), ipstr, ZTS_INET_ADDRSTRLEN);
    }
    if (sa->sa_family == ZTS_AF_INET6) {
        struct zts_sockaddr_in6* in6 = (struct zts_sockaddr_in6*)&(d->addr);
        zts_inet_ntop(ZTS_AF_INET6, &(in6->sin6_addr), ipstr, ZTS_INET6_ADDRSTRLEN);
    }
    DEBUG_INFO("    network : %llx", d->net_id);
    DEBUG_INFO("    addr    : %s", ipstr);
}

//----------------------------------------------------------------------------//
// Event Handler                                                              //
//----------------------------------------------------------------------------//

#define ZTS_NODE_EVENT(code)    code >= ZTS_EVENT_NODE_UP&& code <= ZTS_EVENT_NODE_FATAL_ERROR
#define ZTS_NETWORK_EVENT(code) code >= ZTS_EVENT_NETWORK_NOT_FOUND&& code <= ZTS_EVENT_NETWORK_UPDATE
#define ZTS_STACK_EVENT(code)   code >= ZTS_EVENT_STACK_UP&& code <= ZTS_EVENT_STACK_DOWN
#define ZTS_NETIF_EVENT(code)   code >= ZTS_EVENT_NETIF_UP&& code <= ZTS_EVENT_NETIF_LINK_DOWN
#define ZTS_PEER_EVENT(code)    code >= ZTS_EVENT_PEER_DIRECT&& code <= ZTS_EVENT_PEER_PATH_DEAD
#define ZTS_ROUTE_EVENT(code)   code >= ZTS_EVENT_ROUTE_ADDED&& code <= ZTS_EVENT_ROUTE_REMOVED
#define ZTS_ADDR_EVENT(code)    code >= ZTS_EVENT_ADDR_ADDED_IP4&& code <= ZTS_EVENT_ADDR_REMOVED_IP6
#define ZTS_STORE_EVENT(code)   code >= ZTS_EVENT_STORE_IDENTITY_SECRET&& code <= ZTS_EVENT_STORE_NETWORK

void on_zts_event(void* msgPtr)
{
    /* Used to signal that a callback was sent to the user, we test for this
    value later */
    callback_was_called_flag = 1;

    assert(("msgPtr is null", msgPtr != NULL));
    zts_event_msg_t* msg = (zts_event_msg_t*)msgPtr;
    assert(("Invalid callback event code", msg->event_code > 0));
    DEBUG_INFO("  msg->event_code = %d", msg->event_code);
    if (msg->node) {
        DEBUG_INFO("  msg->node      = %p", (void*)(msg->node));
    }
    if (msg->network) {
        DEBUG_INFO("  msg->network   = %p", (void*)(msg->network));
    }
    if (msg->netif) {
        DEBUG_INFO("  msg->netif     = %p", (void*)(msg->netif));
    }
    if (msg->route) {
        DEBUG_INFO("  msg->route     = %p", (void*)(msg->route));
    }
    if (msg->peer) {
        DEBUG_INFO("  msg->peer      = %p", (void*)(msg->peer));
    }
    if (msg->addr) {
        DEBUG_INFO("  msg->addr      = %p", (void*)(msg->addr));
    }

    // Ensure references to structures are valid when needed

    assert((msg->node == NULL) ^ (msg->node && ZTS_NODE_EVENT(msg->event_code)));
    assert((msg->network == NULL) ^ (msg->network && ZTS_NETWORK_EVENT(msg->event_code)));
    assert((msg->netif == NULL) ^ (msg->netif && ZTS_NETIF_EVENT(msg->event_code)));
    assert((msg->peer == NULL) ^ (msg->peer && ZTS_PEER_EVENT(msg->event_code)));
    assert((msg->route == NULL) ^ (msg->route && ZTS_ROUTE_EVENT(msg->event_code)));
    assert((msg->addr == NULL) ^ (msg->addr && ZTS_ADDR_EVENT(msg->event_code)));
    assert((msg->cache == NULL) ^ (msg->cache && ZTS_STORE_EVENT(msg->event_code)));

    // Node events

    if (msg->event_code == ZTS_EVENT_NODE_UP) {
        print_node_details("ZTS_EVENT_NODE_UP", msg->node);
    }
    if (msg->event_code == ZTS_EVENT_NODE_ONLINE) {
        print_node_details("ZTS_EVENT_NODE_ONLINE", msg->node);
    }
    if (msg->event_code == ZTS_EVENT_NODE_OFFLINE) {
        print_node_details("ZTS_EVENT_NODE_OFFLINE", msg->node);
    }
    if (msg->event_code == ZTS_EVENT_NODE_DOWN) {
        print_node_details("ZTS_EVENT_NODE_DOWN", msg->node);
    }
    if (msg->event_code == ZTS_EVENT_NODE_FATAL_ERROR) {
        print_node_details("ZTS_EVENT_NODE_FATAL_ERROR", msg->node);
    }

    // Network events

    if (msg->event_code == ZTS_EVENT_NETWORK_NOT_FOUND) {
        print_net_details("ZTS_EVENT_NETWORK_NOT_FOUND", msg->network);
    }
    if (msg->event_code == ZTS_EVENT_NETWORK_CLIENT_TOO_OLD) {
        print_net_details("ZTS_EVENT_NETWORK_CLIENT_TOO_OLD", msg->network);
    }
    if (msg->event_code == ZTS_EVENT_NETWORK_REQ_CONFIG) {
        print_net_details("ZTS_EVENT_NETWORK_REQ_CONFIG", msg->network);
    }
    if (msg->event_code == ZTS_EVENT_NETWORK_OK) {
        print_net_details("ZTS_EVENT_NETWORK_OK", msg->network);
    }
    if (msg->event_code == ZTS_EVENT_NETWORK_ACCESS_DENIED) {
        print_net_details("ZTS_EVENT_NETWORK_ACCESS_DENIED", msg->network);
    }
    if (msg->event_code == ZTS_EVENT_NETWORK_READY_IP6) {
        print_net_details("ZTS_EVENT_NETWORK_READY_IP6", msg->network);
    }
    if (msg->event_code == ZTS_EVENT_NETWORK_READY_IP4) {
        print_net_details("ZTS_EVENT_NETWORK_READY_IP4", msg->network);
    }
    if (msg->event_code == ZTS_EVENT_NETWORK_READY_IP4_IP6) {
        print_net_details("ZTS_EVENT_NETWORK_READY_IP4_IP6", msg->network);
    }
    if (msg->event_code == ZTS_EVENT_NETWORK_DOWN) {
        print_net_details("ZTS_EVENT_NETWORK_DOWN", msg->network);
    }
    if (msg->event_code == ZTS_EVENT_NETWORK_UPDATE) {
        print_net_details("ZTS_EVENT_NETWORK_UPDATE", msg->network);
    }

    // Stack events

    if (msg->event_code == ZTS_EVENT_STACK_UP) {
        // print_stack_details("ZTS_EVENT_STACK_UP", msg->stack);
    }
    if (msg->event_code == ZTS_EVENT_STACK_DOWN) {
        // print_stack_details("ZTS_EVENT_STACK_DOWN", msg->stack);
    }

    // Netif events

    if (msg->event_code == ZTS_EVENT_NETIF_UP) {
        print_netif_details("ZTS_EVENT_NETIF_UP", msg->netif);
    }
    if (msg->event_code == ZTS_EVENT_NETIF_DOWN) {
        print_netif_details("ZTS_EVENT_NETIF_DOWN", msg->netif);
    }
    if (msg->event_code == ZTS_EVENT_NETIF_REMOVED) {
        print_netif_details("ZTS_EVENT_NETIF_REMOVED", msg->netif);
    }
    if (msg->event_code == ZTS_EVENT_NETIF_LINK_UP) {
        print_netif_details("ZTS_EVENT_NETIF_LINK_UP", msg->netif);
    }
    if (msg->event_code == ZTS_EVENT_NETIF_LINK_DOWN) {
        print_netif_details("ZTS_EVENT_NETIF_LINK_DOWN", msg->netif);
    }

    // Peer events

    if (msg->peer) {
        if (msg->peer->role == ZTS_PEER_ROLE_PLANET) {
            /* Safe to ignore, these are our roots. They orchestrate the P2P
            connection. You might also see other unknown peers, these are our
            network controllers. */
            return;
        }
    }
    if (msg->event_code == ZTS_EVENT_PEER_DIRECT) {
        print_peer_details("ZTS_EVENT_PEER_DIRECT", msg->peer);
    }
    if (msg->event_code == ZTS_EVENT_PEER_RELAY) {
        print_peer_details("ZTS_EVENT_PEER_RELAY", msg->peer);
    }
    if (msg->event_code == ZTS_EVENT_PEER_UNREACHABLE) {
        print_peer_details("ZTS_EVENT_PEER_UNREACHABLE", msg->peer);
    }
    if (msg->event_code == ZTS_EVENT_PEER_PATH_DISCOVERED) {
        print_peer_details("ZTS_EVENT_PEER_PATH_DISCOVERED", msg->peer);
    }
    if (msg->event_code == ZTS_EVENT_PEER_PATH_DEAD) {
        print_peer_details("ZTS_EVENT_PEER_PATH_DEAD", msg->peer);
    }

    // Route events

    if (msg->event_code == ZTS_EVENT_ROUTE_ADDED) {
        print_route_details("ZTS_EVENT_ROUTE_ADDED", msg->route);
    }
    if (msg->event_code == ZTS_EVENT_ROUTE_REMOVED) {
        print_route_details("ZTS_EVENT_ROUTE_REMOVED", msg->route);
    }

    // Address events

    if (msg->event_code == ZTS_EVENT_ADDR_ADDED_IP4) {
        print_address_details("ZTS_EVENT_ADDR_ADDED_IP4", msg->addr);
    }
    if (msg->event_code == ZTS_EVENT_ADDR_ADDED_IP6) {
        print_address_details("ZTS_EVENT_ADDR_ADDED_IP6", msg->addr);
    }
    if (msg->event_code == ZTS_EVENT_ADDR_REMOVED_IP4) {
        print_address_details("ZTS_EVENT_ADDR_REMOVED_IP4", msg->addr);
    }
    if (msg->event_code == ZTS_EVENT_ADDR_REMOVED_IP6) {
        print_address_details("ZTS_EVENT_ADDR_REMOVED_IP6", msg->addr);
    }

    // Cache events

    if (msg->event_code == ZTS_EVENT_STORE_IDENTITY_PUBLIC) {
        DEBUG_INFO("ZTS_EVENT_STORE_IDENTITY_PUBLIC (len=%d)", msg->len);
    }
    if (msg->event_code == ZTS_EVENT_STORE_IDENTITY_SECRET) {
        DEBUG_INFO("ZTS_EVENT_STORE_IDENTITY_SECRET (len=%d)", msg->len);
    }
    if (msg->event_code == ZTS_EVENT_STORE_PLANET) {
        DEBUG_INFO("ZTS_EVENT_STORE_PLANET (len=%d)", msg->len);
    }
    if (msg->event_code == ZTS_EVENT_STORE_PEER) {
        DEBUG_INFO("ZTS_EVENT_STORE_PEER (len=%d)", msg->len);
    }
    if (msg->event_code == ZTS_EVENT_STORE_NETWORK) {
        DEBUG_INFO("ZTS_EVENT_STORE_NETWORK (len=%d)", msg->len);
    }

    DEBUG_INFO("");
}

void api_value_arg_test(int tid, uint8_t num, int8_t i8, int16_t i16, int32_t i32, int64_t i64, void* nullable)
{
    DEBUG_INFO(
        "fuzzing values: thr = %10d, func [ %3d ] (%4d, %7d, %12d, %32lld, %p)",
        tid,
        num,
        i8,
        i16,
        i32,
        i64,
        nullable);
    int res = ZTS_ERR_OK;

    // Test uninitialized Network Stack API usage
    /*
        res = zts_get_all_stats((struct zts_stats *)nullable);
        assert(("pre-init call to zts_get_all_stats(): res != ZTS_ERR_SERVICE",
            res == ZTS_ERR_SERVICE));
        res = zts_get_protocol_stats(i32, nullable);
        assert(("pre-init call to zts_get_protocol_stats(): res !=
       ZTS_ERR_SERVICE", res == ZTS_ERR_SERVICE));
    */
    res = zts_dns_set_server(i8, (const zts_ip_addr*)nullable);
    assert(("pre-init call to zts_add_dns_nameserver(): res != ZTS_ERR_SERVICE", res == ZTS_ERR_SERVICE));
    const zts_ip_addr* res_ptr = zts_dns_get_server(i8);
    assert(("pre-init call to zts_del_dns_nameserver(): res != ZTS_ERR_SERVICE", res == ZTS_ERR_SERVICE));

    struct zts_sockaddr* null_addr = (struct zts_sockaddr*)nullable;
    struct zts_sockaddr_storage* null_addr_ss = (struct zts_sockaddr_storage*)nullable;
    zts_socklen_t* null_len = (zts_socklen_t*)nullable;
    zts_fd_set* null_fd_set = (zts_fd_set*)nullable;
    zts_timeval* null_timeval = (zts_timeval*)nullable;

    // Test uninitialized control API usage (Node)

    switch (num) {
        //
        // Central
        //
        /*
        case 1:
            assert(zts_central_set_access_mode(i8) == ZTS_ERR_SERVICE);
            break;
        case 2:
            assert(zts_central_set_verbose(i8) == ZTS_ERR_SERVICE);
            break;
        case 3:
            assert(zts_central_clear_resp_buf() == ZTS_ERR_SERVICE);
            break;
        case 4:
            assert(zts_central_init(const NULL, const NULL, NULL, i32) == ZTS_ERR_SERVICE);
            break;
        case 5:
            assert(zts_central_cleanup() == ZTS_ERR_SERVICE);
            break;
        case 6:
            assert(zts_central_get_last_resp_buf(NULL, i32) == ZTS_ERR_SERVICE);
            break;
        case 7:
            assert(zts_central_status_get(NULL) == ZTS_ERR_SERVICE);
            break;
        case 8:
            assert(zts_central_self_get(NULL) == ZTS_ERR_SERVICE);
            break;
        case 9:
            assert(zts_central_net_get(NULL, i64) == ZTS_ERR_SERVICE);
            break;
        case 10:
            assert(zts_central_net_update(NULL, i64) == ZTS_ERR_SERVICE);
            break;
        case 11:
            assert(zts_central_net_delete(NULL, i64) == ZTS_ERR_SERVICE);
            break;
        case 12:
            assert(zts_central_net_get_all(NULL) == ZTS_ERR_SERVICE);
            break;
        case 13:
            assert(zts_central_member_get(NULL, i64, i64) == ZTS_ERR_SERVICE);
            break;
        case 14:
            assert(zts_central_member_update(NULL, i64, i64, NULL) == ZTS_ERR_SERVICE);
            break;
        case 15:
            assert(zts_central_node_auth(NULL, i64, i64, i8) == ZTS_ERR_SERVICE);
            break;
        case 16:
            assert(zts_central_net_get_members(NULL, i64) == ZTS_ERR_SERVICE);
            break;
        */
        //
        // Id (tested in separate section)
        //
        /*
        case 20:
            assert(zts_id_new(NULL, NULL) == ZTS_ERR_SERVICE);
            break;
        case 21:
            assert(zts_id_pair_is_valid(NULL, i32) == ZTS_ERR_SERVICE);
            break;
        */
        //
        // Init
        //
        /*
        int res = 0;
        case 30:
            assert(zts_init_from_storage(NULL) == ZTS_ERR_OK);
            break;
        case 31:
            assert(zts_init_from_memory(NULL, i16) == ZTS_ERR_OK);
            break;
        case 32:
            assert(zts_init_set_event_handler(NULL) == ZTS_ERR_SERVICE);
            break;
        case 33:
            assert(zts_init_blacklist_if(NULL, i32) == ZTS_ERR_SERVICE);
            break;
        case 34:
            assert(zts_init_set_roots(NULL, i32) == ZTS_ERR_SERVICE);
            break;
        case 35:
            assert(zts_init_set_port(i16) == ZTS_ERR_SERVICE);
            break;
        case 36:
            assert(zts_init_allow_net_cache(i32) == ZTS_ERR_SERVICE);
            break;
        case 37:
            assert(zts_init_allow_peer_cache(i32) == ZTS_ERR_SERVICE);
            break;
            */
        //
        // Address
        //
        case 40:
            assert(zts_addr_is_assigned(i64, i32) == 0);
            break;
        case 41:
            assert(zts_addr_get(i64, i32, null_addr_ss) == ZTS_ERR_SERVICE);
            break;
            /*
        case 42:
            assert(zts_addr_get_str(i64, i32, NULL, i32) == ZTS_ERR_SERVICE);
            break;
            */
        case 43:
            assert(zts_addr_get_all(i64, null_addr_ss, NULL) == ZTS_ERR_SERVICE);
            break;
            /*
        case 44:
            assert(zts_addr_compute_6plane(i64, i64, null_addr) == ZTS_ERR_SERVICE);
            break;
        case 45:
            assert(zts_addr_compute_rfc4193(i64, i64, null_addr) == ZTS_ERR_SERVICE);
            break;
        case 46:
            assert(zts_addr_compute_rfc4193_str(i64, i64, NULL, i32) == ZTS_ERR_SERVICE);
            break;
        case 47:
            assert(zts_addr_compute_6plane_str(i64, i64, NULL, i32) == ZTS_ERR_SERVICE);
            break;
        //
        // Network
        //
        case 50:
            assert(zts_net_compute_adhoc_id(i16, i16) == ZTS_ERR_SERVICE);
            break;
            */
        case 51:
            assert(zts_net_join(i64) == ZTS_ERR_SERVICE);
            break;
        case 52:
            assert(zts_net_leave(i64) == ZTS_ERR_SERVICE);
            break;
        // case 53:
        //	assert(zts_net_count() == ZTS_ERR_SERVICE);
        //	break;
        case 54:
            assert(zts_net_get_mac(i64) == ZTS_ERR_SERVICE);
            break;
        case 55:
            assert(zts_net_get_mac_str(i64, NULL, i32) == ZTS_ERR_SERVICE);
            break;
        case 56:
            assert(zts_net_get_broadcast(i64) == ZTS_ERR_SERVICE);
            break;
        case 57:
            assert(zts_net_get_mtu(i64) == ZTS_ERR_SERVICE);
            break;
        case 58:
            assert(zts_net_get_name(i64, NULL, i32) == ZTS_ERR_SERVICE);
            break;
        case 59:
            assert(zts_net_get_status(i64) == ZTS_ERR_SERVICE);
            break;
        case 60:
            assert(zts_net_get_type(i64) == ZTS_ERR_SERVICE);
            break;
        // Route
        case 80:
            assert(zts_route_is_assigned(i64, i32) == ZTS_ERR_SERVICE);
            break;
            // Node
            /*
        case 90:
            assert(zts_node_start() == ZTS_ERR_SERVICE);
            break;
            */
        case 91:
            assert(zts_node_is_online() == 0);
            break;
        case 92:
            assert(zts_node_get_id() == ZTS_ERR_SERVICE);
            break;
        case 93:
            assert(zts_node_get_id_pair(NULL, NULL) == ZTS_ERR_SERVICE);
            break;
        case 94:
            assert(zts_node_get_port() == ZTS_ERR_SERVICE);
            break;
        case 95:
            assert(zts_node_stop() == ZTS_ERR_SERVICE);
            break;
            // case 96:
            //	assert(zts_node_restart() == ZTS_ERR_SERVICE);
            break;
        case 97:
            assert(zts_node_free() == ZTS_ERR_SERVICE);
            break;
            //
            // Moon
            //
            /*
        case 100:
            assert(zts_moon_orbit(i64, i64) == ZTS_ERR_SERVICE);
            break;
        case 101:
            assert(zts_moon_deorbit(i64) == ZTS_ERR_SERVICE);
            break;
            */
        //
        // Utility
        //
        // case 110:
        //	assert(zts_util_delay(i64) == ZTS_ERR_SERVICE);
        //	break;
        // Socket
        case 120:
            assert(zts_bsd_socket(i32, i32, i32) == ZTS_ERR_SERVICE);
            break;
        case 121:
            assert(zts_bsd_connect(i32, null_addr, i32) == ZTS_ERR_SERVICE);
            break;
        case 122:
            assert(zts_connect(i32, NULL, i32, i32) == ZTS_ERR_SERVICE);
            break;
        case 123:
            assert(zts_bsd_bind(i32, null_addr, i32) == ZTS_ERR_SERVICE);
            break;
        case 124:
            assert(zts_bind(i32, NULL, i32) == ZTS_ERR_SERVICE);
            break;
        case 125:
            assert(zts_bsd_listen(i32, i32) == ZTS_ERR_SERVICE);
            break;
        case 126:
            assert(zts_bsd_accept(i32, null_addr, NULL) == ZTS_ERR_SERVICE);
            break;
        case 127:
            assert(zts_accept(i32, NULL, i32, NULL) == ZTS_ERR_SERVICE);
            break;
        case 128:
            assert(zts_bsd_setsockopt(i32, i32, i32, NULL, i32) == ZTS_ERR_SERVICE);
            break;
        case 129:
            assert(zts_bsd_getsockopt(i32, i32, i32, NULL, NULL) == ZTS_ERR_SERVICE);
            break;
        case 130:
            assert(zts_bsd_getsockname(i32, null_addr, NULL) == ZTS_ERR_SERVICE);
            break;
        case 131:
            assert(zts_bsd_getpeername(i32, null_addr, NULL) == ZTS_ERR_SERVICE);
            break;
        case 132:
            assert(zts_bsd_close(i32) == ZTS_ERR_SERVICE);
            break;
        case 133:
            assert(zts_bsd_select(i32, NULL, NULL, NULL, NULL) == ZTS_ERR_SERVICE);
            break;
        case 134:
            assert(zts_bsd_fcntl(i32, i32, i32) == ZTS_ERR_SERVICE);
            break;
        case 135:
            assert(zts_bsd_poll(NULL, (zts_nfds_t)NULL, i32) == ZTS_ERR_SERVICE);
            break;
        case 136:
            assert(zts_bsd_ioctl(i32, i64, NULL) == ZTS_ERR_SERVICE);
            break;
        case 137:
            assert(zts_bsd_send(i32, NULL, i32, i32) == ZTS_ERR_SERVICE);
            break;
        case 138:
            assert(zts_bsd_sendto(i32, NULL, i32, i32, null_addr, i32) == ZTS_ERR_SERVICE);
            break;
        case 139:
            assert(zts_bsd_sendmsg(i32, NULL, i32) == ZTS_ERR_SERVICE);
            break;
        case 140:
            assert(zts_bsd_recv(i32, NULL, i32, i32) == ZTS_ERR_SERVICE);
            break;
        case 141:
            assert(zts_bsd_recvfrom(i32, NULL, i32, i32, null_addr, NULL) == ZTS_ERR_SERVICE);
            break;
        case 142:
            assert(zts_bsd_recvmsg(i32, NULL, i32) == ZTS_ERR_SERVICE);
            break;
        case 143:
            assert(zts_bsd_read(i32, NULL, i32) == ZTS_ERR_SERVICE);
            break;
        case 144:
            assert(zts_bsd_readv(i32, NULL, i32) == ZTS_ERR_SERVICE);
            break;
        case 145:
            assert(zts_bsd_write(i32, NULL, i32) == ZTS_ERR_SERVICE);
            break;
        case 146:
            assert(zts_bsd_writev(i32, nullable, i32) == ZTS_ERR_SERVICE);
            break;
        case 147:
            assert(zts_bsd_shutdown(i32, i32) == ZTS_ERR_SERVICE);
            break;
        case 148:
            assert(zts_set_no_delay(i32, i32) == ZTS_ERR_SERVICE);
            break;
        case 149:
            assert(zts_get_no_delay(i32) == ZTS_ERR_SERVICE);
            break;
        case 150:
            assert(zts_set_linger(i32, i32, i32) == ZTS_ERR_SERVICE);
            break;
        case 151:
            assert(zts_get_linger_value(i32) == ZTS_ERR_SERVICE);
            break;
        case 152:
            assert(zts_get_linger_value(i32) == ZTS_ERR_SERVICE);
            break;
        case 153:
            assert(zts_set_reuse_addr(i32, i32) == ZTS_ERR_SERVICE);
            break;
        case 154:
            assert(zts_get_reuse_addr(i32) == ZTS_ERR_SERVICE);
            break;
        case 155:
            assert(zts_set_recv_timeout(i32, i32, i32) == ZTS_ERR_SERVICE);
            break;
        case 156:
            assert(zts_get_recv_timeout(i32) == ZTS_ERR_SERVICE);
            break;
        case 157:
            assert(zts_set_send_timeout(i32, i32, i32) == ZTS_ERR_SERVICE);
            break;
        case 158:
            assert(zts_get_send_timeout(i32) == ZTS_ERR_SERVICE);
            break;
        case 159:
            assert(zts_set_send_buf_size(i32, i32) == ZTS_ERR_SERVICE);
            break;
        case 160:
            assert(zts_get_send_buf_size(i32) == ZTS_ERR_SERVICE);
            break;
        case 161:
            assert(zts_set_recv_buf_size(i32, i32) == ZTS_ERR_SERVICE);
            break;
        case 162:
            assert(zts_get_recv_buf_size(i32) == ZTS_ERR_SERVICE);
            break;
        case 163:
            assert(zts_set_ttl(i32, i32) == ZTS_ERR_SERVICE);
            break;
        case 164:
            assert(zts_get_ttl(i32) == ZTS_ERR_SERVICE);
            break;
        case 165:
            assert(zts_set_blocking(i32, i32) == ZTS_ERR_SERVICE);
            break;
        case 166:
            assert(zts_get_blocking(i32) == ZTS_ERR_SERVICE);
            break;
        case 167:
            assert(zts_set_keepalive(i32, i32) == ZTS_ERR_SERVICE);
            break;
        case 168:
            assert(zts_get_keepalive(i32) == ZTS_ERR_SERVICE);
            break;
        case 169:
            assert(zts_bsd_gethostbyname(NULL) == NULL);
            break;
        case 170:
            assert(zts_dns_set_server(i8, null_addr) == ZTS_ERR_SERVICE);
            break;
        case 171:
            assert(zts_dns_get_server(i8) == NULL);
            break;
            /*
        case 172:
            assert(zts_ipaddr_ntoa(null_addr) == NULL);
            break;
        case 173:
            assert(zts_ipaddr_aton(NULL, NULL) == ZTS_ERR_SERVICE);
            break;
        case 174:
            assert(zts_inet_ntop(i32, NULL, NULL, i32) == NULL);
            break;
        case 175:
            assert(zts_inet_pton(i32, NULL, NULL) == ZTS_ERR_SERVICE);
            break;
        case 176:
            assert(zts_util_ipstr_to_saddr(i32, NULL, i32, null_addr, NULL) == ZTS_ERR_SERVICE);
            break;
            */
        default:
            break;
    }
}

#define FUZZ_NUM 128

void test_pre_service_fuzz()
{
    DEBUG_INFO("\n\n***\ttest_pre_service_fuzz");

    unsigned int tid = (unsigned int)pthread_self();

    // Test service-related API functions before initializing service

    // Test null values in sequential order
    for (int i = 0; i < FUZZ_NUM; i++) {
        api_value_arg_test(tid, i, 0, 0, 0, 0, NULL);
    }
    // Test all null values in random order (delayed)
    for (int i = 0; i < FUZZ_NUM; i++) {
        uint8_t delay = (uint8_t)random64();
        uint8_t num = (uint8_t)random64();
        zts_util_delay(delay / 16);
        api_value_arg_test(tid, num, 0, 0, 0, 0, NULL);
    }
    // Test random values in random order (delayed)
    for (int i = 0; i < FUZZ_NUM; i++) {
        uint8_t delay = (uint8_t)random64();
        uint8_t num = (uint8_t)random64();
        int8_t i8 = (uint8_t)random64();
        int16_t i16 = (uint16_t)random64();
        int32_t i32 = (uint32_t)random64();
        int64_t i64 = (uint64_t)random64();
        int x;
        void* nullable = &x;
        zts_util_delay(delay / 16);
        api_value_arg_test(tid, num, i8, i16, i32, i64, nullable);
    }
    // Test all null values in random order (no delay)
    for (int i = 0; i < FUZZ_NUM; i++) {
        uint8_t num = (uint8_t)random64();
        api_value_arg_test(tid, num, 0, 0, 0, 0, NULL);
    }
    // Test random values in random order (no delay)
    for (int i = 0; i < FUZZ_NUM; i++) {
        uint8_t num = (uint8_t)random64();
        int8_t i8 = (uint8_t)random64();
        int16_t i16 = (uint16_t)random64();
        int32_t i32 = (uint32_t)random64();
        int64_t i64 = (uint64_t)random64();
        int x;
        void* nullable = &x;
        api_value_arg_test(tid, num, i8, i16, i32, i64, nullable);
    }

    // Test non-service helper functions

    // (B) Test zts_inet_ntop

    char ipstr[ZTS_INET6_ADDRSTRLEN] = { 0 };
    int16_t port = 0;
    struct zts_sockaddr_in in4;

    in4.sin_port = htons(8080);
#if defined(_WIN32)
    zts_inet_pton(ZTS_AF_INET, "192.168.22.1", &(in4.sin_addr.S_addr));
#else
    zts_inet_pton(ZTS_AF_INET, "192.168.22.1", &(in4.sin_addr.s_addr));
#endif

    in4.sin_family = ZTS_AF_INET;

    struct zts_sockaddr* sa = (struct zts_sockaddr*)&in4;
    if (sa->sa_family == ZTS_AF_INET) {
        struct zts_sockaddr_in* in4 = (struct zts_sockaddr_in*)sa;
        zts_inet_ntop(ZTS_AF_INET, &(in4->sin_addr), ipstr, ZTS_INET_ADDRSTRLEN);
        port = ntohs(in4->sin_port);
    }
    if (sa->sa_family == ZTS_AF_INET6) {
        struct zts_sockaddr_in6* in6 = (struct zts_sockaddr_in6*)sa;
        zts_inet_ntop(ZTS_AF_INET6, &(in6->sin6_addr), ipstr, ZTS_INET6_ADDRSTRLEN);
    }

    assert(port == 8080);
    assert(! strcmp(ipstr, "192.168.22.1"));

    // (C) Test zts_inet_pton

    uint8_t buf[sizeof(struct zts_in6_addr)] = { 0 };
    char str[ZTS_INET6_ADDRSTRLEN] = { 0 };

    zts_inet_pton(ZTS_AF_INET, "192.168.22.2", buf);
    zts_inet_ntop(ZTS_AF_INET, buf, str, ZTS_INET6_ADDRSTRLEN);
    assert(! strcmp(str, "192.168.22.2"));
}

void test_sockets()
{
    int res = ZTS_ERR_OK;

    // Test simplified API, proxy for setsockopt/getsockopt/ioctl etc.

    int s4 = zts_bsd_socket(ZTS_AF_INET6, ZTS_SOCK_STREAM, 0);
    assert(s4 >= 0);

    // TCP_NODELAY

    // Check value before doing anything
    assert(zts_get_no_delay(s4) == 0);
    // Turn on
    assert(zts_set_no_delay(s4, 1) == ZTS_ERR_OK);
    // Should return value instead of error code
    assert(zts_get_no_delay(s4) == 1);
    // Turn off
    assert(zts_set_no_delay(s4, 0) == ZTS_ERR_OK);
    assert(zts_get_no_delay(s4) == ZTS_ERR_OK);

    // SO_LINGER

    // Check value before doing anything
    assert(zts_get_linger_enabled(s4) == 0);
    assert(zts_get_linger_value(s4) == 0);
    // Turn on, set to 7 seconds
    assert(zts_set_linger(s4, 1, 7) == ZTS_ERR_OK);
    assert(zts_get_linger_enabled(s4) == 1);
    assert(zts_get_linger_value(s4) == 7);
    assert(zts_set_linger(s4, 0, 0) == ZTS_ERR_OK);
    // Turn off
    assert(zts_get_linger_enabled(s4) == 0);
    assert(zts_get_linger_value(s4) == 0);

    // SO_REUSEADDR

    // Check value before doing anything
    assert(zts_get_reuse_addr(s4) == 0);
    // Turn on
    assert(zts_set_reuse_addr(s4, 1) == ZTS_ERR_OK);
    // Should return value instead of error code
    assert(zts_get_reuse_addr(s4) == 1);
    // Turn off
    assert(zts_set_reuse_addr(s4, 0) == ZTS_ERR_OK);
    assert(zts_get_reuse_addr(s4) == ZTS_ERR_OK);

    // SO_RCVTIMEO

    // Check value before doing anything
    assert(zts_get_recv_timeout(s4) == 0);
    // Set to value
    assert(zts_set_recv_timeout(s4, 3, 0) == ZTS_ERR_OK);
    assert(zts_get_recv_timeout(s4) == 3);
    assert(zts_set_recv_timeout(s4, 0, 0) == ZTS_ERR_OK);
    // Set to zero
    assert(zts_get_recv_timeout(s4) == 0);

    // SO_SNDTIMEO

    // Check value before doing anything
    assert(zts_get_send_timeout(s4) == 0);
    // Set to value
    assert(zts_set_send_timeout(s4, 4, 0) == ZTS_ERR_OK);
    assert(zts_get_send_timeout(s4) == 4);
    assert(zts_set_send_timeout(s4, 0, 0) == ZTS_ERR_OK);
    // Set to zero
    assert(zts_get_send_timeout(s4) == 0);

    // SO_SNDBUF

    // Check value before doing anything
    assert(zts_get_send_buf_size(s4) == -1);   // Unimplemented
    // Set to 7 seconds
    assert(zts_set_send_buf_size(s4, 1024) == ZTS_ERR_OK);
    assert(zts_get_send_buf_size(s4) == -1);   // Unimplemented
    assert(zts_set_send_buf_size(s4, 0) == ZTS_ERR_OK);
    // Set to zero
    assert(zts_get_send_buf_size(s4) == -1);   // Unimplemented

    // SO_RCVBUF

    // Check value before doing anything
    assert(zts_get_recv_buf_size(s4) > 0);
    // Set to value
    assert(zts_set_recv_buf_size(s4, 1024) == ZTS_ERR_OK);
    assert(zts_get_recv_buf_size(s4) == 1024);
    assert(zts_set_recv_buf_size(s4, 0) == ZTS_ERR_OK);
    // Set to zero
    assert(zts_get_recv_buf_size(s4) == 0);

    // IP_TTL

    // Check value before doing anything
    assert(zts_get_ttl(s4) == 255);   // Defaults to max
    // Set to value
    assert(zts_set_ttl(s4, 128) == ZTS_ERR_OK);
    assert(zts_get_ttl(s4) == 128);
    assert(zts_set_ttl(s4, 0) == ZTS_ERR_OK);
    // Set to zero
    assert(zts_get_ttl(s4) == 0);

    // O_NONBLOCK

    // Check value before doing anything
    assert(zts_get_blocking(s4) == 1);
    // Turn off (non-blocking)
    assert(zts_set_blocking(s4, 0) == ZTS_ERR_OK);
    // Should return value instead of error code
    assert(zts_get_blocking(s4) == 0);
    // Turn off
    assert(zts_set_blocking(s4, 1) == ZTS_ERR_OK);
    assert(zts_get_blocking(s4) == 1);

    // SO_KEEPALIVE

    // Check value before doing anything
    assert(zts_get_keepalive(s4) == 0);
    // Turn on
    assert(zts_set_keepalive(s4, 1) == ZTS_ERR_OK);
    // Should return value instead of error code
    assert(zts_get_keepalive(s4) == 1);
    // Turn off
    assert(zts_set_keepalive(s4, 0) == ZTS_ERR_OK);
    assert(zts_get_keepalive(s4) == ZTS_ERR_OK);

    // TODO

    // char peername[ZTS_INET6_ADDRSTRLEN] = { 0 };
    // int port = 0;
    // int res = zts_getpeername(accfd, peername, ZTS_INET6_ADDRSTRLEN, &port);
    // printf("getpeername = %s : %d (%d)\n", peername, port, res);
    // res = zts_getsockname(accfd, peername, ZTS_INET6_ADDRSTRLEN, &port);
    // printf("getsockname = %s : %d (%d)\n", peername, port, res);

    // Test DNS client functionality

    /*
        // Set first nameserver

        char *ns1_addr_str = "FCC5:205E:4FF5:5311:DFF0::1";
        zts_ip_addr ns1;
        zts_ipaddr_aton(ns1_addr_str, &ns1);
        zts_dns_set_server(0, &ns1);

        // Get first nameserver

        const zts_ip_addr *ns1_result;
        ns1_result = zts_dns_get_server(0);
        DEBUG_INFO("dns1 = %s", zts_ipaddr_ntoa(ns1_result));

        // Set second nameserver

        char *ns2_addr_str = "192.168.22.1";
        zts_ip_addr ns2;
        zts_ipaddr_aton(ns2_addr_str, &ns2);
        zts_dns_set_server(1, &ns2);

        // Get second nameserver

        const zts_ip_addr *ns2_result;
        ns2_result = zts_dns_get_server(1);
        DEBUG_INFO("dns1 = %s", zts_ipaddr_ntoa(ns2_result));

        // Check that each nameserver address was properly set and get

        assert(("zts_dns_get_server(): Address mismatch", !strcmp(ns1_addr_str,
       zts_ipaddr_ntoa(ns1_result)))); assert(("zts_dns_get_server(): Address
       mismatch", !strcmp(ns2_addr_str, zts_ipaddr_ntoa(ns2_result))));
    */

    // Test shutting down the service

    zts_node_stop();
    s4 = zts_bsd_socket(ZTS_AF_INET, ZTS_SOCK_STREAM, 0);
    assert(s4 == ZTS_ERR_SERVICE);
}

//----------------------------------------------------------------------------//
// Server                                                                     //
//----------------------------------------------------------------------------//

#define MAX_CONNECT_TIME 60   // outer re-attempt loop
#define CONNECT_TIMEOUT  30   // zts_connect, ms
#define BUFLEN           128
char* msg = "welcome to the machine";

void test_server_socket_usage(uint16_t port4, uint16_t port6)
{
    int err = ZTS_ERR_OK;
    int bytes_read = 0;
    int bytes_sent = 0;

    int msglen = strlen(msg);
    char dstbuf[BUFLEN] = { 0 };
    int buflen = BUFLEN;

    struct timespec start, now;
    int time_diff = 0;

    // IPv4 test

    DEBUG_INFO("server4: will listen on: 0.0.0.0:%d", port4);
    int s4 = zts_bsd_socket(ZTS_AF_INET, ZTS_SOCK_STREAM, 0);
    assert(s4 == 0 && zts_errno == 0);

    err = zts_bind(s4, "0.0.0.0", port4);
    assert(err == ZTS_ERR_OK && zts_errno == 0);

    err = zts_bsd_listen(s4, 1);
    assert(err == ZTS_ERR_OK && zts_errno == 0);

    struct zts_sockaddr_in in4;
    zts_socklen_t addrlen4 = sizeof(in4);

    int acc4 = -1;
    clock_gettime(CLOCK_MONOTONIC, &start);
    do {
        DEBUG_INFO("server4: accepting...");
        acc4 = zts_bsd_accept(s4, &in4, &addrlen4);
        zts_util_delay(250);
        clock_gettime(CLOCK_MONOTONIC, &now);
        time_diff = (now.tv_sec - start.tv_sec);
    } while (err < 0 && time_diff < MAX_CONNECT_TIME);

    assert(acc4 == 1 && zts_errno == 0);

    // Read message

    memset(dstbuf, 0, buflen);

    // Test zts_get_data_available
    while (1) {
        int av = zts_get_data_available(acc4);
        zts_util_delay(50);
        if (av > 0) {
            break;
        }
    }
    bytes_read = zts_bsd_read(acc4, dstbuf, buflen);
    DEBUG_INFO("server4: read (%d) bytes", bytes_read);
    assert(bytes_read == msglen && zts_errno == 0);

    // Send message

    bytes_sent = zts_bsd_write(acc4, msg, msglen);
    DEBUG_INFO("server4: wrote (%d) bytes", bytes_sent);
    assert(bytes_sent == msglen && zts_errno == 0);

    zts_bsd_close(s4);
    assert(err == ZTS_ERR_OK && zts_errno == 0);

    zts_bsd_close(acc4);
    assert(err == ZTS_ERR_OK && zts_errno == 0);

    assert(bytes_sent == bytes_read);
    if (bytes_sent == bytes_read) {
        DEBUG_INFO("server4: Test OK");
    }
    else {
        DEBUG_INFO("server4: Test FAIL");
    }

    // IPv6 test

    DEBUG_INFO("server6: will listen on: [::]:%d", port6);
    int s6 = zts_bsd_socket(ZTS_AF_INET6, ZTS_SOCK_STREAM, 0);
    assert(s6 == 0 && zts_errno == 0);

    err = zts_bind(s6, "::", port6);
    assert(err == ZTS_ERR_OK && zts_errno == 0);

    err = zts_bsd_listen(s6, 1);
    assert(err == ZTS_ERR_OK && zts_errno == 0);

    struct zts_sockaddr_in6 in6;
    zts_socklen_t addrlen6 = sizeof(in6);

    int acc6 = -1;
    clock_gettime(CLOCK_MONOTONIC, &start);
    do {
        DEBUG_INFO("server6: accepting...");
        acc6 = zts_bsd_accept(s6, &in6, &addrlen6);
        zts_util_delay(250);
        clock_gettime(CLOCK_MONOTONIC, &now);
        time_diff = (now.tv_sec - start.tv_sec);
    } while (err < 0 && time_diff < MAX_CONNECT_TIME);

    DEBUG_INFO("server6: accepted connection (fd=%d)", acc6);
    assert(acc6 == 1 && zts_errno == 0);

    // Read message
    memset(dstbuf, 0, buflen);
    bytes_read = zts_bsd_read(acc6, dstbuf, buflen);
    DEBUG_INFO("server6: read (%d) bytes", bytes_read);
    assert(bytes_read == msglen && zts_errno == 0);

    // Send message
    bytes_sent = zts_bsd_write(acc6, msg, msglen);
    DEBUG_INFO("server6: wrote (%d) bytes", bytes_sent);
    assert(bytes_sent == msglen && zts_errno == 0);

    zts_bsd_close(s6);
    assert(err == ZTS_ERR_OK && zts_errno == 0);

    zts_bsd_close(acc6);
    assert(err == ZTS_ERR_OK && zts_errno == 0);

    zts_node_stop();
    assert(err == ZTS_ERR_OK && zts_errno == 0);
    int s = zts_bsd_socket(ZTS_AF_INET, ZTS_SOCK_STREAM, 0);
    assert(s == ZTS_ERR_SERVICE);

    assert(bytes_sent == bytes_read);
    if (bytes_sent == bytes_read) {
        DEBUG_INFO("server6: Test OK");
    }
    else {
        DEBUG_INFO("server6: Test FAIL");
    }
}

//----------------------------------------------------------------------------//
// Client                                                                     //
//----------------------------------------------------------------------------//

void test_client_socket_usage(char* ip4, uint16_t port4, char* ip6, uint16_t port6)
{
    int err = ZTS_ERR_OK;
    int bytes_read = 0;
    int bytes_sent = 0;

    int msglen = strlen(msg);
    char dstbuf[BUFLEN] = { 0 };
    int buflen = BUFLEN;

    struct timespec start, now;
    int time_diff = 0;

    // IPv4 test

    err = ZTS_ERR_OK;
    int s4 = zts_bsd_socket(ZTS_AF_INET, ZTS_SOCK_STREAM, 0);
    assert(err == ZTS_ERR_OK && zts_errno == 0);

    zts_set_blocking(s4, 1);
    assert(err == ZTS_ERR_OK && zts_errno == 0);

    clock_gettime(CLOCK_MONOTONIC, &start);
    do {
        DEBUG_INFO("client4: connecting to: %s:%d", ip4, port4);
        err = zts_connect(s4, ip4, port4, CONNECT_TIMEOUT);
        zts_util_delay(500);
        clock_gettime(CLOCK_MONOTONIC, &now);
        time_diff = (now.tv_sec - start.tv_sec);
    } while (err < 0 && time_diff < MAX_CONNECT_TIME);
    assert(err == ZTS_ERR_OK && zts_errno == 0);

    DEBUG_INFO("client4: connected");
    // Send message
    bytes_sent = zts_bsd_write(s4, msg, msglen);
    DEBUG_INFO("client4: wrote (%d) bytes", bytes_sent);
    assert(bytes_sent == msglen && zts_errno == 0);

    // Read message
    memset(dstbuf, 0, buflen);
    bytes_read = zts_bsd_read(s4, dstbuf, buflen);
    assert(bytes_read == msglen && zts_errno == 0);

    DEBUG_INFO("client4: read (%d) bytes", bytes_read);
    assert(bytes_sent == bytes_read && zts_errno == 0);

    zts_bsd_close(s4);
    assert(err == ZTS_ERR_OK && zts_errno == 0);

    assert(bytes_sent == bytes_read);
    if (bytes_sent == bytes_read) {
        DEBUG_INFO("client4: Test OK");
    }
    else {
        DEBUG_INFO("client4: Test FAIL");
    }

    // IPv6 test

    err = ZTS_ERR_OK;
    int s6 = zts_bsd_socket(ZTS_AF_INET6, ZTS_SOCK_STREAM, 0);
    assert(err == ZTS_ERR_OK && zts_errno == 0);

    zts_set_blocking(s6, 1);
    assert(err == ZTS_ERR_OK && zts_errno == 0);

    clock_gettime(CLOCK_MONOTONIC, &start);
    do {
        DEBUG_INFO("client6: connecting to: %s:%d", ip6, port6);
        err = zts_connect(s6, ip6, port6, CONNECT_TIMEOUT);
        zts_util_delay(500);
        clock_gettime(CLOCK_MONOTONIC, &now);
        time_diff = (now.tv_sec - start.tv_sec);
    } while (err < 0 && time_diff < MAX_CONNECT_TIME);
    assert(err == ZTS_ERR_OK && zts_errno == 0);

    DEBUG_INFO("client6: connected");
    // Send message
    bytes_sent = zts_bsd_write(s6, msg, msglen);
    DEBUG_INFO("client6: wrote (%d) bytes", bytes_sent);
    assert(bytes_sent == msglen && zts_errno == 0);

    // Read message
    memset(dstbuf, 0, buflen);
    bytes_read = zts_bsd_read(s6, dstbuf, buflen);
    assert(bytes_read == msglen && zts_errno == 0);

    DEBUG_INFO("client6: read (%d) bytes", bytes_read);
    assert(bytes_sent == bytes_read && zts_errno == 0);

    zts_bsd_close(s6);
    assert(err == ZTS_ERR_OK && zts_errno == 0);

    zts_node_stop();
    assert(err == ZTS_ERR_OK && zts_errno == 0);
    int s = zts_bsd_socket(ZTS_AF_INET, ZTS_SOCK_STREAM, 0);
    assert(s == ZTS_ERR_SERVICE);

    assert(bytes_sent == bytes_read);
    if (bytes_sent == bytes_read) {
        DEBUG_INFO("client6: Test OK");
    }
    else {
        DEBUG_INFO("client6: Test FAIL");
    }
}

//----------------------------------------------------------------------------//
// Start node                                                                 //
//----------------------------------------------------------------------------//

int test_api_abuse()
{
    /*

    TODO

    DEBUG_INFO("\n\n***\ttest_api_abuse");
    if (join_network) {
        for (int i = 0; i < 1024 * 4; i++) {
            printf("join %d\n", i);
            zts_net_join(net_id);
            printf("leave %d\n", i);
            zts_net_leave(net_id);
        }
        zts_node_stop();
        zts_util_delay(2000);
        exit(0);
    }
    */
    return 0;
}

int test_start_node(
    char* path,
    uint64_t net_id,
    char* keypair,
    int use_storage,
    int use_callbacks,
    int use_identity,
    int join_network,
    int shutdown)
{
    DEBUG_INFO("\n\n***\ttest_start_node");

    struct timespec start, now;
    int time_diff = 0;

    zts_util_delay(5000);           // Allow events to settle (if any)
    callback_was_called_flag = 0;   // Reset

    DEBUG_INFO("starting node...");
    clock_gettime(CLOCK_MONOTONIC, &start);

    int res = ZTS_ERR_OK;

    // Optional initialization

    if (use_storage) {
        assert(zts_init_from_storage(path) == ZTS_ERR_OK);
    }
    if (use_callbacks) {
        assert(zts_init_set_event_handler(&on_zts_event) == ZTS_ERR_OK);
    }
    if (use_identity) {
        // TODO: tomorrow
        assert(zts_init_from_memory(keypair, ZTS_ID_STR_BUF_LEN) == ZTS_ERR_OK);
    }

    // Start

    assert(zts_node_start() == ZTS_ERR_OK);
    do {
        zts_util_delay(25);
        clock_gettime(CLOCK_MONOTONIC, &now);
        time_diff = (now.tv_sec - start.tv_sec);
    } while (! zts_node_is_online() && (time_diff < MAX_CONNECT_TIME));
    if (! zts_node_is_online()) {
        DEBUG_INFO("Node failed to come online");
        exit(-1);
    }

    // Test identity handling
    char keypair_i[ZTS_ID_STR_BUF_LEN] = { 0 };
    unsigned int keypair_len = ZTS_ID_STR_BUF_LEN;
    assert(zts_node_get_id_pair(keypair_i, &keypair_len) == ZTS_ERR_OK);
    DEBUG_INFO("Checking key length");
    assert(keypair_len <= ZTS_ID_STR_BUF_LEN);
    DEBUG_INFO("GET [identity = %s]", keypair_i);
    DEBUG_INFO("Checking validity of identity");
    assert(zts_id_pair_is_valid(keypair_i, ZTS_ID_STR_BUF_LEN) == 1);

    // Test various getters

    assert(zts_node_get_id() != 0);
    DEBUG_INFO("GET [id: %llx]", zts_node_get_id());

    assert(zts_node_get_port() > 0);
    DEBUG_INFO("GET [port: %d]", zts_node_get_port());

    if (join_network) {
        DEBUG_INFO("Joining: %llx", net_id);
        clock_gettime(CLOCK_MONOTONIC, &start);
        if (net_id) {
            zts_net_join(net_id);
            do {
                zts_util_delay(25);
                clock_gettime(CLOCK_MONOTONIC, &now);
                time_diff = (now.tv_sec - start.tv_sec);
            } while ((! zts_addr_is_assigned(net_id, ZTS_AF_INET) || ! zts_addr_is_assigned(net_id, ZTS_AF_INET6))
                     && (time_diff < MAX_CONNECT_TIME));

            if (! zts_addr_is_assigned(net_id, ZTS_AF_INET) || ! zts_addr_is_assigned(net_id, ZTS_AF_INET6)) {
                DEBUG_INFO("Node failed to receive assigned addresses");
                exit(-1);
            }
        }

        char ipstr[ZTS_INET6_ADDRSTRLEN] = { 0 };

        // Get ipv4

        struct zts_sockaddr_storage ss;
        assert(zts_addr_get(net_id, ZTS_AF_INET, &ss) == ZTS_ERR_OK);
        struct zts_sockaddr_in* in4 = (struct zts_sockaddr_in*)&ss;
        zts_inet_ntop(ZTS_AF_INET, &(in4->sin_addr), ipstr, ZTS_INET6_ADDRSTRLEN);
        DEBUG_INFO("ipv4: %s", ipstr);
        assert(! strcmp(ipstr, "172.22.22.2") || ! strcmp(ipstr, "172.22.22.1"));
        memset(ipstr, 0, sizeof(ipstr));

        // Get ipv6

        assert(zts_addr_get(net_id, ZTS_AF_INET6, &ss) == ZTS_ERR_OK);
        struct zts_sockaddr_in6* in6 = (struct zts_sockaddr_in6*)&ss;
        zts_inet_ntop(ZTS_AF_INET6, &(in6->sin6_addr), ipstr, ZTS_INET6_ADDRSTRLEN);
        DEBUG_INFO("ipv6: %s", ipstr);
        assert(! strcmp(ipstr, "FCC5:205E:4F90:9691:8F72::1") || ! strcmp(ipstr, "FCC5:205E:4FF7:46D5:50DD::1"));
        memset(ipstr, 0, sizeof(ipstr));

        // Get ipv4 (string format)

        assert(zts_addr_get_str(net_id, ZTS_AF_INET, ipstr, ZTS_INET6_ADDRSTRLEN) == ZTS_ERR_OK);
        DEBUG_INFO("ipv4_str: %s", ipstr);
        memset(ipstr, 0, sizeof(ipstr));

        // Get ipv6 (string format)

        assert(zts_addr_get_str(net_id, ZTS_AF_INET6, ipstr, ZTS_INET6_ADDRSTRLEN) == ZTS_ERR_OK);
        DEBUG_INFO("ipv6_str: %s", ipstr);
        memset(ipstr, 0, sizeof(ipstr));

        // Get (all) ipv4 addresses in an array of sockaddr_storage objects

        struct zts_sockaddr_storage ss_all[ZTS_MAX_ASSIGNED_ADDRESSES] = { 0 };
        int count = ZTS_MAX_ASSIGNED_ADDRESSES;
        assert(zts_addr_get_all(net_id, ss_all, &count) == ZTS_ERR_OK);
        assert(count > 0 && count <= ZTS_MAX_ASSIGNED_ADDRESSES);
        DEBUG_INFO("assigned addr count: %d", count);
        for (int i = 0; i < count; i++) {
            struct zts_sockaddr* sa = (struct zts_sockaddr*)&ss_all[i];
            if (sa->sa_family == ZTS_AF_INET) {
                struct zts_sockaddr_in* in4 = (struct zts_sockaddr_in*)&ss_all[i];
                zts_inet_ntop(ZTS_AF_INET, &(in4->sin_addr), ipstr, ZTS_INET6_ADDRSTRLEN);
                DEBUG_INFO("ipv(all): %s", ipstr);
            }
            if (sa->sa_family == ZTS_AF_INET6) {
                struct zts_sockaddr_in6* in6 = (struct zts_sockaddr_in6*)&ss_all[i];
                zts_inet_ntop(ZTS_AF_INET6, &(in6->sin6_addr), ipstr, ZTS_INET6_ADDRSTRLEN);
                DEBUG_INFO("ipv(all): %s", ipstr);
            }
        }

        // (C) Test zts_inet_pton

        uint8_t buf[sizeof(struct zts_in6_addr)] = { 0 };
        char str[ZTS_INET6_ADDRSTRLEN] = { 0 };

        zts_inet_pton(ZTS_AF_INET, "192.168.22.2", buf);
        zts_inet_ntop(ZTS_AF_INET, buf, str, ZTS_INET6_ADDRSTRLEN);
        assert(! strcmp(str, "192.168.22.2"));

        // assert(zts_net_count() > 0);
        // DEBUG_INFO("Networks joined : %d", zts_net_count());

        char name[ZTS_MAX_NETWORK_SHORT_NAME_LENGTH] = { 0 };

        assert(zts_net_get_name(net_id, name, ZTS_MAX_NETWORK_SHORT_NAME_LENGTH) == ZTS_ERR_OK);
        DEBUG_INFO("Network name: %s", name);
        assert(! strcmp(name, "TESTNET-1"));

        // Test zts_core_ locking functions to query node

        zts_core_lock_obtain();

        char addr_str[ZTS_INET6_ADDRSTRLEN] = { 0 };
        char target_str[ZTS_INET6_ADDRSTRLEN] = { 0 };
        char via_str[ZTS_INET6_ADDRSTRLEN] = { 0 };
        uint16_t flags;
        uint16_t metric;

        int addr_count = zts_core_query_addr_count(net_id);
        DEBUG_INFO("addr_count = %d", addr_count);
        for (int i = 0; i < addr_count; i++) {
            zts_core_query_addr(net_id, i, addr_str, ZTS_INET6_ADDRSTRLEN);
            DEBUG_INFO("addr = %s", addr_str);
        }

        int route_count = zts_core_query_route_count(net_id);
        DEBUG_INFO("route_count = %d", route_count);
        for (int i = 0; i < route_count; i++) {
            zts_core_query_route(net_id, i, target_str, via_str, ZTS_INET6_ADDRSTRLEN, &flags, &metric);
            DEBUG_INFO("target = %s, via = %s, flags = %d, metric = %d", target_str, via_str, flags, metric);
        }

        int mc_sub_count = zts_core_query_mc_count(net_id);
        DEBUG_INFO("addr_count = %d", addr_count);
        for (int i = 0; i < mc_sub_count; i++) {
            zts_core_query_addr(net_id, i, addr_str, ZTS_INET6_ADDRSTRLEN);
            DEBUG_INFO("addr = %s", addr_str);
        }

        zts_core_lock_release();

    }   // join network

    if (! use_callbacks) {
        DEBUG_INFO("Asserting that no events were generated");
        assert(callback_was_called_flag == 0);
    }

    // Ensure that no networks were joined (if not explicitly joined)

    // if (! join_network) {
    //	assert(zts_net_count() == 0);
    //}

    // Shut down node if requested

    if (shutdown) {
        DEBUG_INFO("Shutting down node...");
        assert(zts_node_stop() == ZTS_ERR_OK);
        DEBUG_INFO("Node has been shut down");
    }

    return ZTS_ERR_OK;
}

void test_identity_key_handling()
{
    DEBUG_INFO("\n\n***\ttest_identity_key_handling");
    char keypair[ZTS_ID_STR_BUF_LEN] = { 0 };
    unsigned int keypair_len = ZTS_ID_STR_BUF_LEN;

    // Test valid key

    DEBUG_INFO("Generating new identity...");
    assert(zts_id_new(keypair, &keypair_len) == ZTS_ERR_OK);
    DEBUG_INFO("Checking key length");
    assert(keypair_len <= ZTS_ID_STR_BUF_LEN);
    DEBUG_INFO("Checking validity of identity");
    DEBUG_INFO("Identity = [%s]", keypair);
    assert(zts_id_pair_is_valid(keypair, ZTS_ID_STR_BUF_LEN) == 1);

    /* Test valid key with incorrect len provided by user. This test should return
    false even if a valid key was provided. This is because we want the user to be
    notified of a possible error the size of the buffer that they may use to contain
    the key for other operations. I think this is the responsible thing to do? */

    DEBUG_INFO("Checking validity of identity with incorrect provided length");
    assert(zts_id_pair_is_valid(keypair, -1024) == 0);
    assert(zts_id_pair_is_valid(keypair, -1) == 0);
    assert(zts_id_pair_is_valid(keypair, 0) == 0);
    assert(zts_id_pair_is_valid(keypair, 1) == 0);
    assert(zts_id_pair_is_valid(keypair, 1024) == 0);

    // Test key that is too short

    char keypair_short[ZTS_ID_STR_BUF_LEN] = { 0 };
    strncpy(keypair_short, keypair, 64);
    DEBUG_INFO("Checking validity of key that is too short (should be false)");
    DEBUG_INFO("Identity = [%s]", keypair_short);
    assert(zts_id_pair_is_valid(keypair_short, ZTS_ID_STR_BUF_LEN) == 0);

    // Test key that is too long

    char keypair_long[ZTS_ID_STR_BUF_LEN] = { 0 };
    strncpy(keypair_long, keypair, ZTS_ID_STR_BUF_LEN);
    strncat(keypair_long, keypair, 16);
    DEBUG_INFO("len=%lu", strlen(keypair_long));
    DEBUG_INFO("Checking validity of key that is too long (should be false)");
    DEBUG_INFO("Identity = [%s]", keypair_long);
    assert(zts_id_pair_is_valid(keypair_long, ZTS_ID_STR_BUF_LEN) == 0);

    // Test empty key

    char keypair_null[ZTS_ID_STR_BUF_LEN] = { 0 };
    DEBUG_INFO("Checking validity of key that is empty (should be false)");
    DEBUG_INFO("Identity = [%s]", keypair_null);
    assert(zts_id_pair_is_valid(keypair_null, ZTS_ID_STR_BUF_LEN) == 0);

    // Test valid key with a corrupted element

    keypair[32]++;
    DEBUG_INFO("Checking validity of identity that is corrupted (should be false)");
    DEBUG_INFO("Identity = [%s]", keypair);
    assert(zts_id_pair_is_valid(keypair, ZTS_ID_STR_BUF_LEN) == 0);
}

void test_addr_computation()
{
    DEBUG_INFO("\n\n***\ttest_addr_computation");
    // Test plausible values
    uint64_t nodeid = 0x75f3543094;
    uint16_t start_port = 2000;
    uint16_t end_port = 3000;
    uint64_t net_id = zts_net_compute_adhoc_id(start_port, end_port);
    assert(net_id == 0xff07d00bb8000000);
    char ipstr[ZTS_IP_MAX_STR_LEN] = { 0 };
    assert(zts_addr_compute_rfc4193_str(net_id, nodeid, ipstr, ZTS_IP_MAX_STR_LEN) == ZTS_ERR_OK);
    assert(! strcmp(ipstr, "FDFF:7D0:BB8:0:99:9375:F354:3094"));
    assert(zts_addr_compute_6plane_str(net_id, nodeid, ipstr, ZTS_IP_MAX_STR_LEN) == ZTS_ERR_OK);
    assert(! strcmp(ipstr, "FC47:7D0:B75:F354:3094::1"));
}

void test_roots_handling()
{
    DEBUG_INFO("\n\n***\ttest_roots_handling");

    char roots_data[ZTS_STORE_DATA_LEN] = { 0 };
    int len = ZTS_STORE_DATA_LEN;
    assert(zts_init_set_roots(NULL, 1) != ZTS_ERR_OK);
    assert(zts_init_set_roots(roots_data, 0) != ZTS_ERR_OK);
    assert(zts_init_set_roots(roots_data, len) == ZTS_ERR_OK);
    // TODO: Test setting when node is already running
}

void test_start_sequences()
{
    DEBUG_INFO("\n\n***\ttest_start_sequences");
    int res;
    char* path = ".";
    uint64_t net_id;
    char keypair_f[ZTS_ID_STR_BUF_LEN] = { 0 };
    unsigned int keypair_len = ZTS_ID_STR_BUF_LEN;
    int use_storage;
    int use_callbacks;
    int use_identity;
    int join_network;
    int shutdown;

    /* Start node with no given identity, no storage access allowed, callbacks
    disabled. Once it is confirmed to be online save the identity for future
    use. */
    DEBUG_INFO("TEST: Node with no id, no storage, no callbacks");
    use_storage = 0;
    use_callbacks = 0;
    use_identity = 0;
    join_network = 0;
    shutdown = 0;
    assert(
        test_start_node(".", 0x0, NULL, use_storage, use_callbacks, use_identity, join_network, shutdown)
        == ZTS_ERR_OK);
    assert(zts_node_get_id_pair(keypair_i, &keypair_len) == ZTS_ERR_OK);
    assert(zts_node_stop() == ZTS_ERR_OK);
    // Confirm that no callbacks were sent to the user

    /* Start a node under similar conditions as above, but this time provide it
    with the previously-generated identity and then try to read it back from
    the node to ensure it was set properly */
    DEBUG_INFO("TEST: Node with given id, no storage, no callbacks");
    use_storage = 0;
    use_callbacks = 0;
    use_identity = 1;
    join_network = 0;
    shutdown = 0;
    assert(
        test_start_node(".", 0x0, keypair_i, use_storage, use_callbacks, use_identity, join_network, shutdown)
        == ZTS_ERR_OK);
    keypair_len = ZTS_ID_STR_BUF_LEN;
    assert(zts_node_get_id_pair(keypair_f, &keypair_len) == ZTS_ERR_OK);
    // Compare keypairs
    DEBUG_INFO("Comparing keys");
    assert(! strcmp(keypair_i, keypair_f));
    assert(zts_node_stop() == ZTS_ERR_OK);

    /* start node with above identity and storage allowed. should not load from
    storage if an identity exists since one was already provided */
    DEBUG_INFO("TEST: Node with given id, storage, no callbacks");
    use_storage = 1;
    use_callbacks = 0;
    use_identity = 1;
    join_network = 0;
    shutdown = 0;
    assert(
        test_start_node(".", 0x0, keypair_i, use_storage, use_callbacks, use_identity, join_network, shutdown)
        == ZTS_ERR_OK);
    keypair_len = ZTS_ID_STR_BUF_LEN;
    assert(zts_node_get_id_pair(keypair_f, &keypair_len) == ZTS_ERR_OK);
    assert(zts_node_stop() == ZTS_ERR_OK);
    // Compare keypairs
    DEBUG_INFO("Comparing keys");
    assert(! strcmp(keypair_i, keypair_f));
}

#define NUM_THREADS 2

int test_thread_safety()
{
    DEBUG_INFO("\n\n***\ttest_thread_safety");
    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        int res = pthread_create(&threads[i], NULL, test_pre_service_fuzz, (void*)NULL);
    }
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    return 0;
}

int test_stats()
{
    DEBUG_INFO("\n\n***\ttest_stats");
    zts_stats_counter_t s = { 0 };

    int err = ZTS_ERR_OK;

    if ((err = zts_stats_get_all(&s)) == ZTS_ERR_NO_RESULT) {
        printf("no results\n");
    }

    printf(
        "  link_tx=%9d,   link_rx=%9d,   link_drop=%9d,   link_err=%9d\n",
        s.link_tx,
        s.link_rx,
        s.link_drop,
        s.link_err);
    printf(
        "etharp_tx=%9d, etharp_rx=%9d, etharp_drop=%9d, etharp_err=%9d\n",
        s.etharp_tx,
        s.etharp_rx,
        s.etharp_drop,
        s.etharp_err);
    printf(
        "   ip4_tx=%9d,    ip4_rx=%9d,    ip4_drop=%9d,    ip4_err=%9d\n",
        s.ip4_tx,
        s.ip4_rx,
        s.ip4_drop,
        s.ip4_err);
    printf(
        "   ip6_tx=%9d,    ip6_rx=%9d,    ip6_drop=%9d,    ip6_err=%9d\n",
        s.ip6_tx,
        s.ip6_rx,
        s.ip6_drop,
        s.ip6_err);
    printf(
        " icmp4_tx=%9d,  icmp4_rx=%9d,  icmp4_drop=%9d,  icmp4_err=%9d\n",
        s.icmp4_tx,
        s.icmp4_rx,
        s.icmp4_drop,
        s.icmp4_err);
    printf(
        " icmp6_tx=%9d,  icmp6_rx=%9d,  icmp6_drop=%9d,  icmp6_err=%9d\n",
        s.icmp6_tx,
        s.icmp6_rx,
        s.icmp6_drop,
        s.icmp6_err);
    printf(
        "   udp_tx=%9d,    udp_rx=%9d,    udp_drop=%9d,    udp_err=%9d\n",
        s.udp_tx,
        s.udp_rx,
        s.udp_drop,
        s.udp_err);
    printf(
        "   tcp_tx=%9d,    tcp_rx=%9d,    tcp_drop=%9d,    tcp_err=%9d\n",
        s.tcp_tx,
        s.tcp_rx,
        s.tcp_drop,
        s.tcp_err);
    printf(
        "   nd6_tx=%9d,    nd6_rx=%9d,    nd6_drop=%9d,    nd6_err=%9d\n",
        s.nd6_tx,
        s.nd6_rx,
        s.nd6_drop,
        s.nd6_err);
    return 0;
}

int test_utils()
{
    DEBUG_INFO("\n\n***\ttest_utils");

    // Test zts_util_get_ip_family
    // TODO: Consider ports erroneously embedded in string

    assert(zts_util_get_ip_family("0") == ZTS_AF_INET);
    assert(zts_util_get_ip_family("0.0") == ZTS_AF_INET);
    assert(zts_util_get_ip_family("0.0.0") == ZTS_AF_INET);

    assert(zts_util_get_ip_family("1") == ZTS_AF_INET);
    assert(zts_util_get_ip_family("255") == ZTS_AF_INET);
    // assert(zts_util_get_ip_family("256") != ZTS_AF_INET);
    assert(zts_util_get_ip_family("-1") != ZTS_AF_INET);

    assert(zts_util_get_ip_family("0.0.0.0") == ZTS_AF_INET);
    assert(zts_util_get_ip_family("0.0.0.1") == ZTS_AF_INET);
    assert(zts_util_get_ip_family("0.0.1.0") == ZTS_AF_INET);
    assert(zts_util_get_ip_family("0.1.0.0") == ZTS_AF_INET);
    assert(zts_util_get_ip_family("1.0.0.0") == ZTS_AF_INET);
    assert(zts_util_get_ip_family("1.2.3.4") == ZTS_AF_INET);
    assert(zts_util_get_ip_family("255.255.255.255") == ZTS_AF_INET);
    assert(zts_util_get_ip_family("a.b.c.d") != ZTS_AF_INET);
    assert(zts_util_get_ip_family("256.256.256.256") != ZTS_AF_INET);
    assert(zts_util_get_ip_family("0.-1.0.0") != ZTS_AF_INET);
    assert(zts_util_get_ip_family(".0.0.0.0") != ZTS_AF_INET);
    assert(zts_util_get_ip_family("0..0.0.0") != ZTS_AF_INET);
    assert(zts_util_get_ip_family("0.0.0.0..") != ZTS_AF_INET);
    assert(zts_util_get_ip_family(".") != ZTS_AF_INET);
    assert(zts_util_get_ip_family("..") != ZTS_AF_INET);
    assert(zts_util_get_ip_family("...") != ZTS_AF_INET);
    assert(zts_util_get_ip_family("....") != ZTS_AF_INET);
    assert(zts_util_get_ip_family(".....") != ZTS_AF_INET);
    assert(zts_util_get_ip_family("") != ZTS_AF_INET);

    assert(zts_util_get_ip_family("::") == ZTS_AF_INET6);

    return 0;
}

//----------------------------------------------------------------------------//
// Main                                                                       //
//----------------------------------------------------------------------------//

int main(int argc, char** argv)
{
    if (argc != 1 && argc != 5 && argc != 7) {
        DEBUG_INFO("Invalid number of arguments.");
        exit(-1);
    }

    // Default selftest
    if (argc == 1) {
        srand(time(NULL));
        DEBUG_INFO("Single node test");
        test_utils();
        test_pre_service_fuzz();
        test_thread_safety();
        test_identity_key_handling();
        test_addr_computation();
        test_roots_handling();
        test_start_sequences();
        test_api_abuse();
        test_stats();
        // test_sockets();
    }

    // Server test
    if (argc == 5) {
        DEBUG_INFO("Server test");
        uint64_t net_id = strtoull(argv[2], NULL, 16);
        int port4 = atoi(argv[3]);
        int port6 = atoi(argv[4]);
        test_start_node(argv[1], net_id, NULL, 1, 1, 0, 1, 0);
        test_server_socket_usage(port4, port6);
    }
    // Client test
    if (argc == 7) {
        DEBUG_INFO("Client test");
        uint64_t net_id = strtoull(argv[2], NULL, 16);
        int port4 = atoi(argv[3]);
        int port6 = atoi(argv[5]);
        test_start_node(argv[1], net_id, NULL, 1, 1, 0, 1, 0);
        test_client_socket_usage(argv[4], port4, argv[6], port6);
    }
    DEBUG_INFO("SUCCESS");
    return 0;
}
