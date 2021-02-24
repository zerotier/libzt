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

/**
 * @file
 *
 * Callback event processing logic
 */

#include "concurrentqueue.h"

#ifdef ZTS_ENABLE_JAVA
	#include <jni.h>
#endif

#include "Constants.hpp"
#include "Node.hpp"
#include "OSUtils.hpp"

#include "Debug.hpp"
#include "Events.hpp"
#include "ZeroTierSockets.h"
#include "NodeService.hpp"

#define NODE_EVENT_TYPE(code) code >= ZTS_EVENT_NODE_UP && code <= ZTS_EVENT_NODE_NORMAL_TERMINATION
#define NETWORK_EVENT_TYPE(code) code >= ZTS_EVENT_NETWORK_NOT_FOUND && code <= ZTS_EVENT_NETWORK_UPDATE
#define STACK_EVENT_TYPE(code) code >= ZTS_EVENT_STACK_UP && code <= ZTS_EVENT_STACK_DOWN
#define NETIF_EVENT_TYPE(code) code >= ZTS_EVENT_NETIF_UP && code <= ZTS_EVENT_NETIF_LINK_DOWN
#define PEER_EVENT_TYPE(code) code >= ZTS_EVENT_PEER_DIRECT && code <= ZTS_EVENT_PEER_PATH_DEAD
#define ROUTE_EVENT_TYPE(code) code >= ZTS_EVENT_ROUTE_ADDED && code <= ZTS_EVENT_ROUTE_REMOVED
#define ADDR_EVENT_TYPE(code) code >= ZTS_EVENT_ADDR_ADDED_IP4 && code <= ZTS_EVENT_ADDR_REMOVED_IP6

#include <execinfo.h>
#include <signal.h>

#ifdef ZTS_ENABLE_PYTHON
	#include "Python.h"
	PythonDirectorCallbackClass *_userEventCallback = NULL;
	void PythonDirectorCallbackClass::on_zerotier_event(struct zts_callback_msg *msg) { }
#endif

