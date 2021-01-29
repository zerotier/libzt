/*
 * Copyright (c)2013-2021 ZeroTier, Inc.
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file in the project's root directory.
 *
 * Change Date: 2025-01-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2.0 of the Apache License.
 */
/****/

#ifndef ZT_CENTRAL_H
#define ZT_CENTRAL_H

#ifdef CENTRAL_API

#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include <iomanip>
#include <iostream>

#include "Mutex.hpp"
#include "Debug.hpp"
#include "ZeroTierSockets.h"

char central_api_url[CENRTAL_API_MAX_URL_LEN];
char central_api_token[CENTRAL_API_TOKEN_LEN+1];

char *_response_buffer;
int _response_buffer_len;
int _response_buffer_offset;

static int8_t _api_access_modes;
static int8_t _bIsVerbose;
static int8_t _bInit;

using namespace ZeroTier;

Mutex _responseBuffer_m;

#ifdef __cplusplus
extern "C" {
#endif

size_t on_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
	DEBUG_INFO("buffer=%p, size=%zu, nmemb=%zu, userp=%p", buffer, size, nmemb, userp);
	int byte_count = (size * nmemb);
	if (_response_buffer_offset + byte_count >= _response_buffer_len) {
		DEBUG_ERROR("Out of buffer space. Cannot store response from server");
		return 0; // Signal to libcurl that our buffer is full (triggers a write error.)
	}
	memcpy(_response_buffer+_response_buffer_offset, buffer, byte_count);
	_response_buffer_offset += byte_count;
	return byte_count;
}

void zts_central_api_set_access(int8_t modes)
{
	_api_access_modes = modes;
}

void zts_central_api_set_verbose(int8_t is_verbose)
{
	_bIsVerbose = is_verbose;
}

void zts_central_api_clear_response_buffer()
{
	Mutex::Lock _l(_responseBuffer_m);
	memset(_response_buffer, 0, _response_buffer_len);
	_response_buffer_offset = 0;
}

int zts_central_api_init(const char *url_str, const char *token_str, char *response_buffer, uint32_t response_buffer_len)
{
	_api_access_modes = ZTS_CENTRAL_READ; // Defauly read-only
	_bIsVerbose = 0; // Default disable libcurl verbose output
	Mutex::Lock _l(_responseBuffer_m);
	if (response_buffer_len == 0) {
		return ZTS_ERR_ARG;
	}
	_response_buffer = response_buffer;
	_response_buffer_len = response_buffer_len;
	_response_buffer_offset = 0;
	// Initialize all curl internal submodules
	curl_global_init(CURL_GLOBAL_ALL);

	int url_len = strlen(url_str);
	if (url_len < 3 || url_len > CENRTAL_API_MAX_URL_LEN) {
		return ZTS_ERR_ARG;
	} else {
		memset(central_api_url, 0, CENRTAL_API_MAX_URL_LEN);
		memcpy(central_api_url, url_str, url_len);
	}
	int token_len = strlen(token_str);
	if (token_len != CENTRAL_API_TOKEN_LEN) {
		return ZTS_ERR_ARG;
	} else {
		memset(central_api_token, 0, CENTRAL_API_TOKEN_LEN);
		memcpy(central_api_token, token_str, token_len);
	}
	_bInit = true;
	return ZTS_ERR_OK;
}

void zts_central_api_cleanup()
{
	curl_global_cleanup();
}

int _central_req(int request_type, char *central_api_str,
	char *api_route_str, char *token_str, int *response_code, char *post_data)
{
	int err = ZTS_ERR_OK;
	if (!_bInit) {
		DEBUG_ERROR("Error: Central API must be initialized first. Call zts_central_api_init()");
		return ZTS_ERR_SERVICE;
	}
	if (request_type == HTTP_GET && !(_api_access_modes & ZTS_CENTRAL_READ)) {
		DEBUG_ERROR("Error: Incorrect access mode. Need (ZTS_CENTRAL_READ) permission");
		return ZTS_ERR_SERVICE;
	}
	if (request_type == HTTP_POST && !(_api_access_modes & ZTS_CENTRAL_WRITE)) {
		DEBUG_ERROR("Error: Incorrect access mode. Need (ZTS_CENTRAL_WRITE) permission");
		return ZTS_ERR_SERVICE;
	}
	zts_central_api_clear_response_buffer();
	int central_api_strlen = strlen(central_api_str);
	int api_route_strlen = strlen(api_route_str);
	int token_strlen = strlen(token_str);
	int url_len = central_api_strlen + api_route_strlen;
	if (token_strlen > CENTRAL_API_TOKEN_LEN) {
		return ZTS_ERR_ARG;
	}
	if (url_len > CENRTAL_API_MAX_URL_LEN) {
		return ZTS_ERR_ARG;
	}
	char req_url[CENRTAL_API_MAX_URL_LEN];
	strcpy(req_url, central_api_str);
	strcat(req_url, api_route_str);

	CURL *curl;
	CURLcode res;
	curl = curl_easy_init();
	if (!curl) {
		return ZTS_ERR_GENERAL;
	}

	struct curl_slist *hs=NULL;
	char auth_str[CENTRAL_API_TOKEN_LEN + 32];
	if (token_strlen == CENTRAL_API_TOKEN_LEN) {
		memset(auth_str, 0, CENTRAL_API_TOKEN_LEN + 32);
		sprintf(auth_str, "Authorization: Bearer %s", token_str);
	}

	hs = curl_slist_append(hs, auth_str);
	hs = curl_slist_append(hs, "Content-Type: application/json");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
	curl_easy_setopt(curl, CURLOPT_URL, req_url);
	// example.com is redirected, so we tell libcurl to follow redirection 
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	if (_bIsVerbose) {
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
	}
	// Tell curl to use our write function
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, on_data);

	if (request_type == HTTP_GET) {
		// Nothing
		DEBUG_INFO("Request (GET) = %s", api_route_str);
	}
	if (request_type == HTTP_POST) {
		DEBUG_INFO("Request (POST) = %s", api_route_str);
		if (post_data) {
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
		}
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
	}
	if (request_type == HTTP_DELETE) {
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
	}
	//curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L); // Consider 400-500 series code as failures
	// Perform request
	res = curl_easy_perform(curl);
	if(res == CURLE_OK) {
		//char* url;
		double elapsed_time = 0.0;
		long hrc = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &hrc);
		curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &elapsed_time);
		DEBUG_INFO("Request took %f second(s). HTTP code (%ld)", elapsed_time, hrc);
		*response_code = hrc;
		//curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url);
	} else {
		DEBUG_ERROR("%s", curl_easy_strerror(res));
		err = ZTS_ERR_SERVICE;
	}
	curl_easy_cleanup(curl);
	return err;
}

