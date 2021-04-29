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

#include "Events.hpp"

#include "Mutex.hpp"
#include "NodeService.hpp"
#include "concurrentqueue.h"

#ifdef ZTS_ENABLE_JAVA
#include <jni.h>
#endif

#ifdef ZTS_ENABLE_PYTHON
#include "Python.h"
PythonDirectorCallbackClass* _userEventCallback = NULL;
void PythonDirectorCallbackClass::on_zerotier_event(zts_event_msg_t* msg)
{
}
#endif

#define ZTS_NODE_EVENT(code)    code >= ZTS_EVENT_NODE_UP&& code <= ZTS_EVENT_NODE_FATAL_ERROR
#define ZTS_NETWORK_EVENT(code) code >= ZTS_EVENT_NETWORK_NOT_FOUND&& code <= ZTS_EVENT_NETWORK_UPDATE
#define ZTS_STACK_EVENT(code)   code >= ZTS_EVENT_STACK_UP&& code <= ZTS_EVENT_STACK_DOWN
#define ZTS_NETIF_EVENT(code)   code >= ZTS_EVENT_NETIF_UP&& code <= ZTS_EVENT_NETIF_LINK_DOWN
#define ZTS_PEER_EVENT(code)    code >= ZTS_EVENT_PEER_DIRECT&& code <= ZTS_EVENT_PEER_PATH_DEAD
#define ZTS_ROUTE_EVENT(code)   code >= ZTS_EVENT_ROUTE_ADDED&& code <= ZTS_EVENT_ROUTE_REMOVED
#define ZTS_ADDR_EVENT(code)    code >= ZTS_EVENT_ADDR_ADDED_IP4&& code <= ZTS_EVENT_ADDR_REMOVED_IP6
#define ZTS_STORE_EVENT(code)   code >= ZTS_EVENT_STORE_IDENTITY_SECRET&& code <= ZTS_EVENT_STORE_NETWORK

