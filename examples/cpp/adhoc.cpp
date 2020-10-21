/**
 * libzt API example
 *
 * Pingable node joined to controller-less adhoc network with a 6PLANE addressing scheme
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

#include "ZeroTierSockets.h"

struct Node
{
	Node() : online(false), joinedAtLeastOneNetwork(false), id(0) {}
	bool online;
	bool joinedAtLeastOneNetwork;
	uint64_t id;
	// etc
} myNode;

/* Callback handler, you should return control from this function as quickly as you can
to ensure timely receipt of future events. You should not call libzt API functions from
this function unless it's something trivial like zts_inet_ntop() or similar that has
no state-change implications. */
void myZeroTierEventCallback(void *msgPtr)
{
	struct zts_callback_msg *msg = (struct zts_callback_msg *)msgPtr;

	// Node events
	if (msg->eventCode == ZTS_EVENT_NODE_ONLINE) {
		printf("ZTS_EVENT_NODE_ONLINE --- This node's ID is %llx\n", msg->node->address);
		myNode.id = msg->node->address;
		myNode.online = true;
	}
	if (msg->eventCode == ZTS_EVENT_NODE_OFFLINE) {
		printf("ZTS_EVENT_NODE_OFFLINE --- Check your physical Internet connection, router, firewall, etc. What ports are you blocking?\n");
		myNode.online = false;
	}

	// Virtual network events
	if (msg->eventCode == ZTS_EVENT_NETWORK_NOT_FOUND) {
		printf("ZTS_EVENT_NETWORK_NOT_FOUND --- Are you sure %llx is a valid network?\n",
			msg->network->nwid);
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_REQ_CONFIG) {
		printf("ZTS_EVENT_NETWORK_REQ_CONFIG --- Requesting config for network %llx, please wait a few seconds...\n", msg->network->nwid);
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_ACCESS_DENIED) {
		printf("ZTS_EVENT_NETWORK_ACCESS_DENIED --- Access to virtual network %llx has been denied. Did you authorize the node yet?\n",
			msg->network->nwid);
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_READY_IP6) {
		printf("ZTS_EVENT_NETWORK_READY_IP6 --- Network config received. IPv6 traffic can now be sent over network %llx\n",
			msg->network->nwid);
		myNode.joinedAtLeastOneNetwork = true;
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
	}
	if (msg->eventCode == ZTS_EVENT_NETIF_DOWN) {
		printf("ZTS_EVENT_NETIF_DOWN --- network=%llx, mac=%llx\n",
			msg->netif->nwid,
			msg->netif->mac);
	}

	// Address events
	if (msg->eventCode == ZTS_EVENT_ADDR_ADDED_IP6) {
		char ipstr[ZTS_INET6_ADDRSTRLEN];
		struct zts_sockaddr_in6 *in6 = (struct zts_sockaddr_in6*)&(msg->addr->addr);
		zts_inet_ntop(ZTS_AF_INET6, &(in6->sin6_addr), ipstr, ZTS_INET6_ADDRSTRLEN);
		printf("ZTS_EVENT_ADDR_NEW_IP6 --- Join %llx and ping me at %s\n",
			msg->addr->nwid, ipstr);
	}

	// Peer events
	if (msg->peer) {
		if (msg->peer->role == ZTS_PEER_ROLE_PLANET) {
			/* Safe to ignore, these are our roots. They orchestrate the P2P connection.
			You might also see other unknown peers, these are our network controllers. */
			return;
		}
		if (msg->eventCode == ZTS_EVENT_PEER_DIRECT) {
			printf("ZTS_EVENT_PEER_DIRECT --- A direct path is known for node=%llx\n",
				msg->peer->address);
		}
		if (msg->eventCode == ZTS_EVENT_PEER_RELAY) {
			printf("ZTS_EVENT_PEER_RELAY --- No direct path to node=%llx\n", msg->peer->address);
		}
		if (msg->eventCode == ZTS_EVENT_PEER_PATH_DISCOVERED) {
			printf("ZTS_EVENT_PEER_PATH_DISCOVERED --- A new direct path was discovered for node=%llx\n",
				msg->peer->address);
		}
		if (msg->eventCode == ZTS_EVENT_PEER_PATH_DEAD) {
			printf("ZTS_EVENT_PEER_PATH_DEAD --- A direct path has died for node=%llx\n",
				msg->peer->address);
		}
	}
}

/*

Ad-hoc Network:

ffSSSSEEEE000000
| |   |   |
| |   |   Reserved for future use, must be 0
| |   End of port range (hex)
| Start of port range (hex)
Reserved ZeroTier address prefix indicating a controller-less network.

Ad-hoc networks are public (no access control) networks that have no network controller. Instead
their configuration and other credentials are generated locally. Ad-hoc networks permit only IPv6
UDP and TCP unicast traffic (no multicast or broadcast) using 6plane format NDP-emulated IPv6
addresses. In addition an ad-hoc network ID encodes an IP port range. UDP packets and TCP SYN
(connection open) packets are only allowed to destination ports within the encoded range.

For example ff00160016000000 is an ad-hoc network allowing only SSH, while ff0000ffff000000 is an
ad-hoc network allowing any UDP or TCP port.

Keep in mind that these networks are public and anyone in the entire world can join them. Care must
be taken to avoid exposing vulnerable services or sharing unwanted files or other resources.

*/

int main(int argc, char **argv)
{
	if (argc != 5) {
		printf("\nlibzt example\n");
		printf("adhoc <config_file_path> <adhocStartPort> <adhocEndPort> <ztServicePort>\n");
		exit(0);
	}
	int adhocStartPort = atoi(argv[2]); // Start of port range your application will use
	int adhocEndPort = atoi(argv[3]); // End of port range your application will use
	int ztServicePort = atoi(argv[4]); // Port ZT uses to send encrypted UDP packets to peers (try something like 9994)

	uint64_t adhoc_nwid = zts_generate_adhoc_nwid_from_range(adhocStartPort, adhocEndPort);
	int err = ZTS_ERR_OK;

	// If disabled: (network) details will NOT be written to or read from (networks.d/). It may take slightly longer to start the node
	zts_allow_network_caching(1);
	// If disabled: (peer) details will NOT be written to or read from (peers.d/). It may take slightly longer to contact a remote peer
	zts_allow_peer_caching(1);
	// If disabled: Settings will NOT be read from local.conf
	zts_allow_local_conf(1);

	if((err = zts_start(argv[1], &myZeroTierEventCallback, ztServicePort)) != ZTS_ERR_OK) {
		printf("Unable to start service, error = %d. Exiting.\n", err);
		exit(1);
	}
	printf("Waiting for node to come online...\n");
	while (!myNode.online) { zts_delay_ms(50); }
	printf("This node's identity is stored in %s\n", argv[1]);

	if((err = zts_join(adhoc_nwid)) != ZTS_ERR_OK) {
		printf("Unable to join network, error = %d. Exiting.\n", err);
		exit(1);
	}
	printf("Joining network %llx\n", adhoc_nwid);
	while (!myNode.joinedAtLeastOneNetwork) { zts_delay_ms(50); }

	// Idle and just show callback events, stack statistics, etc

	printf("Node will now idle...\n");
	while (true) { zts_delay_ms(1000); }

	// Shut down service and stack threads

	zts_stop();
	return 0;
}
