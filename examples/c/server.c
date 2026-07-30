/**
 * libzt C API example — TCP echo server
 *
 * Starts a ZeroTier node, joins a network, then binds and listens on that
 * network. Accepts exactly ONE incoming connection, reads a message, echoes it
 * straight back, then closes the socket and exits. Pair it with the 'client'
 * example for the smallest end-to-end demonstration of the libzt socket API.
 *
 * Usage:
 *   server <id_storage_path> <net_id> <local_addr> <local_port>
 *
 *   <id_storage_path>   Directory used to store this node's identity and cache
 *   <net_id>            16-digit hexadecimal ZeroTier network ID
 *   <local_addr>        Address to bind, e.g. 0.0.0.0 (or :: for IPv6)
 *   <local_port>        TCP port to listen on
 *
 * This server is single-shot: it serves one connection and exits, so restart it
 * for each client.
 *
 * Each concurrently-running node MUST have its own id_storage_path. The node's
 * identity keypair is read from and written to that directory; two nodes sharing
 * one directory present the same node ID to the network and will collide.
 *
 * net_id is the 16-digit hexadecimal ID of an existing ZeroTier network. The
 * device must be authorized for that network at my.zerotier.com (or via the web
 * API) before it is assigned an address.
 *
 * Build from the repository root:
 *   ./build.sh host "release"
 * Binaries are written to dist/<platform>-<arch>-host-release/bin/
 */

#include "ZeroTierSockets.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char* prog)
{
    printf("\nlibzt example: TCP echo server\n\n");
    printf("Usage: %s <id_storage_path> <net_id> <local_addr> <local_port>\n\n", prog);
    printf("  <id_storage_path>   Directory used to store this node's identity and cache\n");
    printf("  <net_id>            16-digit hexadecimal ZeroTier network ID\n");
    printf("  <local_addr>        Address to bind, e.g. 0.0.0.0 (or :: for IPv6)\n");
    printf("  <local_port>        TCP port to listen on\n");
    printf("\nSingle-shot: serves one connection and exits, so restart it for each client.\n");
    printf("Each concurrently-running node needs its own id_storage_path, or the nodes\n");
    printf("share an identity and collide. Authorize this device for net_id at\n");
    printf("my.zerotier.com (or via the web API) before it is assigned an address.\n");
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
    uint64_t net_id = strtoull(argv[2], NULL, 16);   // At least 64 bits
    char* local_addr = argv[3];
    unsigned short local_port = (unsigned short)atoi(argv[4]);
    int accfd;
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
    printf("Public identity (node ID) is %010llx\n", (unsigned long long)zts_node_get_id());

    // Join network

    printf("Joining network %llx\n", (unsigned long long)net_id);
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
    if (family < 0) {
        printf("Invalid local address '%s'. Exiting.\n", local_addr);
        exit(1);
    }

    printf("Waiting for address assignment from network\n");
    // zts_addr_is_assigned() returns 1 when assigned, 0 when not, and a negative
    // error code on failure. Treating anything non-zero as success would exit the
    // loop on error and then print an empty address.
    while ((err = zts_addr_is_assigned(net_id, family)) != 1) {
        if (err < 0) {
            printf("Error checking address assignment, error = %d. Exiting.\n", err);
            exit(1);
        }
        zts_util_delay(50);
    }
    char ipstr[ZTS_IP_MAX_STR_LEN] = { 0 };
    if ((err = zts_addr_get_str(net_id, family, ipstr, ZTS_IP_MAX_STR_LEN)) != ZTS_ERR_OK) {
        printf("Unable to read assigned address, error = %d. Exiting.\n", err);
        exit(1);
    }
    printf("IP address on network %llx is %s\n", (unsigned long long)net_id, ipstr);

    // BEGIN Socket Stuff

    // Accept incoming connection

    // Can also use traditional: zts_bsd_socket(), zts_bsd_bind(), zts_bsd_listen(), zts_bsd_accept(), etc.

    char remote_addr[ZTS_INET6_ADDRSTRLEN] = { 0 };
    unsigned short remote_port = 0;
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
        printf("Error (fd=%d, ret=%d, zts_errno=%d). Exiting.\n", accfd, bytes, zts_errno);
        exit(1);
    }
    printf("Read %d bytes: %s\n", bytes, recvBuf);
    printf("Sending message string to client...\n");
    if ((bytes = zts_write(accfd, recvBuf, bytes)) < 0) {
        printf("Error (fd=%d, ret=%d, zts_errno=%d). Exiting.\n", accfd, bytes, zts_errno);
        exit(1);
    }
    printf("Sent %d bytes: %s\n", bytes, recvBuf);

    // Close

    printf("Closing sockets\n");
    if ((err = zts_close(accfd)) != ZTS_ERR_OK) {
        printf("Error closing socket, error = %d.\n", err);
    }
    return zts_node_stop();
}
