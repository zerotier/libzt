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
 * - Exceptions to the above rule are:
 *    1) Joining a public network (such as "earth")
 *    2) Joining an Ad-hoc network, (no controller and therefore requires no authorization.)
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
 *   established libzt will inform you via ZTS_EVENT_PEER_DIRECT for a specific peer ID. No
 *   action is required on your part for this callback event.
 *
 *   Note: In these initial moments before ZTS_EVENT_PEER_DIRECT has been received for a
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
 *      ZTS_ERR_OK            // No error
 *      ZTS_ERR_SOCKET        // Socket error, see zts_errno
 *      ZTS_ERR_SERVICE       // You probably did something at the wrong time
 *      ZTS_ERR_ARG           // Invalid argument
 *      ZTS_ERR_NO_RESULT     // No result (not necessarily an error)
 *      ZTS_ERR_GENERAL       // Consider filing a bug report
 *
 *   Category 2: Sockets (zts_socket, zts_bind, zts_connect, zts_listen, etc).
 *               Errors returned by these functions can be the same as the above. With
 *               the added possibility of zts_errno being set. Much like standard
 *               errno this will provide a more specific reason for an error's occurrence.
 *               See ZeroTierSockets.h for values.
 *
 *
 *   API COMPATIBILITY WITH HOST OS:
 *
 * - While the ZeroTier socket interface can coexist with your host OS's own interface in
 *   the same file with no type and naming conflicts, try not to mix and match host
 *   OS/libzt structures, functions, or constants. It may look similar and may even work
 *   some of the time but there enough differences that it will cause headaches. Here
 *   are a few guidelines:
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
 *          zts_bind(fd, (struct zts_sockaddr *)&in4, sizeof(struct zts_sockaddr_in)) < 0)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "ZeroTierSockets.h"

#include "winsock.h"

struct Node
{
	Node() : online(false), joinedAtLeastOneNetwork(false), id(0) {}
	bool online;
	bool joinedAtLeastOneNetwork;
	uint64_t id;
	// etc
} myNode;

void printNodeDetails(const char *msgStr, struct zts_node_details *d)
{
	printf("\n%s\n", msgStr);
	printf("\t- id                         : %llx\n", d->address);
	printf("\t- version                    : %d.%d.%d\n", d->versionMajor, d->versionMinor, d->versionRev);
	printf("\t- primaryPort                : %d\n", d->primaryPort);
	printf("\t- secondaryPort              : %d\n", d->secondaryPort);
}

void printPeerDetails(const char *msgStr, struct zts_peer_details *d)
{
	printf("\n%s\n", msgStr);
	printf("\t- peer                       : %llx\n", d->address);
	printf("\t- role                       : %llx\n", d->role);
	printf("\t- latency                    : %llx\n", d->latency);
	printf("\t- pathCount                  : %llx\n", d->pathCount);
	printf("\t- version                    : %d.%d.%d\n", d->versionMajor, d->versionMinor, d->versionRev);
	printf("\t- pathCount                  : %d\n", d->pathCount);
	printf("\t- paths:\n");

	// Print all known paths for each peer
	for (unsigned int j=0; j<d->pathCount; j++) {
		char ipstr[ZTS_INET6_ADDRSTRLEN];
		int port = 0;
		struct zts_sockaddr *sa = (struct zts_sockaddr *)&(d->paths[j].address);
		if (sa->sa_family == ZTS_AF_INET) { // TODO: Probably broken
			struct zts_sockaddr_in *in4 = (struct zts_sockaddr_in*)sa;
			zts_inet_ntop(ZTS_AF_INET, &(in4->sin_addr), ipstr, ZTS_INET_ADDRSTRLEN);
			port = zts_ntohs(in4->sin_port);
		}
		if (sa->sa_family == ZTS_AF_INET6) {
			struct zts_sockaddr_in6 *in6 = (struct zts_sockaddr_in6*)sa;
			zts_inet_ntop(ZTS_AF_INET6, &(in6->sin6_addr), ipstr, ZTS_INET6_ADDRSTRLEN);
		}
		printf("\t  - %15s : %6d\n", ipstr, port);
	}
	printf("\n");
}

