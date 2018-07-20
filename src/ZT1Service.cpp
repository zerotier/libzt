/*
 * ZeroTier SDK - Network Virtualization Everywhere
 * Copyright (C) 2011-2018  ZeroTier, Inc.  https://www.zerotier.com/
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

#include "libzt.h"
#include "ZT1Service.h"
#include "libztDebug.h"
#include "SysUtils.h"

#include "Phy.hpp"
#include "OneService.hpp"
#include "InetAddress.hpp"
#include "OSUtils.hpp"
#include "Mutex.hpp"

std::vector<void*> vtaps;
ZeroTier::Mutex _vtaps_lock;

#ifdef __cplusplus
extern "C" {
#endif

static ZeroTier::OneService *zt1Service;

std::string homeDir; // Platform-specific dir we *must* use internally
std::string netDir;  // Where network .conf files are to be written

ZeroTier::Mutex _multiplexer_lock;

int servicePort = LIBZT_DEFAULT_PORT;

#if defined(_WIN32)
WSADATA wsaData;
#include <Windows.h>
#endif

void api_sleep(int interval_ms);

/****************************************************************************/
/* ZeroTier Core helper functions for libzt - DON'T CALL THESE DIRECTLY     */
/****************************************************************************/

std::vector<ZT_VirtualNetworkRoute> *zts_get_network_routes(const uint64_t nwid)
{
	return zt1Service->getRoutes(nwid);
}

VirtualTap *getTapByNWID(uint64_t nwid)
{
	_vtaps_lock.lock();
	VirtualTap *s, *tap = nullptr;
	for (size_t i=0; i<vtaps.size(); i++) {
		s = (VirtualTap*)vtaps[i];
		if (s->_nwid == nwid) { tap = s; }
	}
	_vtaps_lock.unlock();
	return tap;
}

VirtualTap *getTapByAddr(ZeroTier::InetAddress *addr)
{
	_vtaps_lock.lock();
	VirtualTap *s, *tap = nullptr;
	//char ipbuf[64], ipbuf2[64], ipbuf3[64];
	for (size_t i=0; i<vtaps.size(); i++) {
		s = (VirtualTap*)vtaps[i];
		// check address schemes
		for (ssize_t j=0; j<s->_ips.size(); j++) {
			if ((s->_ips[j].isV4() && addr->isV4()) || (s->_ips[j].isV6() && addr->isV6())) {
				/* DEBUG_EXTRA("looking at tap %s, <addr=%s> --- for <%s>", s->_dev.c_str(),
				s->_ips[j].toString(ipbuf), addr->toIpString(ipbuf2)); */
				if (s->_ips[j].isEqualPrefix(addr)
					|| s->_ips[j].ipsEqual(addr)
					|| s->_ips[j].containsAddress(addr)
					|| (addr->isV6() && _ipv6_in_subnet(&s->_ips[j], addr))
					)
				{
					//DEBUG_EXTRA("selected tap %s, <addr=%s>", s->_dev.c_str(), s->_ips[j].toString(ipbuf));
					_vtaps_lock.unlock();
					return s;
				}
			}
		}
		// check managed routes
		if (tap == NULL) {
			std::vector<ZT_VirtualNetworkRoute> *managed_routes = zt1Service->getRoutes(s->_nwid);
			ZeroTier::InetAddress target, nm, via;
			for (size_t i=0; i<managed_routes->size(); i++) {
				target = managed_routes->at(i).target;
				nm = target.netmask();
				via = managed_routes->at(i).via;
				if (target.containsAddress(addr)) {
					/* DEBUG_EXTRA("chose tap with route <target=%s, nm=%s, via=%s>", target.toString(ipbuf),
					nm.toString(ipbuf2), via.toString(ipbuf3)); */
					_vtaps_lock.unlock();
					return s;
				}
			}
		}
	}
	_vtaps_lock.unlock();
	return tap;
}

VirtualTap *getTapByName(char *ifname)
{
	_vtaps_lock.lock();
	VirtualTap *s, *tap = nullptr;
	for (size_t i=0; i<vtaps.size(); i++) {
		s = (VirtualTap*)vtaps[i];
		if (strcmp(s->_dev.c_str(), ifname) == false) {
			tap = s;
		}
	}
	_vtaps_lock.unlock();
	return tap;
}

VirtualTap *getTapByIndex(size_t index)
{
	_vtaps_lock.lock();
	VirtualTap *s, *tap = nullptr;
	for (size_t i=0; i<vtaps.size(); i++) {
		s = (VirtualTap*)vtaps[i];
		if (s->ifindex == index) {
			tap = s;
		}
	}
	_vtaps_lock.unlock();
	return tap;
}

