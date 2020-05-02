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

int zts_start(const char *path, void (*callback)(void *), uint16_t port)
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
	Mutex::Lock _l(serviceLock);
	if (_getState(ZTS_STATE_FREE_CALLED)) {
		return ZTS_ERR_SERVICE;
	}
	_setState(ZTS_STATE_FREE_CALLED);
	return zts_stop();
	// TODO: add stack shutdown logic
}
#ifdef SDK_JNI
JNIEXPORT void JNICALL Java_com_zerotier_libzt_ZeroTier_free(
	JNIEnv *env, jobject thisObj)
{
	zts_free();
}
#endif

uint64_t zts_get_node_id()
{
	Mutex::Lock _l(serviceLock);
	if (!_canPerformServiceOperation()) {
		return ZTS_ERR_OK; // Not really
	}
	return service->getNode()->address();
}
#ifdef SDK_JNI
JNIEXPORT jlong JNICALL Java_com_zerotier_libzt_ZeroTier_get_1node_1id(
	JNIEnv *env, jobject thisObj)
{
	return zts_get_node_id();
}
#endif

int zts_get_node_status()
{
	Mutex::Lock _l(serviceLock);
	// Don't check _canPerformServiceOperation() here.
	return service
		&& service->getNode()
		&& service->getNode()->online() ? ZTS_EVENT_NODE_ONLINE : ZTS_EVENT_NODE_OFFLINE;
}
#ifdef SDK_JNI
JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_get_1node_1status(
	JNIEnv *env, jobject thisObj)
{
	return zts_get_node_status();
}
#endif

int zts_get_network_status(uint64_t networkId)
{
	Mutex::Lock _l(serviceLock);
	if (!networkId) {
		return ZTS_ERR_ARG;
	}
	if (!_canPerformServiceOperation()) {
		return ZTS_ERR_SERVICE;
	}
	/*
	TODO:
		ZTS_EVENT_NETWORK_READY_IP4
		ZTS_EVENT_NETWORK_READY_IP6
		ZTS_EVENT_NETWORK_READY_IP4_IP6
	*/
	return ZTS_ERR_NO_RESULT;
}
#ifdef SDK_JNI
JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_get_1network_1status(
	JNIEnv *env, jobject thisObj, jlong networkId)
{
	return zts_get_network_status(networkId);
}
#endif

