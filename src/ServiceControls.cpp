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

#include <queue>

#include "OneService.hpp"
#include "Node.hpp"
#include "ZeroTierOne.h"

#include "Constants.hpp"
#include "VirtualTapManager.hpp"
#include "lwIP.h"
#include "OSUtils.hpp"
#include "ServiceControls.hpp"

#include "lwip/stats.h"

#if defined(_WIN32)
WSADATA wsaData;
#include <Windows.h>
#endif

#ifdef SDK_JNI
#include <jni.h>
#endif

namespace ZeroTier {

/*
 * A lock used to protect any call which relies on the presence of a valid pointer
 * to the ZeroTier service.
 */
Mutex _service_lock;

/*
 * A lock which protects flags and state variables used during the startup and
 * shutdown phase.
 */
Mutex _startup_lock;

/*
 * A lock used to protect callback method pointers. With a coarser-grained lock it
 * would be possible for one thread to alter the callback method pointer causing 
 * undefined behaviour.
 */
Mutex _callback_lock;

std::string homeDir;
int servicePort = ZTS_DEFAULT_PORT;
bool _freeHasBeenCalled = false;
bool _serviceIsShuttingDown = false;
bool _startupError = false;
bool _nodeIsOnlineToggle = false;

pthread_t service_thread;
pthread_t callback_thread;

// Collection of virtual tap interfaces
std::map<uint64_t, VirtualTap*> vtapMap;
Mutex _vtaps_lock;

// Global reference to ZeroTier service
OneService *zt1Service;

// User-provided callback for ZeroTier events
void (*_userCallbackFunc)(uint64_t, int);
#ifdef SDK_JNI
// Global references to JNI objects and VM kept for future callbacks
static JavaVM *jvm = NULL;
jobject objRef = NULL;
jmethodID _userCallbackMethodRef = NULL;
#endif
}

using namespace ZeroTier;

//////////////////////////////////////////////////////////////////////////////
// Internal ZeroTier Service Controls (user application shall not use these)//
//////////////////////////////////////////////////////////////////////////////

std::queue<std::pair<uint64_t, int>*> _callbackMsgQueue;

void _push_callback_event(uint64_t nwid, int eventCode)
{
	_callback_lock.lock();
	if (_callbackMsgQueue.size() >= 128) {
		DEBUG_ERROR("too many callback messages in queue");
		_callback_lock.unlock();
		return;
	}
	_callbackMsgQueue.push(new std::pair<uint64_t, int>(nwid,eventCode));
	_callback_lock.unlock();
}

void _process_callback_event_helper(uint64_t nwid, int eventCode)
{
#ifdef SDK_JNI
	if(_userCallbackMethodRef) {
		JNIEnv *env;
		jint rs = jvm->AttachCurrentThread(&env, NULL);
		assert (rs == JNI_OK);
		env->CallVoidMethod(objRef, _userCallbackMethodRef, nwid, eventCode);
	}
#else
	if (_userCallbackFunc) {
		_userCallbackFunc((uint64_t)0, ZTS_EVENT_NODE_OFFLINE);
	}
#endif
}

void _process_callback_event(uint64_t nwid, int eventCode)
{
	_callback_lock.lock();
	_process_callback_event_helper(nwid, eventCode);
	_callback_lock.unlock();
}

