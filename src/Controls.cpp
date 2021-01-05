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
 * Network control interface
 */

#include <inttypes.h>
#include <sys/types.h>

#include "Constants.hpp"
#include "Node.hpp"
#include "Mutex.hpp"
#include "OSUtils.hpp"

#include "Debug.hpp"
#include "NodeService.hpp"
#include "VirtualTap.hpp"
#include "Events.hpp"
#include "ZeroTierSockets.h"

using namespace ZeroTier;

#ifdef SDK_JNI
	#include <jni.h>
#endif

#ifdef __WINDOWS__
#include <Windows.h>
WSADATA wsaData;
#endif

namespace ZeroTier
{
	extern NodeService *service;
	extern Mutex serviceLock;
	extern void (*_userEventCallbackFunc)(void *);
	extern uint8_t allowNetworkCaching;
	extern uint8_t allowPeerCaching;
	extern uint8_t allowLocalConf;

#ifdef SDK_JNI
	// References to JNI objects and VM kept for future callbacks
	JavaVM *jvm = NULL;
	jobject objRef = NULL;
	jmethodID _userCallbackMethodRef = NULL;
#endif
}

#ifdef __cplusplus
extern "C" {
#endif

int zts_allow_network_caching(uint8_t allowed = 1)
{
	Mutex::Lock _l(serviceLock);
	if(!service) {
		allowNetworkCaching = allowed;
		return ZTS_ERR_OK;
	}
	return ZTS_ERR_SERVICE;
}

int zts_allow_peer_caching(uint8_t allowed = 1)
{
	Mutex::Lock _l(serviceLock);
	if(!service) {
		allowPeerCaching = allowed;
		return ZTS_ERR_OK;
	}
	return ZTS_ERR_SERVICE;
}

int zts_allow_local_conf(uint8_t allowed = 1)
{
	Mutex::Lock _l(serviceLock);
	if(!service) {
		allowLocalConf = allowed;
		return ZTS_ERR_OK;
	}
	return ZTS_ERR_SERVICE;
}

#ifdef ZTS_PINVOKE
int zts_start(const char *path, CppCallback callback, uint16_t port)
#else
int zts_start(const char *path, void (*callback)(void *), uint16_t port)
#endif
{
	Mutex::Lock _l(serviceLock);
	_lwip_driver_init();
	if (service || _getState(ZTS_STATE_NODE_RUNNING)) {
		// Service is already initialized
		return ZTS_ERR_SERVICE;
	}
	if (_getState(ZTS_STATE_FREE_CALLED)) {
		// Stack (presumably lwIP) has been dismantled,
		// an application restart is required now
		return ZTS_ERR_SERVICE;
	}
#ifdef SDK_JNI
	_userEventCallbackFunc = callback;
#else
	_userEventCallbackFunc = callback;
#endif
	if (!_isCallbackRegistered()) {
		// Must have a callback
		return ZTS_ERR_ARG;
	}
	if (!path) {
		return ZTS_ERR_ARG;
	}
	if (port < 0 || port > 0xFFFF) {
		return ZTS_ERR_ARG;
	}
	serviceParameters *params = new serviceParameters();

	params->port = port;
	params->path = std::string(path);

	if (params->path.length() == 0) {
		return ZTS_ERR_ARG;
	}
	int err;
	int retval = ZTS_ERR_OK;

	_setState(ZTS_STATE_CALLBACKS_RUNNING);
	_setState(ZTS_STATE_NODE_RUNNING);
	// Start the ZT service thread
#if defined(__WINDOWS__)
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	HANDLE serviceThread = CreateThread(NULL, 0, _runNodeService, (void*)params, 0, NULL);
	HANDLE callbackThread = CreateThread(NULL, 0, _runCallbacks, NULL, 0, NULL);
#else
	pthread_t service_thread;
	pthread_t callback_thread;
	if ((err = pthread_create(&service_thread, NULL, _runNodeService, (void*)params)) != 0) {
		retval = err;
	}
	if ((err = pthread_create(&callback_thread, NULL, _runCallbacks, NULL)) != 0) {
		retval = err;
	}
#endif
#if defined(__linux__)
	pthread_setname_np(service_thread, ZTS_SERVICE_THREAD_NAME);
	pthread_setname_np(callback_thread, ZTS_EVENT_CALLBACK_THREAD_NAME);
#endif
	if (retval != ZTS_ERR_OK) {
		_clrState(ZTS_STATE_CALLBACKS_RUNNING);
		_clrState(ZTS_STATE_NODE_RUNNING);
		_clearRegisteredCallback();
		//delete params;
	}
	return retval;
}

#ifdef SDK_JNI
JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_start(
	JNIEnv *env, jobject thisObj, jstring path, jobject callback, jint port)
{
	if (!path) {
		return ZTS_ERR_ARG;
	}
	jclass eventListenerClass = env->GetObjectClass(callback);
	if(eventListenerClass == NULL) {
		DEBUG_ERROR("Couldn't find class for ZeroTierEventListener instance");
		return ZTS_ERR_ARG;
	}
	jmethodID eventListenerCallbackMethod = env->GetMethodID(eventListenerClass, "onZeroTierEvent", "(JI)V");
	if(eventListenerCallbackMethod == NULL) {
		DEBUG_ERROR("Couldn't find onZeroTierEvent method");
		return ZTS_ERR_ARG;
	}
	// Reference used for later calls
	objRef = env->NewGlobalRef(callback);
	_userCallbackMethodRef = eventListenerCallbackMethod;
	const char* utf_string = env->GetStringUTFChars(path, NULL);
	if (!utf_string) {
		return ZTS_ERR_GENERAL;
	}
	// using _userCallbackMethodRef
	int retval = zts_start(utf_string, NULL, port);
	env->ReleaseStringUTFChars(path, utf_string);
	return retval;
}
#endif

int zts_stop()
{
	Mutex::Lock _l(serviceLock);
	if (_canPerformServiceOperation()) {
		_clrState(ZTS_STATE_NODE_RUNNING);
		service->terminate();
#if defined(__WINDOWS__)
		WSACleanup();
#endif
		return ZTS_ERR_OK;
	}
	return ZTS_ERR_SERVICE;
}
#ifdef SDK_JNI
JNIEXPORT void JNICALL Java_com_zerotier_libzt_ZeroTier_stop(
	JNIEnv *env, jobject thisObj)
{
	zts_stop();
}
#endif

int zts_restart()
{
	serviceLock.lock();
	// Store callback references
#ifdef SDK_JNI
	static jmethodID _tmpUserCallbackMethodRef = _userCallbackMethodRef;
#else
	void (*_tmpUserEventCallbackFunc)(void *);
	_tmpUserEventCallbackFunc = _userEventCallbackFunc;
#endif
	int userProvidedPort = 0;
	std::string userProvidedPath;
	if (service) {
		userProvidedPort = service->_userProvidedPort;
		userProvidedPath = service->_userProvidedPath;
	}
	// Stop the service
	if (_canPerformServiceOperation()) {
		_clrState(ZTS_STATE_NODE_RUNNING);
		service->terminate();
#if defined(__WINDOWS__)
		WSACleanup();
#endif
	}
	else {
		serviceLock.unlock();
		return ZTS_ERR_SERVICE;
	}
	// Start again with same parameters as initial call
	serviceLock.unlock();
	while (service) {
		zts_delay_ms(ZTS_CALLBACK_PROCESSING_INTERVAL);
	}
	/* Some of the logic in Java_com_zerotier_libzt_ZeroTier_start
	is replicated here */
#ifdef SDK_JNI
	_userCallbackMethodRef = _tmpUserCallbackMethodRef;
	return zts_start(userProvidedPath.c_str(), NULL, userProvidedPort);
#else
    return ZTS_ERR_OK;
	//return zts_start(userProvidedPath.c_str(), _tmpUserEventCallbackFunc, userProvidedPort);
#endif
}
#ifdef SDK_JNI
JNIEXPORT void JNICALL Java_com_zerotier_libzt_ZeroTier_restart(
	JNIEnv *env, jobject thisObj)
{
	zts_restart();
}
#endif

int zts_free()
{
	if (_getState(ZTS_STATE_FREE_CALLED)) {
		return ZTS_ERR_SERVICE;
	}
	_setState(ZTS_STATE_FREE_CALLED);
	int err = zts_stop();
	Mutex::Lock _l(serviceLock);
	_lwip_driver_shutdown();
	return err;
}
#ifdef SDK_JNI
JNIEXPORT void JNICALL Java_com_zerotier_libzt_ZeroTier_free(
	JNIEnv *env, jobject thisObj)
{
	zts_free();
}
#endif

#ifdef SDK_JNI
/*
 * Called from Java, saves a static reference to the VM so it can be used
 * later to call a user-specified callback method from C.
 */
JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_init(
	JNIEnv *env, jobject thisObj)
{
    jint rs = env->GetJavaVM(&jvm);
	return rs != JNI_OK ? ZTS_ERR_GENERAL : ZTS_ERR_OK;
}
#endif

int zts_join(const uint64_t networkId)
{
	Mutex::Lock _l(serviceLock);
	if (!_canPerformServiceOperation()) {
		return ZTS_ERR_SERVICE;
	}
	else {
		service->join(networkId);
	}
	return ZTS_ERR_OK;
}
#ifdef SDK_JNI
JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_join(
	JNIEnv *env, jobject thisObj, jlong networkId)
{
	return zts_join((uint64_t)networkId);
}
#endif

int zts_leave(const uint64_t networkId)
{
	Mutex::Lock _l(serviceLock);
	if (!_canPerformServiceOperation()) {
		return ZTS_ERR_SERVICE;
	}
	else {
		service->leave(networkId);
	}
	return ZTS_ERR_OK;
}
#ifdef SDK_JNI
JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_leave(
	JNIEnv *env, jobject thisObj, jlong networkId)
{
	return zts_leave((uint64_t)networkId);
}
#endif

int zts_orbit(uint64_t moonWorldId, uint64_t moonSeed)
{
	Mutex::Lock _l(serviceLock);
	void *tptr = NULL;
	if (!_canPerformServiceOperation()) {
		return ZTS_ERR_SERVICE;
	} else {
		service->getNode()->orbit(tptr, moonWorldId, moonSeed);
	}
	return ZTS_ERR_OK;
}
#ifdef SDK_JNI
#endif

int zts_deorbit(uint64_t moonWorldId)
{
	Mutex::Lock _l(serviceLock);
	void *tptr = NULL;
	if (!_canPerformServiceOperation()) {
		return ZTS_ERR_SERVICE;
	} else {
		service->getNode()->deorbit(tptr, moonWorldId);
	}
	return ZTS_ERR_OK;
}
#ifdef SDK_JNI
#endif

int zts_get_6plane_addr(struct zts_sockaddr_storage *addr, const uint64_t networkId, const uint64_t nodeId)
{
	if (!addr || !networkId || !nodeId) {
		return ZTS_ERR_ARG;
	}
	InetAddress _6planeAddr = InetAddress::makeIpv66plane(networkId,nodeId);
	struct sockaddr_in6 *in6 = (struct sockaddr_in6*)addr;
	memcpy(in6->sin6_addr.s6_addr, _6planeAddr.rawIpData(), sizeof(struct in6_addr));
	return ZTS_ERR_OK;
}

int zts_get_rfc4193_addr(struct zts_sockaddr_storage *addr, const uint64_t networkId, const uint64_t nodeId)
{
	if (!addr || !networkId || !nodeId) {
		return ZTS_ERR_ARG;
	}
	InetAddress _rfc4193Addr = InetAddress::makeIpv6rfc4193(networkId,nodeId);
	struct sockaddr_in6 *in6 = (struct sockaddr_in6*)addr;
	memcpy(in6->sin6_addr.s6_addr, _rfc4193Addr.rawIpData(), sizeof(struct in6_addr));
	return ZTS_ERR_OK;
}

uint64_t zts_generate_adhoc_nwid_from_range(uint16_t startPortOfRange, uint16_t endPortOfRange)
{
	char networkIdStr[INET6_ADDRSTRLEN];
	sprintf(networkIdStr, "ff%04x%04x000000", startPortOfRange, endPortOfRange);
	return strtoull(networkIdStr, NULL, 16);
}

#ifdef __WINDOWS__
#include <windows.h>
#elif _POSIX_C_SOURCE >= 199309L
#include <time.h>   // for nanosleep
#else
#include <unistd.h> // for usleep
#endif

void zts_delay_ms(long milliseconds)
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

#ifdef __cplusplus
}
#endif
