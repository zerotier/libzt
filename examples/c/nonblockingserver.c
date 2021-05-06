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
    unsigned short local_port = atoi(argv[4]);
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

    printf("Waiting for address assignment from network\n");
    while (! (err = zts_addr_is_assigned(net_id, ZTS_AF_INET))) {
        zts_util_delay(500);
    }

    char ipstr[ZTS_IP_MAX_STR_LEN] = { 0 };
    zts_addr_get_str(net_id, ZTS_AF_INET, ipstr, ZTS_IP_MAX_STR_LEN);
    printf("Assigned IP address: %s\n", ipstr);

    // Sockets

    printf("Creating socket...\n");
    if ((fd = zts_socket(ZTS_AF_INET, ZTS_SOCK_STREAM, 0)) < 0) {
        printf("Error (fd=%d, ret=%d, zts_errno=%d). Exiting.\n", fd, err, zts_errno);
        exit(1);
    }
    printf("Binding...\n");
    // Can also use:
    //   zts_bsd_bind(int fd, const struct zts_sockaddr* addr, zts_socklen_t addrlen)
    if ((err = zts_bind(fd, local_addr, local_port) < 0)) {
        printf("Error (fd=%d, ret=%d, zts_errno=%d). Exiting.\n", fd, err, zts_errno);
        exit(1);
    }
    printf("Listening...\n");
    int backlog = 100;
    if ((err = zts_listen(fd, backlog)) < 0) {
        printf("Error (fd=%d, ret=%d, zts_errno=%d). Exiting.\n", fd, err, zts_errno);
        exit(1);
    }

    int bytes = 0;
    char recvBuf[128] = { 0 };

    // Accept
    // Can also use
    //   zts_bsd_accept(int fd, struct zts_sockaddr* addr, zts_socklen_t* addrlen)

    char remote_ipstr[ZTS_INET6_ADDRSTRLEN] = { 0 };
    unsigned int port = 0;
    printf("Accepting on listening socket...\n");
    if ((accfd = zts_accept(fd, remote_ipstr, ZTS_INET6_ADDRSTRLEN, &port)) < 0) {
        printf("Error (fd=%d, ret=%d, zts_errno=%d). Exiting.\n", fd, err, zts_errno);
    }
    printf("Accepted connection from %s:%d\n", remote_ipstr, port);

    // Data I/O

    // Technique 1: ZTS_O_NONBLOCK
    if (0) {
        zts_bsd_fcntl(fd, ZTS_F_SETFL, ZTS_O_NONBLOCK);
        zts_bsd_fcntl(accfd, ZTS_F_SETFL, ZTS_O_NONBLOCK);
        while (1) {
            bytes = zts_bsd_recv(accfd, recvBuf, sizeof(recvBuf), 0);
            printf("zts_bsd_recv(%d, ...)=%d\n", accfd, bytes);
            zts_util_delay(100);
        }
    }

    // Technique 2: zts_bsd_select
    if (0) {
        struct zts_timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 50000;
        int result = 0;
        zts_fd_set active_fd_set, read_fd_set;
        ZTS_FD_ZERO(&active_fd_set);
        ZTS_FD_SET(accfd, &active_fd_set);
        while (1) {
            read_fd_set = active_fd_set;
            if ((result = zts_bsd_select(ZTS_FD_SETSIZE, &read_fd_set, NULL, NULL, &tv) < 0)) {
                // perror ("select");
                exit(1);
            }
            for (int i = 0; i < ZTS_FD_SETSIZE; i++) {
                if (ZTS_FD_ISSET(i, &read_fd_set)) {
                    bytes = zts_bsd_recv(accfd, recvBuf, sizeof(recvBuf), 0);
                    printf("zts_bsd_recv(%d, ...)=%d\n", i, bytes);
                }
                // ZTS_FD_CLR(i, &active_fd_set);
            }
        }
    }

    // Technique 3: zts_bsd_poll
    if (1) {
        int numfds = 0;
        struct zts_pollfd poll_set[16];
        memset(poll_set, '\0', sizeof(poll_set));
        poll_set[0].fd = accfd;
        poll_set[0].events = ZTS_POLLIN;
        numfds++;
        int result = 0;
        int timeout_ms = 50;
        while (1) {
            result = zts_bsd_poll(poll_set, numfds, timeout_ms);
            printf("zts_bsd_poll()=%d\n", result);
            for (int i = 0; i < numfds; i++) {
                if (poll_set[i].revents & ZTS_POLLIN) {
                    bytes = zts_bsd_recv(poll_set[i].fd, recvBuf, sizeof(recvBuf), 0);
                    printf("zts_bsd_recv(%d, ...)=%d\n", i, bytes);
                }
            }
        }
    }

    err = zts_bsd_close(fd);
    return zts_node_stop();
}
