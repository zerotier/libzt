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

#include <node/ECC.hpp>
#include <node/World.hpp>
#include <osdep/OSUtils.hpp>

#ifdef __WINDOWS__
#include <windows.h>
#elif _POSIX_C_SOURCE >= 199309L
#include <time.h>   // for nanosleep
#else
#include <unistd.h>   // for usleep
#endif

// The public header cannot include ZeroTierOne's internal headers, so these
// constants are duplicated there. Fail the build if they ever drift apart.
static_assert(
    ZTS_SIGNING_KEYPAIR_LEN == (ZT_ECC_PUBLIC_KEY_SET_LEN + ZT_ECC_PRIVATE_KEY_SET_LEN),
    "ZTS_SIGNING_KEYPAIR_LEN is out of sync with ZeroTierOne's ECC key sizes");
static_assert(
    ZTS_ROOT_SET_BUF_LEN >= ZT_WORLD_MAX_SERIALIZED_LENGTH,
    "ZTS_ROOT_SET_BUF_LEN is smaller than ZT_WORLD_MAX_SERIALIZED_LENGTH");

namespace ZeroTier {

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

void zts_util_delay(unsigned long milliseconds)
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

int zts_util_sign_root_set(
    char* roots_out,
    unsigned int* roots_len,
    char* prev_key,
    unsigned int* prev_key_len,
    char* curr_key,
    unsigned int* curr_key_len,
    uint64_t id,
    uint64_t ts,
    zts_root_set_t* roots_spec)
{
    if (! roots_spec || ! roots_out || ! roots_len || ! prev_key || ! curr_key || ! prev_key_len
        || ! curr_key_len) {
        return ZTS_ERR_ARG;
    }
    // The *_len parameters are in/out: on input they carry the capacity of the
    // corresponding output buffer, on success they are overwritten with the number
    // of bytes actually written. Without the input capacity there is no way to bound
    // the copies at the end of this function.
    // The key buffers have a fixed known size so they can be checked up front. The
    // root set is variable-length, so it is checked against its capacity just
    // before the copy below.
    const unsigned int keyPairLen = ZT_ECC_PUBLIC_KEY_SET_LEN + ZT_ECC_PRIVATE_KEY_SET_LEN;
    if (*prev_key_len < keyPairLen || *curr_key_len < keyPairLen) {
        return ZTS_ERR_ARG;
    }
    // Generate signing keys
    std::string previous, current;
    if ((! OSUtils::readFile("previous.c25519", previous)) || (! OSUtils::readFile("current.c25519", current))) {
        ECC::Pair np(ECC::generate());
        previous = std::string();
        previous.append((const char*)np.pub.data, ZT_ECC_PUBLIC_KEY_SET_LEN);
        previous.append((const char*)np.priv.data, ZT_ECC_PRIVATE_KEY_SET_LEN);
        current = previous;
    }
    if ((previous.length() != keyPairLen) || (current.length() != keyPairLen)) {
        // Previous.c25519 or current.c25519 empty or invalid
        return ZTS_ERR_ARG;
    }
    ECC::Pair previousKP;
    memcpy(previousKP.pub.data, previous.data(), ZT_ECC_PUBLIC_KEY_SET_LEN);
    memcpy(previousKP.priv.data, previous.data() + ZT_ECC_PUBLIC_KEY_SET_LEN, ZT_ECC_PRIVATE_KEY_SET_LEN);
    ECC::Pair currentKP;
    memcpy(currentKP.pub.data, current.data(), ZT_ECC_PUBLIC_KEY_SET_LEN);
    memcpy(currentKP.priv.data, current.data() + ZT_ECC_PUBLIC_KEY_SET_LEN, ZT_ECC_PRIVATE_KEY_SET_LEN);

    // Set up roots definition
    std::vector<World::Root> roots;
    for (int i = 0; i < ZTS_MAX_NUM_ROOTS; i++) {
        if (! roots_spec->public_id_str[i]) {
            break;
        }
        if (strnlen(roots_spec->public_id_str[i], ZT_IDENTITY_STRING_BUFFER_LENGTH)) {
            // printf("id = %s\n", roots_spec->public_id_str[i]);
            roots.push_back(World::Root());
            roots.back().identity = Identity(roots_spec->public_id_str[i]);
            for (int j = 0; j < ZTS_MAX_ENDPOINTS_PER_ROOT; j++) {
                if (! roots_spec->endpoint_ip_str[i][j]) {
                    break;
                }
                if (strnlen(roots_spec->endpoint_ip_str[i][j], ZTS_MAX_ENDPOINT_STR_LEN)) {
                    roots.back().stableEndpoints.push_back(InetAddress(roots_spec->endpoint_ip_str[i][j]));
                    // printf(" ep = %s\n", roots_spec->endpoint_ip_str[i][j]);
                }
            }
        }
    }

    // Generate
    World nw = World::make(World::TYPE_PLANET, id, ts, currentKP.pub, roots, previousKP);
    // Test
    Buffer<ZT_WORLD_MAX_SERIALIZED_LENGTH> outtmp;
    nw.serialize(outtmp, false);
    World testw;
    testw.deserialize(outtmp, 0);
    if (testw != nw) {
        // Serialization test failed
        return ZTS_ERR_GENERAL;
    }
    // Write output. Capacities were validated against the *_len in-values above.
    if (outtmp.size() > *roots_len) {
        return ZTS_ERR_ARG;
    }
    memcpy(roots_out, (char*)outtmp.data(), outtmp.size());
    *roots_len = outtmp.size();
    memcpy(prev_key, previous.data(), keyPairLen);
    *prev_key_len = keyPairLen;
    memcpy(curr_key, current.data(), keyPairLen);
    *curr_key_len = keyPairLen;
    return ZTS_ERR_OK;
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

}   // namespace ZeroTier