void printNetworkDetails(const char *msgStr, struct zts_network_details *d)
{
	printf("\n%s\n", msgStr);
	printf("\t- nwid                       : %llx\n", d->nwid);
	printf("\t- mac                        : %lx\n", d->mac);
	printf("\t- name                       : %s\n", d->name);
	printf("\t- type                       : %d\n", d->type);
	/* MTU for the virtual network can be set via our web API */
	printf("\t- mtu                        : %d\n", d->mtu);
	printf("\t- dhcp                       : %d\n", d->dhcp);
	printf("\t- bridge                     : %d\n", d->bridge);
	printf("\t- broadcastEnabled           : %d\n", d->broadcastEnabled);
	printf("\t- portError                  : %d\n", d->portError);
	printf("\t- netconfRevision            : %d\n", d->netconfRevision);
	printf("\t- routeCount                 : %d\n", d->routeCount);
	printf("\t- multicastSubscriptionCount : %d\n", d->multicastSubscriptionCount);

	for (int i=0; i<d->multicastSubscriptionCount; i++) {
		printf("\t  - mac=%llx, adi=%x\n", d->multicastSubscriptions[i].mac, d->multicastSubscriptions[i].adi);
	}

	printf("\t- addresses:\n");

	for (int i=0; i<d->assignedAddressCount; i++) {
		if (d->assignedAddresses[i].ss_family == ZTS_AF_INET) {
			char ipstr[ZTS_INET_ADDRSTRLEN];
			struct zts_sockaddr_in *in4 = (struct zts_sockaddr_in*)&(d->assignedAddresses[i]);
			zts_inet_ntop(ZTS_AF_INET, &(in4->sin_addr), ipstr, ZTS_INET_ADDRSTRLEN);
			printf("\t  - %s\n",ipstr);
		}
		if (d->assignedAddresses[i].ss_family == ZTS_AF_INET6) {
			char ipstr[ZTS_INET6_ADDRSTRLEN];
			struct zts_sockaddr_in6 *in6 = (struct zts_sockaddr_in6*)&(d->assignedAddresses[i]);
			zts_inet_ntop(ZTS_AF_INET6, &(in6->sin6_addr), ipstr, ZTS_INET6_ADDRSTRLEN);
			printf("\t  - %s\n",ipstr);
		}
	}

	printf("\t- routes:\n");

	for (int i=0; i<d->routeCount; i++) {
		if (d->routes[i].target.ss_family == ZTS_AF_INET) {
			char ipstr[ZTS_INET_ADDRSTRLEN];
			struct zts_sockaddr_in *in4 = (struct zts_sockaddr_in*)&(d->routes[i].target);
			zts_inet_ntop(ZTS_AF_INET, &(in4->sin_addr), ipstr, ZTS_INET_ADDRSTRLEN);
			printf("\t  - target : %s\n",ipstr);
			in4 = (struct zts_sockaddr_in*)&(d->routes[i].via);
			zts_inet_ntop(ZTS_AF_INET, &(in4->sin_addr), ipstr, ZTS_INET_ADDRSTRLEN);
			printf("\t  - via    : %s\n",ipstr);
		}
		if (d->routes[i].target.ss_family == ZTS_AF_INET6) {
			char ipstr[ZTS_INET6_ADDRSTRLEN];
			struct zts_sockaddr_in6 *in6 = (struct zts_sockaddr_in6*)&(d->routes[i].target);
			zts_inet_ntop(ZTS_AF_INET6, &(in6->sin6_addr), ipstr, ZTS_INET6_ADDRSTRLEN);
			printf("\t  - target : %s\n",ipstr);
			in6 = (struct zts_sockaddr_in6*)&(d->routes[i].via);
			zts_inet_ntop(ZTS_AF_INET6, &(in6->sin6_addr), ipstr, ZTS_INET6_ADDRSTRLEN);
			printf("\t  - via    : %s\n",ipstr);
		}
		printf("\t    - flags  : %d\n", d->routes[i].flags);
		printf("\t    - metric : %d\n", d->routes[i].metric);
	}
}

