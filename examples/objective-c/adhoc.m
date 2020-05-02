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

#import <zt/ZeroTier.h>

#include <arpa/inet.h>

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

void delay_ms(long ms) { usleep(ms*1000); }

bool nodeReady = false;
bool networkReady = false;

// Example callbacks
void myZeroTierEventCallback(struct zts_callback_msg *msg)
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
	if (msg->eventCode == ZTS_EVENT_NETWORK_REQUESTING_CONFIG) {
		NSLog(@"ZTS_EVENT_NETWORK_REQUESTING_CONFIG --- Requesting config for network %llx, please wait a few seconds...\n", msg->network->nwid);
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
	if (msg->eventCode == ZTS_EVENT_PEER_P2P) {
		NSLog(@"ZTS_EVENT_PEER_P2P --- There is now a direct path to peer %llx\n",
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

	zts_set_network_caching(false);

	if((err = zts_start(argv[1], &myZeroTierEventCallback, ztServicePort)) != ZTS_ERR_OK) {
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