VirtualTap *getAnyTap()
{
	_vtaps_lock.lock();
	VirtualTap *vtap = NULL;
	if (vtaps.size()) {
		vtap = (VirtualTap *)vtaps[0];
	}
	_vtaps_lock.unlock();
	return vtap;
}

uint64_t zts_get_node_id_from_file(const char *filepath)
{
	std::string fname("identity.public");
	std::string fpath(filepath);
	std::string oldid;
	if (ZeroTier::OSUtils::fileExists((fpath + ZT_PATH_SEPARATOR_S + fname).c_str(), false)) {
		ZeroTier::OSUtils::readFile((fpath + ZT_PATH_SEPARATOR_S + fname).c_str(), oldid);
		return Utils::hexStrToU64(oldid.c_str());
	}
	return 0;
}

// Starts a ZeroTier service in the background
#if defined(_WIN32)
DWORD WINAPI zts_start_service(LPVOID thread_id)
#else
void *zts_start_service(void *thread_id)
#endif
{
	DEBUG_INFO("zto-thread, path=%s", homeDir.c_str());
	// Where network .conf files will be stored
	netDir = homeDir + "/networks.d";
	zt1Service = (ZeroTier::OneService *)0;
	// Construct path for network config and supporting service files
	if (homeDir.length()) {
		std::vector<std::string> hpsp(ZeroTier::OSUtils::split(homeDir.c_str(), ZT_PATH_SEPARATOR_S,"",""));
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
				if (ZeroTier::OSUtils::mkdir(ptmp) == false) {
					DEBUG_ERROR("home path does not exist, and could not create");
					perror("error\n");
				}
			}
		}
	}
	else {
		DEBUG_ERROR("homeDir is empty, could not construct path");
		return NULL;
	}
	if (servicePort <= 0) {
		DEBUG_INFO("no port specified, will bind to random port. use zts_set_service_port() if you want.");
	}
	else {
		DEBUG_INFO("binding to port=%d", servicePort);
	}
	for (;;) {
		zt1Service = ZeroTier::OneService::newInstance(homeDir.c_str(),servicePort);
		switch(zt1Service->run()) {
			case ZeroTier::OneService::ONE_STILL_RUNNING:
			case ZeroTier::OneService::ONE_NORMAL_TERMINATION:
				break;
			case ZeroTier::OneService::ONE_UNRECOVERABLE_ERROR:
				DEBUG_ERROR("fatal error: %s",zt1Service->fatalErrorMessage().c_str());
				break;
			case ZeroTier::OneService::ONE_IDENTITY_COLLISION: {
				delete zt1Service;
				zt1Service = (ZeroTier::OneService *)0;
				std::string oldid;
				ZeroTier::OSUtils::readFile((homeDir + ZT_PATH_SEPARATOR_S
					+ "identity.secret").c_str(),oldid);
				if (oldid.length()) {
					ZeroTier::OSUtils::writeFile((homeDir + ZT_PATH_SEPARATOR_S
						+ "identity.secret.saved_after_collision").c_str(),oldid);
					ZeroTier::OSUtils::rm((homeDir + ZT_PATH_SEPARATOR_S
						+ "identity.secret").c_str());
					ZeroTier::OSUtils::rm((homeDir + ZT_PATH_SEPARATOR_S
						+ "identity.public").c_str());
				}
			}
			continue; // restart!
		}
		break; // terminate loop -- normally we don't keep restarting
	}
	delete zt1Service;
	zt1Service = (ZeroTier::OneService *)0;
	return NULL;
}

int zts_get_num_assigned_addresses(const uint64_t nwid)
{
	if (!zt1Service) {
		return -1;
	}
	VirtualTap *tap = getTapByNWID(nwid);
	if (!tap) {
		return -1;
	}
	int sz = -1;
	_vtaps_lock.lock();
	sz = tap->_ips.size();
	_vtaps_lock.unlock();
	return sz;
}

int zts_get_address_at_index(
	const uint64_t nwid, const int index, struct sockaddr *addr, socklen_t *addrlen)
{
	if (!zt1Service) {
		return -1;
	}
	VirtualTap *tap = getTapByNWID(nwid);
	int err = -1;
	if (!tap) {
		return err;
	}
	_vtaps_lock.lock();
	if (index > -1 && index <= tap->_ips.size()) {
		memcpy(addr, &(tap->_ips[index]), *addrlen);
		*addrlen = tap->_ips[index].isV4() ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
		err = 0;
	}
	_vtaps_lock.unlock();
	return err;
}