void printNetifDetails(const char *msgStr, struct zts_netif_details *d)
{
	printf("\n%s\n", msgStr);
	printf("\t- nwid : %llx\n", d->nwid);
	printf("\t- mac  : %llx\n", d->mac);
	printf("\t- mtu  : %d\n", d->mtu);
}

/* Callback handler, you should return control from this function as quickly as you can
to ensure timely receipt of future events. You should not call libzt API functions from
this function unless it's something trivial like zts_inet_ntop() or similar that has
no state-change implications. */
void myZeroTierEventCallback(void *msgPtr)
{
	struct zts_callback_msg *msg = (struct zts_callback_msg *)msgPtr;
	printf("eventCode=%d\n", msg->eventCode);

	// Node events
	if (msg->eventCode == ZTS_EVENT_NODE_ONLINE) {
		printNodeDetails("nZTS_EVENT_NODE_ONLINE", msg->node);
		myNode.id = msg->node->address;
		myNode.online = true;
	}
	if (msg->eventCode == ZTS_EVENT_NODE_OFFLINE) {
		printf("\nZTS_EVENT_NODE_OFFLINE --- Check your Internet connection, router, firewall, etc. What ports are you blocking?\n");
		myNode.online = false;
	}
	if (msg->eventCode == ZTS_EVENT_NODE_NORMAL_TERMINATION) {
		printf("\nZTS_EVENT_NODE_NORMAL_TERMINATION -- A call to zts_start() will restart ZeroTier.\n");
		myNode.online = false;
	}

	// Virtual network events
	if (msg->eventCode == ZTS_EVENT_NETWORK_NOT_FOUND) {
		printf("\nZTS_EVENT_NETWORK_NOT_FOUND --- Are you sure %llx is a valid network?\n",
			msg->network->nwid);
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_REQ_CONFIG) {
		printf("\nZTS_EVENT_NETWORK_REQ_CONFIG --- Requesting config for network %llx, please wait a few seconds...\n",
			msg->network->nwid);
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_ACCESS_DENIED) {
		printf("\nZTS_EVENT_NETWORK_ACCESS_DENIED --- Access to virtual network %llx has been denied. Did you authorize the node yet?\n",
			msg->network->nwid);
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_READY_IP4) {
		printNetworkDetails("ZTS_EVENT_NETWORK_READY_IP4 --- Network config received.", msg->network);
		myNode.joinedAtLeastOneNetwork = true;
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_READY_IP6) {
		printNetworkDetails("ZTS_EVENT_NETWORK_READY_IP6 --- Network config received.", msg->network);
		myNode.joinedAtLeastOneNetwork = true;
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_DOWN) {
		printf("\nZTS_EVENT_NETWORK_DOWN --- %llx\n", msg->network->nwid);
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_UPDATE) {
		printNetworkDetails("ZTS_EVENT_NETWORK_UPDATE --- Network config received.", msg->network);
	}

	// Address events
	if (msg->eventCode == ZTS_EVENT_ADDR_ADDED_IP4) {
		char ipstr[ZTS_INET_ADDRSTRLEN];
		struct zts_sockaddr_in *in4 = (struct zts_sockaddr_in*)&(msg->addr->addr);
		zts_inet_ntop(ZTS_AF_INET, &(in4->sin_addr), ipstr, ZTS_INET_ADDRSTRLEN);
		printf("\nZTS_EVENT_ADDR_ADDED_IP4 --- This node's virtual address on network %llx is %s\n",
			msg->addr->nwid, ipstr);
	}
	if (msg->eventCode == ZTS_EVENT_ADDR_ADDED_IP6) {
		char ipstr[ZTS_INET6_ADDRSTRLEN];
		struct zts_sockaddr_in6 *in6 = (struct zts_sockaddr_in6*)&(msg->addr->addr);
		zts_inet_ntop(ZTS_AF_INET6, &(in6->sin6_addr), ipstr, ZTS_INET6_ADDRSTRLEN);
		printf("\nZTS_EVENT_ADDR_ADDED_IP6 --- This node's virtual address on network %llx is %s\n",
			msg->addr->nwid, ipstr);
	}
	if (msg->eventCode == ZTS_EVENT_ADDR_REMOVED_IP4) {
		char ipstr[ZTS_INET_ADDRSTRLEN];
		struct zts_sockaddr_in *in4 = (struct zts_sockaddr_in*)&(msg->addr->addr);
		zts_inet_ntop(ZTS_AF_INET, &(in4->sin_addr), ipstr, ZTS_INET_ADDRSTRLEN);
		printf("\nZTS_EVENT_ADDR_REMOVED_IP4 --- The virtual address %s for this node on network %llx has been removed.\n",
			ipstr, msg->addr->nwid);
	}
	if (msg->eventCode == ZTS_EVENT_ADDR_REMOVED_IP6) {
		char ipstr[ZTS_INET6_ADDRSTRLEN];
		struct zts_sockaddr_in6 *in6 = (struct zts_sockaddr_in6*)&(msg->addr->addr);
		zts_inet_ntop(ZTS_AF_INET6, &(in6->sin6_addr), ipstr, ZTS_INET6_ADDRSTRLEN);
		printf("\nZTS_EVENT_ADDR_REMOVED_IP6 --- The virtual address %s for this node on network %llx has been removed.\n",
			ipstr, msg->addr->nwid);
	}

	// Peer events
	if (msg->peer) {
		if (msg->peer->role == ZTS_PEER_ROLE_PLANET) {
			/* Safe to ignore, these are our roots. They orchestrate the P2P connection.
			You might also see other unknown peers, these are our network controllers. */
			return;
		}
		if (msg->eventCode == ZTS_EVENT_PEER_DIRECT) {
			printPeerDetails("ZTS_EVENT_PEER_DIRECT --- A direct path is known.", msg->peer);
		}
		if (msg->eventCode == ZTS_EVENT_PEER_RELAY) {
			printPeerDetails("ZTS_EVENT_PEER_RELAY --- No direct path known.", msg->peer);
		}
		if (msg->eventCode == ZTS_EVENT_PEER_PATH_DISCOVERED) {
			printPeerDetails("ZTS_EVENT_PEER_PATH_DISCOVERED --- A new direct path was discovered.", msg->peer);
		}
		if (msg->eventCode == ZTS_EVENT_PEER_PATH_DEAD) {
			printPeerDetails("ZTS_EVENT_PEER_PATH_DEAD --- A direct path has died.", msg->peer);
		}
	}

	// Network stack (netif) events (used for debugging, can be ignored)
	if (msg->eventCode == ZTS_EVENT_NETIF_UP) {
		printNetifDetails("ZTS_EVENT_NETIF_UP --- No action required.", msg->netif);
	}
	if (msg->eventCode == ZTS_EVENT_NETIF_DOWN) {
		printNetifDetails("ZTS_EVENT_NETIF_DOWN --- No action required.", msg->netif);
	}
	if (msg->eventCode == ZTS_EVENT_NETIF_REMOVED) {
		printNetifDetails("ZTS_EVENT_NETIF_REMOVED --- No action required.", msg->netif);
	}
	if (msg->eventCode == ZTS_EVENT_NETIF_LINK_UP) {
		printNetifDetails("ZTS_EVENT_NETIF_LINK_UP --- No action required.", msg->netif);
	}
	if (msg->eventCode == ZTS_EVENT_NETIF_LINK_DOWN) {
		printNetifDetails("ZTS_EVENT_NETIF_LINK_DOWN --- No action required.", msg->netif);
	}
	// Network stack events (used for debugging, can be ignored)
	if (msg->eventCode == ZTS_EVENT_STACK_UP) {
		printf("\nZTS_EVENT_STACK_UP --- No action required.\n");
	}
	if (msg->eventCode == ZTS_EVENT_STACK_DOWN) {
		printf("\nZTS_EVENT_STACK_DOWN --- No action required. An app restart is needed to use ZeroTier again.\n");
	}
}

