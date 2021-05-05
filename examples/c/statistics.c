/**
 * libzt C API example
 *
 * Pingable node that also displays protocol statistics that are
 * useful for debugging.
 */

#include "ZeroTierSockets.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
    if (argc != 2) {
        printf("\nUsage:\n");
        printf("pingable-node <net_id>\n");
        exit(0);
    }
    long long int net_id = strtoull(argv[1], NULL, 16);   // At least 64 bits

    printf("Starting node...\n");
    zts_node_start();

    printf("Waiting for node to come online\n");
    while (! zts_node_is_online()) {
        zts_util_delay(50);
    }

    printf("My public identity (node ID) is %llx\n", zts_node_get_id());
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
    while (! (err = zts_addr_is_assigned(net_id, ZTS_AF_INET))) {
        zts_util_delay(500);
    }

    char ipstr[ZTS_IP_MAX_STR_LEN] = { 0 };
    zts_addr_get_str(net_id, ZTS_AF_INET, ipstr, ZTS_IP_MAX_STR_LEN);
    printf("Join %llx from another machine and ping me at %s\n", net_id, ipstr);

    // Do network stuff!
    // zts_bsd_socket, zts_bsd_connect, etc

    // Show protocol statistics

    zts_stats_counter_t s = { 0 };

    while (1) {
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
