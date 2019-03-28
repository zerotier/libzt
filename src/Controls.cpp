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
 * ZeroTier service controls
 */

#if defined(__linux__)
#include <sys/resource.h>
#endif

#include "Node.hpp"
#include "ZeroTierOne.h"
#include "OSUtils.hpp"

#include "Service.hpp"
#include "VirtualTap.hpp"
#include "Debug.hpp"
#include "concurrentqueue.h"
#include "ZeroTier.h"
#include "lwipDriver.hpp"

#if defined(_WIN32)
WSADATA wsaData;
#include <Windows.h>
#endif

#ifdef SDK_JNI
#include <jni.h>
#endif

namespace ZeroTier {

#ifdef __cplusplus
extern "C" {
#endif
// Custom errno to prevent conflicts with platform's own errno
int zts_errno;
#ifdef __cplusplus
}
#endif

struct serviceParameters
{
	int port;
	std::string path;
};

int _port;
std::string _path;

/*
 * A lock used to protect any call which relies on the presence of a valid
 * pointer to the ZeroTier service.
 */
Mutex _service_lock;

/*
 * A lock used to protect callback method pointers. With a coarser-grained
 * lock it would be possible for one thread to alter the callback method
 * pointer causing undefined behaviour.
 */
Mutex _callback_lock;

bool _freeHasBeenCalled = false;
bool _run_service = false;
bool _run_callbacks = false;
bool _run_lwip_tcpip = false;

//bool _startupError = false;

pthread_t service_thread;
pthread_t callback_thread;

// Global reference to ZeroTier service
OneService *service;

// User-provided callback for ZeroTier events
#ifdef SDK_JNI
	// Global references to JNI objects and VM kept for future callbacks
	static JavaVM *jvm = NULL;
	static jobject objRef = NULL;
	static jmethodID _userCallbackMethodRef = NULL;
#endif

void (*_userEventCallbackFunc)(struct zts_callback_msg *);

moodycamel::ConcurrentQueue<struct zts_callback_msg*> _callbackMsgQueue;

//////////////////////////////////////////////////////////////////////////////
// Internal ZeroTier Service Controls (user application shall not use these)//
//////////////////////////////////////////////////////////////////////////////

void postEvent(int eventCode, void *arg)
{
	struct zts_callback_msg *msg = new zts_callback_msg();

	msg->node = NULL;
	msg->network = NULL;
	msg->netif = NULL;
	msg->route = NULL;
	msg->path = NULL;
	msg->peer = NULL;
	msg->addr = NULL;

	msg->eventCode = eventCode;

	if (NODE_EVENT_TYPE(eventCode)) {
		msg->node = (struct zts_node_details*)arg;
	} if (NETWORK_EVENT_TYPE(eventCode)) {
		msg->network = (struct zts_network_details*)arg;
	} if (NETIF_EVENT_TYPE(eventCode)) {
		msg->netif = (struct zts_netif_details*)arg;
	} if (ROUTE_EVENT_TYPE(eventCode)) {
		msg->route = (struct zts_virtual_network_route*)arg;
	} if (PATH_EVENT_TYPE(eventCode)) {
		msg->path = (struct zts_physical_path*)arg;
	} if (PEER_EVENT_TYPE(eventCode)) {
		msg->peer = (struct zts_peer_details*)arg;
	} if (ADDR_EVENT_TYPE(eventCode)) {
		msg->addr = (struct zts_addr_details*)arg;
	}
    _callbackMsgQueue.enqueue(msg);
}

void postEvent(int eventCode) {
	postEvent(eventCode, (void*)0);
}

void freeEvent(struct zts_callback_msg *msg)
{
	if (!msg) {
		return;
	}
	if (msg->node) { delete msg->node; }
	if (msg->network) { delete msg->network; }
	if (msg->netif) { delete msg->netif; }
	if (msg->route) { delete msg->route; }
	if (msg->path) { delete msg->path; }
	if (msg->peer) { delete msg->peer; }
	if (msg->addr) { delete msg->addr; }
}

void _process_callback_event_helper(struct zts_callback_msg *msg)
{
#ifdef SDK_JNI
/* Old style callback messages are simply a uint64_t with a network/peer/node
if of some sort and an associated message code id. This is deprecated and here
only for legacy reasons. */
#if 1
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
		freeEvent(msg);
	}
#else
	if(_userCallbackMethodRef) {
		JNIEnv *env;
		jint rs = jvm->AttachCurrentThread(&env, NULL);
		assert (rs == JNI_OK);
		uint64_t arg = 0;
		if (NODE_EVENT_TYPE(msg->eventCode)) {
			DEBUG_INFO("NODE_EVENT_TYPE(%d)", msg->eventCode);
			arg = msg->node->address;
		}
		if (NETWORK_EVENT_TYPE(msg->eventCode)) {
			DEBUG_INFO("NETWORK_EVENT_TYPE(%d)", msg->eventCode);
			arg = msg->network->nwid;
		}
		if (PEER_EVENT_TYPE(msg->eventCode)) {
			DEBUG_INFO("PEER_EVENT_TYPE(%d)", msg->eventCode);
			arg = msg->peer->address;
		}
		env->CallVoidMethod(objRef, _userCallbackMethodRef, arg, msg->eventCode);
		freeEvent(msg);
#endif
#else
	if (_userEventCallbackFunc) {
		_userEventCallbackFunc(msg);
		freeEvent(msg);
	}
#endif
}

void _process_callback_event(struct zts_callback_msg *msg)
{
	_callback_lock.lock();
	_process_callback_event_helper(msg);
	_callback_lock.unlock();
}

bool _is_callback_registered()
{
	_callback_lock.lock();
	bool retval = false;
#ifdef SDK_JNI
	retval = (jvm && objRef && _userCallbackMethodRef);
#else
	retval = _userEventCallbackFunc;
#endif
	_callback_lock.unlock();
	return retval;
}

void _clear_registered_callback()
{
	_callback_lock.lock();
#ifdef SDK_JNI
	objRef = NULL;
    _userCallbackMethodRef = NULL;
#else
	_userEventCallbackFunc = NULL;
#endif
	_callback_lock.unlock();
}

int __zts_node_online()
{
	return service && service->getNode() && service->getNode()->online();
}

int __zts_can_perform_service_operation()
{
	return service
		&& service->isRunning()
		&& service->getNode()
		&& service->getNode()->online()
		&& !_freeHasBeenCalled;
}

void _api_sleep(int interval_ms)
{
#if defined(_WIN32)
	Sleep(interval_ms);
#else
	struct timespec sleepValue = {0};
	sleepValue.tv_nsec = interval_ms * 500000;
	nanosleep(&sleepValue, NULL);
#endif
}

int _change_nice(int increment)
{
	if (increment == 0) {
		return 0;
	}
	int  priority = getpriority(PRIO_PROCESS, 0);
	return setpriority(PRIO_PROCESS, 0, priority+increment);
}

//////////////////////////////////////////////////////////////////////////////
// Callback thread                                                          //
//////////////////////////////////////////////////////////////////////////////

#if defined(_WIN32)
DWORD WINAPI _zts_run_callbacks(LPVOID thread_id)
#else
void *_zts_run_callbacks(void *thread_id)
#endif
{
	_change_nice(CALLBACK_THREAD_NICENESS);
#if defined(__APPLE__)
	pthread_setname_np(ZTS_EVENT_CALLBACK_THREAD_NAME);
#endif
	while (_run_callbacks || _callbackMsgQueue.size_approx() > 0)
    {
        struct zts_callback_msg *msg;
		int sz = _callbackMsgQueue.size_approx();
		for (int j = 0; j < sz; j++) {
			if (_callbackMsgQueue.try_dequeue(msg)) {
				_process_callback_event(msg);
				delete msg;
			}
		}
        _api_sleep(ZTS_CALLBACK_PROCESSING_INTERVAL);
    }
#if SDK_JNI
	JNIEnv *env;
	jint rs = jvm->DetachCurrentThread();
    pthread_exit(0);
#endif
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////
// Service thread                                                           //
//////////////////////////////////////////////////////////////////////////////

// Starts a ZeroTier service in the background
#if defined(_WIN32)
DWORD WINAPI _zts_run_service(LPVOID arg)
#else
void *_zts_run_service(void *arg)
#endif
{	
#if defined(__APPLE__)
	pthread_setname_np(ZTS_SERVICE_THREAD_NAME);
#endif
	//struct serviceParameters *params = arg;
	//DEBUG_INFO("path=%s", params->path.c_str());
	int err;

	_change_nice(SERVICE_THREAD_NICENESS);

	try {
		std::vector<std::string> hpsp(OSUtils::split(_path.c_str(), ZT_PATH_SEPARATOR_S,"",""));
		std::string ptmp;
		if (_path[0] == ZT_PATH_SEPARATOR) {
			ptmp.push_back(ZT_PATH_SEPARATOR);
		}
		for (std::vector<std::string>::iterator pi(hpsp.begin());pi!=hpsp.end();++pi) {
			if (ptmp.length() > 0) {
				ptmp.push_back(ZT_PATH_SEPARATOR);
			}
			ptmp.append(*pi);
			if ((*pi != ".")&&(*pi != "..")) {
				if (OSUtils::mkdir(ptmp) == false) {
					DEBUG_ERROR("home path does not exist, and could not create");
					err = true;
					perror("error\n");
				}
			}
		}
		for(;;) {
			_service_lock.lock();
			service = OneService::newInstance(_path.c_str(),_port);
			_service_lock.unlock();
			switch(service->run()) {
				case OneService::ONE_STILL_RUNNING:
				case OneService::ONE_NORMAL_TERMINATION:
					postEvent(ZTS_EVENT_NODE_NORMAL_TERMINATION);
					break;
				case OneService::ONE_UNRECOVERABLE_ERROR:
					DEBUG_ERROR("fatal error: %s", service->fatalErrorMessage().c_str());
					err = true;
					postEvent(ZTS_EVENT_NODE_UNRECOVERABLE_ERROR);
					break;
				case OneService::ONE_IDENTITY_COLLISION: {
					err = true;
					delete service;
					service = (OneService *)0;
					std::string oldid;
					OSUtils::readFile((_path + ZT_PATH_SEPARATOR_S + "identity.secret").c_str(),oldid);
					if (oldid.length()) {
						OSUtils::writeFile((_path + ZT_PATH_SEPARATOR_S + "identity.secret.saved_after_collision").c_str(),oldid);
						OSUtils::rm((_path + ZT_PATH_SEPARATOR_S + "identity.secret").c_str());
						OSUtils::rm((_path + ZT_PATH_SEPARATOR_S + "identity.public").c_str());
					}
					postEvent(ZTS_EVENT_NODE_IDENTITY_COLLISION);
				}	continue; // restart!
			}
			break; // terminate loop -- normally we don't keep restarting
		}
		_service_lock.lock();
		_run_service = false;
		delete service;
		service = (OneService *)0;
		_service_lock.unlock();
		postEvent(ZTS_EVENT_NODE_DOWN);
	} catch ( ... ) {
		DEBUG_ERROR("unexpected exception starting ZeroTier instance");
	}
	//delete params;
	// TODO: Find a more elegant solution
	_api_sleep(ZTS_CALLBACK_PROCESSING_INTERVAL*2);
	_run_callbacks = false;
	pthread_exit(0);
}

#ifdef __cplusplus
extern "C" {
#endif

//////////////////////////////////////////////////////////////////////////////
// ZeroTier Service Controls                                                //
//////////////////////////////////////////////////////////////////////////////

#ifdef SDK_JNI
/*
 * Called from Java, saves a static reference to the VM so it can be used
 * later to call a user-specified callback method from C.
 */
JNIEXPORT int JNICALL Java_com_zerotier_libzt_ZeroTier_init(
	JNIEnv *env, jobject thisObj)
{
    jint rs = env->GetJavaVM(&jvm);
	return rs != JNI_OK ? ZTS_ERR_GENERAL : ZTS_ERR_OK;
}
#endif

int zts_join(const uint64_t nwid)
{
	Mutex::Lock _l(_service_lock);
	if (!__zts_can_perform_service_operation()) {
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
	Mutex::Lock _l(_service_lock);
	if (!__zts_can_perform_service_operation()) {
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
	Mutex::Lock _l(_service_lock);
	if (!__zts_can_perform_service_operation()) {
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
	Mutex::Lock _l(_service_lock);
	void *tptr = NULL;
	if (!__zts_can_perform_service_operation()) {
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
	Mutex::Lock _l(_service_lock);
	void *tptr = NULL;
	if (!__zts_can_perform_service_operation()) {
		return ZTS_ERR_SERVICE;
	} else {
		service->getNode()->deorbit(tptr, moonWorldId);
	}
	return ZTS_ERR_OK;
}
#ifdef SDK_JNI
#endif

int zts_start(
	const char *path, void (*callback)(struct zts_callback_msg*), int port)
{
	Mutex::Lock _l(_service_lock);
	lwip_driver_init();
	if (service || _run_service) {
		// Service is already initialized
		return ZTS_ERR_INVALID_OP;
	}
	if (_freeHasBeenCalled) {
		// Stack (presumably lwIP) has been dismantled,
		// an application restart is required now
		return ZTS_ERR_INVALID_OP;
	}
#ifdef SDK_JNI
	_userEventCallbackFunc = callback;
#endif
	_userEventCallbackFunc = callback;
	if (!_is_callback_registered()) {
		// Must have a callback
		return ZTS_ERR_INVALID_ARG;
	}
	if (!path) {
		return ZTS_ERR_INVALID_ARG;
	}
	if (port < 0 || port > 0xFFFF) {
		return ZTS_ERR_INVALID_ARG;
	}

	_path = std::string(path);
	_port = port;
	
	serviceParameters *params = new serviceParameters();

	/*
	params->port = port;
	DEBUG_INFO("path=%s", path);
	params->path = std::string(path);
	DEBUG_INFO("path=%s", params->path.c_str());

	if (params->path.length() == 0) {
		return ZTS_ERR_INVALID_ARG;
	}
	*/

	int err;
	int retval = ZTS_ERR_OK;
	_run_callbacks = true;
	_run_service = true;

	// Start the ZT service thread
#if defined(_WIN32)
	// Initialize WinSock. Used in Phy for loopback pipe
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	HANDLE serviceThread = CreateThread(NULL, 0, _zts_run_service, (void*)params, 0, NULL);
	HANDLE callbackThread = CreateThread(NULL, 0, _zts_run_callbacks, NULL, 0, NULL);
#endif
	if ((err = pthread_create(&service_thread, NULL, _zts_run_service, NULL)) != 0) {
		retval = err;
	}
	if ((err = pthread_create(&callback_thread, NULL, _zts_run_callbacks, NULL)) != 0) {
		retval = err;
	}
#if defined(__linux__)
	pthread_setname_np(service_thread, ZTS_SERVICE_THREAD_NAME);
	pthread_setname_np(callback_thread, ZTS_EVENT_CALLBACK_THREAD_NAME);
#endif

	if (retval != ZTS_ERR_OK) {
		_run_callbacks = false;
		_run_service = false;
		_clear_registered_callback();
		delete params;
	}
	return retval;
}

#ifdef SDK_JNI
JNIEXPORT int JNICALL Java_com_zerotier_libzt_ZeroTier_start(
	JNIEnv *env, jobject thisObj, jstring path, jobject callback, jint port)
{
	if (!path) {
		return ZTS_ERR_INVALID_ARG;
	}
	jclass eventListenerClass = env->GetObjectClass(callback);
	if(eventListenerClass == NULL) {
		DEBUG_ERROR("Couldn't find class for ZeroTierEventListener instance");
		return ZTS_ERR_INVALID_ARG;
	}
	jmethodID eventListenerCallbackMethod = env->GetMethodID(eventListenerClass, "onZeroTierEvent", "(JI)V");
	if(eventListenerCallbackMethod == NULL) {
		DEBUG_ERROR("Couldn't find onZeroTierEvent method");
		return ZTS_ERR_INVALID_ARG;
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
	Mutex::Lock _l(_service_lock);
	if (__zts_can_perform_service_operation()) {
		_run_service = false;
		service->terminate();
#if defined(_WIN32)
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
	_service_lock.lock();
	// Store callback references
#ifdef SDK_JNI
	static jmethodID _tmpUserCallbackMethodRef = _userCallbackMethodRef;
#else
	void (*_tmpUserEventCallbackFunc)(struct zts_callback_msg *);
	_tmpUserEventCallbackFunc = _userEventCallbackFunc;
#endif
	int tmpPort = _port;
	std::string tmpPath = _path;
	// Stop the service
	if (__zts_can_perform_service_operation()) {
		_run_service = false;
		service->terminate();
#if defined(_WIN32)
		WSACleanup();
#endif
	}
	else {
		_service_lock.unlock();
		return ZTS_ERR_SERVICE;
	}
	// Start again with same parameters as initial call
	_service_lock.unlock();
	while (service) {
		_api_sleep(ZTS_CALLBACK_PROCESSING_INTERVAL);
	}
	/* Some of the logic in Java_com_zerotier_libzt_ZeroTier_start
	is replicated here */
#ifdef SDK_JNI
	_userCallbackMethodRef = _tmpUserCallbackMethodRef;
	return zts_start(tmpPath.c_str(), NULL, tmpPort);
#else
	return zts_start(tmpPath.c_str(), _tmpUserEventCallbackFunc, tmpPort);
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
	Mutex::Lock _l(_service_lock);
	int retval = 0;
	if (_freeHasBeenCalled) {
		return ZTS_ERR_INVALID_OP;
	}
	_freeHasBeenCalled = true;
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
	Mutex::Lock _l(_service_lock);
	if (!__zts_can_perform_service_operation()) {
		return ZTS_ERR_SERVICE;
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

//////////////////////////////////////////////////////////////////////////////
// Peers                                                                    //
//////////////////////////////////////////////////////////////////////////////

int zts_get_peer_count()
{
	Mutex::Lock _l(_service_lock);
	if (!__zts_can_perform_service_operation()) {
		return ZTS_ERR_SERVICE;
	}
	return service->getNode()->peers()->peerCount;
}
#ifdef SDK_JNI
JNIEXPORT jlong JNICALL Java_com_zerotier_libzt_ZeroTier_get_1peer_1count(
	JNIEnv *env, jobject thisObj)
{
	return zts_get_peer_count();
}
#endif

int zts_get_peers(struct zts_peer_details *pds, int *num)
{
	Mutex::Lock _l(_service_lock);
	if (!pds || !num) {
		return ZTS_ERR_INVALID_ARG;
	}
	if (!__zts_can_perform_service_operation()) {
		return ZTS_ERR_SERVICE;
	}
	ZT_PeerList *pl = service->getNode()->peers();
	if (pl) {
		if (*num < pl->peerCount) {
			service->getNode()->freeQueryResult((void *)pl);
			return ZTS_ERR_INVALID_ARG;
		}
		*num = pl->peerCount;
		for(unsigned long i=0;i<pl->peerCount;++i) {
			memcpy(&(pds[i]), &(pl->peers[i]), sizeof(struct zts_peer_details));
			for (int j=0; j<pl->peers[i].pathCount; j++) {
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
	Mutex::Lock _l(_service_lock);
	if (!pd || !peerId) {
		return ZTS_ERR_INVALID_ARG;
	}
	if (!__zts_can_perform_service_operation()) {
		return ZTS_ERR_SERVICE;
	}
	ZT_PeerList *pl = service->getNode()->peers();
	int retval = ZTS_ERR_NO_RESULT;
	if (pl) {
		for(unsigned long i=0;i<pl->peerCount;++i) {
			if (pl->peers[i].address == peerId) {
				memcpy(pd, &(pl->peers[i]), sizeof(struct zts_peer_details));
				for (int j=0; j<pl->peers[i].pathCount; j++) {
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

//////////////////////////////////////////////////////////////////////////////
// Networks                                                                 //
//////////////////////////////////////////////////////////////////////////////

int zts_get_num_joined_networks()
{
	Mutex::Lock _l(_service_lock);
	int retval = ZTS_ERR_OK;
	if (!__zts_can_perform_service_operation()) {
		return ZTS_ERR_SERVICE;
	}
	return service->networkCount();
}
#ifdef SDK_JNI
JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_get_1num_1joined_1networks(
		JNIEnv *env, jobject thisObj)
{
	return zts_get_num_joined_networks();
}
#endif

//////////////////////////////////////////////////////////////////////////////
// Network Details                                                          //
//////////////////////////////////////////////////////////////////////////////

void __get_network_details_helper(uint64_t nwid, struct zts_network_details *nd)
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
    __get_network_details_helper(nwid, nd);
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
	_service_lock.lock();
	int retval = ZTS_ERR_OK;
	if (!nd || nwid == 0) {
		retval = ZTS_ERR_INVALID_ARG;
	}
	if (!service || _freeHasBeenCalled || _serviceIsShuttingDown) {
		retval = ZTS_ERR_SERVICE;
	}
	if (retval == ZTS_ERR_OK) {
		_get_network_details(nwid, nd);
	}
	_service_lock.unlock();
	return retval;
	*/
	return 0;
}
#ifdef SDK_JNI
#endif

int zts_get_all_network_details(struct zts_network_details *nds, int *num)
{
	/*
	_service_lock.lock();
	int retval = ZTS_ERR_OK;
	if (!nds || !num) {
		retval = ZTS_ERR_INVALID_ARG;
	}
	if (!service || _freeHasBeenCalled || _serviceIsShuttingDown) {
		retval = ZTS_ERR_SERVICE;
	}
	if (retval == ZTS_ERR_OK) {
		_get_all_network_details(nds, num);
	}
	_service_lock.unlock();
	return retval;	
	*/
	return 0;
}
#ifdef SDK_JNI
#endif

//////////////////////////////////////////////////////////////////////////////
// Statistics                                                               //
//////////////////////////////////////////////////////////////////////////////

#include "lwip/stats.h"

extern struct stats_ lwip_stats;

int zts_get_all_stats(struct zts_stats *statsDest)
{
#if LWIP_STATS
	if (!statsDest) {
		return ZTS_ERR_INVALID_ARG;
	}
	memset(statsDest, 0, sizeof(struct zts_stats));
	// Copy lwIP stats
	memcpy(&(statsDest->link), &(lwip_stats.link), sizeof(struct stats_proto));
	memcpy(&(statsDest->etharp), &(lwip_stats.etharp), sizeof(struct stats_proto));
	memcpy(&(statsDest->ip_frag), &(lwip_stats.ip_frag), sizeof(struct stats_proto));
	memcpy(&(statsDest->ip), &(lwip_stats.ip), sizeof(struct stats_proto));
	memcpy(&(statsDest->icmp), &(lwip_stats.icmp), sizeof(struct stats_proto));
	//memcpy(&(statsDest->igmp), &(lwip_stats.igmp), sizeof(struct stats_igmp));
	memcpy(&(statsDest->udp), &(lwip_stats.udp), sizeof(struct stats_proto));
	memcpy(&(statsDest->tcp), &(lwip_stats.tcp), sizeof(struct stats_proto));
	// mem omitted
	// memp omitted
	memcpy(&(statsDest->sys), &(lwip_stats.sys), sizeof(struct stats_sys));
	memcpy(&(statsDest->ip6), &(lwip_stats.ip6), sizeof(struct stats_proto));
	memcpy(&(statsDest->icmp6), &(lwip_stats.icmp6), sizeof(struct stats_proto));
	memcpy(&(statsDest->ip6_frag), &(lwip_stats.ip6_frag), sizeof(struct stats_proto));
	memcpy(&(statsDest->mld6), &(lwip_stats.mld6), sizeof(struct stats_igmp));
	memcpy(&(statsDest->nd6), &(lwip_stats.nd6), sizeof(struct stats_proto));
	memcpy(&(statsDest->ip_frag), &(lwip_stats.ip_frag), sizeof(struct stats_proto));
	// mib2 omitted
	// Copy ZT stats
	// ...
	return ZTS_ERR_OK;
#else
	return ZTS_ERR_NO_RESULT;
#endif
}
#ifdef SDK_JNI
	// No implementation for JNI
#endif

int zts_get_protocol_stats(int protocolType, void *protoStatsDest)
{
#if LWIP_STATS
	if (!protoStatsDest) {
		return ZTS_ERR_INVALID_ARG;
	}
	memset(protoStatsDest, 0, sizeof(struct stats_proto));
	switch (protocolType)
	{
		case ZTS_STATS_PROTOCOL_LINK:
			memcpy(protoStatsDest, &(lwip_stats.link), sizeof(struct stats_proto));
			break;
		case ZTS_STATS_PROTOCOL_ETHARP:
			memcpy(protoStatsDest, &(lwip_stats.etharp), sizeof(struct stats_proto));
			break;
		case ZTS_STATS_PROTOCOL_IP:
			memcpy(protoStatsDest, &(lwip_stats.ip), sizeof(struct stats_proto));
			break;
		case ZTS_STATS_PROTOCOL_UDP:
			memcpy(protoStatsDest, &(lwip_stats.udp), sizeof(struct stats_proto));
			break;
		case ZTS_STATS_PROTOCOL_TCP:
			memcpy(protoStatsDest, &(lwip_stats.tcp), sizeof(struct stats_proto));
			break;
		case ZTS_STATS_PROTOCOL_ICMP:
			memcpy(protoStatsDest, &(lwip_stats.icmp), sizeof(struct stats_proto));
			break;
		case ZTS_STATS_PROTOCOL_IP_FRAG:
			memcpy(protoStatsDest, &(lwip_stats.ip_frag), sizeof(struct stats_proto));
			break;
		case ZTS_STATS_PROTOCOL_IP6:
			memcpy(protoStatsDest, &(lwip_stats.ip6), sizeof(struct stats_proto));
			break;
		case ZTS_STATS_PROTOCOL_ICMP6:
			memcpy(protoStatsDest, &(lwip_stats.icmp6), sizeof(struct stats_proto));
			break;
		case ZTS_STATS_PROTOCOL_IP6_FRAG:
			memcpy(protoStatsDest, &(lwip_stats.ip6_frag), sizeof(struct stats_proto));
			break;
		default:
			return ZTS_ERR_INVALID_ARG;
	}
	return ZTS_ERR_OK;
#else
	return ZTS_ERR_NO_RESULT;
#endif
}
#ifdef SDK_JNI
JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_get_1protocol_1stats(
	JNIEnv *env, jobject thisObj, jint protocolType, jobject protoStatsObj)
{
	struct stats_proto stats;
	int retval = zts_get_protocol_stats(protocolType, &stats);
	// Copy stats into Java object
	jclass c = env->GetObjectClass(protoStatsObj);
	if (!c) {
		return ZTS_ERR_INVALID_ARG;
	}
	jfieldID fid;
	fid = env->GetFieldID(c, "xmit", "I");
	env->SetIntField(protoStatsObj, fid, stats.xmit);
	fid = env->GetFieldID(c, "recv", "I");
	env->SetIntField(protoStatsObj, fid, stats.recv);
	fid = env->GetFieldID(c, "fw", "I");
	env->SetIntField(protoStatsObj, fid, stats.fw);
	fid = env->GetFieldID(c, "drop", "I");
	env->SetIntField(protoStatsObj, fid, stats.drop);
	fid = env->GetFieldID(c, "chkerr", "I");
	env->SetIntField(protoStatsObj, fid, stats.chkerr);
	fid = env->GetFieldID(c, "lenerr", "I");
	env->SetIntField(protoStatsObj, fid, stats.lenerr);
	fid = env->GetFieldID(c, "memerr", "I");
	env->SetIntField(protoStatsObj, fid, stats.memerr);
	fid = env->GetFieldID(c, "rterr", "I");
	env->SetIntField(protoStatsObj, fid, stats.rterr);
	fid = env->GetFieldID(c, "proterr", "I");
	env->SetIntField(protoStatsObj, fid, stats.proterr);
	fid = env->GetFieldID(c, "opterr", "I");
	env->SetIntField(protoStatsObj, fid, stats.opterr);
	fid = env->GetFieldID(c, "err", "I");
	env->SetIntField(protoStatsObj, fid, stats.err);
	fid = env->GetFieldID(c, "cachehit", "I");
	env->SetIntField(protoStatsObj, fid, stats.cachehit);
	return retval;
}
#endif

//////////////////////////////////////////////////////////////////////////////
// Multipath/QoS                                                            //
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Status getters                                                           //
//////////////////////////////////////////////////////////////////////////////

int zts_get_node_status()
{
	Mutex::Lock _l(_service_lock);
	// Don't check __zts_can_perform_service_operation() here.
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
	Mutex::Lock _l(_service_lock);
	if (!networkId) {
		return ZTS_ERR_INVALID_ARG;
	}
	if (!__zts_can_perform_service_operation()) {
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
	Mutex::Lock _l(_service_lock);
	int retval = ZTS_ERR_OK;
	if (!__zts_can_perform_service_operation()) {
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

#ifdef __cplusplus
}
#endif

} // namespace ZeroTier