/*
 * ZeroTier SDK - Network Virtualization Everywhere
 * Copyright (C) 2011-2019  ZeroTier, Inc.  https://www.zerotier.com/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * --
 *
 * You can be released from the requirements of the license by purchasing
 * a commercial license. Buying such a license is mandatory as soon as you
 * develop commercial closed-source software that incorporates or links
 * directly against ZeroTier software without disclosing the source code
 * of your own application.
 */

/**
 * @file
 *
 * Useful constants
 */

#ifndef LIBZT_CONSTANTS_HPP
#define LIBZT_CONSTANTS_HPP

//////////////////////////////////////////////////////////////////////////////
// Error codes returned by libzt API                                        //
//////////////////////////////////////////////////////////////////////////////

typedef int zts_err_t;

#define ZTS_ERR_OK 0
#define ZTS_ERR_INVALID_ARG -1 // A parameter provided by the user application is invalid (e.g. our of range, NULL, etc)
#define ZTS_ERR_SERVICE -2 // The service isn't initialized or is for some other reason currently unavailable
#define ZTS_ERR_INVALID_OP -3 // For some reason this API operation is not permitted (perhaps the service is still starting?)

//////////////////////////////////////////////////////////////////////////////
// libzt config                                                             //
//////////////////////////////////////////////////////////////////////////////

/**
 * Default port that libzt will use to support all virtual communication
 */
#define ZTS_DEFAULT_PORT 9994

/**
 * Maximum port number allowed
 */
#define ZTS_MAX_PORT 65535

/**
 * For layer-2 only (this will omit all user-space network stack code)
 */
#define ZTS_NO_STACK 0

/**
 * How fast service states are re-checked (in milliseconds)
 */
#define ZTS_WRAPPER_CHECK_INTERVAL 50

/**
 * By how much thread I/O and callback loop delays are multiplied (unitless)
 */
#define ZTS_HIBERNATION_MULTIPLIER 50

/**
 * Maximum allowed number of networks joined to concurrently
 */
#define ZTS_MAX_JOINED_NETWORKS 64

/**
 * Maximum address assignments per network
 */
#define ZTS_MAX_ASSIGNED_ADDRESSES 16

/**
 * Maximum routes per network
 */
#define ZTS_MAX_NETWORK_ROUTES 32

/**
 * Length of buffer required to hold a ztAddress/nodeID
 */
#define ZTS_ID_LEN 16

/**
 * Polling interval (in ms) for file descriptors wrapped in the Phy I/O loop (for raw drivers only)
 */
#define ZTS_PHY_POLL_INTERVAL 1

/**
 * Maximum length of libzt/ZeroTier home path (where keys, and config files are stored)
 */
#define ZTS_HOME_PATH_MAX_LEN 256

/**
 * Length of human-readable MAC address string
 */
#define ZTS_MAC_ADDRSTRLEN 18

/**
 * Interval (in ms) for performing background tasks
 */
#define ZTS_HOUSEKEEPING_INTERVAL 1000

//////////////////////////////////////////////////////////////////////////////
// lwIP driver config                                                       //
// For more LWIP configuration options see: include/lwipopts.h              //
//////////////////////////////////////////////////////////////////////////////

/* 
 * The following three quantities are related and govern how incoming frames are fed into the 
 * network stack's core:

 * Every LWIP_GUARDED_BUF_CHECK_INTERVAL milliseconds, a callback will be called from the core and 
 * will input a maximum of LWIP_FRAMES_HANDLED_PER_CORE_CALL frames before returning control back
 * to the core. Meanwhile, incoming frames from the ZeroTier wire will be allocated and their 
 * pointers will be cached in the receive frame buffer of the size LWIP_MAX_GUARDED_RX_BUF_SZ to 
 * await the next callback from the core
 */

#define LWIP_GUARDED_BUF_CHECK_INTERVAL 5 // in ms
#define LWIP_MAX_GUARDED_RX_BUF_SZ 1024 // number of frame pointers that can be cached waiting for receipt into core
#define LWIP_FRAMES_HANDLED_PER_CORE_CALL 16 // How many frames are handled per call from core

typedef signed char err_t;

#define ND6_DISCOVERY_INTERVAL 1000
#define ARP_DISCOVERY_INTERVAL ARP_TMR_INTERVAL

//////////////////////////////////////////////////////////////////////////////
// Subset of: ZeroTierOne.h and Constants.hpp                               //
// We redefine a few ZT structures here so that we don't need to drag the   //
// entire ZeroTierOne.h file into the user application                      //
//////////////////////////////////////////////////////////////////////////////