bool _is_callback_registered()
{
	_callback_lock.lock();
	bool retval = false;
#ifdef SDK_JNI
	retval = (jvm && objRef && _userCallbackMethodRef);
#else
	retval = _userCallbackFunc;
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
	_userCallbackFunc = NULL;
#endif
	_callback_lock.unlock();
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

int _zts_node_online()
{
	return zt1Service && zt1Service->getNode() && zt1Service->getNode()->online();
}

int _zts_can_perform_service_operation()
{
	return zt1Service && zt1Service->isRunning() && zt1Service->getNode() && zt1Service->getNode()->online() && !_serviceIsShuttingDown;
}

void _hibernate_if_needed()
{
	if (VirtualTapManager::get_vtaps_size()) {
		lwip_wake_driver();
	} else {
		lwip_hibernate_driver();
	}
}
#ifdef SDK_JNI
#endif

/*
 * Monitors the conditions required for triggering callbacks into user code. This was made
 * into its own thread to prevent user application abuse of callbacks from affecting
 * the timing of more sensitive aspects of the library such as polling and RX/TX of packets
 */
#if defined(_WIN32)
DWORD WINAPI _zts_monitor_callback_conditions(LPVOID thread_id)
#else
void *_zts_monitor_callback_conditions(void *thread_id)
#endif
{
#if defined(__APPLE__)
	pthread_setname_np(ZTS_EVENT_CALLBACK_THREAD_NAME);
#endif
	while (!_serviceIsShuttingDown)
	{
		if (!_zts_node_online()) {
			if (_nodeIsOnlineToggle) {
				_nodeIsOnlineToggle = false;
				_process_callback_event((uint64_t)0, ZTS_EVENT_NODE_OFFLINE);
			}
		} if (_zts_node_online()) { // Only process pending network callbacks if the node is online
			if (!_nodeIsOnlineToggle) {
				_nodeIsOnlineToggle = true;
				_process_callback_event((uint64_t)0, ZTS_EVENT_NODE_ONLINE);
			}

			// First, handle queued messages from other threads
			_callback_lock.lock();
			while (_callbackMsgQueue.size()) {
				std::pair<uint64_t,int> *msg = _callbackMsgQueue.front();
				_callbackMsgQueue.pop();
				_process_callback_event_helper(msg->first, msg->second);
				delete msg;
			}
			_callback_lock.unlock();

			// Second, inspect network states for changes we should report
			_vtaps_lock.lock();
			ZT_VirtualNetworkList *nl = zt1Service->getNode()->networks();
			for(unsigned long i=0;i<nl->networkCount;++i) {
				OneService::NetworkSettings localSettings;
				zt1Service->getNetworkSettings(nl->networks[i].nwid,localSettings);
				if (vtapMap[nl->networks[i].nwid]->_lastReportedStatus != nl->networks[i].status) {
					switch (nl->networks[i].status) {
						case ZT_NETWORK_STATUS_NOT_FOUND:
							_process_callback_event(nl->networks[i].nwid, ZTS_EVENT_NETWORK_NOT_FOUND);
							break;
						case ZT_NETWORK_STATUS_CLIENT_TOO_OLD:
							_process_callback_event(nl->networks[i].nwid, ZTS_EVENT_NETWORK_CLIENT_TOO_OLD);
							break;
						case ZT_NETWORK_STATUS_REQUESTING_CONFIGURATION:
							_process_callback_event(nl->networks[i].nwid, ZTS_EVENT_NETWORK_REQUESTING_CONFIG);
							break;
						case ZT_NETWORK_STATUS_OK:
							_process_callback_event(nl->networks[i].nwid, ZTS_EVENT_NETWORK_OK);
							break;
						case ZT_NETWORK_STATUS_ACCESS_DENIED:
							_process_callback_event(nl->networks[i].nwid, ZTS_EVENT_NETWORK_ACCESS_DENIED);
							break;
						default:
							break;
					}
					vtapMap[nl->networks[i].nwid]->_lastReportedStatus = nl->networks[i].status;
				}
			}
			zt1Service->getNode()->freeQueryResult((void *)nl);
			// Finally, check for a more useful definition of "readiness"
			std::map<uint64_t, VirtualTap*>::iterator it;
			for (it = vtapMap.begin(); it != vtapMap.end(); it++) {
				VirtualTap *tap = it->second;
				if (tap->_lastConfigUpdateTime > 0 && !tap->_lastReadyReportTime && tap->_ips.size() > 0) {
					tap->_lastReadyReportTime = tap->_lastConfigUpdateTime;
					_process_callback_event(tap->_nwid, ZTS_EVENT_NETWORK_READY);
				}
			}
    		_vtaps_lock.unlock();
		}
		// Doesn't need to happen as often as other API operations
		_api_sleep(ZTS_WRAPPER_CHECK_INTERVAL);
	}
	DEBUG_ERROR("exiting from monitor loop");
}

// Starts a ZeroTier service in the background
#if defined(_WIN32)
DWORD WINAPI _zts_start_service(LPVOID thread_id)
#else
void *_zts_start_service(void *thread_id)
#endif
{	
#if defined(__APPLE__)
	pthread_setname_np(ZTS_SERVICE_THREAD_NAME);
#endif
	void *retval;
	zt1Service = (OneService *)0;

	if (!homeDir.length()) {
		DEBUG_ERROR("homeDir is empty, could not construct path");
		_startupError = true;
		retval = NULL;
	} if (zt1Service) {
		DEBUG_INFO("service already started, doing nothing");
		retval = NULL;
	}

	try {
		std::vector<std::string> hpsp(OSUtils::split(homeDir.c_str(), ZT_PATH_SEPARATOR_S,"",""));
		std::string ptmp;
		if (homeDir[0] == ZT_PATH_SEPARATOR) {
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
					_startupError = true;
					retval = NULL;
					perror("error\n");
				}
			}
		}
		if (!_startupError) {
			for(;;) {
				_service_lock.lock();
				zt1Service = OneService::newInstance(homeDir.c_str(),servicePort);
				_service_lock.unlock();
				switch(zt1Service->run()) {
					case OneService::ONE_STILL_RUNNING:
					case OneService::ONE_NORMAL_TERMINATION:
						_process_callback_event((uint64_t)0, ZTS_EVENT_NODE_NORMAL_TERMINATION);
						break;
					case OneService::ONE_UNRECOVERABLE_ERROR:
						DEBUG_ERROR("fatal error: %s", zt1Service->fatalErrorMessage().c_str());
						_startupError = true;
						_process_callback_event((uint64_t)0, ZTS_EVENT_NODE_UNRECOVERABLE_ERROR);
						break;
					case OneService::ONE_IDENTITY_COLLISION: {
						_startupError = true;
						delete zt1Service;
						zt1Service = (OneService *)0;
						std::string oldid;
						OSUtils::readFile((homeDir + ZT_PATH_SEPARATOR_S + "identity.secret").c_str(),oldid);
						if (oldid.length()) {
							OSUtils::writeFile((homeDir + ZT_PATH_SEPARATOR_S + "identity.secret.saved_after_collision").c_str(),oldid);
							OSUtils::rm((homeDir + ZT_PATH_SEPARATOR_S + "identity.secret").c_str());
							OSUtils::rm((homeDir + ZT_PATH_SEPARATOR_S + "identity.public").c_str());
						}
						_process_callback_event((uint64_t)0, ZTS_EVENT_NODE_IDENTITY_COLLISION);
					}	continue; // restart!
				}
				break; // terminate loop -- normally we don't keep restarting
			}
		}
		
		_serviceIsShuttingDown = true;
		_service_lock.lock();
		delete zt1Service;
		zt1Service = (OneService *)0;
		_service_lock.unlock();
		_serviceIsShuttingDown = false;
		_process_callback_event((uint64_t)0, ZTS_EVENT_NODE_DOWN);
	} catch ( ... ) {
		DEBUG_ERROR("unexpected exception starting ZeroTier instance");
	}
	pthread_exit(NULL);
}

