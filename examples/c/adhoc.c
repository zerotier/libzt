/**
 * libzt C API example — Ad-hoc network node
 *
 * Starts a ZeroTier node, joins a controller-less ad-hoc network, prints its
 * 6PLANE IPv6 address, and then idles so that another member of the same ad-hoc
 * network can ping it. The network ID is computed locally from the given port
 * range, so no network controller and no my.zerotier.com account is involved.
 *
 * Usage:
 *   adhoc <adhocStartPort> <adhocEndPort>
 *
 *   <adhocStartPort>   First port of the range encoded into the network ID
 *   <adhocEndPort>     Last port of the range encoded into the network ID
 *
 * Both ports must be in the range 1-65535 and the start port must not exceed the
 * end port.
 *
 * Ad-hoc networks are PUBLIC: anyone who computes the same network ID can join,
 * and no authorization is needed or possible. Addressing is 6PLANE IPv6 only —
 * there is no IPv4, no multicast and no broadcast, and reachable destination
 * ports are restricted to the encoded range. Take care not to expose vulnerable
 * services on one.
 *
 * This example does not persist its identity, so it generates a new node ID on
 * every run and its address changes each time.
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

/*

Ad-hoc Network:

ffSSSSEEEE000000
| |   |   |
| |   |   Reserved for future use, must be 0
| |   End of port range (hex)
| Start of port range (hex)
Reserved ZeroTier address prefix indicating a controller-less network.

Ad-hoc networks are public (no access control) networks that have no network controller. Instead
their configuration and other credentials are generated locally. Ad-hoc networks permit only IPv6
UDP and TCP unicast traffic (no multicast or broadcast) using 6plane format NDP-emulated IPv6
addresses. In addition an ad-hoc network ID encodes an IP port range. UDP packets and TCP SYN
(connection open) packets are only allowed to destination ports within the encoded range.

For example ff00160016000000 is an ad-hoc network allowing only SSH, while ff0000ffff000000 is an
ad-hoc network allowing any UDP or TCP port.

Keep in mind that these networks are public and anyone in the entire world can join them. Care must
be taken to avoid exposing vulnerable services or sharing unwanted files or other resources.

*/

static volatile sig_atomic_t keep_running = 1;
static void handle_sigint(int sig)
{
    (void)sig;
    keep_running = 0;
}

static void usage(const char* prog)
{
    printf("\nlibzt example: Ad-hoc network node\n\n");
    printf("Usage: %s <adhocStartPort> <adhocEndPort>\n\n", prog);
    printf("  <adhocStartPort>   First port of the range encoded into the network ID\n");
    printf("  <adhocEndPort>     Last port of the range encoded into the network ID\n");
    printf("\nBoth ports must be in 1-65535 and the start port must not exceed the end port.\n");
    printf("Ad-hoc networks are public and controller-less: no account or authorization is\n");
    printf("needed or possible. Addressing is 6PLANE IPv6 only (no IPv4, no multicast or\n");
    printf("broadcast) and only destination ports in the encoded range are reachable. The\n");
    printf("identity is not persisted, so the address changes on every run. Press Ctrl-C to\n");
    printf("shut down cleanly.\n");
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
    if (argc != 3) {
        usage(argv[0]);
        return 1;
    }
    int err = ZTS_ERR_OK;

    signal(SIGINT, handle_sigint);

    int start_port = atoi(argv[1]);
    int end_port = atoi(argv[2]);
    if (start_port < 1 || start_port > 65535 || end_port < 1 || end_port > 65535) {
        printf("Ports must be in the range 1-65535. Exiting.\n");
        exit(1);
    }
    if (start_port > end_port) {
        printf("Start port (%d) must not exceed end port (%d). Exiting.\n", start_port, end_port);
        exit(1);
    }
    uint16_t adhocStartPort = (uint16_t)start_port;   // Start of port range your application will use
    uint16_t adhocEndPort = (uint16_t)end_port;       // End of port range your application will use
    long long int net_id = zts_net_compute_adhoc_id(adhocStartPort, adhocEndPort);   // At least 64 bits

    // Start node and get identity

    printf("Starting node...\n");
    zts_node_start();
    printf("Waiting for node to come online\n");
    while (! zts_node_is_online()) {
        zts_util_delay(50);
    }
    uint64_t node_id = zts_node_get_id();
    printf("My public identity (node ID) is %010llx\n", (unsigned long long)node_id);
    char keypair[ZTS_ID_STR_BUF_LEN] = { 0 };
    unsigned int len = ZTS_ID_STR_BUF_LEN;
    if (zts_node_get_id_pair(keypair, &len) != ZTS_ERR_OK) {
        printf("Error getting identity keypair. Exiting.\n");
    }
    printf("Identity [public/secret pair] = %s\n", keypair);

    // Join the adhoc network

    printf("Joining network %llx\n", (unsigned long long)net_id);
    if (zts_net_join(net_id) != ZTS_ERR_OK) {
        printf("Unable to join network. Exiting.\n");
        exit(1);
    }
    printf("Waiting for join to complete\n");
    while (! zts_net_transport_is_ready(net_id)) {
        zts_util_delay(50);
    }

    // Get address

    char ipstr[ZTS_IP_MAX_STR_LEN] = { 0 };
    if ((err = zts_addr_compute_6plane_str(net_id, node_id, ipstr, ZTS_IP_MAX_STR_LEN)) != ZTS_ERR_OK) {
        printf("Unable to compute address (error = %d). Exiting.\n", err);
        exit(1);
    }
    printf("Join %llx from another machine and ping6 me at %s\n", (unsigned long long)net_id, ipstr);

    // Do network stuff!
    // zts_bsd_socket, zts_bsd_connect, etc

    while (keep_running) {
        zts_util_delay(500);   // Idle until Ctrl-C
    }

    printf("Stopping node\n");
    return zts_node_stop();
}
