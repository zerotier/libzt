/**
 * libzt C API example — Pingable node
 *
 * Joins a ZeroTier network and then idles, so that you can ping this node from
 * another device that is a member of the same network. This is the smallest
 * possible libzt program: start the node, join a network, wait for an address
 * assignment, then sit still.
 *
 * Usage:
 *   pingable-node <net_id>
 *
 *   <net_id>   ID of the ZeroTier network to join
 *
 * net_id is the 16-digit hexadecimal ID of an existing ZeroTier network. The
 * device must be authorized for that network at my.zerotier.com (or via the web
 * API) before it is assigned an address. This example does not persist its
 * identity, so it generates a new node ID on every run and must be
 * re-authorized each time.
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
    printf("\nlibzt example: Pingable node\n\n");
    printf("Usage: %s <net_id>\n\n", prog);
    printf("  <net_id>   16-digit hexadecimal ID of an existing ZeroTier network\n");
    printf("\nThe device must be authorized for the network at my.zerotier.com before it is\n");
    printf("assigned an address. This example does not persist its identity, so it generates a\n");
    printf("new node ID on every run and must be re-authorized each time. Press Ctrl-C to shut\n");
    printf("down cleanly.\n");
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

    printf("My public identity (node ID) is %llx\n", (long long int)zts_node_get_id());
    char keypair[ZTS_ID_STR_BUF_LEN] = { 0 };
    unsigned int len = ZTS_ID_STR_BUF_LEN;
    if (zts_node_get_id_pair(keypair, &len) != ZTS_ERR_OK) {
        printf("Error getting identity keypair. Exiting.\n");
    }
    printf("Identity [public/secret pair] = %s\n", keypair);

    printf("Joining network %llx\n", net_id);
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
    printf("Join %llx from another machine and ping me at %s\n", net_id, ipstr);

    // Do network stuff!
    // zts_bsd_socket, zts_bsd_connect, etc

    while (keep_running) {
        zts_util_delay(500);   // Idle until Ctrl-C
    }

    printf("Stopping node\n");
    return zts_node_stop();
}
