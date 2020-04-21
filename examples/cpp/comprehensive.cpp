/**
 * libzt API example
 *
 * For more straight-to-the-point examples, see the other files in this same directory.
 */

/**
 *
 *   IDENTITIES and AUTHORIZATION:
 *
 * - Upon the first execution of this code, a new identity will be generated and placed in
 *   the location given in the first argument to zts_start(path, ...). If you accidentally
 *   duplicate the identity files and use them simultaneously in a different node instance
 *   you will experience undefined behavior and it is likely nothing will work.
 * 
 * - You must authorize the node ID provided by the ZTS_EVENT_NODE_ONLINE callback to join
 *   your network, otherwise nothing will happen. This can be done manually or via
 *   our web API: https://my.zerotier.com/help/api
 *
 * - An exception to the above rule is if you are using an Ad-hoc network, it has no 
 *   controller and therefore requires no authorization.
 *
 *
 *   ESTABLISHING A CONNECTION:
 *
 * - Creating a standard socket connection generally works the same as it would using
 *   an ordinary socket interface, however with libzt there is a subtle difference in
 *   how connections are established which may cause confusion:
 *
 *   The underlying virtual ZT layer creates what are called "transport-triggered links"
 *   between nodes. That is, links are not established until an attempt to communicate
 *   with a peer has taken place. The side effect is that the first few packets sent from
 *   a libzt instance are usually relayed via our free infrastructure and it isn't until a
 *   root server has passed contact information to both peers that a direct connection will be
 *   established. Therefore, it is required that multiple connection attempts be undertaken
 *   when initially communicating with a peer. After a transport-triggered link is
 *   established libzt will inform you via ZTS_EVENT_PEER_P2P for a specific peer ID. No
 *   action is required on your part for this callback event.
 *
 *   Note: In these initial moments before ZTS_EVENT_PEER_P2P has been received for a
 *         specific peer, traffic may be slow, jittery and there may be high packet loss.
 *         This will subside within a couple of seconds.
 *
 *
 *   ERROR HANDLING:
 *
 * - libzt's API is actually composed of two categories of functions with slightly
 *   different error reporting mechanisms.
 *
 *   Category 1: Control functions (zts_start, zts_join, zts_get_peer_status, etc). Errors
 *                returned by these functions can be any of the following:
 *
 *      [ 0] ZTS_ERR_OK          - No error.
 *      [-1] ZTS_ERR             - Error (see zts_errno for more information).
 *      [-2] ZTS_ERR_INVALID_ARG - An argument provided is invalid.
 *      [-3] ZTS_ERR_SERVICE     - ZT is not yet initialized. Try again.
 *      [-4] ZTS_ERR_INVALID_OP  - Operation is not permitted (Doesn't make sense in this state).
 *      [-5] ZTS_ERR_NO_RESULT   - Call succeeded but no result was available. Not always an error.
 *      [-6] ZTS_ERR_GENERAL     - General internal failure. Consider filing a bug report.
 *
 *   Category 2: Sockets (zts_socket, zts_bind, zts_connect, zts_listen, etc).
 *               Errors returned by these functions can be the same as the above. With
 *               the added possibility of zts_errno being set. Much like standard
 *               errno this will provide a more specific reason for an error's occurrence.
 *               These error values are defined in: libzt/ext/lwip/src/include/lwip/errno.h
 *               and closely map to standard Linux error values.
 *
 *
 *   API COMPATIBILITY WITH HOST OS:
 *
 * - Since libzt re-implements a socket API probably very similar to your host OS's own
 *   API it may be tempting to mix and match host OS structures and functions with those
 *   of libzt. This may work on occasion, but you are tempting fate, so here are a few
 *   guidelines:
 *
 *   If you are calling a zts_* function, use the appropriate ZTS_* constants:
 *             
 *          zts_socket(ZTS_AF_INET6, ZTS_SOCK_DGRAM, 0); (CORRECT)
 *          zts_socket(AF_INET6, SOCK_DGRAM, 0);         (INCORRECT)
 *
 *   If you are calling a zts_* function, use the appropriate zts_* structure:
 *
 *          struct zts_sockaddr_in in4;  <------ Note the zts_* prefix
 *             ...
 *          zts_bind(fd, (struct sockaddr *)&in4, sizeof(struct zts_sockaddr_in)) < 0)
 *
 *   If you are calling a host OS function, use your host OS's constants (and structures!):
 *       
 *          inet_ntop(AF_INET6, &(in6->sin6_addr), ...);     (CORRECT)
 *          inet_ntop(ZTS_AF_INET6, &(in6->sin6_addr), ...); (INCORRECT)
 *
 *   If you are calling a host OS function but passing a zts_* structure, this can
 *       work sometimes but you should take care to pass the correct host OS constants:
 *
 *          struct zts_sockaddr_in6 in6;
 *             ...
 *          inet_ntop(AF_INET6, &(in6->sin6_addr), dstStr, INET6_ADDRSTRLEN);
 */

