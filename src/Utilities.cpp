/*
 * Copyright (c)2013-2021 ZeroTier, Inc.
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file in the project's root directory.
 *
 * Change Date: 2026-01-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2.0 of the Apache License.
 */
/****/

#include "Utilities.hpp"

#include "ZeroTierSockets.h"

#include <netinet/in.h>
#include <string.h>

#ifdef __WINDOWS__
#include <windows.h>
#elif _POSIX_C_SOURCE >= 199309L
#include <time.h>   // for nanosleep
#else
#include <unistd.h>   // for usleep
#endif

#ifdef __cplusplus
extern "C" {
#endif

int zts_util_get_ip_family(const char* ipstr)
{
	if (! ipstr) {
		return ZTS_ERR_ARG;
	}
	int family = -1;
	struct zts_sockaddr_in sa4;
	if (zts_inet_pton(ZTS_AF_INET, ipstr, &(sa4.sin_addr)) == 1) {
		family = ZTS_AF_INET;
	}
	struct zts_sockaddr_in6 sa6;
	if (zts_inet_pton(ZTS_AF_INET6, ipstr, &(sa6.sin6_addr)) == 1) {
		family = ZTS_AF_INET6;
	}
	return family;
}

void zts_util_delay(long milliseconds)
{
#ifdef __WINDOWS__
	Sleep(milliseconds);
#elif _POSIX_C_SOURCE >= 199309L
	struct timespec ts;
	ts.tv_sec = milliseconds / 1000;
	ts.tv_nsec = (milliseconds % 1000) * 1000000;
	nanosleep(&ts, NULL);
#else
	usleep(milliseconds * 1000);
#endif
}

void native_ss_to_zts_ss(struct zts_sockaddr_storage* ss_out, const struct sockaddr_storage* ss_in)
{
	if (ss_in->ss_family == AF_INET) {
		struct sockaddr_in* s_in4 = (struct sockaddr_in*)ss_in;
		struct zts_sockaddr_in* d_in4 = (struct zts_sockaddr_in*)ss_out;
#ifndef __WINDOWS__
		d_in4->sin_len = 0;   // s_in4->sin_len;
#endif
		d_in4->sin_family = ZTS_AF_INET;
		d_in4->sin_port = s_in4->sin_port;
		memcpy(&(d_in4->sin_addr), &(s_in4->sin_addr), sizeof(s_in4->sin_addr));
	}
	if (ss_in->ss_family == AF_INET6) {
		struct sockaddr_in6* s_in6 = (struct sockaddr_in6*)ss_in;
		struct zts_sockaddr_in6* d_in6 = (struct zts_sockaddr_in6*)ss_out;
#ifndef __WINDOWS__
		d_in6->sin6_len = 0;   // s_in6->sin6_len;
#endif
		d_in6->sin6_family = ZTS_AF_INET6;
		d_in6->sin6_port = s_in6->sin6_port;
		d_in6->sin6_flowinfo = s_in6->sin6_flowinfo;
		memcpy(&(d_in6->sin6_addr), &(s_in6->sin6_addr), sizeof(s_in6->sin6_addr));
		d_in6->sin6_scope_id = s_in6->sin6_scope_id;
	}
}

#ifdef __cplusplus
}
#endif
