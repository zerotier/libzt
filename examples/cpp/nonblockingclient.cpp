/**
 * libzt API example
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
#include <string.h>
#include <string>

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
	if (msg->eventCode == ZTS_EVENT_NODE_NORMAL_TERMINATION) {
		printf("ZTS_EVENT_NODE_NORMAL_TERMINATION\n");
		myNode.online = false;
	}

	// Virtual network events
	if (msg->eventCode == ZTS_EVENT_NETWORK_NOT_FOUND) {
		printf("ZTS_EVENT_NETWORK_NOT_FOUND --- Are you sure %llx is a valid network?\n",
			msg->network->nwid);
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_REQ_CONFIG) {
		printf("ZTS_EVENT_NETWORK_REQ_CONFIG --- Requesting config for network %llx, please wait a few seconds...\n",
			msg->network->nwid);
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_ACCESS_DENIED) {
		printf("ZTS_EVENT_NETWORK_ACCESS_DENIED --- Access to virtual network %llx has been denied. Did you authorize the node yet?\n",
			msg->network->nwid);
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_READY_IP4) {
		printf("ZTS_EVENT_NETWORK_READY_IP4 --- Network config received. IPv4 traffic can now be sent over network %llx\n",
			msg->network->nwid);
		myNode.joinedAtLeastOneNetwork = true;
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_READY_IP6) {
		printf("ZTS_EVENT_NETWORK_READY_IP6 --- Network config received. IPv6 traffic can now be sent over network %llx\n",
			msg->network->nwid);
		myNode.joinedAtLeastOneNetwork = true;
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_DOWN) {
		printf("ZTS_EVENT_NETWORK_DOWN --- %llx\n", msg->network->nwid);
	}

	// Address events
	if (msg->eventCode == ZTS_EVENT_ADDR_ADDED_IP4) {
		char ipstr[ZTS_INET_ADDRSTRLEN];
		struct zts_sockaddr_in *in4 = (struct zts_sockaddr_in*)&(msg->addr->addr);
		zts_inet_ntop(ZTS_AF_INET, &(in4->sin_addr), ipstr, ZTS_INET_ADDRSTRLEN);
		printf("ZTS_EVENT_ADDR_NEW_IP4 --- This node's virtual address on network %llx is %s\n",
			msg->addr->nwid, ipstr);
	}
	if (msg->eventCode == ZTS_EVENT_ADDR_ADDED_IP6) {
		char ipstr[ZTS_INET6_ADDRSTRLEN];
		struct zts_sockaddr_in6 *in6 = (struct zts_sockaddr_in6*)&(msg->addr->addr);
		zts_inet_ntop(ZTS_AF_INET6, &(in6->sin6_addr), ipstr, ZTS_INET6_ADDRSTRLEN);
		printf("ZTS_EVENT_ADDR_NEW_IP6 --- This node's virtual address on network %llx is %s\n",
			msg->addr->nwid, ipstr);
	}
	if (msg->eventCode == ZTS_EVENT_ADDR_REMOVED_IP4) {
		char ipstr[ZTS_INET_ADDRSTRLEN];
		struct zts_sockaddr_in *in4 = (struct zts_sockaddr_in*)&(msg->addr->addr);
		zts_inet_ntop(ZTS_AF_INET, &(in4->sin_addr), ipstr, ZTS_INET_ADDRSTRLEN);
		printf("ZTS_EVENT_ADDR_REMOVED_IP4 --- The virtual address %s for this node on network %llx has been removed.\n",
			ipstr, msg->addr->nwid);
	}
	if (msg->eventCode == ZTS_EVENT_ADDR_REMOVED_IP6) {
		char ipstr[ZTS_INET6_ADDRSTRLEN];
		struct zts_sockaddr_in6 *in6 = (struct zts_sockaddr_in6*)&(msg->addr->addr);
		zts_inet_ntop(ZTS_AF_INET6, &(in6->sin6_addr), ipstr, ZTS_INET6_ADDRSTRLEN);
		printf("ZTS_EVENT_ADDR_REMOVED_IP6 --- The virtual address %s for this node on network %llx has been removed.\n",
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

int main(int argc, char **argv)
{
	if (argc != 6) {
		printf("\nlibzt example non-blocking client\n");
		printf("nonblockingclient <config_file_path> <nwid> <remoteAddr> <remotePort> <ztServicePort>\n");
		exit(0);
	}
	uint64_t nwid = strtoull(argv[2],NULL,16); // Network ID to join
	std::string remoteAddr = argv[3]; // Remote application's virtual ZT address
	int remotePort = atoi(argv[4]); // Port the application will try to connect to the server on
	int ztServicePort = atoi(argv[5]); // Port ZT uses to send encrypted UDP packets to peers (try something like 9994)

	struct zts_sockaddr_in in4;
	in4.sin_port = zts_htons(remotePort);
#if defined(_WIN32)
	in4.sin_addr.S_addr = zts_inet_addr(remoteAddr.c_str());
#else
	in4.sin_addr.s_addr = zts_inet_addr(remoteAddr.c_str());
#endif
	in4.sin_family = ZTS_AF_INET;

	// Bring up ZeroTier service and join network

	// Enable/Disable caching of network details in networks.d
	// (read function documentation before disabling!)
	// zts_allow_network_caching(0)

	// Enable/Disable caching of peer details in peers.d
	// (read function documentation before disabling!)
	// zts_allow_network_caching(1)

	int err = ZTS_ERR_OK;

	if((err = zts_start(argv[1], &myZeroTierEventCallback, ztServicePort)) != ZTS_ERR_OK) {
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

	// Socket-like API example

	char *msgStr = (char*)"Welcome to the machine";
	int bytes=0, fd;
	char recvBuf[128];
	memset(recvBuf, 0, sizeof(recvBuf));

	if ((fd = zts_socket(ZTS_AF_INET, ZTS_SOCK_STREAM, 0)) < 0) {
		printf("Error creating ZeroTier socket (fd=%d, zts_errno=%d). Exiting.\n", fd, zts_errno);
		exit(1);
	}
	// Retries are often required since ZT uses transport-triggered links (explained above)
	for (;;) {
		printf("Connecting to remote host...\n");
		if ((err = zts_connect(fd, (const struct zts_sockaddr *)&in4, sizeof(in4))) < 0) {
			printf("Error connecting to remote host (fd=%d, ret=%d, zts_errno=%d). Trying again.\n",
				fd, err, zts_errno);
			zts_close(fd);
			printf("Creating socket...\n");
			if ((fd = zts_socket(ZTS_AF_INET, ZTS_SOCK_STREAM, 0)) < 0) {
				printf("Error creating ZeroTier socket (fd=%d, zts_errno=%d). Exiting.\n", fd, zts_errno);
				exit(1);
			}
			zts_delay_ms(250);
		}
		else {
			printf("Connected.\n");
			break;
		}
	}

	// Wait random intervals to send a message to the server
	// The non-blocking aspect of this example is server-side
	while(1) {
		if((bytes = zts_send(fd, msgStr, strlen(msgStr), 0)) < 0) {
			printf("Error writing to socket (fd=%d, ret=%d, zts_errno=%d). Exiting.\n", fd, bytes, zts_errno);
			exit(1);
		}
		printf("zts_send()=%d\n", bytes);
		zts_delay_ms((rand() % 100) * 50);
	}
	printf("Read %d bytes: %s\n", bytes, recvBuf);
	printf("Closing socket\n");
	zts_close(fd);
	printf("Shutting down service\n");
	zts_stop();
	return 0;
}
