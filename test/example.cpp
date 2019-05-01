#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ZeroTier.h"

bool node_ready = false;
bool network_ready = false;

// Example callbacks
void myZeroTierEventCallback(struct zts_callback_msg *msg)
{
	//
	// Node events
	//
	if (msg->eventCode == ZTS_EVENT_NODE_ONLINE) {
		printf("ZTS_EVENT_NODE_ONLINE, node=%llx\n", msg->node->address);
		node_ready = true;
		// ZeroTier service is running and online
	}
	if (msg->eventCode == ZTS_EVENT_NODE_OFFLINE) {
		printf("ZTS_EVENT_NODE_OFFLINE\n");
		node_ready = false;
		// ZeroTier service is running and online
	}
	if (msg->eventCode == ZTS_EVENT_NODE_NORMAL_TERMINATION) {
		printf("ZTS_EVENT_NODE_NORMAL_TERMINATION\n");
		// ZeroTier service has stopped
	}

	//
	// Virtual network events
	//
	if (msg->eventCode == ZTS_EVENT_NETWORK_NOT_FOUND) {
		printf("ZTS_EVENT_NETWORK_NOT_FOUND --- network=%llx\n", msg->network->nwid);
		// Is your nwid incorrect?
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_REQUESTING_CONFIG) {
		printf("ZTS_EVENT_NETWORK_REQUESTING_CONFIG --- network=%llx\n", msg->network->nwid);
		// Node is requesting network config details from controller, please wait
	} 
	if (msg->eventCode == ZTS_EVENT_NETWORK_ACCESS_DENIED) {
		printf("ZTS_EVENT_NETWORK_ACCESS_DENIED --- network=%llx\n", msg->network->nwid);
		// This node is not authorized to join nwid
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_READY_IP4) {
		printf("ZTS_EVENT_NETWORK_READY_IP4 --- network=%llx\n", msg->network->nwid);
		network_ready = true;
		// IPv4 traffic can now be processed for nwid
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_READY_IP6) {
		printf("ZTS_EVENT_NETWORK_READY_IP6 --- network=%llx\n", msg->network->nwid);
		network_ready = true;
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_DOWN) {
		printf("ZTS_EVENT_NETWORK_DOWN --- network=%llx\n", msg->network->nwid);
		// Can happen if another thread called zts_leave()
	}

	//
	// Network stack events
	//
	if (msg->eventCode == ZTS_EVENT_NETIF_UP) {
		printf("ZTS_EVENT_NETIF_UP --- network=%llx, mac=%llx, mtu=%d\n", 
			msg->netif->nwid,
			msg->netif->mac,
			msg->netif->mtu);
		network_ready = true;
	}
	if (msg->eventCode == ZTS_EVENT_NETIF_DOWN) {
		printf("ZTS_EVENT_NETIF_DOWN --- network=%llx, mac=%llx\n", 
			msg->netif->nwid,
			msg->netif->mac);
		
		network_ready = true;
	}

	//
	// Address events
	//
	if (msg->eventCode == ZTS_EVENT_ADDR_ADDED_IP4) {
		char ipstr[INET_ADDRSTRLEN];
		struct sockaddr_in *in4 = (struct sockaddr_in*)&(msg->addr->addr);
		inet_ntop(AF_INET, &(in4->sin_addr), ipstr, INET_ADDRSTRLEN);
		printf("ZTS_EVENT_ADDR_NEW_IP4 --- addr=%s (on network=%llx)\n", 
			ipstr, msg->addr->nwid);
	}
	if (msg->eventCode == ZTS_EVENT_ADDR_ADDED_IP6) {
		char ipstr[INET6_ADDRSTRLEN];
		struct sockaddr_in6 *in6 = (struct sockaddr_in6*)&(msg->addr->addr);
		inet_ntop(AF_INET6, &(in6->sin6_addr), ipstr, INET6_ADDRSTRLEN);
		printf("ZTS_EVENT_ADDR_NEW_IP6 --- addr=%s (on network=%llx)\n", 
			ipstr, msg->addr->nwid);
	}
	if (msg->eventCode == ZTS_EVENT_ADDR_REMOVED_IP4) {
		char ipstr[INET_ADDRSTRLEN];
		struct sockaddr_in *in4 = (struct sockaddr_in*)&(msg->addr->addr);
		inet_ntop(AF_INET, &(in4->sin_addr), ipstr, INET_ADDRSTRLEN);
		printf("ZTS_EVENT_ADDR_REMOVED_IP4 --- addr=%s (on network=%llx)\n", 
			ipstr, msg->addr->nwid);
	}
	if (msg->eventCode == ZTS_EVENT_ADDR_REMOVED_IP6) {
		char ipstr[INET6_ADDRSTRLEN];
		struct sockaddr_in6 *in6 = (struct sockaddr_in6*)&(msg->addr->addr);
		inet_ntop(AF_INET6, &(in6->sin6_addr), ipstr, INET6_ADDRSTRLEN);
		printf("ZTS_EVENT_ADDR_REMOVED_IP6 --- addr=%s (on network=%llx)\n", 
			ipstr, msg->addr->nwid);
	}

	//
	// Peer events
	//
	if (msg->eventCode == ZTS_EVENT_PEER_P2P) {
		printf("ZTS_EVENT_PEER_P2P --- node=%llx\n", msg->peer->address);
		// A direct path is known for nodeId
	}
	if (msg->eventCode == ZTS_EVENT_PEER_RELAY) {
		printf("ZTS_EVENT_PEER_RELAY --- node=%llx\n", msg->peer->address);
		// No direct path is known for nodeId
	}
}

