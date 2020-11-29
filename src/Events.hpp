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

/**
 * @file
 *
 * Header for callback event processing logic
 */

#ifndef ZT_EVENTS_HPP
#define ZT_EVENTS_HPP

#include <string>

#include "Constants.hpp"
#include "ZeroTierSockets.h"

#ifdef __WINDOWS__
#include <BaseTsd.h>
#endif

#ifdef SDK_JNI
	#include <jni.h>
#endif
namespace ZeroTier {

#define ZTS_STATE_NODE_RUNNING              0x01
#define ZTS_STATE_STACK_RUNNING             0x02
#define ZTS_STATE_NET_SERVICE_RUNNING       0x04
#define ZTS_STATE_CALLBACKS_RUNNING         0x08
#define ZTS_STATE_FREE_CALLED               0x10

#ifdef SDK_JNI
	// References to JNI objects and VM kept for future callbacks
	extern JavaVM *jvm;
	extern jobject objRef;
	extern jmethodID _userCallbackMethodRef;
#endif

/**
 * How often callback messages are assembled and/or sent
 */
#define ZTS_CALLBACK_PROCESSING_INTERVAL 25

/**
 * Enqueue an event to be sent to the user application
 */
void _enqueueEvent(int16_t eventCode, void *arg);

/**
 * Send callback message to user application
 */
void _passDequeuedEventToUser(struct ::zts_callback_msg *msg);

/**
 * Free memory occupied by callback structures
 */
void _freeEvent(struct ::zts_callback_msg *msg);

/**
 * Return whether a callback method has been set
 */
bool _isCallbackRegistered();

/**
 * Clear pointer reference to user-provided callback function
 */
void _clearRegisteredCallback();

/**
 * Return whether service operation can be performed at this time
 */
int _canPerformServiceOperation();

/**
 * Set internal state flags
 */
void _setState(uint8_t newFlags);

/**
 * Clear internal state flags
 */
void _clrState(uint8_t newFlags);

/**
 * Get internal state flags
 */
bool _getState(uint8_t testFlags);

#ifdef __WINDOWS__
DWORD WINAPI _runCallbacks(LPVOID thread_id);
#else
/**
 * Event callback thread
 */
void *_runCallbacks(void *thread_id);
#endif

} // namespace ZeroTier

#endif // _H