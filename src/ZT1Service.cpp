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
#include "libztDebug.h"
#include "SysUtils.h"

#include "Phy.hpp"
#include "OneService.hpp"
#include "InetAddress.hpp"
#include "OSUtils.hpp"

std::vector<void*> vtaps;
ZeroTier::Mutex _vtaps_lock;

#ifdef __cplusplus
extern "C" {
#endif

static ZeroTier::OneService *zt1Service;

std::string homeDir; // Platform-specific dir we *must* use internally
std::string netDir;  // Where network .conf files are to be written

ZeroTier::Mutex _multiplexer_lock;

#if defined(__MINGW32__) || defined(__MINGW64__)
WSADATA wsaData;
#endif

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
	DEBUG_EXTRA();
	std::string fname("identity.public");
	std::string fpath(filepath);
	if (ZeroTier::OSUtils::fileExists((fpath + ZT_PATH_SEPARATOR_S + fname).c_str(),false)) {
		std::string oldid;
		ZeroTier::OSUtils::readFile((fpath + ZT_PATH_SEPARATOR_S + fname).c_str(),oldid);
		return Utils::hexStrToU64(oldid);
	}
	return 0;
}

// Starts a ZeroTier service in the background
void *zts_start_service(void *thread_id)
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

	// Generate random port for new service instance
	unsigned int randp = 0;
	ZeroTier::Utils::getSecureRandom(&randp,sizeof(randp));
	// TODO: Better port random range selection
	int servicePort = 9000 + (randp % 1000);
	for (;;) {
		zt1Service = ZeroTier::OneService::newInstance(homeDir.c_str(),servicePort);
		switch(zt1Service->run()) {
			case ZeroTier::OneService::ONE_STILL_RUNNING:
			case ZeroTier::OneService::ONE_NORMAL_TERMINATION:
				break;
			case ZeroTier::OneService::ONE_UNRECOVERABLE_ERROR:
				DEBUG_ERROR("ZTO service port = %d", servicePort);
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

void zts_get_address(const uint64_t nwid, struct sockaddr_storage *addr, const size_t addrlen)
{
	if (!zt1Service) {
		return;
	}
	VirtualTap *tap = getTapByNWID(nwid);
	if (tap && tap->_ips.size()) {
		for (size_t i=0; i<tap->_ips.size(); i++) {
			if (tap->_ips[i].isV4()) {
				memcpy(addr, &(tap->_ips[i]), addrlen);
				return;
			}
		}
	}
}

int zts_has_address(const uint64_t nwid)
{
	struct sockaddr_storage ss;
	memset(&ss, 0, sizeof(ss));
	zts_get_address(nwid, &ss, sizeof(ss));
	return ss.ss_family == AF_INET || ss.ss_family == AF_INET6;
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

void zts_join(const uint64_t nwid)
{
	DEBUG_EXTRA();
	if (zt1Service) {
		std::string confFile = zt1Service->givenHomePath() + "/networks.d/" + std::to_string(nwid) + ".conf";
		if (ZeroTier::OSUtils::mkdir(netDir) == false) {
			DEBUG_ERROR("unable to create: %s", netDir.c_str());
		}
		if (ZeroTier::OSUtils::writeFile(confFile.c_str(), "") == false) {
			DEBUG_ERROR("unable to write network conf file: %s", confFile.c_str());
		}
		zt1Service->join(nwid);
	}
	// provide ZTO service reference to virtual taps
	// TODO: This might prove to be unreliable, but it works for now
	for (size_t i=0;i<vtaps.size(); i++) {
		VirtualTap *s = (VirtualTap*)vtaps[i];
		s->zt1ServiceRef=(void*)zt1Service;
	}
}

void zts_leave(const uint64_t nwid)
{
	DEBUG_EXTRA();
	if (zt1Service) {
		zt1Service->leave(nwid);
	}
}

int zts_running()
{
	return zt1Service == NULL ? false : zt1Service->isRunning();
}

int zts_start(const char *path, bool blocking = false)
{
	DEBUG_EXTRA();
	if (zt1Service) {
		return 0; // already initialized, ok
	}
	if (path) {
		homeDir = path;
	}
#if defined(__MINGW32__) || defined(__MINGW64__)
		WSAStartup(MAKEWORD(2, 2), &wsaData); // initialize WinSock. Used in Phy for loopback pipe
#endif
	pthread_t service_thread;
	int err = pthread_create(&service_thread, NULL, zts_start_service, NULL);
	if (blocking) { // block to prevent service calls before we're ready
		ZT_NodeStatus status;
		while (zts_running() == false || zt1Service->getNode() == NULL) {
			nanosleep((const struct timespec[]) {{0, (ZTO_WRAPPER_CHECK_INTERVAL * 500000)}}, NULL);
		}
		while (zt1Service->getNode()->address() <= 0) {
			nanosleep((const struct timespec[]) {{0, (ZTO_WRAPPER_CHECK_INTERVAL * 500000)}}, NULL);
		}
		while (status.online <= 0) {
			nanosleep((const struct timespec[]) {{0, (ZTO_WRAPPER_CHECK_INTERVAL * 500000)}}, NULL);
			zt1Service->getNode()->status(&status);
		}
	}
	return err;
}

int zts_startjoin(const char *path, const uint64_t nwid)
{
	DEBUG_EXTRA();
	int err = zts_start(path, true);
	// only now can we attempt a join
	while (true) {
		try {
			zts_join(nwid);
			break;
		}
		catch( ... ) {
			DEBUG_ERROR("there was a problem joining the virtual network %s", nwid);
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
	if (zt1Service) {
		zt1Service->terminate();
		// disableTaps();
	}
#if defined(__MINGW32__) || defined(__MINGW64__)
	WSACleanup(); // clean up WinSock
#endif
}

void zts_get_homepath(char *homePath, size_t len)
{
	DEBUG_EXTRA();
	if (homeDir.length()) {
		memset(homePath, 0, len);
		size_t buf_len = len < homeDir.length() ? len : homeDir.length();
		memcpy(homePath, homeDir.c_str(), buf_len);
	}
}

uint64_t zts_get_node_id()
{
	DEBUG_EXTRA();
	if (zt1Service) {
		return zt1Service->getNode()->address();
	}
	return -1;
}

unsigned long zts_get_peer_count()
{
	DEBUG_EXTRA();
	if (zt1Service) {
		return zt1Service->getNode()->peers()->peerCount;
	}
	else {
		return 0;
	}
}

int zts_get_peer_address(char *peer, const uint64_t nodeId)
{
	DEBUG_EXTRA();
	if (zt1Service) {
		ZT_PeerList *pl = zt1Service->getNode()->peers();
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
#ifdef __cplusplus
}
#endif
