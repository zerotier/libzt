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

#ifndef ZTS_UTILITIES_HPP
#define ZTS_UTILITIES_HPP

struct zts_sockaddr_storage;

#ifdef __cplusplus
extern "C" {
#endif

void native_ss_to_zts_ss(struct zts_sockaddr_storage* ss_out, const struct sockaddr_storage* ss_in);

#ifdef __cplusplus
}
#endif

#endif   // ZTS_UTILITIES_HPP