/**
 * Maximum MTU size for ZeroTier
 */
#define ZT_MAX_MTU 10000

/**
 * Maximum number of direct network paths to a given peer
 */
#define ZT_MAX_PEER_NETWORK_PATHS 16

//
// This include file also auto-detects and canonicalizes some environment
// information defines:
//
// __LINUX__
// __APPLE__
// __BSD__ (OSX also defines this)
// __UNIX_LIKE__ (Linux, BSD, etc.)
// __WINDOWS__
//
// Also makes sure __BYTE_ORDER is defined reasonably.
//

// Hack: make sure __GCC__ is defined on old GCC compilers
#ifndef __GCC__
#if defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_1) || defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_2) || defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)
#define __GCC__
#endif
#endif

#if defined(__linux__) || defined(linux) || defined(__LINUX__) || defined(__linux)
#ifndef __LINUX__
#define __LINUX__
#endif
#ifndef __UNIX_LIKE__
#define __UNIX_LIKE__
#endif
#include <endian.h>
#endif

#ifdef __APPLE__
#define likely(x) __builtin_expect((x),1)
#define unlikely(x) __builtin_expect((x),0)
#include <TargetConditionals.h>
#ifndef __UNIX_LIKE__
#define __UNIX_LIKE__
#endif
#ifndef __BSD__
#define __BSD__
#endif
#include <machine/endian.h>
#endif

// Defined this macro to disable "type punning" on a number of targets that
// have issues with unaligned memory access.
#if defined(__arm__) || defined(__ARMEL__) || (defined(__APPLE__) && ( (defined(TARGET_OS_IPHONE) && (TARGET_OS_IPHONE != 0)) || (defined(TARGET_OS_WATCH) && (TARGET_OS_WATCH != 0)) || (defined(TARGET_IPHONE_SIMULATOR) && (TARGET_IPHONE_SIMULATOR != 0)) ) )
#ifndef ZT_NO_TYPE_PUNNING
#define ZT_NO_TYPE_PUNNING
#endif
#endif

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#ifndef __UNIX_LIKE__
#define __UNIX_LIKE__
#endif
#ifndef __BSD__
#define __BSD__
#endif
#include <machine/endian.h>
#ifndef __BYTE_ORDER
#define __BYTE_ORDER _BYTE_ORDER
#define __LITTLE_ENDIAN _LITTLE_ENDIAN
#define __BIG_ENDIAN _BIG_ENDIAN
#endif
#endif

#if defined(_WIN32) || defined(_WIN64)
#ifndef __WINDOWS__
#define __WINDOWS__
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#pragma warning(disable : 4290)
#pragma warning(disable : 4996)
#pragma warning(disable : 4101)
#undef __UNIX_LIKE__
#undef __BSD__
#define ZT_PATH_SEPARATOR '\\'
#define ZT_PATH_SEPARATOR_S "\\"
#define ZT_EOL_S "\r\n"
#include <WinSock2.h>
#include <Windows.h>
#endif

// Assume little endian if not defined
#if (defined(__APPLE__) || defined(__WINDOWS__)) && (!defined(__BYTE_ORDER))
#undef __BYTE_ORDER
#undef __LITTLE_ENDIAN
#undef __BIG_ENDIAN
#define __BIG_ENDIAN 4321
#define __LITTLE_ENDIAN 1234
#define __BYTE_ORDER 1234
#endif

#ifdef __UNIX_LIKE__
#define ZT_PATH_SEPARATOR '/'
#define ZT_PATH_SEPARATOR_S "/"
#define ZT_EOL_S "\n"
#endif

#ifndef __BYTE_ORDER
#include <endian.h>
#endif

#ifdef __NetBSD__
#define RTF_MULTICAST   0x20000000
#endif

#if (defined(__GNUC__) && (__GNUC__ >= 3)) || (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 800)) || defined(__clang__)
#ifndef likely
#define likely(x) __builtin_expect((x),1)
#endif
#ifndef unlikely
#define unlikely(x) __builtin_expect((x),0)
#endif
#else
#ifndef likely
#define likely(x) (x)
#endif
#ifndef unlikely
#define unlikely(x) (x)
#endif
#endif

#ifdef __WINDOWS__
#define ZT_PACKED_STRUCT(D) __pragma(pack(push,1)) D __pragma(pack(pop))
#else
#define ZT_PACKED_STRUCT(D) D __attribute__((packed))
#endif

#endif // _H