/**
 * libzt C API example
 *
 * Pingable node
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
    while (! (err = zts_addr_is_assigned(net_id, ZTS_AF_INET))) {
        zts_util_delay(500);
    }

    char ipstr[ZTS_IP_MAX_STR_LEN] = { 0 };
    zts_addr_get_str(net_id, ZTS_AF_INET, ipstr, ZTS_IP_MAX_STR_LEN);
    printf("Join %llx from another machine and ping me at %s\n", net_id, ipstr);

    // Do network stuff!
    // zts_bsd_socket, zts_bsd_connect, etc

    while (1) {
        zts_util_delay(500);   // Idle indefinitely
    }

    printf("Stopping node\n");
    return zts_node_stop();
}
