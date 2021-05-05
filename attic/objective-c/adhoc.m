/**
 * libzt API example
 *
 * Specify location of zt.framework and link to standard C++ library:
 *
 * clang -lc++ -framework Foundation -F . -framework zt adhoc.m -o adhoc;
 *
 * Pingable node joined to controller-less adhoc network with a 6PLANE addressing scheme
 */

#import <Foundation/Foundation.h>

#import <zt/ZeroTierSockets.h>

#include <arpa/inet.h>

void delay_ms(long ms) { usleep(ms*1000); }

bool nodeReady = false;
bool networkReady = false;

// Example callbacks
void on_zts_event(struct zts_callback_msg *msg)
{
	// Node events
	if (msg->eventCode == ZTS_EVENT_NODE_ONLINE) {
		NSLog(@"ZTS_EVENT_NODE_ONLINE --- This node's ID is %llx\n", msg->node->address);
		nodeReady = true;
	}
	if (msg->eventCode == ZTS_EVENT_NODE_OFFLINE) {
		NSLog(@"ZTS_EVENT_NODE_OFFLINE --- Check your physical Internet connection, router, firewall, etc. What ports are you blocking?\n");
		nodeReady = false;
	}
	// Virtual network events
	if (msg->eventCode == ZTS_EVENT_NETWORK_NOT_FOUND) {
		NSLog(@"ZTS_EVENT_NETWORK_NOT_FOUND --- Are you sure %llx is a valid network?\n",
			msg->network->nwid);
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_REQ_CONFIG) {
		NSLog(@"ZTS_EVENT_NETWORK_REQ_CONFIG --- Requesting config for network %llx, please wait a few seconds...\n", msg->network->nwid);
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_ACCESS_DENIED) {
		NSLog(@"ZTS_EVENT_NETWORK_ACCESS_DENIED --- Access to virtual network %llx has been denied. Did you authorize the node yet?\n",
			msg->network->nwid);
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_READY_IP6) {
		NSLog(@"ZTS_EVENT_NETWORK_READY_IP6 --- Network config received. IPv6 traffic can now be sent over network %llx\n",
			msg->network->nwid);
		networkReady = true;
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_DOWN) {
		NSLog(@"ZTS_EVENT_NETWORK_DOWN --- %llx\n", msg->network->nwid);
	}
	// Network stack events
	if (msg->eventCode == ZTS_EVENT_NETIF_UP) {
		NSLog(@"ZTS_EVENT_NETIF_UP --- network=%llx, mac=%llx, mtu=%d\n",
			msg->netif->nwid,
			msg->netif->mac,
			msg->netif->mtu);
		networkReady = true;
	}
	if (msg->eventCode == ZTS_EVENT_NETIF_DOWN) {
		NSLog(@"ZTS_EVENT_NETIF_DOWN --- network=%llx, mac=%llx\n",
			msg->netif->nwid,
			msg->netif->mac);

		networkReady = true;
	}
	// Address events
	if (msg->eventCode == ZTS_EVENT_ADDR_ADDED_IP6) {
		char ipstr[INET6_ADDRSTRLEN];
		struct zts_sockaddr_in6 *in6 = (struct zts_sockaddr_in6*)&(msg->addr->addr);
		inet_ntop(AF_INET6, &(in6->sin6_addr), ipstr, INET6_ADDRSTRLEN);
		NSLog(@"ZTS_EVENT_ADDR_NEW_IP6 --- Join %llx and ping me at %s\n",
			msg->addr->nwid, ipstr);
	}
	// Peer events
	// If you don't recognize the peer ID, don't panic, this is most likely one of our root servers
	if (msg->eventCode == ZTS_EVENT_PEER_DIRECT) {
		NSLog(@"ZTS_EVENT_PEER_DIRECT --- There is now a direct path to peer %llx\n",
			msg->peer->address);
	}
	if (msg->eventCode == ZTS_EVENT_PEER_RELAY) {
		NSLog(@"ZTS_EVENT_PEER_RELAY --- No direct path to peer %llx\n",
			msg->peer->address);
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
		NSLog(@"\nlibzt example\n");
		NSLog(@"adhoc <config_file_path> <adhocStartPort> <adhocEndPort> <ztServicePort>\n");
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

	if((err = zts_start(argv[1], &on_zts_event, ztServicePort)) != ZTS_ERR_OK) {
		NSLog(@"Unable to start service, error = %d. Exiting.\n", err);
		exit(1);
	}
	NSLog(@"Waiting for node to come online...\n");
	while (!nodeReady) { delay_ms(50); }
	NSLog(@"This node's identity is stored in %s\n", argv[1]);

	if((err = zts_join(adhoc_nwid)) != ZTS_ERR_OK) {
		NSLog(@"Unable to join network, error = %d. Exiting.\n", err);
		exit(1);
	}
	NSLog(@"Joining network %llx\n", adhoc_nwid);
	while (!networkReady) { delay_ms(50); }

	// Idle and just show callback events, stack statistics, etc

	NSLog(@"Node will now idle...\n");
	while (true) { delay_ms(1000); }

	// Shut down service and stack threads

	zts_stop();
	return 0;
}
