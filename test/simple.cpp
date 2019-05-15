#include <ZeroTier.h>

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

bool node_ready = false;
bool network_ready = false;

void myZeroTierEventCallback(struct zts_callback_msg *msg)
{
    switch (msg->eventCode)
    {
        case ZTS_EVENT_NODE_ONLINE:
            printf("ZTS_EVENT_NODE_ONLINE, node=%llx\n", msg->node->address);
            node_ready = true;
            break;
        case ZTS_EVENT_NODE_OFFLINE:
            printf("ZTS_EVENT_NODE_OFFLINE\n");
            node_ready = false;
            break;
        case ZTS_EVENT_NETWORK_READY_IP4:
            printf("ZTS_EVENT_NETWORK_READY_IP4 --- network=%llx\n", msg->network->nwid);
            network_ready = true;
            break;
        case ZTS_EVENT_PEER_P2P:
            printf("ZTS_EVENT_PEER_P2P --- node=%llx\n", msg->peer->address);
            break;
        case ZTS_EVENT_PEER_RELAY:
            printf("ZTS_EVENT_PEER_RELAY --- node=%llx\n", msg->peer->address);
            break;
        // ...
        default:
            break;
    }
}

int main() 
{
    char *str = "welcome to the machine";
    char *remoteIp = "11.7.7.223";
    int remotePort = 8082;
    int fd, err = 0;
    struct zts_sockaddr_in addr;	
    addr.sin_family = ZTS_AF_INET;
    addr.sin_addr.s_addr = inet_addr(remoteIp);
    addr.sin_port = htons(remotePort);

    // Set up ZeroTier service and wai for callbacks
    int port = 9994;
    uint64_t nwid = 0x0123456789abcdef;
    zts_start("zt_config/path", &myZeroTierEventCallback, port);
    printf("Waiting for node to come online...\n");
    while (!node_ready) { sleep(1); }
    zts_join(nwid);
    printf("Joined virtual network. Requesting configuration...\n");
    while (!network_ready) { sleep(1); }

    printf("I am %llx\n", zts_get_node_id());
    // Socket API example
    if ((fd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("error creating socket\n");
    }
    if ((err = zts_connect(fd, (const struct sockaddr *)&addr, sizeof(addr))) < 0) {
        printf("error connecting to remote host (%s)\n", remoteIp);
    }
    if ((err = zts_write(fd, str, strlen(str))) < 0) {
        printf("error writing to socket\n");
    }
    zts_close(fd);
    zts_stop();
    return 0;
}