/****************************************************************************/
/* ZeroTier Service Controls                                                */
/****************************************************************************/

int zts_set_service_port(int portno)
{
	if (portno > -1 && portno < 65535) {
		// 0 is allowed, signals zt service to bind to a random port
		servicePort = portno;
		return 0;
	}
	return -1;
}

int zts_get_address(const uint64_t nwid, struct sockaddr_storage *addr, 
	const int address_family)
{
	int err = -1;
	if (!zt1Service) {
		return -1;
	}
	VirtualTap *tap = getTapByNWID(nwid);
	if (!tap) {
		return -1;
	}
	_vtaps_lock.lock();
	socklen_t addrlen = sizeof(struct sockaddr_storage);
	for (size_t i=0; i<tap->_ips.size(); i++) {
		if (address_family == AF_INET) {
			if (tap->_ips[i].isV4()) {
				memcpy(addr, &(tap->_ips[i]), addrlen);
				addr->ss_family = AF_INET;
				err = 0;
				break;
			}
		}
		if (address_family == AF_INET6) {
			if (tap->_ips[i].isV6()) {
				memcpy(addr, &(tap->_ips[i]), addrlen);
				addr->ss_family = AF_INET6;
				err = 0;
				break;
			}
		}
	}
	_vtaps_lock.unlock();
	return err; // nothing found
}

int zts_has_address(const uint64_t nwid)
{
	struct sockaddr_storage ss;
	memset(&ss, 0, sizeof(ss));
	zts_get_address(nwid, &ss, AF_INET);
	if (ss.ss_family == AF_INET) {
		return true;
	}
	zts_get_address(nwid, &ss, AF_INET6);
	if (ss.ss_family == AF_INET6) {
		return true;
	}
	return false;
}

void zts_get_6plane_addr(struct sockaddr_storage *addr, const uint64_t nwid, const uint64_t nodeId)
{
	ZeroTier::InetAddress _6planeAddr = ZeroTier::InetAddress::makeIpv66plane(nwid,nodeId);
	memcpy(addr, _6planeAddr.rawIpData(), sizeof(struct sockaddr_storage));
}

void zts_get_rfc4193_addr(struct sockaddr_storage *addr, const uint64_t nwid, const uint64_t nodeId)
{
	ZeroTier::InetAddress _rfc4193Addr = ZeroTier::InetAddress::makeIpv6rfc4193(nwid,nodeId);
	memcpy(addr, _rfc4193Addr.rawIpData(), sizeof(struct sockaddr_storage));
}

int zts_join(const uint64_t nwid)
{
	DEBUG_INFO("joining %llx", (unsigned long long)nwid);
	if (nwid == 0) {
		return -1;
	}
	if (zt1Service) {
		zt1Service->join(nwid);
	}
	// provide ZTO service reference to virtual taps
	// TODO: This might prove to be unreliable, but it works for now
	_vtaps_lock.lock();
	for (size_t i=0;i<vtaps.size(); i++) {
		VirtualTap *s = (VirtualTap*)vtaps[i];
		s->zt1ServiceRef=(void*)zt1Service;
	}
	_vtaps_lock.unlock();
	return 0;
}

int zts_leave(const uint64_t nwid)
{
	DEBUG_INFO("leaving %llx", (unsigned long long)nwid);
	if (nwid == 0) {
		return -1;
	}
	if (zt1Service) {
		zt1Service->leave(nwid);
	}
	return 0;
}

int zts_core_running()
{
	return zt1Service == NULL ? false : zt1Service->isRunning();
}

int zts_stack_running()
{
	_vtaps_lock.lock();
	// TODO: Perhaps a more robust way to check for this
	int running = vtaps.size() > 0 ? true : false;
	_vtaps_lock.unlock();
	return running;
}

int zts_ready()
{
	return zts_core_running() && zts_stack_running();
}

