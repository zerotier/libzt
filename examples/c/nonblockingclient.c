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

    // Sockets

    char* msgStr = (char*)"Welcome to the machine";
    int bytes = 0, fd;
    char recvBuf[128] = { 0 };
    memset(recvBuf, 0, sizeof(recvBuf));

    // Create socket

    if ((fd = zts_socket(ZTS_AF_INET, ZTS_SOCK_STREAM, 0)) < 0) {
        printf("Error (fd=%d, zts_errno=%d). Exiting.\n", fd, zts_errno);
        exit(1);
    }

    // Connect

    // Can also use:
    // zts_bsd_connect(int fd, const struct zts_sockaddr* addr, zts_socklen_t addrlen);
    while (zts_connect(fd, remote_addr, remote_port, 0) != ZTS_ERR_OK) {
        printf("Attempting to connect...\n");
    }

    // Data I/O

    // Wait random intervals to send a message to the server
    // The non-blocking aspect of this example is server-side
    while (1) {
        if ((bytes = zts_send(fd, msgStr, strlen(msgStr), 0)) < 0) {
            printf("Error (fd=%d, ret=%d, zts_errno=%d). Exiting.\n", fd, bytes, zts_errno);
            exit(1);
        }
        printf("zts_send()=%d\n", bytes);
        zts_util_delay((rand() % 100) * 50);
    }

    zts_close(fd);
    return zts_node_stop();
}
