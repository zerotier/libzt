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

/**
 * @file
 *
 * Debug macros
 */

#ifndef ZTS_DEBUG_HPP
#define ZTS_DEBUG_HPP

#include <string.h>

#define ZT_COLOR true

// Debug output colors
#if defined(__APPLE__)
#include "TargetConditionals.h"
#endif
#if defined(ZT_COLOR) && ! defined(_WIN32) && ! defined(__ANDROID__) && ! defined(TARGET_OS_IPHONE)                    \
    && ! defined(TARGET_IPHONE_SIMULATOR) && ! defined(__APP_FRAMEWORK__)
#define ZT_RED   "\x1B[31m"
#define ZT_GRN   "\x1B[32m"
#define ZT_YEL   "\x1B[33m"
#define ZT_BLU   "\x1B[34m"
#define ZT_MAG   "\x1B[35m"
#define ZT_CYN   "\x1B[36m"
#define ZT_WHT   "\x1B[37m"
#define ZT_RESET "\x1B[0m"
#else
#define ZT_RED
#define ZT_GRN
#define ZT_YEL
#define ZT_BLU
#define ZT_MAG
#define ZT_CYN
#define ZT_WHT
#define ZT_RESET
#endif

#define ZT_FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)   // short

#if defined(__ANDROID__)
#include <android/log.h>
#define ZT_LOG_TAG "ZTSDK"
#endif

#if defined(LIBZT_DEBUG)
#if defined(__ANDROID__)
#define DEBUG_INFO(fmt, args...)                                                                                       \
    ((void)__android_log_print(                                                                                        \
        ANDROID_LOG_VERBOSE,                                                                                           \
        ZT_LOG_TAG,                                                                                                    \
        "%17s:%5d:%20s: " fmt "\n",                                                                                    \
        ZT_FILENAME,                                                                                                   \
        __LINE__,                                                                                                      \
        __FUNCTION__,                                                                                                  \
        ##args))
#elif defined(_WIN32)
#define DEBUG_INFO(fmt, ...)                                                                                           \
    fprintf(stderr, ZT_WHT "%17s:%5d:%25s: " fmt "\n" ZT_RESET, ZT_FILENAME, __LINE__, __FUNCTION__, __VA_ARGS__)
#else
#define DEBUG_INFO(fmt, args...)                                                                                       \
    fprintf(stderr, ZT_WHT "%17s:%5d:%25s: " fmt "\n" ZT_RESET, ZT_FILENAME, __LINE__, __FUNCTION__, ##args)
#endif
#else   // !LIBZT_DEBUG
#if defined(_WIN32)
#define DEBUG_INFO(...)
#else
#define DEBUG_INFO(fmt, args...)
#endif
#endif

#endif   // _H