int zts_start(const char *path, bool blocking = false)
{
	if (zt1Service) {
		return 0; // already initialized, ok
	}
	if (path) {
		homeDir = path;
	}
	int err = 0;
#if defined(_WIN32)
		WSAStartup(MAKEWORD(2, 2), &wsaData); // initialize WinSock. Used in Phy for loopback pipe
		HANDLE thr = CreateThread(NULL, 0, zts_start_service, NULL, 0, NULL);
#else
		pthread_t service_thread;
		err = pthread_create(&service_thread, NULL, zts_start_service, NULL);
#endif

	if (blocking) { // block to prevent service calls before we're ready
		ZT_NodeStatus status;
		status.online = 0;
		DEBUG_EXTRA("waiting for zerotier service thread to start");
		while (zts_core_running() == false || zt1Service->getNode() == NULL) {
			api_sleep(ZTO_WRAPPER_CHECK_INTERVAL);
		}
		DEBUG_EXTRA("waiting for node address assignment");
		while (zt1Service->getNode()->address() <= 0) {
			api_sleep(ZTO_WRAPPER_CHECK_INTERVAL);
		}
		DEBUG_EXTRA("node=%llx", (unsigned long long)zts_get_node_id());
		DEBUG_EXTRA("waiting for node to come online. ensure the node is authorized to join the network");
		while (status.online <= 0) {
			api_sleep(ZTO_WRAPPER_CHECK_INTERVAL);
			zt1Service->getNode()->status(&status);
		}
	}
	return err;
}

int zts_startjoin(const char *path, const uint64_t nwid)
{
	int err = zts_start(path, true);
	while (true) {
		try {
			zts_join(nwid);
			break;
		}
		catch( ... ) {
			DEBUG_ERROR("there was a problem joining the virtual network %llx", 
				(unsigned long long)nwid);
			api_sleep(ZTO_WRAPPER_CHECK_INTERVAL);
		}
	}
	while (zts_has_address(nwid) == false) {
		api_sleep(ZTO_WRAPPER_CHECK_INTERVAL);
	}
	return err;
}

void zts_stop()
{
	if (zt1Service) {
		zt1Service->terminate();
		// disableTaps();
	}
#if defined(_WIN32)
	WSACleanup();
#endif
}

void zts_get_path(char *homePath, size_t len)
{
	if (homeDir.length()) {
		memset(homePath, 0, len);
		size_t buf_len = len < homeDir.length() ? len : homeDir.length();
		memcpy(homePath, homeDir.c_str(), buf_len);
	}
}

uint64_t zts_get_node_id()
{
	if (zt1Service) {
		return zt1Service->getNode()->address();
	}
	return 0;
}

unsigned long zts_get_peer_count()
{
	if (zt1Service) {
		return zt1Service->getNode()->peers()->peerCount;
	}
	else {
		return 0;
	}
}

bool _ipv6_in_subnet(ZeroTier::InetAddress *subnet, ZeroTier::InetAddress *addr)
{
	ZeroTier::InetAddress r(addr);
	ZeroTier::InetAddress b(subnet);
	const unsigned int bits = subnet->netmaskBits();
	switch(r.ss_family) {
		case AF_INET:
			reinterpret_cast<struct sockaddr_in *>(&r)->sin_addr.s_addr &= ZeroTier::Utils::hton((uint32_t)(0xffffffff << (32 - bits)));
			break;
		case AF_INET6: {
			uint64_t nm[2];
			uint64_t nm2[2];
			memcpy(nm,reinterpret_cast<struct sockaddr_in6 *>(&r)->sin6_addr.s6_addr,16);
			memcpy(nm2,reinterpret_cast<struct sockaddr_in6 *>(&b)->sin6_addr.s6_addr,16);

			nm[0] &= ZeroTier::Utils::hton((uint64_t)((bits >= 64) ? 0xffffffffffffffffULL : (0xffffffffffffffffULL << (64 - bits))));
			nm[1] &= ZeroTier::Utils::hton((uint64_t)((bits <= 64) ? 0ULL : (0xffffffffffffffffULL << (128 - bits))));

			nm2[0] &= ZeroTier::Utils::hton((uint64_t)((bits >= 64) ? 0xffffffffffffffffULL : (0xffffffffffffffffULL << (64 - bits))));
			nm2[1] &= ZeroTier::Utils::hton((uint64_t)((bits <= 64) ? 0ULL : (0xffffffffffffffffULL << (128 - bits))));

			memcpy(reinterpret_cast<struct sockaddr_in6 *>(&r)->sin6_addr.s6_addr,nm,16);
			memcpy(reinterpret_cast<struct sockaddr_in6 *>(&b)->sin6_addr.s6_addr,nm2,16);
		}
		break;
	}
	char b0[64], b1[64];
	memset(b0, 0, 64);
	memset(b1, 0, 64);
	return !strcmp(r.toIpString(b0), b.toIpString(b1));
}

void api_sleep(int interval_ms)
{
#if defined(_WIN32)
	Sleep(interval_ms);
#else
	struct timespec sleepValue = {0};
	sleepValue.tv_nsec = interval_ms * 500000;
	nanosleep(&sleepValue, NULL);
#endif
}

#ifdef __cplusplus
}
#endif
