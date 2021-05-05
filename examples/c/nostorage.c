/**
 * libzt C API example
 *
 * Demonstrates how to manage ZeroTier node identities (public/secret keypairs) without
 * local storage (e.g. zts_init_from_storage().)
 *
 * WARNING: This prints secret keys to your terminal.
 *
 */

#include "ZeroTierSockets.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char cache_data[ZTS_STORE_DATA_LEN];

void on_zts_event(void* msgPtr)
{
    zts_event_msg_t* msg = (zts_event_msg_t*)msgPtr;
    int len = msg->len;   // Length of message (or structure)

    if (msg->event_code == ZTS_EVENT_NODE_ONLINE) {
        printf("ZTS_EVENT_NODE_ONLINE\n");
    }

    // Copy data to a buffer that you have allocated or write it to storage.
    // The data pointed to by msg->cache will be invalid after this function
    // returns.

    memset(cache_data, 0, ZTS_STORE_DATA_LEN);

    if (msg->event_code == ZTS_EVENT_STORE_IDENTITY_PUBLIC) {
        printf("ZTS_EVENT_STORE_IDENTITY_PUBLIC (len=%d)\n", msg->len);
        printf("identity.public = [ %.*s ]\n", len, (char*)msg->cache);
        memcpy(cache_data, msg->cache, len);
    }
    if (msg->event_code == ZTS_EVENT_STORE_IDENTITY_SECRET) {
        printf("ZTS_EVENT_STORE_IDENTITY_SECRET (len=%d)\n", msg->len);
        printf("identity.secret = [ %.*s ]\n", len, (char*)msg->cache);
        memcpy(cache_data, msg->cache, len);
        // Same data can be retrieved via: zts_node_get_id_pair()
    }
    if (msg->event_code == ZTS_EVENT_STORE_PLANET) {
        printf("ZTS_EVENT_STORE_PLANET (len=%d)\n", msg->len);
        // Binary data
        memcpy(cache_data, msg->cache, len);
    }
    if (msg->event_code == ZTS_EVENT_STORE_PEER) {
        printf("ZTS_EVENT_STORE_PEER (len=%d)\n", msg->len);
        // Binary data
        memcpy(cache_data, msg->cache, len);
    }
    if (msg->event_code == ZTS_EVENT_STORE_NETWORK) {
        printf("ZTS_EVENT_STORE_NETWORK (len=%d)\n", msg->len);
        // Binary data
        memcpy(cache_data, msg->cache, len);
    }
}

int main(int argc, char** argv)
{
    int err = ZTS_ERR_OK;

    // Initialize node

    zts_init_set_event_handler(&on_zts_event);

    // Start node

    printf("Starting node...\n");
    int generate_new_id = 1;
    if (generate_new_id) {
        // OPTION A
        // Generate new automatically ID if no prior init called
        zts_node_start();
    }
    else {
        // OPTION B
        // Copy your key here
        char identity[ZTS_ID_STR_BUF_LEN] = { 0 };
        int len = ZTS_ID_STR_BUF_LEN;

        // Generate key (optional):
        //   int key_len;
        //   zts_id_new(identity, &key_len);

        // Load pre-existing identity from buffer
        zts_init_from_memory(identity, len);
        zts_node_start();
    }

    printf("Waiting for node to come online\n");
    while (! zts_node_is_online()) {
        zts_util_delay(50);
    }

    // Do network stuff!
    // zts_bsd_socket, zts_bsd_connect, etc

    printf("Node %llx is now online. Idling.\n", zts_node_get_id());
    while (1) {
        zts_util_delay(500);   // Idle indefinitely
    }

    printf("Stopping node\n");
    return zts_node_stop();
}
