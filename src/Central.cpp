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

#ifdef ZTS_ENABLE_CENTRAL_API

#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include <iomanip>
#include <iostream>

#include "Mutex.hpp"
#include "Debug.hpp"
#include "ZeroTierSockets.h"

char api_url[ZTS_CENRTAL_MAX_URL_LEN];
char api_token[ZTS_CENTRAL_TOKEN_LEN+1];

char *_resp_buf;
int _resp_buf_len;
int _resp_buf_offset;

static int8_t _access_modes;
static int8_t _bIsVerbose;
static int8_t _bInit;

using namespace ZeroTier;

Mutex _responseBuffer_m;

#ifdef __cplusplus
extern "C" {
#endif

size_t on_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
	DEBUG_INFO("buf=%p,size=%zu,nmemb=%zu,userp=%p", buffer, size, nmemb, userp);
	int byte_count = (size * nmemb);
	if (_resp_buf_offset + byte_count >= _resp_buf_len) {
		DEBUG_ERROR("Out of buffer space. Cannot store response from server");
		return 0; // Signal to libcurl that our buffer is full (triggers a write error.)
	}
	memcpy(_resp_buf+_resp_buf_offset, buffer, byte_count);
	_resp_buf_offset += byte_count;
	return byte_count;
}

void zts_central_set_access_mode(int8_t modes)
{
	_access_modes = modes;
}

void zts_central_set_verbose(int8_t is_verbose)
{
	_bIsVerbose = is_verbose;
}

void zts_central_clear_resp_buf()
{
	Mutex::Lock _l(_responseBuffer_m);
	memset(_resp_buf, 0, _resp_buf_len);
	_resp_buf_offset = 0;
}

int zts_central_init(
	const char *url_str, const char *token_str, char *resp_buf, uint32_t resp_buf_len)
{
	_access_modes = ZTS_CENTRAL_READ; // Defauly read-only
	_bIsVerbose = 0; // Default disable libcurl verbose output
	Mutex::Lock _l(_responseBuffer_m);
	if (resp_buf_len == 0) {
		return ZTS_ERR_ARG;
	}
	_resp_buf = resp_buf;
	_resp_buf_len = resp_buf_len;
	_resp_buf_offset = 0;
	// Initialize all curl internal submodules
	curl_global_init(CURL_GLOBAL_ALL);

	int url_len = strlen(url_str);
	if (url_len < 3 || url_len > ZTS_CENRTAL_MAX_URL_LEN) {
		return ZTS_ERR_ARG;
	} else {
		memset(api_url, 0, ZTS_CENRTAL_MAX_URL_LEN);
		memcpy(api_url, url_str, url_len);
	}
	int token_len = strlen(token_str);
	if (token_len != ZTS_CENTRAL_TOKEN_LEN) {
		return ZTS_ERR_ARG;
	} else {
		memset(api_token, 0, ZTS_CENTRAL_TOKEN_LEN);
		memcpy(api_token, token_str, token_len);
	}
	_bInit = true;
	return ZTS_ERR_OK;
}

void zts_central_cleanup()
{
	curl_global_cleanup();
}

