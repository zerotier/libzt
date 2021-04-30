/**
 * libzt C API example
 *
 * An example demonstrating how to define your own planet. In this example
 * we limit the roots to US-only.
 */

#include "ZeroTierSockets.h"

#include <stdio.h>
#include <stdlib.h>

void print_peer_details(const char* msg, zts_peer_info_t* d)
{
    printf(" %s\n", msg);
    printf("\t- peer       : %llx\n", d->peer_id);
    printf("\t- role       : %d\n", d->role);
    printf("\t- latency    : %d\n", d->latency);
    printf("\t- version    : %d.%d.%d\n", d->ver_major, d->ver_minor, d->ver_rev);
    printf("\t- path_count : %d\n", d->path_count);
    printf("\t- paths:\n");

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
        printf("\t  - %15s : %6d\n", ipstr, port);
    }
    printf("\n\n");
}

void on_zts_event(void* msgPtr)
{
    zts_event_msg_t* msg = (zts_event_msg_t*)msgPtr;
    printf("event_code = %d\n", msg->event_code);

    if (msg->peer) {
        if (msg->peer->role != ZTS_PEER_ROLE_PLANET) {
            return;   // Don't print controllers and ordinary nodes.
        }
    }
    if (msg->event_code == ZTS_EVENT_PEER_DIRECT) {
        print_peer_details("ZTS_EVENT_PEER_DIRECT", msg->peer);
    }
    if (msg->event_code == ZTS_EVENT_PEER_RELAY) {
        print_peer_details("ZTS_EVENT_PEER_RELAY", msg->peer);
    }
}

int main()
{
    // World generation

    // Buffers that will be filled after generating the roots
    char roots_data_out[4096] = { 0 };   // (binary) Your new custom roots definition
    unsigned int roots_len = 0;
    unsigned int prev_key_len = 0;
    unsigned int curr_key_len = 0;
    char prev_key[4096] = { 0 };   // (binary) (optional) For updating roots
    char curr_key[4096] = { 0 };   // (binary) You should save this

    // Arbitrary World ID
    uint64_t id = 149604618;

    // Timestamp indicating when this signed root blob was generated
    uint64_t ts = 1567191349589ULL;

    // struct containing public keys and stable IP endpoints for roots
    zts_root_set_t roots = { 0 };

    roots.public_id_str[0] =
        "992fcf1db7:0:"
        "206ed59350b31916f749a1f85dffb3a8787dcbf83b8c6e9448d4e3ea0e3369301be716c3609344a9d1533850fb4460c5"
        "0af43322bcfc8e13d3301a1f1003ceb6";
    roots.endpoint_ip_str[0][0] = "195.181.173.159/9993";
    roots.endpoint_ip_str[0][1] = "2a02:6ea0:c024::/9993";

    // Generate roots

    zts_util_sign_root_set(
        roots_data_out,
        &roots_len,
        prev_key,
        &prev_key_len,
        curr_key,
        &curr_key_len,
        id,
        ts,
        &roots);

    printf("roots_data_out= ");
    for (int i = 0; i < roots_len; i++) {
        if (i > 0) {
            printf(",");
        }
        printf("0x%.2x", (unsigned char)roots_data_out[i]);
    }
    printf("\n");
    printf("roots_len    = %d\n", roots_len);
    printf("prev_key_len = %d\n", prev_key_len);
    printf("curr_key_len = %d\n", curr_key_len);

    // Now, initialize node and use newly-generated roots definition

    zts_init_set_roots(&roots_data_out, roots_len);
    zts_init_set_event_handler(&on_zts_event);
    zts_init_from_storage(".");

    //  Start node

    zts_node_start();

    while (1) {
        zts_util_delay(500);
    }

    return zts_node_stop();
}
