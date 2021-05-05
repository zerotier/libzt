/**
 * libzt C API example
 *
 * Pingable node that demonstrates basic usage of the callback API. To see
 * more exhaustive examples look at test/selftest.c
 */

#include "ZeroTierSockets.h"

#include <stdio.h>
#include <stdlib.h>

void on_zts_event(void* msgPtr)
{
    zts_event_msg_t* msg = (zts_event_msg_t*)msgPtr;
    // Node events
    if (msg->event_code == ZTS_EVENT_NODE_ONLINE) {
        printf("ZTS_EVENT_NODE_ONLINE --- This node's ID is %llx\n", msg->node->node_id);
    }
    if (msg->event_code == ZTS_EVENT_NODE_OFFLINE) {
        printf("ZTS_EVENT_NODE_OFFLINE --- Check your physical Internet connection, router, "
               "firewall, etc. What ports are you blocking?\n");
    }
    // Virtual network events
    if (msg->event_code == ZTS_EVENT_NETWORK_NOT_FOUND) {
        printf("ZTS_EVENT_NETWORK_NOT_FOUND --- Are you sure %llx is a valid network?\n", msg->network->net_id);
    }
    if (msg->event_code == ZTS_EVENT_NETWORK_ACCESS_DENIED) {
        printf(
            "ZTS_EVENT_NETWORK_ACCESS_DENIED --- Access to virtual network %llx has been denied. "
            "Did you authorize the node yet?\n",
            msg->network->net_id);
    }
    if (msg->event_code == ZTS_EVENT_NETWORK_READY_IP6) {
        printf(
            "ZTS_EVENT_NETWORK_READY_IP6 --- Network config received. IPv6 traffic can now be sent "
            "over network %llx\n",
            msg->network->net_id);
    }
    // Address events
    if (msg->event_code == ZTS_EVENT_ADDR_ADDED_IP6) {
        char ipstr[ZTS_INET6_ADDRSTRLEN] = { 0 };
        struct zts_sockaddr_in6* in6 = (struct zts_sockaddr_in6*)&(msg->addr->addr);
        zts_inet_ntop(ZTS_AF_INET6, &(in6->sin6_addr), ipstr, ZTS_INET6_ADDRSTRLEN);
        printf("ZTS_EVENT_ADDR_NEW_IP6 --- Join %llx and ping me at %s\n", msg->addr->net_id, ipstr);
    }

    // To see more exhaustive examples look at test/selftest.c
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        printf("\nUsage:\n");
        printf("pingable-node <net_id>\n");
        exit(0);
    }
    long long int net_id = strtoull(argv[1], NULL, 16);   // At least 64 bits

    zts_init_set_event_handler(&on_zts_event);

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

    while (1) {
        zts_util_delay(500);   // Idle indefinitely
    }

    printf("Stopping node\n");
    return zts_node_stop();
}
