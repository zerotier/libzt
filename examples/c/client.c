/**
 * libzt C API example
 *
 * Simple socket-based client application
 */

#include "ZeroTierSockets.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv)
{
    if (argc != 5) {
        printf("\nlibzt example client\n");
        printf("client <id_storage_path> <net_id> <remote_addr> <remote_port>\n");
        exit(0);
    }
    char* storage_path = argv[1];
    long long int net_id = strtoull(argv[2], NULL, 16);   // At least 64 bits
    char* remote_addr = argv[3];
    int remote_port = atoi(argv[4]);
    int err = ZTS_ERR_OK;

    // Initialize node

    if ((err = zts_init_from_storage(storage_path)) != ZTS_ERR_OK) {
        printf("Unable to start service, error = %d. Exiting.\n", err);
        exit(1);
    }

    // Start node

    if ((err = zts_node_start()) != ZTS_ERR_OK) {
        printf("Unable to start service, error = %d. Exiting.\n", err);
        exit(1);
    }
    printf("Waiting for node to come online\n");
    while (! zts_node_is_online()) {
        zts_util_delay(50);
    }
    printf("Public identity (node ID) is %llx\n", (long long int)zts_node_get_id());

    // Join network

    printf("Joining network %llx\n", net_id);
    if (zts_net_join(net_id) != ZTS_ERR_OK) {
        printf("Unable to join network. Exiting.\n");
        exit(1);
    }
    printf("Don't forget to authorize this device in my.zerotier.com or the web API!\n");
    printf("Waiting for join to complete\n");
    while (! zts_net_transport_is_ready(net_id)) {
        zts_util_delay(50);
    }

    // Get assigned address (of the family type we care about)

    int family = zts_util_get_ip_family(remote_addr);

    printf("Waiting for address assignment from network\n");
    while (! (err = zts_addr_is_assigned(net_id, family))) {
        zts_util_delay(50);
    }
    char ipstr[ZTS_IP_MAX_STR_LEN] = { 0 };
    zts_addr_get_str(net_id, family, ipstr, ZTS_IP_MAX_STR_LEN);
    printf("IP address on network %llx is %s\n", net_id, ipstr);

    // BEGIN Socket Stuff

    char* msgStr = (char*)"Welcome to the machine";
    int bytes = 0, fd;
    char recvBuf[128] = { 0 };
    memset(recvBuf, 0, sizeof(recvBuf));

    // Connect to remote host

    // Can also use traditional: zts_bsd_socket(), zts_bsd_connect(), etc

    printf("Attempting to connect...\n");
    while ((fd = zts_tcp_client(remote_addr, remote_port)) < 0) {
        printf("Re-attempting to connect...\n");
    }

    // Data I/O

    printf("Sending message string to server...\n");
    if ((bytes = zts_write(fd, msgStr, strlen(msgStr))) < 0) {
        printf("Error (fd=%d, ret=%d, zts_errno=%d). Exiting.\n", fd, bytes, zts_errno);
        exit(1);
    }
    printf("Sent %d bytes: %s\n", bytes, msgStr);
    printf("Reading message string from server...\n");
    if ((bytes = zts_read(fd, recvBuf, sizeof(recvBuf))) < 0) {
        printf("Error (fd=%d, ret=%d, zts_errno=%d). Exiting.\n", fd, bytes, zts_errno);
        exit(1);
    }
    printf("Read %d bytes: %s\n", bytes, recvBuf);

    // Close

    printf("Closing sockets\n");
    zts_close(fd);
    return zts_node_stop();
}