namespace ZeroTier {

extern NodeService *service;

// Global state variable shared between Socket, Control, Event and NodeService logic.
uint8_t _serviceStateFlags;

// Lock to guard access to callback function pointers.
Mutex _callbackLock;

#ifdef ZTS_ENABLE_PINVOKE
	void (*_userEventCallback)(void *);
#endif
#ifdef ZTS_C_API_ONLY
	void (*_userEventCallback)(void *);
#endif

moodycamel::ConcurrentQueue<struct zts_callback_msg*> _callbackMsgQueue;

void _enqueueEvent(int16_t eventCode, void *arg)
{
	struct zts_callback_msg *msg = new zts_callback_msg();
	msg->eventCode = eventCode;

	if (NODE_EVENT_TYPE(eventCode)) {
		msg->node = (struct zts_node_details*)arg;
	} if (NETWORK_EVENT_TYPE(eventCode)) {
		msg->network = (struct zts_network_details*)arg;
	} if (STACK_EVENT_TYPE(eventCode)) {
		/* nothing to convey to user */
	} if (NETIF_EVENT_TYPE(eventCode)) {
		msg->netif = (struct zts_netif_details*)arg;
	} if (ROUTE_EVENT_TYPE(eventCode)) {
		msg->route = (struct zts_virtual_network_route*)arg;
	} if (PEER_EVENT_TYPE(eventCode)) {
		msg->peer = (struct zts_peer_details*)arg;
	} if (ADDR_EVENT_TYPE(eventCode)) {
		msg->addr = (struct zts_addr_details*)arg;
	}

	if (msg && _callbackMsgQueue.size_approx() > 1024) {
		// Rate-limit number of events
		_freeEvent(msg);
	}
	else {
		_callbackMsgQueue.enqueue(msg);
	}
}

void _freeEvent(struct zts_callback_msg *msg)
{
	if (!msg) {
		return;
	}
	if (msg->node) { delete msg->node; }
	if (msg->network) { delete msg->network; }
	if (msg->netif) { delete msg->netif; }
	if (msg->route) { delete msg->route; }
	if (msg->peer) { delete msg->peer; }
	if (msg->addr) { delete msg->addr; }
}

void _passDequeuedEventToUser(struct zts_callback_msg *msg)
{
	bool bShouldStopCallbackThread = (msg->eventCode == ZTS_EVENT_STACK_DOWN);
#ifdef ZTS_ENABLE_PYTHON
	PyGILState_STATE state = PyGILState_Ensure();
	_userEventCallback->on_zerotier_event(msg);
	PyGILState_Release(state);
#endif
#ifdef ZTS_ENABLE_JAVA
	if(_userCallbackMethodRef) {
		JNIEnv *env;
	#if defined(__ANDROID__)
		jint rs = jvm->AttachCurrentThread(&env, NULL);
	#else
		jint rs = jvm->AttachCurrentThread((void **)&env, NULL);
	#endif
		assert (rs == JNI_OK);
		uint64_t arg = 0;
		uint64_t id = 0;
		if (NODE_EVENT_TYPE(msg->eventCode)) {
			id = msg->node ? msg->node->address : 0;
		}
		if (NETWORK_EVENT_TYPE(msg->eventCode)) {
			id = msg->network ? msg->network->nwid : 0;
		}
		if (PEER_EVENT_TYPE(msg->eventCode)) {
			id = msg->peer ? msg->peer->address : 0;
		}
		env->CallVoidMethod(objRef, _userCallbackMethodRef, id, msg->eventCode);
	}
#endif // ZTS_ENABLE_JAVA
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
	_freeEvent(msg);
	if (bShouldStopCallbackThread) {
		/* Ensure last possible callback ZTS_EVENT_STACK_DOWN is
		delivered before callback thread is finally stopped. */
		_clrState(ZTS_STATE_CALLBACKS_RUNNING);
	}
}

bool _isCallbackRegistered()
{
	_callbackLock.lock();
	bool retval = false;
#ifdef ZTS_ENABLE_JAVA
	retval = (jvm && objRef && _userCallbackMethodRef);
#else
	retval = _userEventCallback;
#endif
	_callbackLock.unlock();
	return retval;
}

void _clearRegisteredCallback()
{
	_callbackLock.lock();
#ifdef ZTS_ENABLE_JAVA
	objRef = NULL;
    _userCallbackMethodRef = NULL;
#else
	_userEventCallback = NULL;
#endif
	_callbackLock.unlock();
}

int _canPerformServiceOperation()
{
	return service
		&& service->isRunning()
		&& service->getNode()
		&& service->getNode()->online()
		&& !_getState(ZTS_STATE_FREE_CALLED);
}

#define RESET_FLAGS( )   _serviceStateFlags  =  0;
#define   SET_FLAGS(f)   _serviceStateFlags |=  f;
#define   CLR_FLAGS(f)   _serviceStateFlags &= ~f;
#define   GET_FLAGS(f) ((_serviceStateFlags &   f) > 0)

void _setState(uint8_t newFlags)
{
	if ((newFlags ^ _serviceStateFlags) & ZTS_STATE_NET_SERVICE_RUNNING) {
		return; // No effect. Not allowed to set this flag manually
	}
	SET_FLAGS(newFlags);
	if (   GET_FLAGS(ZTS_STATE_NODE_RUNNING)
      &&   GET_FLAGS(ZTS_STATE_STACK_RUNNING)
      && !(GET_FLAGS(ZTS_STATE_FREE_CALLED)))
	{
		SET_FLAGS(ZTS_STATE_NET_SERVICE_RUNNING);
	}
	else {
		CLR_FLAGS(ZTS_STATE_NET_SERVICE_RUNNING);
	}
}

void _clrState(uint8_t newFlags)
{
	if (newFlags & ZTS_STATE_NET_SERVICE_RUNNING) {
		return; // No effect. Not allowed to set this flag manually
	}
	CLR_FLAGS(newFlags);
	if (   GET_FLAGS(ZTS_STATE_NODE_RUNNING)
      &&   GET_FLAGS(ZTS_STATE_STACK_RUNNING)
      && !(GET_FLAGS(ZTS_STATE_FREE_CALLED)))
	{
		SET_FLAGS(ZTS_STATE_NET_SERVICE_RUNNING);
	}
	else {
		CLR_FLAGS(ZTS_STATE_NET_SERVICE_RUNNING);
	}
}

bool _getState(uint8_t testFlags)
{
	return testFlags & _serviceStateFlags;
}

#if defined(__WINDOWS__)
DWORD WINAPI _runCallbacks(LPVOID thread_id)
#else
void *_runCallbacks(void *thread_id)
#endif
{
#if defined(__APPLE__)
	pthread_setname_np(ZTS_EVENT_CALLBACK_THREAD_NAME);
#endif
	while (_getState(ZTS_STATE_CALLBACKS_RUNNING) || _callbackMsgQueue.size_approx() > 0)
    {
        struct zts_callback_msg *msg;
		size_t sz = _callbackMsgQueue.size_approx();
		for (size_t j = 0; j < sz; j++) {
			if (_callbackMsgQueue.try_dequeue(msg)) {
				_callbackLock.lock();
				_passDequeuedEventToUser(msg);
				_callbackLock.unlock();
				delete msg;
			}
		}
        zts_delay_ms(ZTS_CALLBACK_PROCESSING_INTERVAL);
    }
#if ZTS_ENABLE_JAVA
	JNIEnv *env;
	jint rs = jvm->DetachCurrentThread();
    pthread_exit(0);
#endif
	return NULL;
}

} // namespace ZeroTier