#include <stdio.h>
#include <string.h>
#include <string>
#include <inttypes.h>
#include <stdlib.h>

#if defined(_WIN32)
#include <WinSock2.h>
#include <stdint.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

void delay_ms(long ms)
{
#if defined(_WIN32)
	Sleep(ms);
#else
	usleep(ms*1000);
#endif
}

#include "ZeroTier.h"

bool nodeReady = false;
bool networkReady = false;

// Example callbacks
void myZeroTierEventCallback(struct zts_callback_msg *msg)
{
	// Node events
	if (msg->eventCode == ZTS_EVENT_NODE_ONLINE) {
		printf("ZTS_EVENT_NODE_ONLINE --- This node's ID is %llx\n", msg->node->address);
		nodeReady = true;
	}
	if (msg->eventCode == ZTS_EVENT_NODE_OFFLINE) {
		printf("ZTS_EVENT_NODE_OFFLINE --- Check your physical Internet connection, router, firewall, etc. What ports are you blocking?\n");
		nodeReady = false;
	}
	if (msg->eventCode == ZTS_EVENT_NODE_NORMAL_TERMINATION) {
		printf("ZTS_EVENT_NODE_NORMAL_TERMINATION\n");
	}

	// Virtual network events
	if (msg->eventCode == ZTS_EVENT_NETWORK_NOT_FOUND) {
		printf("ZTS_EVENT_NETWORK_NOT_FOUND --- Are you sure %llx is a valid network?\n",
			msg->network->nwid);
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_REQUESTING_CONFIG) {
		printf("ZTS_EVENT_NETWORK_REQUESTING_CONFIG --- Requesting config for network %llx, please wait a few seconds...\n", msg->network->nwid);
	} 
	if (msg->eventCode == ZTS_EVENT_NETWORK_ACCESS_DENIED) {
		printf("ZTS_EVENT_NETWORK_ACCESS_DENIED --- Access to virtual network %llx has been denied. Did you authorize the node yet?\n",
			msg->network->nwid);
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_READY_IP4) {
		printf("ZTS_EVENT_NETWORK_READY_IP4 --- Network config received. IPv4 traffic can now be sent over network %llx\n",
			msg->network->nwid);
		networkReady = true;
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_READY_IP6) {
		printf("ZTS_EVENT_NETWORK_READY_IP6 --- Network config received. IPv6 traffic can now be sent over network %llx\n",
			msg->network->nwid);
		networkReady = true;
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_DOWN) {
		printf("ZTS_EVENT_NETWORK_DOWN --- %llx\n", msg->network->nwid);
	}

	// Network stack events
	if (msg->eventCode == ZTS_EVENT_NETIF_UP) {
		printf("ZTS_EVENT_NETIF_UP --- network=%llx, mac=%llx, mtu=%d\n", 
			msg->netif->nwid,
			msg->netif->mac,
			msg->netif->mtu);
		networkReady = true;
	}
	if (msg->eventCode == ZTS_EVENT_NETIF_DOWN) {
		printf("ZTS_EVENT_NETIF_DOWN --- network=%llx, mac=%llx\n", 
			msg->netif->nwid,
			msg->netif->mac);
		
		networkReady = true;
	}

	// Address events
	if (msg->eventCode == ZTS_EVENT_ADDR_ADDED_IP4) {
		char ipstr[INET_ADDRSTRLEN];
		struct zts_sockaddr_in *in4 = (struct zts_sockaddr_in*)&(msg->addr->addr);
		inet_ntop(AF_INET, &(in4->sin_addr), ipstr, INET_ADDRSTRLEN);
		printf("ZTS_EVENT_ADDR_NEW_IP4 --- This node's virtual address on network %llx is %s\n", 
			msg->addr->nwid, ipstr);
	}
	if (msg->eventCode == ZTS_EVENT_ADDR_ADDED_IP6) {
		char ipstr[INET6_ADDRSTRLEN];
		struct zts_sockaddr_in6 *in6 = (struct zts_sockaddr_in6*)&(msg->addr->addr);
		inet_ntop(AF_INET6, &(in6->sin6_addr), ipstr, INET6_ADDRSTRLEN);
		printf("ZTS_EVENT_ADDR_NEW_IP6 --- This node's virtual address on network %llx is %s\n", 
			msg->addr->nwid, ipstr);
	}
	if (msg->eventCode == ZTS_EVENT_ADDR_REMOVED_IP4) {
		char ipstr[INET_ADDRSTRLEN];
		struct zts_sockaddr_in *in4 = (struct zts_sockaddr_in*)&(msg->addr->addr);
		inet_ntop(AF_INET, &(in4->sin_addr), ipstr, INET_ADDRSTRLEN);
		printf("ZTS_EVENT_ADDR_REMOVED_IP4 --- The virtual address %s for this node on network %llx has been removed.\n", 
			ipstr, msg->addr->nwid);
	}
	if (msg->eventCode == ZTS_EVENT_ADDR_REMOVED_IP6) {
		char ipstr[INET6_ADDRSTRLEN];
		struct zts_sockaddr_in6 *in6 = (struct zts_sockaddr_in6*)&(msg->addr->addr);
		inet_ntop(AF_INET6, &(in6->sin6_addr), ipstr, INET6_ADDRSTRLEN);
		printf("ZTS_EVENT_ADDR_REMOVED_IP6 --- The virtual address %s for this node on network %llx has been removed.\n", 
			ipstr, msg->addr->nwid);
	}

	// Peer events
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
	for (unsigned int j=0; j<pd->pathCount; j++) {
		char ipstr[INET6_ADDRSTRLEN];
		int port = 0;
		struct sockaddr *sa = (struct sockaddr *)&(pd->paths[j].address);
		if (sa->sa_family == AF_INET) {
			struct zts_sockaddr_in *in4 = (struct zts_sockaddr_in*)sa;
			inet_ntop(AF_INET, &(in4->sin_addr), ipstr, INET_ADDRSTRLEN);
			port = ntohs(in4->sin_port);
		}
		if (sa->sa_family == AF_INET6) {
			struct zts_sockaddr_in6 *in6 = (struct zts_sockaddr_in6*)sa;
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
	unsigned int num = 128;
	int err;
	if ((err = zts_get_peers(pd, &num)) < 0) {
		printf("error (%d)\n", err);
		return;
	}
	if (num) {
		printf("num=%d\n", num);
		for (unsigned int i=0; i<num; i++) {
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

int main(int argc, char **argv) 
{
	if (argc != 4) {
		printf("\nlibzt example server\n");
		printf("comprehensive <config_file_path> <nwid> <ztServicePort>\n");
		exit(0);
	}
	uint64_t nwid = strtoull(argv[2],NULL,16); // Network ID to join
	int ztServicePort = atoi(argv[3]); // Port ZT uses to send encrypted UDP packets to peers (try something like 9994)
	
	// Bring up ZeroTier service and join network

	int err = ZTS_ERR_OK;

	// Disable caching of network details in networks.d
	// (read function documentation before disabling!)
	// zts_set_network_caching(false)

	// Disable caching of peer details in peers.d
	// (read function documentation before disabling!)
	// zts_set_network_caching(false)

	if((err = zts_start(argv[1], &myZeroTierEventCallback, ztServicePort)) != ZTS_ERR_OK) {
		printf("Unable to start service, error = %d. Exiting.\n", err);
		exit(1);
	}
	printf("Waiting for node to come online...\n");
	while (!nodeReady) { delay_ms(50); }
	printf("This node ID is %llx\n", zts_get_node_id());
	printf("This node's identity is stored in %s\n", argv[1]);

	if((err = zts_join(nwid)) != ZTS_ERR_OK) {
		printf("Unable to join network, error = %d. Exiting.\n", err);
		exit(1);
	}
	printf("Joining network %llx\n", nwid);
	printf("Don't forget to authorize this device in my.zerotier.com or the web API!\n");
	while (!networkReady) { delay_ms(50); }

	// Get multiple peer's details 
	getAllPeerDetails();

	// Get a single peer's details
	uint64_t peerId = 0xabcdef1234;
	getSinglePeerDetails(peerId);
	int status = -1;
	
	// Get status of the node/service
	status = zts_get_node_status();
	printf("zts_get_node_status()=%d\n", status);
	
	// Get status of a network
	status = zts_get_network_status(nwid);
	printf("zts_get_network_status()=%d\n", status);

	// Idle and just show callback events, stack statistics, etc

	while (true) {
		delay_ms(1000);
		status = zts_get_node_status();
		printf("zts_get_node_status()=%d\n", status);
		display_stack_stats();
	}

	// Shut down service and stack threads

	zts_stop();
	return 0;
}