int _central_req(int request_type, char *central_str,
	char *api_route_str, char *token_str, int *response_code, char *post_data)
{
	int err = ZTS_ERR_OK;
	if (!_bInit) {
		DEBUG_ERROR("Error: Central API must be initialized first. Call zts_central_init()");
		return ZTS_ERR_SERVICE;
	}
	if (request_type == ZTS_HTTP_GET && !(_access_modes & ZTS_CENTRAL_READ)) {
		DEBUG_ERROR("Error: Incorrect access mode. Need (ZTS_CENTRAL_READ) permission");
		return ZTS_ERR_SERVICE;
	}
	if (request_type == ZTS_HTTP_POST && !(_access_modes & ZTS_CENTRAL_WRITE)) {
		DEBUG_ERROR("Error: Incorrect access mode. Need (ZTS_CENTRAL_WRITE) permission");
		return ZTS_ERR_SERVICE;
	}
	zts_central_clear_resp_buf();
	int central_strlen = strlen(central_str);
	int api_route_strlen = strlen(api_route_str);
	int token_strlen = strlen(token_str);
	int url_len = central_strlen + api_route_strlen;
	if (token_strlen > ZTS_CENTRAL_TOKEN_LEN) {
		return ZTS_ERR_ARG;
	}
	if (url_len > ZTS_CENRTAL_MAX_URL_LEN) {
		return ZTS_ERR_ARG;
	}
	char req_url[ZTS_CENRTAL_MAX_URL_LEN];
	strcpy(req_url, central_str);
	strcat(req_url, api_route_str);

	CURL *curl;
	CURLcode res;
	curl = curl_easy_init();
	if (!curl) {
		return ZTS_ERR_GENERAL;
	}

	struct curl_slist *hs=NULL;
	char auth_str[ZTS_CENTRAL_TOKEN_LEN + 32];
	if (token_strlen == ZTS_CENTRAL_TOKEN_LEN) {
		memset(auth_str, 0, ZTS_CENTRAL_TOKEN_LEN + 32);
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

	if (request_type == ZTS_HTTP_GET) {
		// Nothing
		DEBUG_INFO("Request (GET) = %s", api_route_str);
	}
	if (request_type == ZTS_HTTP_POST) {
		DEBUG_INFO("Request (POST) = %s", api_route_str);
		if (post_data) {
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
		}
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
	}
	if (request_type == ZTS_HTTP_DELETE) {
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
		DEBUG_INFO("Req. took %f second(s). HTTP code (%ld)", elapsed_time, hrc);
		*response_code = hrc;
		//curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url);
	} else {
		DEBUG_ERROR("%s", curl_easy_strerror(res));
		err = ZTS_ERR_SERVICE;
	}
	curl_easy_cleanup(curl);
	return err;
}

int zts_get_last_resp_buf(char *dest_buffer, int dest_buf_len)
{
	if (dest_buf_len <= _resp_buf_offset) {
		return ZTS_ERR_ARG;
	}
	int sz = dest_buf_len < _resp_buf_len ? dest_buf_len : _resp_buf_len;
	memcpy(dest_buffer, _resp_buf, sz);
	return ZTS_ERR_OK;
}

int zts_central_get_status(int *resp_code)
{
	return _central_req(
		ZTS_HTTP_GET, api_url, (char*)"/api/status", api_token, resp_code, NULL);
}

int zts_central_get_self(int *resp_code)
{
	return _central_req(
		ZTS_HTTP_GET, api_url, (char*)"/api/self", api_token, resp_code, NULL);
}

int zts_central_get_network(int *resp_code, uint64_t nwid)
{
	char req[64];
	sprintf(req, "/api/network/%llx", nwid);
	return _central_req(
		ZTS_HTTP_GET, api_url, req, api_token, resp_code, NULL);
}

int zts_central_update_network(int *resp_code, uint64_t nwid)
{
	char req[64];
	sprintf(req, "/api/network/%llx", nwid);
	return _central_req(
		ZTS_HTTP_POST, api_url, req, api_token, resp_code, NULL);
}

int zts_central_delete_network(int *resp_code, uint64_t nwid)
{
	char req[64];
	sprintf(req, "/api/network/%llx", nwid);
	return _central_req(
		ZTS_HTTP_DELETE, api_url, req, api_token, resp_code, NULL);
}

int zts_central_get_networks(int *resp_code)
{
	return _central_req(
		ZTS_HTTP_GET, api_url, (char*)"/api/network", api_token, resp_code, NULL);
}

int zts_central_get_member(int *resp_code, uint64_t nwid, uint64_t nodeid)
{
	if (nwid == 0 || nodeid == 0) {
		return ZTS_ERR_ARG;
	}
	char req[64];
	sprintf(req, "/api/network/%llx/member/%llx", nwid, nodeid);
	return _central_req(
		ZTS_HTTP_GET, api_url, req, api_token, resp_code, NULL);
}

int zts_central_update_member(
	int *resp_code, uint64_t nwid, uint64_t nodeid, char *post_data)
{
	if (nwid == 0 || nodeid == 0 || post_data == NULL) {
		return ZTS_ERR_ARG;
	}
	char req[64];
	sprintf(req, "/api/network/%llx/member/%llx", nwid, nodeid);
	return _central_req(
		ZTS_HTTP_POST, api_url, req, api_token, resp_code, post_data);
}

int zts_central_set_node_auth(
	int *resp_code, uint64_t nwid, uint64_t nodeid, uint8_t is_authed)
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
	return zts_central_update_member(resp_code, nwid, nodeid, config_data);
}

int zts_central_get_members_of_network(int *resp_code, uint64_t nwid)
{
	char req[64];
	sprintf(req, "/api/network/%llx/member", nwid);
	return _central_req(
		ZTS_HTTP_GET, api_url, req, api_token, resp_code, NULL);
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // ZTS_ENABLE_CENTRAL_API
#endif // _H