int zts_get_peer_status(uint64_t peerId)
{
	Mutex::Lock _l(serviceLock);
	int retval = ZTS_ERR_OK;
	if (!_canPerformServiceOperation()) {
		return ZTS_ERR_SERVICE;
	}
	return service->getPeerStatus(peerId);
}
#ifdef SDK_JNI
JNIEXPORT jlong JNICALL Java_com_zerotier_libzt_ZeroTier_get_1peer_1status(
	JNIEnv *env, jobject thisObj, jlong peerId)
{
	return zts_get_peer_status(peerId);
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

int zts_join(const uint64_t nwid)
{
	Mutex::Lock _l(serviceLock);
	if (!_canPerformServiceOperation()) {
		return ZTS_ERR_SERVICE;
	}
	else {
		service->join(nwid);
	}
	return ZTS_ERR_OK;
}
#ifdef SDK_JNI
JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_join(
	JNIEnv *env, jobject thisObj, jlong nwid)
{
	return zts_join((uint64_t)nwid);
}
#endif

int zts_leave(const uint64_t nwid)
{
	Mutex::Lock _l(serviceLock);
	if (!_canPerformServiceOperation()) {
		return ZTS_ERR_SERVICE;
	}
	else {
		service->leave(nwid);
	}
	return ZTS_ERR_OK;
}
#ifdef SDK_JNI
JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_leave(
	JNIEnv *env, jobject thisObj, jlong nwid)
{
	return zts_leave((uint64_t)nwid);
}
#endif

int zts_leave_all()
{
	Mutex::Lock _l(serviceLock);
	if (!_canPerformServiceOperation()) {
		return ZTS_ERR_SERVICE;
	}
	else {
		service->leaveAll();
	}
	return ZTS_ERR_OK;
}
#ifdef SDK_JNI
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

int zts_get_6plane_addr(struct sockaddr_storage *addr, const uint64_t nwid, const uint64_t nodeId)
{
	if (!addr || !nwid || !nodeId) {
		return ZTS_ERR_ARG;
	}
	InetAddress _6planeAddr = InetAddress::makeIpv66plane(nwid,nodeId);
	struct sockaddr_in6 *in6 = (struct sockaddr_in6*)addr;
	memcpy(in6->sin6_addr.s6_addr, _6planeAddr.rawIpData(), sizeof(struct in6_addr));
	return ZTS_ERR_OK;
}

int zts_get_rfc4193_addr(struct sockaddr_storage *addr, const uint64_t nwid, const uint64_t nodeId)
{
	if (!addr || !nwid || !nodeId) {
		return ZTS_ERR_ARG;
	}
	InetAddress _rfc4193Addr = InetAddress::makeIpv6rfc4193(nwid,nodeId);
	struct sockaddr_in6 *in6 = (struct sockaddr_in6*)addr;
	memcpy(in6->sin6_addr.s6_addr, _rfc4193Addr.rawIpData(), sizeof(struct in6_addr));
	return ZTS_ERR_OK;
}


uint64_t zts_generate_adhoc_nwid_from_range(uint16_t startPortOfRange, uint16_t endPortOfRange)
{
	char nwidStr[INET6_ADDRSTRLEN];
	sprintf(nwidStr, "ff%04x%04x000000", startPortOfRange, endPortOfRange);
	return strtoull(nwidStr, NULL, 16);
}

int zts_get_peers(struct zts_peer_details *pds, uint32_t *num)
{
	Mutex::Lock _l(serviceLock);
	if (!pds || !num) {
		return ZTS_ERR_ARG;
	}
	if (!_canPerformServiceOperation()) {
		return ZTS_ERR_SERVICE;
	}
	ZT_PeerList *pl = service->getNode()->peers();
	if (pl) {
		if (*num < pl->peerCount) {
			service->getNode()->freeQueryResult((void *)pl);
			return ZTS_ERR_ARG;
		}
		*num = pl->peerCount;
		for(unsigned long i=0;i<pl->peerCount;++i) {
			memcpy(&(pds[i]), &(pl->peers[i]), sizeof(struct zts_peer_details));
			for (unsigned int j=0; j<pl->peers[i].pathCount; j++) {
				memcpy(&(pds[i].paths[j].address),
					&(pl->peers[i].paths[j].address), sizeof(struct sockaddr_storage));
			}
		}
	}
	service->getNode()->freeQueryResult((void *)pl);
	return ZTS_ERR_OK;
}
#ifdef SDK_JNI
#endif

int zts_get_peer(struct zts_peer_details *pd, uint64_t peerId)
{
	Mutex::Lock _l(serviceLock);
	if (!pd || !peerId) {
		return ZTS_ERR_ARG;
	}
	if (!_canPerformServiceOperation()) {
		return ZTS_ERR_SERVICE;
	}
	ZT_PeerList *pl = service->getNode()->peers();
	int retval = ZTS_ERR_NO_RESULT;
	if (pl) {
		for(unsigned long i=0;i<pl->peerCount;++i) {
			if (pl->peers[i].address == peerId) {
				memcpy(pd, &(pl->peers[i]), sizeof(struct zts_peer_details));
				for (unsigned int j=0; j<pl->peers[i].pathCount; j++) {
					memcpy(&(pd->paths[j].address),
						&(pl->peers[i].paths[j].address), sizeof(struct sockaddr_storage));
				}
				retval = ZTS_ERR_OK;
				break;
			}
		}
	}
	service->getNode()->freeQueryResult((void *)pl);
	return retval;
}
#ifdef SDK_JNI
#endif

void _get_network_details_helper(uint64_t nwid, struct zts_network_details *nd)
{
	/*
    socklen_t addrlen;
    VirtualTap *tap = vtapMap[nwid];
    nd->nwid = tap->_nwid;
    nd->mtu = tap->_mtu;
    // assigned addresses
    nd->num_addresses = tap->_ips.size() < ZTS_MAX_ASSIGNED_ADDRESSES ? tap->_ips.size() : ZTS_MAX_ASSIGNED_ADDRESSES;
    for (int j=0; j<nd->num_addresses; j++) {
        addrlen = tap->_ips[j].isV4() ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
        memcpy(&(nd->addr[j]), &(tap->_ips[j]), addrlen);
    }
    // routes
    nd->num_routes = ZTS_MAX_NETWORK_ROUTES;;
    service->getRoutes(nwid, (ZT_VirtualNetworkRoute*)&(nd->routes)[0], &(nd->num_routes));
	*/
}

void _get_network_details(uint64_t nwid, struct zts_network_details *nd)
{
	/*
    _vtaps_lock.lock();
    _get_network_details_helper(nwid, nd);
    _vtaps_lock.unlock();
	*/
}

void _get_all_network_details(struct zts_network_details *nds, int *num)
{
	/*
    _vtaps_lock.lock();
    *num = vtapMap.size();
    int idx = 0;
    std::map<uint64_t, VirtualTap*>::iterator it;
    for (it = vtapMap.begin(); it != vtapMap.end(); it++) {
        _get_network_details(it->first, &nds[idx]);
        idx++;
    }
    _vtaps_lock.unlock();
	*/
}

int zts_get_network_details(uint64_t nwid, struct zts_network_details *nd)
{
	/*
	serviceLock.lock();
	int retval = ZTS_ERR_OK;
	if (!nd || nwid == 0) {
		retval = ZTS_ERR_ARG;
	}
	if (!service || _freeHasBeenCalled || _serviceIsShuttingDown) {
		retval = ZTS_ERR_SERVICE;
	}
	if (retval == ZTS_ERR_OK) {
		_get_network_details(nwid, nd);
	}
	serviceLock.unlock();
	return retval;
	*/
	return 0;
}
#ifdef SDK_JNI
#endif

int zts_get_all_network_details(struct zts_network_details *nds, int *num)
{
	/*
	serviceLock.lock();
	int retval = ZTS_ERR_OK;
	if (!nds || !num) {
		retval = ZTS_ERR_ARG;
	}
	if (!service || _freeHasBeenCalled || _serviceIsShuttingDown) {
		retval = ZTS_ERR_SERVICE;
	}
	if (retval == ZTS_ERR_OK) {
		_get_all_network_details(nds, num);
	}
	serviceLock.unlock();
	return retval;	
	*/
	return 0;
}
#ifdef SDK_JNI
#endif

void zts_delay_ms(long interval_ms)
{
#if defined(__WINDOWS__)
	Sleep(interval_ms);
#else
	struct timespec sleepValue = {0};
	sleepValue.tv_nsec = interval_ms * 500000;
	nanosleep(&sleepValue, NULL);
#endif
}