int zts_get_last_response_buffer(char *dest_buffer, int dest_buffer_len)
{
	if (dest_buffer_len <= _response_buffer_offset) {
		return ZTS_ERR_ARG;
	}
	int amount_to_copy = dest_buffer_len < _response_buffer_len ? dest_buffer_len : _response_buffer_len;
	memcpy(dest_buffer, _response_buffer, amount_to_copy);
	return ZTS_ERR_OK;
}

int zts_central_api_get_status(int *http_response_code)
{
	return _central_req(HTTP_GET, central_api_url, (char*)"/api/status", central_api_token, http_response_code, NULL);
}

int zts_central_api_get_self(int *http_response_code)
{
	return _central_req(HTTP_GET, central_api_url, (char*)"/api/self", central_api_token, http_response_code, NULL);
}

int zts_central_api_get_network(int *http_response_code, int64_t nwid)
{
	char req[64];
	sprintf(req, "/api/network/%llx", nwid);
	return _central_req(HTTP_GET, central_api_url, req, central_api_token, http_response_code, NULL);
}

int zts_central_api_update_network(int *http_response_code, int64_t nwid)
{
	char req[64];
	sprintf(req, "/api/network/%llx", nwid);
	return _central_req(HTTP_POST, central_api_url, req, central_api_token, http_response_code, NULL);
}

int zts_central_api_delete_network(int *http_response_code, int64_t nwid)
{
	char req[64];
	sprintf(req, "/api/network/%llx", nwid);
	return _central_req(HTTP_DELETE, central_api_url, req, central_api_token, http_response_code, NULL);
}

int zts_central_api_get_networks(int *http_response_code)
{
	return _central_req(HTTP_GET, central_api_url, (char*)"/api/network", central_api_token, http_response_code, NULL);
}

int zts_central_api_get_member(int *http_response_code, int64_t nwid, int64_t nodeid)
{
	if (nwid == 0 || nodeid == 0) {
		return ZTS_ERR_ARG;
	}
	char req[64];
	sprintf(req, "/api/network/%llx/member/%llx", nwid, nodeid);
	return _central_req(HTTP_GET, central_api_url, req, central_api_token, http_response_code, NULL);
}

int zts_central_api_update_member(int *http_response_code, int64_t nwid, int64_t nodeid, char *post_data)
{
	if (nwid == 0 || nodeid == 0 || post_data == NULL) {
		return ZTS_ERR_ARG;
	}
	char req[64];
	sprintf(req, "/api/network/%llx/member/%llx", nwid, nodeid);
	return _central_req(HTTP_POST, central_api_url, req, central_api_token, http_response_code, post_data);
}

int zts_set_node_auth(int *http_response_code, int64_t nwid, int64_t nodeid, int8_t is_authed)
{
	if (is_authed != 0 && is_authed != 1) {
		return ZTS_ERR_ARG;
	}
	char config_data[64];
	if (is_authed == ZTS_CENTRAL_NODE_AUTH_TRUE) {
		sprintf(config_data, "{\"config\": {\"authorized\": true} }");
	}
	if (is_authed == ZTS_CENTRAL_NODE_AUTH_FALSE) {
		sprintf(config_data, "{\"config\": {\"authorized\": false} }");
	}
	return zts_central_api_update_member(http_response_code, nwid, nodeid, config_data);
}

int zts_central_api_get_members_of_network(int *http_response_code, int64_t nwid)
{
	char req[64];
	sprintf(req, "/api/network/%llx/member", nwid);
	return _central_req(HTTP_GET, central_api_url, req, central_api_token, http_response_code, NULL);
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // NO_CENTRAL_API
#endif // _H
