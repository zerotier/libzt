/**
 * libzt C API example — Custom roots (planet definition)
 *
 * Generates and signs a custom root set ("planet") definition, then starts a
 * node that uses it instead of ZeroTier's default roots. Use this as a starting
 * point when you want your nodes to bootstrap off your own root servers, or off
 * a chosen subset of ZeroTier's.
 *
 * Usage:
 *   customroots
 *
 *   (this example takes no arguments)
 *
 * The root set hardcoded below is not a US-only set: it contains a single
 * ZeroTier root in Amsterdam (195.181.173.159 and 2a02:6ea0:c024::). Replace
 * roots.public_id_str[] and roots.endpoint_ip_str[][] with your own roots to
 * make this useful.
 *
 * This example calls zts_init_from_storage("."), so it reads and writes
 * identity.public, identity.secret, roots, networks.d/ and peers.d/ in the
 * CURRENT WORKING DIRECTORY. Two instances started from the same directory read
 * the same identity keypair, present the same node ID to the network and will
 * collide — run each instance from its own directory.
 *
 * Press Ctrl-C to shut down cleanly; zts_node_stop() runs before exit.
 *
 * Build from the repository root:
 *   ./build.sh host "release"
 * Binaries are written to dist/<platform>-<arch>-host-release/bin/
 */

#include "ZeroTierSockets.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <arpa/inet.h>
#endif

static volatile sig_atomic_t keep_running = 1;
static void handle_sigint(int sig)
{
    (void)sig;
    keep_running = 0;
}

void print_peer_details(const char* msg, zts_peer_info_t* d)
{
    printf(" %s\n", msg);
    printf("\t- peer       : %010llx\n", (unsigned long long)d->peer_id);
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

static void usage(const char* prog)
{
    printf("\nlibzt example: Custom roots (planet definition)\n\n");
    printf("Usage: %s\n\n", prog);
    printf("  (this example takes no arguments)\n");
    printf("\nThe node's identity and state are read from and written to the CURRENT WORKING\n");
    printf("DIRECTORY, so run each instance from its own directory or they will collide. The\n");
    printf("roots hardcoded in this example point at a single ZeroTier root in Amsterdam.\n");
    printf("Press Ctrl-C to shut down cleanly.\n");
}

int main(int argc, char** argv)
{
    // Line-buffer stdout so progress output appears immediately when it is
    // redirected to a file or pipe (docker logs, systemd, CI) rather than
    // sitting in the 4K stdio buffer.
    setvbuf(stdout, NULL, _IOLBF, 0);
    if (argc > 1) {
        int help = (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0);
        usage(argv[0]);
        return help ? 0 : 1;
    }
    // World generation

    // Buffers that will be filled after generating the roots
    char roots_data_out[4096] = { 0 };   // (binary) Your new custom roots definition
    char prev_key[4096] = { 0 };         // (binary) (optional) For updating roots
    char curr_key[4096] = { 0 };         // (binary) You should save this

    // The length parameters are in/out: on input they must carry the capacity
    // of their buffer, on success they are overwritten with the number of
    // bytes actually written.
    unsigned int roots_len = sizeof(roots_data_out);
    unsigned int prev_key_len = sizeof(prev_key);
    unsigned int curr_key_len = sizeof(curr_key);

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

    int err;
    if ((err = zts_util_sign_root_set(
             roots_data_out,
             &roots_len,
             prev_key,
             &prev_key_len,
             curr_key,
             &curr_key_len,
             id,
             ts,
             &roots))
        != ZTS_ERR_OK) {
        printf("Unable to generate root set, error = %d. Exiting.\n", err);
        exit(1);
    }

    printf("roots_data_out= ");
    for (unsigned int i = 0; i < roots_len; i++) {
        if (i > 0) {
            printf(",");
        }
        printf("0x%.2x", (unsigned char)roots_data_out[i]);
    }
    printf("\n");
    printf("roots_len    = %u\n", roots_len);
    printf("prev_key_len = %u\n", prev_key_len);
    printf("curr_key_len = %u\n", curr_key_len);

    // Now, initialize node and use newly-generated roots definition

    zts_init_set_roots(roots_data_out, roots_len);
    zts_init_set_event_handler(&on_zts_event);
    zts_init_from_storage(".");

    //  Start node

    signal(SIGINT, handle_sigint);
    zts_node_start();

    while (keep_running) {
        zts_util_delay(500);
    }

    return zts_node_stop();
}