void get6PLANEAddressOfPeer(uint64_t peerId, uint64_t nwId)
{
	char peerAddrStr[ZTS_INET6_ADDRSTRLEN] = {0};
	struct zts_sockaddr_storage sixplane_addr;
	zts_get_6plane_addr(&sixplane_addr, nwId, peerId);
	struct zts_sockaddr_in6 *p6 = (struct zts_sockaddr_in6*)&sixplane_addr;
	zts_inet_ntop(ZTS_AF_INET6, &(p6->sin6_addr), peerAddrStr, ZTS_INET6_ADDRSTRLEN);
	printf("6PLANE address of peer is: %s\n", peerAddrStr);
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
	fprintf(stderr, "AF_INET=%d, SOCK_STREAM=%d, IPPROTO_TCP=%d", AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (argc != 4) {
		printf("\nlibzt example server\n");
		printf("comprehensive <config_file_path> <nwid> <ztServicePort>\n");
		exit(0);
	}
	std::string configPath = std::string(argv[1]);
	uint64_t nwid = strtoull(argv[2],NULL,16); // Network ID to join
	int ztServicePort = atoi(argv[3]); // Port ZT uses to send encrypted UDP packets to peers (try something like 9994)

	// Bring up ZeroTier service and join network

	// Enable/Disable caching of network details in networks.d
	// (read function documentation before disabling!)
	// zts_allow_network_caching(0)

	// Enable/Disable caching of peer details in peers.d
	// (read function documentation before disabling!)
	// zts_allow_network_caching(1)

	int err = ZTS_ERR_OK;

	if((err = zts_start(configPath.c_str(), &myZeroTierEventCallback, ztServicePort)) != ZTS_ERR_OK) {
		printf("Unable to start service, error = %d. Exiting.\n", err);
		exit(1);
	}
	printf("Waiting for node to come online...\n");
	while (!myNode.online) { zts_delay_ms(50); }
	printf("This node's identity is stored in %s\n", argv[1]);

	if((err = zts_join(nwid)) != ZTS_ERR_OK) {
		printf("Unable to join network, error = %d. Exiting.\n", err);
		exit(1);
	}
	printf("Joining network %llx\n", nwid);
	printf("Don't forget to authorize this device in my.zerotier.com or the web API!\n");
	while (!myNode.joinedAtLeastOneNetwork) { zts_delay_ms(50); }

	// Idle and just show callback events, stack statistics, etc
	// Alternatively, this is where you could start making calls to the socket API

	/*
	while(true) {
		display_stack_stats();
		zts_delay_ms(1000);
	}
	*/

	int delay = 500000;
	printf("This program will delay for %d seconds and then shut down.\n", (delay / 1000));
	zts_delay_ms(delay);
	//printf("Leaving network %llx\n", nwid);
	//zts_leave(nwid);
	//zts_delay_ms(3000); /* added for demo purposes so that events show up */
	printf("Stopping ZeroTier\n");
	zts_stop();
	zts_delay_ms(delay); /* added for demo purposes so that events show up */
	printf("Stopping network stack\n");
	zts_free();
	zts_delay_ms(delay); /* added for demo purposes so that events show up */
	return 0;
}
