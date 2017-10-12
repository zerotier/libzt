/*
 * ZeroTier SDK - Network Virtualization Everywhere
 * Copyright (C) 2011-2017  ZeroTier, Inc.  https://www.zerotier.com/
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
 * ZeroTier One service control wrapper
 */

#include "ZT1Service.h"

#include "Debug.hpp"

#include "Phy.hpp"
#include "OneService.hpp"
#include "Utilities.h"
#include "OSUtils.hpp"

#ifdef __cplusplus
extern "C" {
#endif

namespace ZeroTier {
	std::vector<void*> vtaps;

	static ZeroTier::OneService *zt1Service;

	std::string homeDir; // Platform-specific dir we *must* use internally
	std::string netDir;  // Where network .conf files are to be written

	ZeroTier::Mutex _vtaps_lock;
	ZeroTier::Mutex _multiplexer_lock;
}

#if defined(__MINGW32__) || defined(__MINGW64__)
WSADATA wsaData;
#endif

/****************************************************************************/
/* ZeroTier Core helper functions for libzt - DON'T CALL THESE DIRECTLY     */
/****************************************************************************/

std::vector<ZT_VirtualNetworkRoute> *zts_get_network_routes(char *nwid)
{
	uint64_t nwid_int = strtoull(nwid, NULL, 16);
	return ZeroTier::zt1Service->getRoutes(nwid_int);
}

ZeroTier::VirtualTap *getTapByNWID(uint64_t nwid)
{
	ZeroTier::_vtaps_lock.lock();
	ZeroTier::VirtualTap *s, *tap = nullptr;
	for (size_t i=0; i<ZeroTier::vtaps.size(); i++) {
		s = (ZeroTier::VirtualTap*)ZeroTier::vtaps[i];
		if (s->_nwid == nwid) { tap = s; }
	}
	ZeroTier::_vtaps_lock.unlock();
	return tap;
}

ZeroTier::VirtualTap *getTapByAddr(ZeroTier::InetAddress *addr)
{
	ZeroTier::_vtaps_lock.lock();
	ZeroTier::VirtualTap *s, *tap = nullptr;
	//char ipbuf[64], ipbuf2[64], ipbuf3[64];
	for (size_t i=0; i<ZeroTier::vtaps.size(); i++) {
		s = (ZeroTier::VirtualTap*)ZeroTier::vtaps[i];
		// check address schemes
		for (ssize_t j=0; j<s->_ips.size(); j++) {
			if ((s->_ips[j].isV4() && addr->isV4()) || (s->_ips[j].isV6() && addr->isV6())) {
				//DEBUG_EXTRA("looking at tap %s, <addr=%s> --- for <%s>", s->_dev.c_str(), s->_ips[j].toString(ipbuf), addr->toIpString(ipbuf2));
				if (s->_ips[j].isEqualPrefix(addr)
					|| s->_ips[j].ipsEqual(addr)
					|| s->_ips[j].containsAddress(addr)
					|| (addr->isV6() && ipv6_in_subnet(&s->_ips[j], addr))
					)
				{
					//DEBUG_EXTRA("selected tap %s, <addr=%s>", s->_dev.c_str(), s->_ips[j].toString(ipbuf));
					ZeroTier::_vtaps_lock.unlock();
					return s;
				}
			}
		}
		// check managed routes
		if (tap == NULL) {
			std::vector<ZT_VirtualNetworkRoute> *managed_routes = ZeroTier::zt1Service->getRoutes(s->_nwid);
			ZeroTier::InetAddress target, nm, via;
			for (size_t i=0; i<managed_routes->size(); i++) {
				target = managed_routes->at(i).target;
				nm = target.netmask();
				via = managed_routes->at(i).via;
				if (target.containsAddress(addr)) {
					//DEBUG_EXTRA("chose tap with route <target=%s, nm=%s, via=%s>", target.toString(ipbuf), nm.toString(ipbuf2), via.toString(ipbuf3));
					ZeroTier::_vtaps_lock.unlock();
					return s;
				}
			}
		}
	}
	ZeroTier::_vtaps_lock.unlock();
	return tap;
}

ZeroTier::VirtualTap *getTapByName(char *ifname)
{
	ZeroTier::_vtaps_lock.lock();
	ZeroTier::VirtualTap *s, *tap = nullptr;
	for (size_t i=0; i<ZeroTier::vtaps.size(); i++) {
		s = (ZeroTier::VirtualTap*)ZeroTier::vtaps[i];
		if (strcmp(s->_dev.c_str(), ifname) == false) {
			tap = s;
		}
	}
	ZeroTier::_vtaps_lock.unlock();
	return tap;
}

ZeroTier::VirtualTap *getTapByIndex(size_t index)
{
	ZeroTier::_vtaps_lock.lock();
	ZeroTier::VirtualTap *s, *tap = nullptr;
	for (size_t i=0; i<ZeroTier::vtaps.size(); i++) {
		s = (ZeroTier::VirtualTap*)ZeroTier::vtaps[i];
		if (s->ifindex == index) {
			tap = s;
		}
	}
	ZeroTier::_vtaps_lock.unlock();
	return tap;
}

ZeroTier::VirtualTap *getAnyTap()
{
	ZeroTier::_vtaps_lock.lock();
	ZeroTier::VirtualTap *vtap = NULL;
	if (ZeroTier::vtaps.size()) {
		vtap = (ZeroTier::VirtualTap *)ZeroTier::vtaps[0];
	}
	ZeroTier::_vtaps_lock.unlock();
	return vtap;
}

int zts_get_device_id_from_file(const char *filepath, char *devID) 
{
	DEBUG_EXTRA();
	std::string fname("identity.public");
	std::string fpath(filepath);
	if (ZeroTier::OSUtils::fileExists((fpath + ZT_PATH_SEPARATOR_S + fname).c_str(),false)) {
		std::string oldid;
		ZeroTier::OSUtils::readFile((fpath + ZT_PATH_SEPARATOR_S + fname).c_str(),oldid);
		memcpy(devID, oldid.c_str(), 10); // first 10 bytes of file
		return 0;
	}
	return -1;
}

// Starts a ZeroTier service in the background
void *zts_start_service(void *thread_id)
{
	DEBUG_INFO("path=%s", ZeroTier::homeDir.c_str());
	// Where network .conf files will be stored
	ZeroTier::netDir = ZeroTier::homeDir + "/networks.d";
	ZeroTier::zt1Service = (ZeroTier::OneService *)0;
	// Construct path for network config and supporting service files
	if (ZeroTier::homeDir.length()) {
		std::vector<std::string> hpsp(ZeroTier::OSUtils::split(ZeroTier::homeDir.c_str(), ZT_PATH_SEPARATOR_S,"",""));
		std::string ptmp;
		if (ZeroTier::homeDir[0] == ZT_PATH_SEPARATOR) {
			ptmp.push_back(ZT_PATH_SEPARATOR);
		}
		for (std::vector<std::string>::iterator pi(hpsp.begin());pi!=hpsp.end();++pi) {
			if (ptmp.length() > 0) {
				ptmp.push_back(ZT_PATH_SEPARATOR);
			}
			ptmp.append(*pi);
			if ((*pi != ".")&&(*pi != "..")) {
				if (ZeroTier::OSUtils::mkdir(ptmp) == false) {
					DEBUG_ERROR("home path does not exist, and could not create");
					handle_general_failure();
					perror("error\n");
				}
			}
		}
	}
	else {
		DEBUG_ERROR("homeDir is empty, could not construct path");
		handle_general_failure();
		return NULL;
	}

	// Generate random port for new service instance
	unsigned int randp = 0;
	ZeroTier::Utils::getSecureRandom(&randp,sizeof(randp));
	// TODO: Better port random range selection
	int servicePort = 9000 + (randp % 1000);
	for (;;) {
		ZeroTier::zt1Service = ZeroTier::OneService::newInstance(ZeroTier::homeDir.c_str(),servicePort);
		switch(ZeroTier::zt1Service->run()) {
			case ZeroTier::OneService::ONE_STILL_RUNNING:
			case ZeroTier::OneService::ONE_NORMAL_TERMINATION:
				break;
			case ZeroTier::OneService::ONE_UNRECOVERABLE_ERROR:
				DEBUG_ERROR("ZTO service port = %d", servicePort);
				DEBUG_ERROR("fatal error: %s",ZeroTier::zt1Service->fatalErrorMessage().c_str());
				break;
			case ZeroTier::OneService::ONE_IDENTITY_COLLISION: {
				delete ZeroTier::zt1Service;
				ZeroTier::zt1Service = (ZeroTier::OneService *)0;
				std::string oldid;
				ZeroTier::OSUtils::readFile((ZeroTier::homeDir + ZT_PATH_SEPARATOR_S
					+ "identity.secret").c_str(),oldid);
				if (oldid.length()) {
					ZeroTier::OSUtils::writeFile((ZeroTier::homeDir + ZT_PATH_SEPARATOR_S
						+ "identity.secret.saved_after_collision").c_str(),oldid);
					ZeroTier::OSUtils::rm((ZeroTier::homeDir + ZT_PATH_SEPARATOR_S
						+ "identity.secret").c_str());
					ZeroTier::OSUtils::rm((ZeroTier::homeDir + ZT_PATH_SEPARATOR_S
						+ "identity.public").c_str());
				}
			}
			continue; // restart!
		}
		break; // terminate loop -- normally we don't keep restarting
	}
	delete ZeroTier::zt1Service;
	ZeroTier::zt1Service = (ZeroTier::OneService *)0;
	return NULL;
}

void disableTaps()
{
	DEBUG_EXTRA();
	ZeroTier::_vtaps_lock.lock();
	for (size_t i=0; i<ZeroTier::vtaps.size(); i++) {
		DEBUG_EXTRA("vt=%p", ZeroTier::vtaps[i]);
		((ZeroTier::VirtualTap*)ZeroTier::vtaps[i])->_enabled = false;
	}
	ZeroTier::_vtaps_lock.unlock();
}

void zts_get_ipv4_address(const char *nwid, char *addrstr, const size_t addrlen)
{
	DEBUG_EXTRA();
	if (ZeroTier::zt1Service) {
		uint64_t nwid_int = strtoull(nwid, NULL, 16);
		ZeroTier::VirtualTap *tap = getTapByNWID(nwid_int);
		if (tap && tap->_ips.size()) {
			for (size_t i=0; i<tap->_ips.size(); i++) {
				if (tap->_ips[i].isV4()) {
					char ipbuf[INET_ADDRSTRLEN];
					std::string addr = tap->_ips[i].toString(ipbuf);
					int len = addrlen < addr.length() ? addrlen : addr.length();
					memset(addrstr, 0, len);
					memcpy(addrstr, addr.c_str(), len);
					return;
				}
			}
		}
	}
	else
		memcpy(addrstr, "\0", 1);
}

void zts_get_ipv6_address(const char *nwid, char *addrstr, size_t addrlen)
{
	DEBUG_EXTRA();
	if (ZeroTier::zt1Service) {
		uint64_t nwid_int = strtoull(nwid, NULL, 16);
		ZeroTier::VirtualTap *tap = getTapByNWID(nwid_int);
		if (tap && tap->_ips.size()) {
			for (size_t i=0; i<tap->_ips.size(); i++) {
				if (tap->_ips[i].isV6()) {
					char ipbuf[INET6_ADDRSTRLEN];
					std::string addr = tap->_ips[i].toString(ipbuf);
					int len = addrlen < addr.length() ? addrlen : addr.length();
					memset(addrstr, 0, len);
					memcpy(addrstr, addr.c_str(), len);
					return;
				}
			}
		}
	}
	else
		memcpy(addrstr, "\0", 1);
}

int zts_has_ipv4_address(const char *nwid)
{
	DEBUG_EXTRA();
	char ipv4_addr[INET_ADDRSTRLEN];
	memset(ipv4_addr, 0, INET_ADDRSTRLEN);
	zts_get_ipv4_address(nwid, ipv4_addr, INET_ADDRSTRLEN);
	return strcmp(ipv4_addr, "\0");
}

int zts_has_ipv6_address(const char *nwid)
{
	DEBUG_EXTRA();
	char ipv6_addr[INET6_ADDRSTRLEN];
	memset(ipv6_addr, 0, INET6_ADDRSTRLEN);
	zts_get_ipv6_address(nwid, ipv6_addr, INET6_ADDRSTRLEN);
	return strcmp(ipv6_addr, "\0");
}

int zts_has_address(const char *nwid)
{
	DEBUG_EXTRA();
	return zts_has_ipv4_address(nwid) || zts_has_ipv6_address(nwid);
}


void zts_get_6plane_addr(char *addr, const char *nwid, const char *devID)
{
	DEBUG_EXTRA();
	ZeroTier::InetAddress _6planeAddr = ZeroTier::InetAddress::makeIpv66plane(
		ZeroTier::Utils::hexStrToU64(nwid),ZeroTier::Utils::hexStrToU64(devID));
	char ipbuf[INET6_ADDRSTRLEN];
	memcpy(addr, _6planeAddr.toIpString(ipbuf), 40);
}

void zts_get_rfc4193_addr(char *addr, const char *nwid, const char *devID)
{
	DEBUG_EXTRA();
	ZeroTier::InetAddress _6planeAddr = ZeroTier::InetAddress::makeIpv6rfc4193(
		ZeroTier::Utils::hexStrToU64(nwid),ZeroTier::Utils::hexStrToU64(devID));
	char ipbuf[INET6_ADDRSTRLEN];
	memcpy(addr, _6planeAddr.toIpString(ipbuf), 40);
}

void zts_join(const char * nwid) 
{
	DEBUG_EXTRA();
	if (ZeroTier::zt1Service) {
		std::string confFile = ZeroTier::zt1Service->givenHomePath() + "/networks.d/" + nwid + ".conf";
		if (ZeroTier::OSUtils::mkdir(ZeroTier::netDir) == false) {
			DEBUG_ERROR("unable to create: %s", ZeroTier::netDir.c_str());
			handle_general_failure();
		}
		if (ZeroTier::OSUtils::writeFile(confFile.c_str(), "") == false) {
			DEBUG_ERROR("unable to write network conf file: %s", confFile.c_str());
			handle_general_failure();
		}
		ZeroTier::zt1Service->join(nwid);
	}
	// provide ZTO service reference to virtual taps
	// TODO: This might prove to be unreliable, but it works for now
	for (size_t i=0;i<ZeroTier::vtaps.size(); i++) {
		ZeroTier::VirtualTap *s = (ZeroTier::VirtualTap*)ZeroTier::vtaps[i];
		s->zt1ServiceRef=(void*)ZeroTier::zt1Service;
	}
}

void zts_join_soft(const char * filepath, const char * nwid) 
{
	DEBUG_EXTRA();
	std::string net_dir = std::string(filepath) + "/networks.d/";
	std::string confFile = net_dir + std::string(nwid) + ".conf";
	if (ZeroTier::OSUtils::mkdir(net_dir) == false) {
		DEBUG_ERROR("unable to create: %s", net_dir.c_str());
		handle_general_failure();
	}
	if (ZeroTier::OSUtils::fileExists(confFile.c_str(), false) == false) {
		if (ZeroTier::OSUtils::writeFile(confFile.c_str(), "") == false) {
			DEBUG_ERROR("unable to write network conf file: %s", confFile.c_str());
			handle_general_failure();
		}
	}
}

void zts_leave(const char * nwid) 
{
	DEBUG_EXTRA();
	if (ZeroTier::zt1Service) {
		ZeroTier::zt1Service->leave(nwid);
	}
}

void zts_leave_soft(const char * filepath, const char * nwid) 
{
	DEBUG_EXTRA();
	std::string net_dir = std::string(filepath) + "/networks.d/";
	ZeroTier::OSUtils::rm((net_dir + nwid + ".conf").c_str());
}

int zts_running() 
{
	DEBUG_EXTRA();
	return ZeroTier::zt1Service == NULL ? false : ZeroTier::zt1Service->isRunning();
}

int zts_start(const char *path)
{
	DEBUG_EXTRA();
	if (ZeroTier::zt1Service) {
		return 0; // already initialized, ok
	}
	if (path) {
		ZeroTier::homeDir = path;
	}
#if defined(__MINGW32__) || defined(__MINGW64__)
		WSAStartup(MAKEWORD(2, 2), &wsaData); // initialize WinSock. Used in Phy for loopback pipe
#endif
	pthread_t service_thread;
	return pthread_create(&service_thread, NULL, zts_start_service, NULL);
}

int zts_startjoin(const char *path, const char *nwid)
{
	DEBUG_EXTRA();
	ZT_NodeStatus status;
	int err = zts_start(path);
	while (zts_running() == false || ZeroTier::zt1Service->getNode() == NULL) {
		nanosleep((const struct timespec[]) {{0, (ZTO_WRAPPER_CHECK_INTERVAL * 500000)}}, NULL);
	}
	while (ZeroTier::zt1Service->getNode()->address() <= 0) {
		nanosleep((const struct timespec[]) {{0, (ZTO_WRAPPER_CHECK_INTERVAL * 500000)}}, NULL);
	}
	while (status.online <= 0) {
		nanosleep((const struct timespec[]) {{0, (ZTO_WRAPPER_CHECK_INTERVAL * 500000)}}, NULL);
		ZeroTier::zt1Service->getNode()->status(&status);
	}
	// only now can we attempt a join
	while (true) {
		try {
			zts_join(nwid);
			break;
		}
		catch( ... ) {
			DEBUG_ERROR("there was a problem joining the virtual network %s", nwid);
			handle_general_failure();
		}
	}
	while (zts_has_address(nwid) == false) {
		nanosleep((const struct timespec[]) {{0, (ZTO_WRAPPER_CHECK_INTERVAL * 500000)}}, NULL);
	}
	return err;
}

void zts_stop() 
{
	DEBUG_EXTRA();
	if (ZeroTier::zt1Service) {
		ZeroTier::zt1Service->terminate();
		disableTaps();
	}
#if defined(__MINGW32__) || defined(__MINGW64__)
	WSACleanup(); // clean up WinSock
#endif
}

void zts_get_homepath(char *homePath, size_t len) 
{
	DEBUG_EXTRA();
	if (ZeroTier::homeDir.length()) {
		memset(homePath, 0, len);
		size_t buf_len = len < ZeroTier::homeDir.length() ? len : ZeroTier::homeDir.length();
		memcpy(homePath, ZeroTier::homeDir.c_str(), buf_len);
	}
}

int zts_get_device_id(char *devID) 
{
	DEBUG_EXTRA();
	if (ZeroTier::zt1Service) {
		char id[ZTO_ID_LEN];
		sprintf(id, "%lx",ZeroTier::zt1Service->getNode()->address());
		memcpy(devID, id, ZTO_ID_LEN);
		return 0;
	}
	else // Service isn't online, try to read ID from file
	{
		std::string fname("identity.public");
		std::string fpath(ZeroTier::homeDir);
		if (ZeroTier::OSUtils::fileExists((fpath + ZT_PATH_SEPARATOR_S + fname).c_str(),false)) {
			std::string oldid;
			ZeroTier::OSUtils::readFile((fpath + ZT_PATH_SEPARATOR_S + fname).c_str(),oldid);
			memcpy(devID, oldid.c_str(), ZTO_ID_LEN); // first 10 bytes of file
			return 0;
		}
	}
	return -1;
}

unsigned long zts_get_peer_count() 
{
	DEBUG_EXTRA();
	if (ZeroTier::zt1Service) {
		return ZeroTier::zt1Service->getNode()->peers()->peerCount;
	}
	else {
		return 0;
	}
}

int zts_get_peer_address(char *peer, const char *devID) 
{
	DEBUG_EXTRA();
	if (ZeroTier::zt1Service) {
		ZT_PeerList *pl = ZeroTier::zt1Service->getNode()->peers();
		// uint64_t addr;
		for (size_t i=0; i<pl->peerCount; i++) {
			// ZT_Peer *p = &(pl->peers[i]);
			// DEBUG_INFO("peer[%d] = %lx", i, p->address);
		}
		return pl->peerCount;
	}
	else
		return -1;
}

void zts_allow_http_control(bool allowed)
{
	DEBUG_EXTRA();
	// TODO
}


#if defined(SDK_JNI)

namespace ZeroTier {

	#include <jni.h>

	JNIEXPORT void JNICALL Java_zerotier_ZeroTier_ztjni_1start(JNIEnv *env, jobject thisObj, jstring path) {
		if (path) {
			homeDir = env->GetStringUTFChars(path, NULL);
			zts_start(homeDir.c_str());
		}
	}
	// Shuts down ZeroTier service and SOCKS5 Proxy server
	JNIEXPORT void JNICALL Java_zerotier_ZeroTier_ztjni_1stop(JNIEnv *env, jobject thisObj) {
		if (ZeroTier::zt1Service) {
			zts_stop();
		}
	}

	// Returns whether the ZeroTier service is running
	JNIEXPORT jboolean JNICALL Java_zerotier_ZeroTier_ztjni_1running(
		JNIEnv *env, jobject thisObj)
	{
		return  zts_running();
	}
	// Returns path for ZT config/data files
	JNIEXPORT jstring JNICALL Java_zerotier_ZeroTier_ztjni_1homepath(
		JNIEnv *env, jobject thisObj)
	{
		// TODO: fix, should copy into given arg
		// return (*env).NewStringUTF(zts_get_homepath());
		return (*env).NewStringUTF("");
	}
	// Join a network
	JNIEXPORT void JNICALL Java_zerotier_ZeroTier_ztjni_1join(
		JNIEnv *env, jobject thisObj, jstring nwid)
	{
		const char *nwidstr;
		if (nwid) {
			nwidstr = env->GetStringUTFChars(nwid, NULL);
			zts_join(nwidstr);
		}
	}
	// Leave a network
	JNIEXPORT void JNICALL Java_zerotier_ZeroTier_ztjni_1leave(
		JNIEnv *env, jobject thisObj, jstring nwid)
	{
		const char *nwidstr;
		if (nwid) {
			nwidstr = env->GetStringUTFChars(nwid, NULL);
			zts_leave(nwidstr);
		}
	}
	// FIXME: Re-implemented to make it play nicer with the C-linkage required for Xcode integrations
	// Now only returns first assigned address per network. Shouldn't normally be a problem
	JNIEXPORT jobject JNICALL Java_zerotier_ZeroTier_ztjni_1get_1ipv4_1address(
		JNIEnv *env, jobject thisObj, jstring nwid)
	{
		const char *nwid_str = env->GetStringUTFChars(nwid, NULL);
		char address_string[INET_ADDRSTRLEN];
		memset(address_string, 0, INET_ADDRSTRLEN);
		zts_get_ipv4_address(nwid_str, address_string, INET_ADDRSTRLEN);
		jclass clazz = (*env).FindClass("java/util/ArrayList");
		jobject addresses = (*env).NewObject(clazz, (*env).GetMethodID(clazz, "<init>", "()V"));
		jstring _str = (*env).NewStringUTF(address_string);
		env->CallBooleanMethod(addresses, env->GetMethodID(clazz, "add", "(Ljava/lang/Object;)Z"), _str);
		return addresses;
	}

	JNIEXPORT jobject JNICALL Java_zerotier_ZeroTier_ztjni_1get_1ipv6_1address(
		JNIEnv *env, jobject thisObj, jstring nwid)
	{
		const char *nwid_str = env->GetStringUTFChars(nwid, NULL);
		char address_string[INET6_ADDRSTRLEN];
		memset(address_string, 0, INET6_ADDRSTRLEN);
		zts_get_ipv6_address(nwid_str, address_string, INET6_ADDRSTRLEN);
		jclass clazz = (*env).FindClass("java/util/ArrayList");
		jobject addresses = (*env).NewObject(clazz, (*env).GetMethodID(clazz, "<init>", "()V"));
		jstring _str = (*env).NewStringUTF(address_string);
		env->CallBooleanMethod(addresses, env->GetMethodID(clazz, "add", "(Ljava/lang/Object;)Z"), _str);
		return addresses;
	}

	// Returns the device is in integer form
	JNIEXPORT jint Java_zerotier_ZeroTier_ztjni_1get_1device_1id()
	{
		return zts_get_device_id(NULL); // TODO
	}
}

#endif // SDK_JNI

#ifdef __cplusplus
}
#endif