#ifdef __cplusplus
extern "C" {
#endif

//////////////////////////////////////////////////////////////////////////////
// ZeroTier Service Controls                                                //
//////////////////////////////////////////////////////////////////////////////

#ifdef SDK_JNI
/*
 * Called from Java, saves a reference to the VM so it can be used later to call
 * a user-specified callback method from C.
 */
JNIEXPORT void JNICALL Java_com_zerotier_libzt_ZeroTier_init(JNIEnv *env, jobject thisObj)
{
	jint rs = env->GetJavaVM(&jvm);
   assert (rs == JNI_OK);
}
#endif

zts_err_t zts_set_service_port(int portno)
{
	zts_err_t retval = ZTS_ERR_OK;
	_service_lock.lock();
	if (zt1Service) {
		// Stop service before attempting to set a port
		retval = ZTS_ERR_SERVICE;
	}
	else {
		if (portno > -1 && portno < ZTS_MAX_PORT) {
			// 0 is allowed, signals to ZT service to bind to a random port
			servicePort = portno;
			retval = ZTS_ERR_OK;
		}
	}
	_service_lock.unlock();
	return retval;
}
#ifdef SDK_JNI
JNIEXPORT void JNICALL Java_com_zerotier_libzt_ZeroTier_set_1service_1port(
	JNIEnv *env, jobject thisObj, jint port)
{
	zts_set_service_port(port);
}
#endif

int zts_get_service_port()
{
	return servicePort;
}
#ifdef SDK_JNI
JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_get_1service_1port(
	JNIEnv *env, jobject thisObj)
{
	return zts_get_service_port();
}
#endif

zts_err_t zts_join(const uint64_t nwid, int blocking)
{
	zts_err_t retval = ZTS_ERR_OK;
	retval = VirtualTapManager::get_vtaps_size() >= ZTS_MAX_JOINED_NETWORKS ? ZTS_ERR_INVALID_OP : ZTS_ERR_OK;
	if (retval == ZTS_ERR_OK) {
		_service_lock.lock();
		if (blocking) {
			if (!zt1Service) {
				retval = ZTS_ERR_SERVICE;
			} else {
				while (!_zts_node_online()) {
					if (_serviceIsShuttingDown) {
						retval = ZTS_ERR_SERVICE;
						break;
					}
					_api_sleep(ZTS_WRAPPER_CHECK_INTERVAL);
				}
			}
		} else {
			if (!zt1Service || !_zts_node_online()) {
				retval = ZTS_ERR_SERVICE;
			}
		}
		if (retval == ZTS_ERR_OK) {
			if (nwid == 0) {
				retval = ZTS_ERR_INVALID_ARG;
			}
			if (zt1Service) {
				zt1Service->getNode()->join(nwid, NULL, NULL);
			}
		}
		_service_lock.unlock();
		_hibernate_if_needed();
	}
	return retval;
}
#ifdef SDK_JNI
JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_join(
	JNIEnv *env, jobject thisObj, jlong nwid)
{
	return zts_join((uint64_t)nwid);
}
#endif

zts_err_t zts_leave(const uint64_t nwid, int blocking)
{
	zts_err_t retval = ZTS_ERR_OK;
	_service_lock.lock();
	if (blocking) {
		if (!zt1Service) {
			retval = ZTS_ERR_SERVICE;
		} else {
			while (!_zts_node_online()) {
				if (_serviceIsShuttingDown) {
					retval = ZTS_ERR_SERVICE;
					break;
				}
				_api_sleep(ZTS_WRAPPER_CHECK_INTERVAL);
			}
		}
	} else {
		if (!zt1Service || !_zts_node_online()) {
			retval = ZTS_ERR_SERVICE;
		}
	}
	if (retval == ZTS_ERR_OK) {
		if (nwid == 0) {
			retval = ZTS_ERR_INVALID_ARG;
		}
		if (zt1Service) {
			zt1Service->getNode()->leave(nwid, NULL, NULL);
		}
	}
	VirtualTapManager::remove_by_nwid(nwid);
	_hibernate_if_needed();
	_service_lock.unlock();
	return retval;
}
#ifdef SDK_JNI
JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_leave(
	JNIEnv *env, jobject thisObj, jlong nwid)
{
	return zts_leave((uint64_t)nwid);
}
#endif

zts_err_t zts_leave_all(int blocking)
{
	zts_err_t retval = ZTS_ERR_OK;
	if (!zt1Service || _freeHasBeenCalled || _serviceIsShuttingDown) {
		retval = ZTS_ERR_SERVICE;
	}
	else {
		struct zts_network_details nds[ZTS_MAX_JOINED_NETWORKS];
		int numJoined = ZTS_MAX_JOINED_NETWORKS;
		zts_get_all_network_details(nds, &numJoined);
		for (int i=0; i<numJoined; i++) {
			zts_leave(nds[i].nwid);
		}
	}
	return retval;
}
#ifdef SDK_JNI
#endif

zts_err_t zts_orbit(uint64_t moonWorldId, uint64_t moonSeed)
{
	zts_err_t retval = ZTS_ERR_OK;
	void *tptr = NULL;
	_service_lock.lock();
	if (!zt1Service || _freeHasBeenCalled || _serviceIsShuttingDown) {
		retval = ZTS_ERR_SERVICE;
	}
	else if (_zts_can_perform_service_operation()) {
		zt1Service->getNode()->orbit(tptr, moonWorldId, moonSeed);
	}
	_service_lock.unlock();
	return retval;
}
#ifdef SDK_JNI
#endif

zts_err_t zts_deorbit(uint64_t moonWorldId)
{
	zts_err_t retval = ZTS_ERR_OK;
	void *tptr = NULL;
	_service_lock.lock();
	if (!zt1Service || _freeHasBeenCalled || _serviceIsShuttingDown) {
		retval = ZTS_ERR_SERVICE;
	}
	else if (_zts_can_perform_service_operation()) {
		zt1Service->getNode()->deorbit(tptr, moonWorldId);
	}
	_service_lock.unlock();
	return retval;
}
#ifdef SDK_JNI
#endif

int zts_core_running()
{
	_service_lock.lock();
	int retval = zt1Service == NULL ? false : zt1Service->isRunning();
	_service_lock.unlock();
	return retval;
}
#ifdef SDK_JNI
JNIEXPORT jboolean JNICALL Java_com_zerotier_libzt_ZeroTier_core_1running(
	JNIEnv *env, jobject thisObj)
{
	return zts_core_running();
}
#endif

int zts_ready()
{
	_service_lock.lock();
	bool stackRunning = VirtualTapManager::get_vtaps_size() > 0 ? true : false;
	_service_lock.unlock();
	return zts_core_running() && stackRunning;
}
#ifdef SDK_JNI
JNIEXPORT jboolean JNICALL Java_com_zerotier_libzt_ZeroTier_ready(
	JNIEnv *env, jobject thisObj)
{
	return zts_ready();
}
#endif

zts_err_t zts_start_with_callback(const char *path, void (*callback)(uint64_t, int), int blocking)
{
	_startup_lock.lock();
	zts_err_t retval = ZTS_ERR_OK;
	if (zt1Service) {
		// Service is already initialized
		retval = ZTS_ERR_SERVICE;
	}
	if (_freeHasBeenCalled) {
		// Stack (presumably lwIP) has been dismantled, an application restart is required now
		retval = ZTS_ERR_INVALID_OP;
	}
	if (!path) {
		retval = ZTS_ERR_INVALID_ARG;
	}

    _userCallbackFunc = callback;

	if (retval == ZTS_ERR_OK) {
		homeDir = path;
#if defined(_WIN32)
		// initialize WinSock. Used in Phy for loopback pipe
		WSAStartup(MAKEWORD(2, 2), &wsaData);
		HANDLE serviceThread = CreateThread(NULL, 0, _zts_start_service, NULL, 0, NULL);
		if (_is_callback_registered()) {
			HANDLE callbackThread = CreateThread(NULL, 0, _zts_monitor_callback_conditions, NULL, 0, NULL);
		}
		// TODO: Add thread names on Windows (optional)
#else
		_startupError = false;
		retval = pthread_create(&service_thread, NULL, _zts_start_service, NULL);
#if defined(__linux__)
		pthread_setname_np(service_thread, ZTS_SERVICE_THREAD_NAME);
#endif
		if (_is_callback_registered()) {
			retval = pthread_create(&callback_thread, NULL, _zts_monitor_callback_conditions, NULL);
#if defined(__linux__)
			pthread_setname_np(callback_thread, ZTS_EVENT_CALLBACK_THREAD_NAME);
#endif	
		}	
		// Wait for confirmation that the ZT service has been initialized, 
		// this wait condition is so brief and so rarely used that it should be
		// acceptable even in a non-blocking context.
		while(!zt1Service) {
			if (_serviceIsShuttingDown || _startupError) {
				// ZT service startup/binding might have failed for some reason
				retval = ZTS_ERR_SERVICE;
				break;
			}
			_api_sleep(10);
		}
#endif
		if (blocking && retval == ZTS_ERR_OK) {
			// block to prevent service calls before we're ready
			// waiting for zerotier service thread to start
			while (zts_core_running() == false || zt1Service->getNode() == NULL) {
				if (_serviceIsShuttingDown || _startupError) {
					// ZT service startup/binding might have failed for some reason
					retval = ZTS_ERR_SERVICE;
					break;
				}
				_api_sleep(ZTS_WRAPPER_CHECK_INTERVAL);
			}
			if (retval == ZTS_ERR_OK) {
				// waiting for node address assignment
				while (zt1Service->getNode()->address() <= 0) {
					if (_serviceIsShuttingDown || _startupError) {
						retval = ZTS_ERR_SERVICE;
						break;
					}
					_api_sleep(ZTS_WRAPPER_CHECK_INTERVAL);
				}
			}
			if (retval == ZTS_ERR_OK) {
				// Waiting for node to come online. Ensure the node is authorized to join the network
				while (true) {
					_service_lock.lock();
					if (_serviceIsShuttingDown || _startupError) {
						retval = ZTS_ERR_SERVICE;
						break;
					}
					if (zt1Service && zt1Service->getNode() && zt1Service->getNode()->online()) {
						// Node is fully online
						break;
					}
					_api_sleep(ZTS_WRAPPER_CHECK_INTERVAL);
					_service_lock.unlock();
				}
				_service_lock.unlock();
			}
		}
	}
	_startup_lock.unlock();
	// if (blocking && retval == ZTS_ERR_OK) { DEBUG_INFO("node=%llx online", (unsigned long long)zts_get_node_id());}
	_hibernate_if_needed();
	return retval;
}

#ifdef SDK_JNI
JNIEXPORT void JNICALL Java_com_zerotier_libzt_ZeroTier_start_1with_1callback(
	JNIEnv *env, jobject thisObj, jstring path, jobject userCallbackClass)
{
	jclass eventListenerClass = env->GetObjectClass(userCallbackClass);
	if(eventListenerClass == NULL) {
		DEBUG_ERROR("Couldn't find class for ZeroTierEventListener instance");
		return;
	}
	jmethodID eventListenerCallbackMethod = env->GetMethodID(eventListenerClass, "onZeroTierEvent", "(JI)V");
	if(eventListenerCallbackMethod == NULL) {
		DEBUG_ERROR("Couldn't find onZeroTierEvent method");
		return;
	}
	objRef = env->NewGlobalRef(userCallbackClass); // Reference used for later calls
	_userCallbackMethodRef = eventListenerCallbackMethod;
	if (path) {
		const char* utf_string = env->GetStringUTFChars(path, NULL);
		zts_start_with_callback(utf_string, NULL, false);
		env->ReleaseStringUTFChars(path, utf_string);
	}
}
#endif

zts_err_t zts_start(const char *path, int blocking = false)
{
	return zts_start_with_callback(path, NULL, blocking);
}
#ifdef SDK_JNI
JNIEXPORT void JNICALL Java_com_zerotier_libzt_ZeroTier_start(
	JNIEnv *env, jobject thisObj, jstring path, jboolean blocking)
{
	if (path) {
		const char* utf_string = env->GetStringUTFChars(path, NULL);
		zts_start(utf_string, blocking);
		env->ReleaseStringUTFChars(path, utf_string);
	}
}
#endif

zts_err_t zts_startjoin(const char *path, const uint64_t nwid)
{
	zts_err_t retval = ZTS_ERR_OK;
	if ((retval = zts_start(path, true)) < 0) {
		return retval;
	}
	while (true) {
		try {
			zts_join(nwid);
			break;
		}
		catch( ... ) {
			_api_sleep(ZTS_WRAPPER_CHECK_INTERVAL);
			retval = ZTS_ERR_SERVICE;
		}
	}
	_hibernate_if_needed();
	return retval;
}
#ifdef SDK_JNI
JNIEXPORT void JNICALL Java_com_zerotier_libzt_ZeroTier_startjoin(
	JNIEnv *env, jobject thisObj, jstring path, jlong nwid)
{
	if (path && nwid) {
		const char* utf_string = env->GetStringUTFChars(path, NULL);
		zts_startjoin(utf_string, (uint64_t)nwid);
		env->ReleaseStringUTFChars(path, utf_string);
	}
}
#endif

zts_err_t zts_stop(int blocking)
{
	zts_err_t retval = ZTS_ERR_OK;
	_service_lock.lock();
	bool didStop = false;
	if (_zts_can_perform_service_operation()) {
		didStop = true;
		zt1Service->terminate();
		VirtualTapManager::clear();
	}
	else {
		// Nothing to do
		retval = ZTS_ERR_SERVICE;
	}
#if defined(_WIN32)
	WSACleanup();
#endif
	_service_lock.unlock();
	if (blocking && retval == ZTS_ERR_OK && didStop) {
		// Block until ZT service thread successfully exits
		pthread_join(service_thread, NULL);
	}
	_hibernate_if_needed();
	_clear_registered_callback();
	return retval;
}
#ifdef SDK_JNI
JNIEXPORT void JNICALL Java_com_zerotier_libzt_ZeroTier_stop(
	JNIEnv *env, jobject thisObj)
{
	zts_stop();
}
#endif

zts_err_t zts_free()
{
	zts_err_t retval = 0;
	_service_lock.lock();
	if (_freeHasBeenCalled) {
		retval = ZTS_ERR_INVALID_OP;
		_service_lock.unlock();
	} else {
		_freeHasBeenCalled = true;
		_service_lock.unlock();
		retval = zts_stop();
	}
	// PENDING: add stack shutdown logic
	return retval;
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
	uint64_t nodeId = 0;
	_service_lock.lock();
	if (_zts_can_perform_service_operation()) {
		nodeId = zt1Service->getNode()->address();
	}
	_service_lock.unlock();
	return nodeId;
}
#ifdef SDK_JNI
JNIEXPORT jlong JNICALL Java_com_zerotier_libzt_ZeroTier_get_1node_1id(
	JNIEnv *env, jobject thisObj)
{
	return zts_get_node_id();
}
#endif

int zts_get_peer_count()
{
	unsigned int peerCount = 0;
	_service_lock.lock();
	if (_zts_can_perform_service_operation()) {
		peerCount = zt1Service->getNode()->peers()->peerCount;
	} else {
		peerCount = ZTS_ERR_SERVICE;
	}
	_service_lock.unlock();
	return peerCount;
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
	zts_err_t retval = ZTS_ERR_OK;
	if (!pds || !num) {
		retval = ZTS_ERR_INVALID_ARG;
	}
	if (retval == ZTS_ERR_OK) {
		_service_lock.lock();
		if (_zts_can_perform_service_operation()) {
			ZT_PeerList *pl = zt1Service->getNode()->peers();
			if (pl) {
				*num = pl->peerCount;
				for(unsigned long i=0;i<pl->peerCount;++i) {
					memcpy(&(pds[i]), &(pl->peers[i]), sizeof(struct zts_peer_details));
				}
			}
		}
		else {
			retval = ZTS_ERR_SERVICE;
		}
		_service_lock.unlock();
	}
	return retval;
}
#ifdef SDK_JNI
#endif

zts_err_t zts_get_num_joined_networks()
{
	zts_err_t retval = ZTS_ERR_OK;
	if (!zt1Service || _freeHasBeenCalled || _serviceIsShuttingDown) {
		retval = ZTS_ERR_SERVICE;
	}
	else {
		retval = VirtualTapManager::get_vtaps_size();
	}
	return retval;
}
#ifdef SDK_JNI
JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_get_1num_1joined_1networks(
		JNIEnv *env, jobject thisObj)
{
	return zts_get_num_joined_networks();
}
#endif

zts_err_t zts_get_network_details(uint64_t nwid, struct zts_network_details *nd)
{
	zts_err_t retval = ZTS_ERR_OK;
	if (!nd || nwid == 0) {
		retval = ZTS_ERR_INVALID_ARG;
	}
	if (!zt1Service || _freeHasBeenCalled || _serviceIsShuttingDown) {
		retval = ZTS_ERR_SERVICE;
	}
	if (retval == ZTS_ERR_OK) {
		VirtualTapManager::get_network_details(zt1Service, nwid, nd);
	}
	return retval;
}
#ifdef SDK_JNI
#endif

zts_err_t zts_get_all_network_details(struct zts_network_details *nds, int *num)
{
	zts_err_t retval = ZTS_ERR_OK;
	if (!nds || !num) {
		retval = ZTS_ERR_INVALID_ARG;
	}
	if (!zt1Service || _freeHasBeenCalled || _serviceIsShuttingDown) {
		retval = ZTS_ERR_SERVICE;
	}
	if (retval == ZTS_ERR_OK) {
		VirtualTapManager::get_all_network_details(zt1Service, nds, num);
	}
	return retval;	
}
#ifdef SDK_JNI
#endif

zts_err_t zts_enable_http_backplane_mgmt()
{
	zts_err_t retval = ZTS_ERR_OK;
	_service_lock.lock();
	if (!zt1Service || _freeHasBeenCalled || _serviceIsShuttingDown) {
		retval = ZTS_ERR_SERVICE;
	}
	else {
		zt1Service->allowHttpBackplaneManagement = true;
	}
	_service_lock.unlock();
	return retval;
}
#ifdef SDK_JNI
#endif

zts_err_t zts_disable_http_backplane_mgmt()
{
	zts_err_t retval = ZTS_ERR_OK;
	_service_lock.lock();
	if (!zt1Service || _freeHasBeenCalled || _serviceIsShuttingDown) {
		retval = ZTS_ERR_SERVICE;
	}
	else {
		zt1Service->allowHttpBackplaneManagement = false;
	}
	_service_lock.unlock();
	return retval;
}
#ifdef SDK_JNI
#endif

#ifdef __cplusplus
}
#endif