void printPeerDetails(struct zts_peer_details *pd)
{
	printf("\npeer=%llx, latency=%d, version=%d.%d.%d, pathCount=%d\n",
		pd->address,
		pd->latency,
		pd->versionMajor,
		pd->versionMinor,
		pd->versionRev,
		pd->pathCount);
	// Print all known paths for each peer
	for (int j=0; j<pd->pathCount; j++) {
		char ipstr[INET6_ADDRSTRLEN];
		int port;
		struct sockaddr *sa = (struct sockaddr *)&(pd->paths[j].address);
		if (sa->sa_family == AF_INET) {
			struct sockaddr_in *in4 = (struct sockaddr_in*)sa;
			inet_ntop(AF_INET, &(in4->sin_addr), ipstr, INET_ADDRSTRLEN);
			port = ntohs(in4->sin_port);
		}
		if (sa->sa_family == AF_INET6) {
			struct sockaddr_in6 *in6 = (struct sockaddr_in6*)sa;
			inet_ntop(AF_INET6, &(in6->sin6_addr), ipstr, INET6_ADDRSTRLEN);
		}
		printf("\tpath[%d]=%s, port=%d\n", j, ipstr, port);	
	}
}

void getSinglePeerDetails(uint64_t peerId)
{
	struct zts_peer_details pd;
	int err = zts_get_peer(&pd, peerId);

	if (err == ZTS_ERR_OK) {
		printf("(%d) call succeeded\n", err);
	} if (err == ZTS_ERR_INVALID_ARG) {
		printf("(%d) invalid argument\n", err);
		return;
	} if (err == ZTS_ERR_SERVICE) {
		printf("(%d) error: service is unavailable\n", err);
		return;
	} if (err == ZTS_ERR_INVALID_OP) {
		printf("(%d) error: invalid API operation\n", err);
		return;
	} if (err == ZTS_ERR_NO_RESULT) {
		printf("(%d) error: object or result not found\n", err);
		return;
	}
	if (err == 0) { // ZTS_ERR_OK
		printPeerDetails(&pd);
	}
}

// Similar to "zerotier-cli listpeers"
void getAllPeerDetails()
{
	struct zts_peer_details pd[128];
	/* This number should be large enough to handle the
	expected number of peers. This call can also get
	expensive for large numbers of peers. Consider using
	get_peer(struct zts_peer_details *pds, uint64_t peerId)
	instead */
	int num = 128;
	int err;
	if ((err = zts_get_peers(pd, &num)) < 0) {
		printf("error (%d)\n", err);
		return;
	}
	if (num) {
		printf("num=%d\n", num);
		for (int i=0; i<num; i++) {
			printPeerDetails(&pd[i]);
		}
	}
}

struct zts_stats_proto protoSpecificStats;

void display_stack_stats()
{
	int err = 0;
	// Count received pings
	if ((err = zts_get_protocol_stats(ZTS_STATS_PROTOCOL_ICMP, &protoSpecificStats)) != ZTS_ERR_OK) {
		printf("zts_get_proto_stats()=%d", err);
		return;
	}
	printf("icmp.recv=%d\n", protoSpecificStats.recv);
	// Count dropped TCP packets
	if ((err = zts_get_protocol_stats(ZTS_STATS_PROTOCOL_TCP, &protoSpecificStats)) != ZTS_ERR_OK) {
		printf("zts_get_proto_stats()=%d", err);
		return;
	}
	printf("tcp.drop=%d\n", protoSpecificStats.drop);
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

	// Set up ZeroTier service
	int defaultServicePort = 9994;
	int nwid = 0x0123456789abcdef;
	zts_start("test/path", &myZeroTierEventCallback, defaultServicePort);
	printf("Waiting for node to come online...\n");
	
	// Wait for the node to come online before joining a network
	while (!node_ready) { sleep(1); }
	zts_join(nwid);
	printf("Joined virtual network. Requesting configuration...\n");
	
	//sleep(1);

	// Get multiple peer's details 
	getAllPeerDetails();

	// Get a single peer's details
	getSinglePeerDetails(0x01b34f67c90);
	int status = -1;
	
	// Get status of the node/service
	status = zts_get_node_status();
	printf("zts_get_node_status()=%d\n", status);
	
	// Get status of a network
	status = zts_get_network_status(0x0123456789abcdef);
	printf("zts_get_network_status()=%d\n", status);

	while (true) {
		sleep(1);
		status = zts_get_node_status();
		printf("zts_get_node_status()=%d\n", status);
		display_stack_stats();
	}

	// Socket API example
	printf("zts_errno=%d\n",zts_errno);
	if ((fd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("error creating socket\n");
	}
	printf("fd=%d, zts_errno=%d\n", fd, zts_errno);
	if ((err = zts_connect(fd, (const struct sockaddr *)&addr, sizeof(addr))) < 0) {
		printf("error connecting to remote host\n");
	}
	if ((err = zts_write(fd, str, strlen(str))) < 0) {
		printf("error writing to socket\n");
	}

	zts_close(fd);
	zts_stop();
	return 0;
}
