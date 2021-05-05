/**
 * libzt C API example
 *
 * Pingable node joined to controller-less adhoc network with a 6PLANE addressing scheme
 */

#include "ZeroTierSockets.h"

#include <stdio.h>
#include <stdlib.h>

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
int main(int argc, char** argv)
{
    if (argc != 3) {
        printf("\nUsage:\n");
        printf("adhoc <adhocStartPort> <adhocEndPort>\n");
        exit(0);
    }
    int err = ZTS_ERR_OK;

    uint16_t adhocStartPort = atoi(argv[1]);   // Start of port range your application will use
    uint16_t adhocEndPort = atoi(argv[2]);     // End of port range your application will use
    long long int net_id = zts_net_compute_adhoc_id(adhocStartPort, adhocEndPort);   // At least 64 bits

    // Start node and get identity

    printf("Starting node...\n");
    zts_node_start();
    printf("Waiting for node to come online\n");
    while (! zts_node_is_online()) {
        zts_util_delay(50);
    }
    uint64_t node_id = zts_node_get_id();
    printf("My public identity (node ID) is %llx\n", node_id);
    char keypair[ZTS_ID_STR_BUF_LEN] = { 0 };
    unsigned int len = ZTS_ID_STR_BUF_LEN;
    if (zts_node_get_id_pair(keypair, &len) != ZTS_ERR_OK) {
        printf("Error getting identity keypair. Exiting.\n");
    }
    printf("Identity [public/secret pair] = %s\n", keypair);

    // Join the adhoc network

    printf("Joining network %llx\n", net_id);
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
    if ((err = zts_addr_compute_rfc4193_str(net_id, node_id, ipstr, ZTS_IP_MAX_STR_LEN)) != ZTS_ERR_OK) {
        printf("Unable to compute address (error = %d). Exiting.\n", err);
        exit(1);
    }
    printf("Join %llx from another machine and ping6 me at %s\n", net_id, ipstr);

    // Do network stuff!
    // zts_bsd_socket, zts_bsd_connect, etc

    while (1) {
        zts_util_delay(500);   // Idle indefinitely
    }

    printf("Stopping node\n");
    return zts_node_stop();
}
