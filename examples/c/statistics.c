/**
 * libzt C API example — Protocol statistics
 *
 * A pingable node that prints the lwIP protocol statistics (link, IP, ICMP,
 * UDP, TCP and others) once per second. Useful for debugging traffic that is
 * not arriving or is being dropped somewhere in the stack.
 *
 * Usage:
 *   statistics <net_id>
 *
 *   <net_id>   ID of the ZeroTier network to join
 *
 * net_id is the 16-digit hexadecimal ID of an existing ZeroTier network. The
 * device must be authorized for that network at my.zerotier.com (or via the web
 * API) before it is assigned an address. This example does not persist its
 * identity, so it generates a new node ID on every run and must be
 * re-authorized each time.
 *
 * This example requires the library to be built with ZTS_ENABLE_STATS, which
 * the BUILD_HOST configuration used by ./build.sh host sets automatically.
 * Linking against a stock release build without it makes every statistics query
 * return ZTS_ERR_NO_RESULT.
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

static volatile sig_atomic_t keep_running = 1;
static void handle_sigint(int sig)
{
    (void)sig;
    keep_running = 0;
}

static void usage(const char* prog)
{
    printf("\nlibzt example: Protocol statistics\n\n");
    printf("Usage: %s <net_id>\n\n", prog);
    printf("  <net_id>   16-digit hexadecimal ID of an existing ZeroTier network\n");
    printf("\nThe device must be authorized for the network at my.zerotier.com before it is\n");
    printf("assigned an address. This example does not persist its identity, so it generates a\n");
    printf("new node ID on every run and must be re-authorized each time. It also requires a\n");
    printf("library built with ZTS_ENABLE_STATS (./build.sh host sets this automatically);\n");
    printf("without it every query returns ZTS_ERR_NO_RESULT. Press Ctrl-C to shut down\n");
    printf("cleanly.\n");
}

int main(int argc, char** argv)
{
    // Line-buffer stdout so progress output appears immediately when it is
    // redirected to a file or pipe (docker logs, systemd, CI) rather than
    // sitting in the 4K stdio buffer.
    setvbuf(stdout, NULL, _IOLBF, 0);
    if (argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        usage(argv[0]);
        return 0;
    }
    if (argc != 2) {
        usage(argv[0]);
        return 1;
    }
    long long int net_id = strtoull(argv[1], NULL, 16);   // At least 64 bits

    signal(SIGINT, handle_sigint);

    printf("Starting node...\n");
    zts_node_start();

    printf("Waiting for node to come online\n");
    while (! zts_node_is_online()) {
        zts_util_delay(50);
    }

    printf("My public identity (node ID) is %010llx\n", (unsigned long long)zts_node_get_id());
    char keypair[ZTS_ID_STR_BUF_LEN] = { 0 };
    unsigned int len = ZTS_ID_STR_BUF_LEN;
    if (zts_node_get_id_pair(keypair, &len) != ZTS_ERR_OK) {
        printf("Error getting identity keypair. Exiting.\n");
    }
    printf("Identity [public/secret pair] = %s\n", keypair);

    printf("Joining network %llx\n", (unsigned long long)net_id);
    if (zts_net_join(net_id) != ZTS_ERR_OK) {
        printf("Unable to join network. Exiting.\n");
        exit(1);
    }

    printf("Waiting for join to complete\n");
    while (! zts_net_transport_is_ready(net_id)) {
        zts_util_delay(50);
    }

    printf("Waiting for address assignment from network\n");
    int err = 0;
    // zts_addr_is_assigned() returns 1 when assigned, 0 when not, and a negative
    // error code on failure. Treating anything non-zero as success would exit the
    // loop on error and then print an empty address.
    while ((err = zts_addr_is_assigned(net_id, ZTS_AF_INET)) != 1) {
        if (err < 0) {
            printf("Error checking address assignment, error = %d. Exiting.\n", err);
            exit(1);
        }
        zts_util_delay(50);
    }

    char ipstr[ZTS_IP_MAX_STR_LEN] = { 0 };
    zts_addr_get_str(net_id, ZTS_AF_INET, ipstr, ZTS_IP_MAX_STR_LEN);
    printf("Join %llx from another machine and ping me at %s\n", (unsigned long long)net_id, ipstr);

    // Do network stuff!
    // zts_bsd_socket, zts_bsd_connect, etc

    // Show protocol statistics

    zts_stats_counter_t s = { 0 };

    while (keep_running) {
        zts_util_delay(1000);
        if ((err = zts_stats_get_all(&s)) == ZTS_ERR_NO_RESULT) {
            printf("no results\n");
            continue;
        }
        printf("\n\n");

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
    }
    return zts_node_stop();
}
