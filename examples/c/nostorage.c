/**
 * libzt C API example — Identity handling without local storage
 *
 * Demonstrates how to manage a ZeroTier node identity (public/secret keypair)
 * and its state via storage events instead of zts_init_from_storage(). This is
 * what you want on embedded targets, or anywhere else without a writable
 * filesystem: the library hands you each object to cache and you supply it back
 * on the next run.
 *
 * WARNING: This prints secret keys to your terminal.
 *
 * Usage:
 *   nostorage
 *
 *   (this example takes no arguments)
 *
 * This example joins no network; it only starts a node and idles so that the
 * store events can be observed. Note also that it caches all five store-event
 * types into a single shared buffer, so each event overwrites the previous one.
 * It demonstrates the mechanism only — a real implementation needs one buffer
 * (or storage record) per object type.
 *
 * Press Ctrl-C to shut down cleanly; zts_node_stop() runs before exit.
 *
 * Build from the repository root:
 *   ./build.sh host "release"
 * Binaries are written to dist/<platform>-<arch>-host-release/bin/
 */

#include "ZeroTierSockets.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// NOTE: The five store events handled below all share this single buffer, so
// each event overwrites whatever the previous one cached. This example only
// demonstrates the caching mechanism; a real implementation needs a separate
// buffer (or storage record) per object type.
char cache_data[ZTS_STORE_DATA_LEN];

static volatile sig_atomic_t keep_running = 1;
static void handle_sigint(int sig)
{
    (void)sig;
    keep_running = 0;
}

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

    // cache_data is exactly ZTS_STORE_DATA_LEN bytes. Truncate (with a
    // warning) if the payload doesn't fit.
    if (len > ZTS_STORE_DATA_LEN) {
        printf("Warning: event payload (len=%d) exceeds cache buffer (%d bytes). Truncating.\n", len, ZTS_STORE_DATA_LEN);
        len = ZTS_STORE_DATA_LEN;
    }

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

static void usage(const char* prog)
{
    printf("\nlibzt example: Identity handling without local storage\n\n");
    printf("Usage: %s\n\n", prog);
    printf("  (this example takes no arguments)\n");
    printf("\nWARNING: this prints secret keys to your terminal. It joins no network, and it\n");
    printf("caches every store-event type into one shared buffer, so it demonstrates the\n");
    printf("mechanism only. Press Ctrl-C to shut down cleanly.\n");
}

int main(int argc, char** argv)
{
    // Line-buffer stdout so progress output appears immediately when it is
    // redirected to a file or pipe (docker logs, systemd, CI) rather than
    // sitting in the 4K stdio buffer.
    setvbuf(stdout, NULL, _IOLBF, 0);
    if (argc > 1) {
        int help = (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0);
        usage(argv[0]);
        return help ? 0 : 1;
    }
    signal(SIGINT, handle_sigint);

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

    printf("Node %010llx is now online. Idling.\n", (unsigned long long)zts_node_get_id());
    while (keep_running) {
        zts_util_delay(500);   // Idle until Ctrl-C
    }

    printf("Stopping node\n");
    return zts_node_stop();
}
