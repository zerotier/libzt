/**
 * libzt C API example — Non-blocking I/O client
 *
 * Starts a ZeroTier node, joins a network, connects to a peer running the
 * 'nonblockingserver' example, and then sends the same short message over and
 * over at random intervals so the server has traffic to react to.
 *
 * Despite the name, this client itself is BLOCKING. The non-blocking behaviour
 * being demonstrated is entirely server-side; see nonblockingserver.c. This
 * program exists only to feed that server sporadic traffic.
 *
 * Usage:
 *   nonblockingclient <id_storage_path> <net_id> <remote_addr> <remote_port>
 *
 *   <id_storage_path>   Directory used to store this node's identity and cache
 *   <net_id>            16-digit hexadecimal ZeroTier network ID
 *   <remote_addr>       IPv4 address of the peer running 'nonblockingserver'
 *   <remote_port>       TCP port that peer is listening on
 *
 * The socket is created with ZTS_AF_INET only, so an IPv6 remote_addr will not
 * work.
 *
 * Each concurrently-running node MUST have its own id_storage_path. The node's
 * identity keypair is read from and written to that directory; two nodes sharing
 * one directory present the same node ID to the network and will collide.
 *
 * net_id is the 16-digit hexadecimal ID of an existing ZeroTier network. The
 * device must be authorized for that network at my.zerotier.com (or via the web
 * API) before it is assigned an address.
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

static void usage(const char* prog)
{
    printf("\nlibzt example: Non-blocking I/O client\n\n");
    printf("Usage: %s <id_storage_path> <net_id> <remote_addr> <remote_port>\n\n", prog);
    printf("  <id_storage_path>   Directory used to store this node's identity and cache\n");
    printf("  <net_id>            16-digit hexadecimal ZeroTier network ID\n");
    printf("  <remote_addr>       IPv4 address of the peer running 'nonblockingserver'\n");
    printf("  <remote_port>       TCP port that peer is listening on\n");
    printf("\nThis client is blocking; the non-blocking behaviour is server-side. IPv4 only.\n");
    printf("Each concurrently-running node needs its own id_storage_path, or the nodes\n");
    printf("share an identity and collide. Authorize this device for net_id at\n");
    printf("my.zerotier.com (or via the web API) first. Press Ctrl-C to shut down cleanly.\n");
}

static volatile sig_atomic_t keep_running = 1;
static void handle_sigint(int sig)
{
    (void)sig;
    keep_running = 0;
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
    if (argc != 5) {
        usage(argv[0]);
        return 1;
    }
    char* storage_path = argv[1];
    long long int net_id = strtoull(argv[2], NULL, 16);   // At least 64 bits
    char* remote_addr = argv[3];
    int remote_port = atoi(argv[4]);
    int err = ZTS_ERR_OK;

    signal(SIGINT, handle_sigint);

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
    while (keep_running) {
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
