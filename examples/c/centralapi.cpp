/**
 * libzt C API example — ZeroTier Central web API client
 *
 * Queries the ZeroTier Central web API: hosted service status, network
 * configuration, and member authorization. This portion of the libzt API is a
 * thin wrapper around the web API at https://my.zerotier.com/help/api, with the
 * HTTP requests performed by libcurl.
 *
 * Usage:
 *   centralapi <central_url> <api_token>
 *
 *   <central_url>   API endpoint, e.g. https://my.zerotier.com
 *   <api_token>     User API token, generate one at my.zerotier.com
 *
 * This example does NOT start a ZeroTier node and joins no network — it is a
 * pure REST client, so no identity storage or network authorization is
 * involved. It is also only compiled when the Central API is enabled, which is
 * NOT the default: configure with cmake -DZTS_DISABLE_CENTRAL_API=OFF, which
 * requires libcurl to be installed.
 *
 * Build from the repository root:
 *   ./build.sh host "release"
 * Binaries are written to dist/<platform>-<arch>-host-release/bin/
 */

#include "ZeroTierSockets.h"

#include <iomanip>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

// For optional JSON parsing
#include "../../ext/ZeroTierOne/ext/nlohmann/json.hpp"

void process_response(char* response, int http_resp_code)
{
	if (http_resp_code == 0) {
		// Request failed at library level, do nothing. There would be no HTTP code at this point.
		return;
	}
	printf("Raw response string (%d) = %s\n", http_resp_code, response);
	// Parse into navigable JSON object
	if (http_resp_code < 200 || http_resp_code >= 300) {
		return;
	}
	nlohmann::json res = nlohmann::json::parse(response);
	if (! res.is_object()) {
		fprintf(stderr, "Unable to parse (root element is not a JSON object)");
	}
	// Pretty print JSON blob
	std::cout << std::setw(4) << res << std::endl;
}

static void usage(const char* prog)
{
	printf("\nlibzt example: ZeroTier Central web API client\n\n");
	printf("Usage: %s <central_url> <api_token>\n\n", prog);
	printf("  <central_url>   API endpoint, e.g. https://my.zerotier.com\n");
	printf("  <api_token>     User API token, generate one at my.zerotier.com\n");
	printf("\nThis example does not start a node and joins no network; it is a pure REST\n");
	printf("client. It is only built when the Central API is enabled (cmake\n");
	printf("-DZTS_DISABLE_CENTRAL_API=OFF), which requires libcurl.\n");
}

int main(int argc, char** argv)
{
    // Line-buffer stdout so progress output appears immediately when it is
    // redirected to a file or pipe (docker logs, systemd, CI) rather than
    // sitting in the 4K stdio buffer.
    setvbuf(stdout, NULL, _IOLBF, 0);
	if (argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
		usage(argv[0]);
		return 0;
	}
	if (argc != 3) {
		usage(argv[0]);
		return 1;
	}
	char* central_url = argv[1];   // API endpoint
	char* api_token = argv[2];     // User token (generate at my.zerotier.com)

	/**
	 * This example demonstrates how to use the ZeroTier Central API to:
	 *
	 *  - Get the status of our hosted service (or your own)
	 *  - Create a network
	 *  - Get the full configuration of a network
	 *  - Authorize/Deauthorize nodes on a network
	 *
	 * This example does not start a node (though you can if you wish.) This portion of the
	 * libzt API is merely a wrapper around our web API endpoint (https://my.zerotier.com/help/api).
	 * The HTTP requests are done via libcurl. This API is thread-safe but not multi-threaded.
	 *
	 * Error Codes:
	 *         -2 : [ZTS_ERR_SERVICE] The API may not have been initialized properly
	 *         -3 : [ZTS_ERR_ARG] Invalid argument
	 *  [100-500] : Standard HTTP error codes
	 *
	 * Usage example: centralapi https://my.zerotier.com e7no7nVRFItge7no7cVR5Ibge7no8nV1
	 *
	 */

	int err = ZTS_ERR_OK;
	// Buffer to store server response as JSON string blobs
	char rbuf[ZTS_CENTRAL_RESP_BUF_DEFAULT_SZ] = { 0 };

	// Provide URL to Central API server and user API token generated at https://my.zerotier.com
	printf("Initializing Central API client...\n");
	if ((err = zts_central_init(central_url, api_token, rbuf, ZTS_CENTRAL_RESP_BUF_DEFAULT_SZ))
	    != ZTS_ERR_OK) {
		fprintf(stderr, "Error while initializing client's Central API parameters\n");
		return 0;
	}

	zts_central_set_verbose(false);   // (optional) Turn on reporting from libcurl
	zts_central_set_access_mode(ZTS_CENTRAL_READ /*| ZTS_CENTRAL_WRITE*/);

	int http_res_code = 0;

	// Get hosted service status
	printf("Requesting Central API server status (/api/status):\n");
	if ((err = zts_central_status_get(&http_res_code)) != ZTS_ERR_OK) {
		fprintf(stderr, "Error (%d) making the request.\n", err);
	}
	else {
		process_response(rbuf, http_res_code);
	}
	// Get network config
	int64_t nwid = 0x1234567890abcdef;
	printf("Requesting network config: /api/network/%llx\n", nwid);
	if ((err = zts_central_net_get(&http_res_code, nwid)) != ZTS_ERR_OK) {
		fprintf(stderr, "Error (%d) making the request.\n", err);
	}
	else {
		process_response(rbuf, http_res_code);
	}
	// Authorize a node on a network
	int64_t nodeid = 0x9934343434;
	printf("Authorizing: /api/network/%llx/member/%llx\n", nwid, nodeid);
	if ((err = zts_central_node_auth(&http_res_code, nwid, nodeid, ZTS_CENTRAL_NODE_AUTH_TRUE))
	    != ZTS_ERR_OK) {
		fprintf(stderr, "Error (%d) making the request.\n", err);
	}
	else {
		process_response(rbuf, http_res_code);
	}

	return 0;
}