namespace ZeroTier {

#ifdef ZTS_ENABLE_JAVA
// References to JNI objects and VM kept for future callbacks
JavaVM* jvm;
jobject javaCbObjRef = NULL;
jmethodID javaCbMethodId = NULL;
#endif

extern NodeService* zts_service;

// Global state variable shared between Socket, Control, Event and
// NodeService logic.
volatile uint8_t service_state = 0;
int last_state_check;

#define RESET_FLAGS() service_state = 0;
#define SET_FLAGS(f)  service_state |= f;
#define CLR_FLAGS(f)  service_state &= ~f;
#define GET_FLAGS(f)  ((service_state & f) > 0)

// Lock to guard access to callback function pointers.
Mutex events_m;

#ifdef ZTS_ENABLE_PINVOKE
void (*_userEventCallback)(void*);
#endif
#ifdef ZTS_C_API_ONLY
void (*_userEventCallback)(void*);
#endif

moodycamel::ConcurrentQueue<zts_event_msg_t*> _callbackMsgQueue;

void Events::run()
{
    while (getState(ZTS_STATE_CALLBACKS_RUNNING) || _callbackMsgQueue.size_approx() > 0) {
        zts_event_msg_t* msg;
        size_t sz = _callbackMsgQueue.size_approx();
        for (size_t j = 0; j < sz; j++) {
            if (_callbackMsgQueue.try_dequeue(msg)) {
                events_m.lock();
                sendToUser(msg);
                events_m.unlock();
            }
        }
        zts_util_delay(ZTS_CALLBACK_PROCESSING_INTERVAL);
    }
}

void Events::enqueue(unsigned int event_code, const void* arg, int len)
{
    if (! _enabled) {
        return;
    }
    zts_event_msg_t* msg = new zts_event_msg_t();
    msg->event_code = event_code;

    if (ZTS_NODE_EVENT(event_code)) {
        msg->node = (zts_node_info_t*)arg;
        msg->len = sizeof(zts_node_info_t);
    }
    if (ZTS_NETWORK_EVENT(event_code)) {
        msg->network = (zts_net_info_t*)arg;
        msg->len = sizeof(zts_net_info_t);
    }
    if (ZTS_STACK_EVENT(event_code)) {
        /* nothing to convey to user */
    }
    if (ZTS_NETIF_EVENT(event_code)) {
        msg->netif = (zts_netif_info_t*)arg;
        msg->len = sizeof(zts_netif_info_t);
    }
    if (ZTS_ROUTE_EVENT(event_code)) {
        msg->route = (zts_route_info_t*)arg;
        msg->len = sizeof(zts_route_info_t);
    }
    if (ZTS_PEER_EVENT(event_code)) {
        msg->peer = (zts_peer_info_t*)arg;
        msg->len = sizeof(zts_peer_info_t);
    }
    if (ZTS_ADDR_EVENT(event_code)) {
        msg->addr = (zts_addr_info_t*)arg;
        msg->len = sizeof(zts_addr_info_t);
    }
    if (ZTS_STORE_EVENT(event_code)) {
        msg->cache = (void*)arg;
        msg->len = len;
    }
    if (msg && _callbackMsgQueue.size_approx() > 1024) {
        /* Rate-limit number of events. This value should only grow if the
        user application isn't returning from the event handler in a timely manner.
        For most applications it should hover around 1 to 2 */
        destroy(msg);
    }
    else {
        _callbackMsgQueue.enqueue(msg);
    }
}

void Events::destroy(zts_event_msg_t* msg)
{
    if (! msg) {
        return;
    }
    if (msg->node) {
        delete msg->node;
    }
    if (msg->network) {
        delete msg->network;
    }
    if (msg->netif) {
        delete msg->netif;
    }
    if (msg->route) {
        delete msg->route;
    }
    if (msg->peer) {
        delete msg->peer;
    }
    if (msg->addr) {
        delete msg->addr;
    }
    delete msg;
    msg = NULL;
}

void Events::sendToUser(zts_event_msg_t* msg)
{
    bool bShouldStopCallbackThread = (msg->event_code == ZTS_EVENT_STACK_DOWN);
#ifdef ZTS_ENABLE_PYTHON
    PyGILState_STATE state = PyGILState_Ensure();
    _userEventCallback->on_zerotier_event(msg);
    PyGILState_Release(state);
#endif
#ifdef ZTS_ENABLE_JAVA
    if (javaCbMethodId) {
        JNIEnv* env;
#if defined(__ANDROID__)
        jint rs = jvm->AttachCurrentThread(&env, NULL);
#else
        jint rs = jvm->AttachCurrentThread((void**)&env, NULL);
#endif
        uint64_t arg = 0;
        uint64_t id = 0;
        if (ZTS_NODE_EVENT(msg->event_code)) {
            id = msg->node ? msg->node->node_id : 0;
        }
        if (ZTS_NETWORK_EVENT(msg->event_code)) {
            id = msg->network ? msg->network->net_id : 0;
        }
        if (ZTS_PEER_EVENT(msg->event_code)) {
            id = msg->peer ? msg->peer->peer_id : 0;
        }
        env->CallVoidMethod(javaCbObjRef, javaCbMethodId, id, msg->event_code);
    }
#endif   // ZTS_ENABLE_JAVA
#ifdef ZTS_ENABLE_PINVOKE
    if (_userEventCallback) {
        _userEventCallback(msg);
    }
#endif
#ifdef ZTS_C_API_ONLY
    if (_userEventCallback) {
        _userEventCallback(msg);
    }
#endif
    destroy(msg);
    if (bShouldStopCallbackThread) {
        /* Ensure last possible callback ZTS_EVENT_STACK_DOWN is
        delivered before callback thread is finally stopped. */
        clrState(ZTS_STATE_CALLBACKS_RUNNING);
    }
}

#ifdef ZTS_ENABLE_JAVA
void Events::setJavaCallback(jobject objRef, jmethodID methodId)
{
    javaCbObjRef = objRef;
    javaCbMethodId = methodId;
}
#endif
bool Events::hasCallback()
{
    events_m.lock();
    bool retval = false;
#ifdef ZTS_ENABLE_JAVA
    retval = (jvm && javaCbObjRef && javaCbMethodId);
#else
    retval = _userEventCallback;
#endif
    events_m.unlock();
    return retval;
}

void Events::clrCallback()
{
    events_m.lock();
#ifdef ZTS_ENABLE_JAVA
    javaCbObjRef = NULL;
    javaCbMethodId = NULL;
#else
    _userEventCallback = NULL;
#endif
    events_m.unlock();
}

int Events::canPerformServiceOperation()
{
    return zts_service && zts_service->isRunning() && ! getState(ZTS_STATE_FREE_CALLED);
}

void Events::setState(uint8_t newFlags)
{
    if ((newFlags ^ service_state) & ZTS_STATE_NET_SERVICE_RUNNING) {
        return;   // No effect. Not allowed to set this flag manually
    }
    SET_FLAGS(newFlags);
    if (GET_FLAGS(ZTS_STATE_NODE_RUNNING) && GET_FLAGS(ZTS_STATE_STACK_RUNNING)
        && ! (GET_FLAGS(ZTS_STATE_FREE_CALLED))) {
        SET_FLAGS(ZTS_STATE_NET_SERVICE_RUNNING);
    }
    else {
        CLR_FLAGS(ZTS_STATE_NET_SERVICE_RUNNING);
    }
}

void Events::clrState(uint8_t newFlags)
{
    if (newFlags & ZTS_STATE_NET_SERVICE_RUNNING) {
        return;   // No effect. Not allowed to set this flag manually
    }
    CLR_FLAGS(newFlags);
    if (GET_FLAGS(ZTS_STATE_NODE_RUNNING) && GET_FLAGS(ZTS_STATE_STACK_RUNNING)
        && ! (GET_FLAGS(ZTS_STATE_FREE_CALLED))) {
        SET_FLAGS(ZTS_STATE_NET_SERVICE_RUNNING);
    }
    else {
        CLR_FLAGS(ZTS_STATE_NET_SERVICE_RUNNING);
    }
}

bool Events::getState(uint8_t testFlags)
{
    return testFlags & service_state;
}

void Events::enable()
{
    _enabled = true;
}

void Events::disable()
{
    _enabled = false;
}

}   // namespace ZeroTier
