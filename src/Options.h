/*
 * Copyright (c)2013-2020 ZeroTier, Inc.
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file in the project's root directory.
 *
 * Change Date: 2024-01-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2.0 of the Apache License.
 */
/****/

#ifndef LIBZT_OPTIONS_H
#define LIBZT_OPTIONS_H

//////////////////////////////////////////////////////////////////////////////
// Callbacks                                                                //
//////////////////////////////////////////////////////////////////////////////

/**
 * The maximum number of un-processed callback messages
 */
#define ZTS_CALLBACK_MSG_QUEUE_LEN 256

//////////////////////////////////////////////////////////////////////////////
// Timing                                                                   //
//////////////////////////////////////////////////////////////////////////////

/**
 * How often callback messages are assembled and/or sent
 */
#define ZTS_CALLBACK_PROCESSING_INTERVAL 25

/**
 * Polling interval (in ms) for fds wrapped in the Phy I/O loop
 */
#define ZTS_TAP_THREAD_POLLING_INTERVAL 50

#define ZTS_HOUSEKEEPING_INTERVAL 50

/**
 * By how much thread I/O and callback loop delays are multiplied (unitless)
 */
#define ZTS_HIBERNATION_MULTIPLIER 50

//////////////////////////////////////////////////////////////////////////////
// Threading                                                                //
//////////////////////////////////////////////////////////////////////////////

#define SERVICE_THREAD_NICENESS       0 // -10
#define CALLBACK_THREAD_NICENESS      0 //  10
#define LWIP_DRIVER_THREAD_NICENESS   0 //  10
#define TCPIP_THREAD_NICENESS         0 // -10
#define TAP_THREAD_NICENESS           0 //  10

#define ZTS_SERVICE_THREAD_NAME "ZeroTierServiceThread"
#define ZTS_EVENT_CALLBACK_THREAD_NAME "ZeroTierEventCallbackThread"
#define ZTS_LWIP_DRIVER_THREAD_NAME "lwipDriver"

//////////////////////////////////////////////////////////////////////////////
// lwIP behaviour (tcpip driver)                                            //
//////////////////////////////////////////////////////////////////////////////

/**
 * How many frames are handled per call from core
 */
#define LWIP_FRAMES_HANDLED_PER_CORE_CALL 16

/**
 * How often the lwIP tcpip thread callback checks for incoming frames
 */
#define LWIP_DRIVER_LOOP_INTERVAL 250

/**
 * Number of packets that can be queued for ingress into the lwIP core
 */
#define ZTS_LWIP_MAX_RX_QUEUE_LEN 1024

#endif