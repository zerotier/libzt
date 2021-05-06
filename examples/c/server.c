/**
 * libzt C API example
 *
 * Simple socket-based server application
 */

#include "ZeroTierSockets.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv)
{
    if (argc != 5) {
        printf("\nlibzt example server\n");
        printf("server <id_storage_path> <net_id> <local_addr> <local_port>\n");
        exit(0);
    }
    char* storage_path = argv[1];
    long long int net_id = strtoull(argv[2], NULL, 16);   // At least 64 bits
    char* local_addr = argv[3];
    unsigned int local_port = atoi(argv[4]);
    int fd, accfd;
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
    printf("Public identity (node ID) is %llx\n", zts_node_get_id());

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

    int family = zts_util_get_ip_family(local_addr);

    printf("Waiting for address assignment from network\n");
    while (! (err = zts_addr_is_assigned(net_id, family))) {
        zts_util_delay(50);
    }
    char ipstr[ZTS_IP_MAX_STR_LEN] = { 0 };
    zts_addr_get_str(net_id, family, ipstr, ZTS_IP_MAX_STR_LEN);
    printf("IP address on network %llx is %s\n", net_id, ipstr);

    // BEGIN Socket Stuff

    // Accept incoming connection

    // Can also use traditional: zts_bsd_socket(), zts_bsd_bind(), zts_bsd_listen(), zts_bsd_accept(), etc.

    char remote_addr[ZTS_INET6_ADDRSTRLEN] = { 0 };
    int remote_port = 0;
    int len = ZTS_INET6_ADDRSTRLEN;
    if ((accfd = zts_tcp_server(local_addr, local_port, remote_addr, len, &remote_port)) < 0) {
        printf("Error (fd=%d, zts_errno=%d). Exiting.\n", accfd, zts_errno);
        exit(1);
    }
    printf("Accepted connection from %s:%d\n", remote_addr, remote_port);

    // Data I/O

    int bytes = 0;
    char recvBuf[128] = { 0 };

    printf("Reading message string from client...\n");
    if ((bytes = zts_read(accfd, recvBuf, sizeof(recvBuf))) < 0) {
        printf("Error (fd=%d, ret=%d, zts_errno=%d). Exiting.\n", fd, bytes, zts_errno);
        exit(1);
    }
    printf("Read %d bytes: %s\n", bytes, recvBuf);
    printf("Sending message string to client...\n");
    if ((bytes = zts_write(accfd, recvBuf, bytes)) < 0) {
        printf("Error (fd=%d, ret=%d, zts_errno=%d). Exiting.\n", fd, bytes, zts_errno);
        exit(1);
    }
    printf("Sent %d bytes: %s\n", bytes, recvBuf);

    // Close

    printf("Closing sockets\n");
    err = zts_close(accfd);
    err = zts_close(fd);
    return zts_node_stop();
}
