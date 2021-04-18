/**
 * libzt API example
 *
 * Demonstrates how to manage ZeroTier node identities (public/secret keypairs) without
 * local storage. In this mode you are responsible for saving keys.
 */

#include "ZeroTierSockets.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Node {
	Node() : online(false), joinedAtLeastOneNetwork(false), id(0)
	{
	}
	bool online;
	bool joinedAtLeastOneNetwork;
	uint64_t id;
	// etc
} myNode;

/* Callback handler, you should return control from this function as quickly as you can
to ensure timely receipt of future events. You should not call libzt API functions from
this function unless it's something trivial like zts_inet_ntop() or similar that has
no state-change implications. */
void on_zts_event(void* msgPtr)
{
	struct zts_callback_msg* msg = (struct zts_callback_msg*)msgPtr;

	if (msg->eventCode == ZTS_EVENT_NODE_ONLINE) {
		printf("ZTS_EVENT_NODE_ONLINE --- This node's ID is %llx\n", msg->node->address);
		myNode.id = msg->node->address;
		myNode.online = true;
	}
	if (msg->eventCode == ZTS_EVENT_NODE_OFFLINE) {
		printf("ZTS_EVENT_NODE_OFFLINE --- Check your physical Internet connection, router, "
		       "firewall, etc. What ports are you blocking?\n");
		myNode.online = false;
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_READY_IP6) {
		printf(
		    "ZTS_EVENT_NETWORK_READY_IP6 --- Network config received. IPv6 traffic can now be sent "
		    "over network %llx\n",
		    msg->network->nwid);
		myNode.joinedAtLeastOneNetwork = true;
	}
}

#define KEY_BUF_LEN 2048

int main(int argc, char** argv)
{
	if (argc != 3) {
		printf("\nlibzt example\n");
		printf("earthtest <config_file_path> <ztServicePort>\n");
		exit(0);
	}
	int ztServicePort = atoi(
	    argv[2]);   // Port ZT uses to send encrypted UDP packets to peers (try something like 9994)
	int err = ZTS_ERR_OK;

	// BEGIN key handling

	// Do not allow ZT to write anything to disk
	zts_disable_local_storage(1);

	// Buffer used to store identity keypair (if someone can read this, they can impersonate your
	// node!)
	char keypair[KEY_BUF_LEN];
	memset(keypair, 0, KEY_BUF_LEN);

	printf("\n\nGenerating new identity...\n");
	uint16_t keypair_len = KEY_BUF_LEN;
	zts_generate_orphan_identity(keypair, &keypair_len);
	printf("keypair(len=%d) = [%s]\n", keypair_len, keypair);

	// Verification is not necessary, but could be useful after reading identities from
	// your custom data store.
	printf("\n\nVerifying ident...\n");
	if (zts_verify_identity(keypair)) {
		printf("\tIdentity is valid\n");
	}
	else {
		printf("\tIdentity is invalid\n");
	}

	printf("\n\nStarting node with generated identity...\n");
	zts_start_with_identity(keypair, keypair_len, &on_zts_event, ztServicePort);

	printf("\n\nWaiting for node to come online...\n");
	while (! myNode.online) {
		zts_delay_ms(50);
	}

	printf("\n\nAs a test, copy node's identity keypair back into buffer...\n");
	memset(keypair, 0, KEY_BUF_LEN);
	keypair_len = KEY_BUF_LEN;
	zts_get_node_identity(keypair, &keypair_len);
	printf("keypair(len=%d) = [%s]\n", keypair_len, keypair);

	// END key handling

	uint64_t nwid = 0x8056c2e21c000001;

	if ((err = zts_join(nwid)) != ZTS_ERR_OK) {
		printf("Unable to join network, error = %d. Exiting.\n", err);
		exit(1);
	}
	printf("Joining network %llx\n", nwid);
	while (! myNode.joinedAtLeastOneNetwork) {
		zts_delay_ms(50);
	}

	// Idle and just show callback events, stack statistics, etc

	printf("Node will now idle...\n");
	while (true) {
		zts_delay_ms(1000);
	}

	// Shut down service and stack threads

	zts_stop();
	return 0;
}
