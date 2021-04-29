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
 * Callback event creation and distribution to user application
 */

#ifndef ZTS_USER_EVENTS_HPP
#define ZTS_USER_EVENTS_HPP

#include "ZeroTierSockets.h"

#ifdef __WINDOWS__
#include <BaseTsd.h>
#endif

/* Macro substitutions to standardize state checking of service, node, callbacks, and TCP/IP
 * stack. These are used only for control functions that are called at a low frequency. All higher
 * frequency socket calls use a unguarded state flags */

// Lock service and check that it is running
#define ACQUIRE_SERVICE(x)                                                                                             \
    Mutex::Lock _ls(service_m);                                                                                        \
    if (! zts_service || ! zts_service->isRunning()) {                                                                 \
        return x;                                                                                                      \
    }
// Lock service and check that it is not currently running
#define ACQUIRE_SERVICE_OFFLINE()                                                                                      \
    Mutex::Lock _ls(service_m);                                                                                        \
    if (zts_service && zts_service->isRunning()) {                                                                     \
        return ZTS_ERR_SERVICE;                                                                                        \
    }                                                                                                                  \
    if (! zts_service) {                                                                                               \
        init_subsystems();                                                                                             \
    }
// Unlock service
#define RELEASE_SERVICE() service_m.unlock();
// Lock service, ensure node is online
#define ACQUIRE_ONLINE_NODE()                                                                                          \
    ACQUIRE_SERVICE() if (! zts_service->nodeIsOnline())                                                               \
    {                                                                                                                  \
        return ZTS_ERR_SERVICE;                                                                                        \
    }
// Lock event callback
#define ACQUIRE_EVENTS()                                                                                               \
    Mutex::Lock _lc(events_m);                                                                                         \
    if (! zts_events) {                                                                                                \
        return ZTS_ERR_SERVICE;                                                                                        \
    }

namespace ZeroTier {

#ifdef ZTS_ENABLE_JAVA
#include <jni.h>
// References to JNI objects and VM kept for future callbacks
extern JavaVM* jvm;
extern jobject objRef;
extern jmethodID _userCallbackMethodRef;
#endif

#define ZTS_STATE_NODE_RUNNING        0x01
#define ZTS_STATE_STACK_RUNNING       0x02
#define ZTS_STATE_NET_SERVICE_RUNNING 0x04
#define ZTS_STATE_CALLBACKS_RUNNING   0x08
#define ZTS_STATE_FREE_CALLED         0x10

extern volatile uint8_t service_state;
extern int last_state_check;

inline int transport_ok()
{
    last_state_check = service_state & ZTS_STATE_NET_SERVICE_RUNNING;
    return last_state_check;
}

/**
 * How often callback messages are assembled and/or sent
 */
#define ZTS_CALLBACK_PROCESSING_INTERVAL 25

class Events {
    bool _enabled;

  public:
    Events() : _enabled(false)
    {
    }

    /**
     * Perform one iteration of callback processing
     */
    void run();

    /**
     * Enable callback event processing
     */
    void enable();

    /**
     * Disable callback event processing
     */
    void disable();

    /**
     * Enqueue an event to be sent to the user application
     */
    void enqueue(unsigned int event_code, const void* arg, int len = 0);

    /**
     * Send callback message to user application
     */
    void sendToUser(zts_event_msg_t* msg);

    /**
     * Free memory occupied by callback structures
     */
    void destroy(zts_event_msg_t* msg);

#ifdef ZTS_ENABLE_JAVA
    void setJavaCallback(jobject objRef, jmethodID methodId);
#endif

    /**
     * Return whether a callback method has been set
     */
    bool hasCallback();

    /**
     * Clear pointer reference to user-provided callback function
     */
    void clrCallback();

    /**
     * Return whether service operation can be performed at this time
     */
    int canPerformServiceOperation();

    /**
     * Set internal state flags
     */
    void setState(uint8_t newFlags);

    /**
     * Clear internal state flags
     */
    void clrState(uint8_t newFlags);

    /**
     * Get internal state flags
     */
    bool getState(uint8_t testFlags);
};

}   // namespace ZeroTier

#endif   // _H
