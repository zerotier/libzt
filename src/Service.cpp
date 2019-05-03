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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <list>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "version.h"
#include "ZeroTierOne.h"

#include "Constants.hpp"
#include "Mutex.hpp"
#include "Node.hpp"
#include "Utils.hpp"
#include "InetAddress.hpp"
#include "MAC.hpp"
#include "Identity.hpp"
#include "World.hpp"
#include "Salsa20.hpp"
#include "Poly1305.hpp"
#include "SHA512.hpp"

#include "Phy.hpp"
#include "Thread.hpp"
#include "OSUtils.hpp"
#include "PortMapper.hpp"
#include "Binder.hpp"
#include "ManagedRoute.hpp"
#include "BlockingQueue.hpp"

#include "Service.hpp"
#include "Debug.hpp"
#include "concurrentqueue.h"

#include "ZeroTier.h"
#include "lwipDriver.hpp"

#ifdef __WINDOWS__
#include <WinSock2.h>
#include <Windows.h>
#include <ShlObj.h>
#include <netioapi.h>
#include <iphlpapi.h>
//#include <unistd.h>
#define stat _stat
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ifaddrs.h>
#endif

#include "Controls.hpp"

// Use the virtual netcon endpoint instead of a tun/tap port driver
#include "VirtualTap.hpp"
namespace ZeroTier { typedef VirtualTap EthernetTap; }

// Interface metric for ZeroTier taps -- this ensures that if we are on WiFi and also
// bridged via ZeroTier to the same LAN traffic will (if the OS is sane) prefer WiFi.
#define ZT_IF_METRIC 5000

// How often to check for new multicast subscriptions on a tap device
#define ZT_TAP_CHECK_MULTICAST_INTERVAL 5000

// How often to check for local interface addresses
#define ZT_LOCAL_INTERFACE_CHECK_INTERVAL 60000

namespace ZeroTier {

extern void postEvent(uint64_t id, int eventCode);

namespace {

static std::string _trimString(const std::string &s)
{
	unsigned long end = (unsigned long)s.length();
	while (end) {
		char c = s[end - 1];
		if ((c == ' ')||(c == '\r')||(c == '\n')||(!c)||(c == '\t'))
			--end;
		else break;
	}
	unsigned long start = 0;
	while (start < end) {
		char c = s[start];
		if ((c == ' ')||(c == '\r')||(c == '\n')||(!c)||(c == '\t'))
			++start;
		else break;
	}
	return s.substr(start,end - start);
}

class OneServiceImpl;

static int SnodeVirtualNetworkConfigFunction(ZT_Node *node,void *uptr,void *tptr,uint64_t nwid,void **nuptr,enum ZT_VirtualNetworkConfigOperation op,const ZT_VirtualNetworkConfig *nwconf);
static void SnodeEventCallback(ZT_Node *node,void *uptr,void *tptr,enum ZT_Event event,const void *metaData);
static void SnodeStatePutFunction(ZT_Node *node,void *uptr,void *tptr,enum ZT_StateObjectType type,const uint64_t id[2],const void *data,int len);
static int SnodeStateGetFunction(ZT_Node *node,void *uptr,void *tptr,enum ZT_StateObjectType type,const uint64_t id[2],void *data,unsigned int maxlen);
static int SnodeWirePacketSendFunction(ZT_Node *node,void *uptr,void *tptr,int64_t localSocket,const struct sockaddr_storage *addr,const void *data,unsigned int len,unsigned int ttl);
static void SnodeVirtualNetworkFrameFunction(ZT_Node *node,void *uptr,void *tptr,uint64_t nwid,void **nuptr,uint64_t sourceMac,uint64_t destMac,unsigned int etherType,unsigned int vlanId,const void *data,unsigned int len);
static int SnodePathCheckFunction(ZT_Node *node,void *uptr,void *tptr,uint64_t ztaddr,int64_t localSocket,const struct sockaddr_storage *remoteAddr);
static int SnodePathLookupFunction(ZT_Node *node,void *uptr,void *tptr,uint64_t ztaddr,int family,struct sockaddr_storage *result);
static void StapFrameHandler(void *uptr,void *tptr,uint64_t nwid,const MAC &from,const MAC &to,unsigned int etherType,unsigned int vlanId,const void *data,unsigned int len);

struct OneServiceIncomingPacket
{
	uint64_t now;
	int64_t sock;
	struct sockaddr_storage from;
	unsigned int size;
	uint8_t data[ZT_MAX_MTU];
};

class OneServiceImpl : public OneService
{
public:
	// begin member variables --------------------------------------------------

	const std::string _homePath;
	std::string _authToken;
	const std::string _networksPath;
	const std::string _moonsPath;

	Phy<OneServiceImpl *> _phy;
	Node *_node;
	bool _updateAutoApply;
	unsigned int _multipathMode = 0;
	unsigned int _primaryPort;
	unsigned int _secondaryPort = 0;
	unsigned int _tertiaryPort;
	volatile unsigned int _udpPortPickerCounter;

	//
	std::map<uint64_t, bool> peerCache;

	//
	unsigned long _incomingPacketConcurrency;
	std::vector<OneServiceIncomingPacket *> _incomingPacketMemoryPool;
	BlockingQueue<OneServiceIncomingPacket *> _incomingPacketQueue;
	std::vector<std::thread> _incomingPacketThreads;
	Mutex _incomingPacketMemoryPoolLock,_incomingPacketThreadsLock;

	// Local configuration and memo-ized information from it
	Hashtable< uint64_t,std::vector<InetAddress> > _v4Hints;
	Hashtable< uint64_t,std::vector<InetAddress> > _v6Hints;
	Hashtable< uint64_t,std::vector<InetAddress> > _v4Blacklists;
	Hashtable< uint64_t,std::vector<InetAddress> > _v6Blacklists;
	std::vector< InetAddress > _globalV4Blacklist;
	std::vector< InetAddress > _globalV6Blacklist;
	std::vector< InetAddress > _allowManagementFrom;
	std::vector< std::string > _interfacePrefixBlacklist;
	Mutex _localConfig_m;

	std::vector<InetAddress> explicitBind;

	/*
	 * To attempt to handle NAT/gateway craziness we use three local UDP ports:
	 *
	 * [0] is the normal/default port, usually 9993
	 * [1] is a port derived from our ZeroTier address
	 * [2] is a port computed from the normal/default for use with uPnP/NAT-PMP mappings
	 *
	 * [2] exists because on some gateways trying to do regular NAT-t interferes
	 * destructively with uPnP port mapping behavior in very weird buggy ways.
	 * It's only used if uPnP/NAT-PMP is enabled in this build.
	 */
	unsigned int _ports[3];
	Binder _binder;

	// Time we last received a packet from a global address
	uint64_t _lastDirectReceiveFromGlobal;

	// Last potential sleep/wake event
	uint64_t _lastRestart;

	// Deadline for the next background task service function
	volatile int64_t _nextBackgroundTaskDeadline;

	// Configured networks
	struct NetworkState
	{
		NetworkState() :
			tap((EthernetTap *)0)
		{
			// Real defaults are in network 'up' code in network event handler
			settings.allowManaged = true;
			settings.allowGlobal = false;
			settings.allowDefault = false;
		}

		EthernetTap *tap;
		ZT_VirtualNetworkConfig config; // memcpy() of raw config from core
		std::vector<InetAddress> managedIps;
		std::list< SharedPtr<ManagedRoute> > managedRoutes;
		NetworkSettings settings;
	};
	std::map<uint64_t,NetworkState> _nets;
	Mutex _nets_m;

	// Termination status information
	ReasonForTermination _termReason;
	std::string _fatalErrorMessage;
	Mutex _termReason_m;

	// uPnP/NAT-PMP port mapper if enabled
	bool _portMappingEnabled; // local.conf settings
#ifdef ZT_USE_MINIUPNPC
	PortMapper *_portMapper;
#endif

	// Set to false to force service to stop
	volatile bool _run;
	Mutex _run_m;

	// end member variables ----------------------------------------------------

	OneServiceImpl(const char *hp,unsigned int port) :
		_homePath((hp) ? hp : ".")
		,_phy(this,false,true)
		,_node((Node *)0)
		,_updateAutoApply(false)
		,_primaryPort(port)
		,_udpPortPickerCounter(0)
		,_lastDirectReceiveFromGlobal(0)
		,_lastRestart(0)
		,_nextBackgroundTaskDeadline(0)
		,_termReason(ONE_STILL_RUNNING)
		,_portMappingEnabled(true)
#ifdef ZT_USE_MINIUPNPC
		,_portMapper((PortMapper *)0)
#endif
		,_run(true)
	{
		_ports[0] = 0;
		_ports[1] = 0;
		_ports[2] = 0;

		/* Packet input concurrency is disabled intentially since it
		would force the user-space network stack to constantly re-order
		frames, resulting in lower RX performance */

		/*
		_incomingPacketConcurrency = 1;
		// std::max((unsigned long)1,std::min((unsigned long)16,(unsigned long)std::thread::hardware_concurrency()));
		char *envPool = std::getenv("INCOMING_PACKET_CONCURRENCY");
		if (envPool != NULL) {
			int tmp = atoi(envPool);
			if (tmp > 0) {
				_incomingPacketConcurrency = tmp;
			}
		}
		for(long t=0;t<_incomingPacketConcurrency;++t) {
			_incomingPacketThreads.push_back(std::thread([this]() {
				OneServiceIncomingPacket *pkt = nullptr;
				for(;;) {
					if (!_incomingPacketQueue.get(pkt))
						break;
					if (!pkt)
						break;
					if (!_run)
						break;

					const ZT_ResultCode rc = _node->processWirePacket(nullptr,pkt->now,pkt->sock,&(pkt->from),pkt->data,pkt->size,&_nextBackgroundTaskDeadline);
					{
						Mutex::Lock l(_incomingPacketMemoryPoolLock);
						_incomingPacketMemoryPool.push_back(pkt);
					}
					if (ZT_ResultCode_isFatal(rc)) {
						char tmp[256];
						OSUtils::ztsnprintf(tmp,sizeof(tmp),"fatal error code from processWirePacket: %d",(int)rc);
						Mutex::Lock _l(_termReason_m);
						_termReason = ONE_UNRECOVERABLE_ERROR;
						_fatalErrorMessage = tmp;
						this->terminate();
						break;
					}
				}
			}));
		}*/
	}

	virtual ~OneServiceImpl()
	{
		_incomingPacketQueue.stop();
		_incomingPacketThreadsLock.lock();
		for(auto t=_incomingPacketThreads.begin();t!=_incomingPacketThreads.end();++t)
			t->join();
		_incomingPacketThreadsLock.unlock();

		_binder.closeAll(_phy);

		_incomingPacketMemoryPoolLock.lock();
		while (!_incomingPacketMemoryPool.empty()) {
			delete _incomingPacketMemoryPool.back();
			_incomingPacketMemoryPool.pop_back();
		}
		_incomingPacketMemoryPoolLock.unlock();

#ifdef ZT_USE_MINIUPNPC
		delete _portMapper;
#endif
	}

	virtual ReasonForTermination run()
	{
		try {
			{
				const std::string authTokenPath(_homePath + ZT_PATH_SEPARATOR_S "authtoken.secret");
				if (!OSUtils::readFile(authTokenPath.c_str(),_authToken)) {
					unsigned char foo[24];
					Utils::getSecureRandom(foo,sizeof(foo));
					_authToken = "";
					for(unsigned int i=0;i<sizeof(foo);++i)
						_authToken.push_back("abcdefghijklmnopqrstuvwxyz0123456789"[(unsigned long)foo[i] % 36]);
					if (!OSUtils::writeFile(authTokenPath.c_str(),_authToken)) {
						Mutex::Lock _l(_termReason_m);
						_termReason = ONE_UNRECOVERABLE_ERROR;
						_fatalErrorMessage = "authtoken.secret could not be written";
						return _termReason;
					} else {
						OSUtils::lockDownFile(authTokenPath.c_str(),false);
					}
				}
				_authToken = _trimString(_authToken);
			}

			{
				struct ZT_Node_Callbacks cb;
				cb.version = 0;
				cb.stateGetFunction = SnodeStateGetFunction;
				cb.statePutFunction = SnodeStatePutFunction;
				cb.wirePacketSendFunction = SnodeWirePacketSendFunction;
				cb.virtualNetworkFrameFunction = SnodeVirtualNetworkFrameFunction;
				cb.virtualNetworkConfigFunction = SnodeVirtualNetworkConfigFunction;
				cb.eventCallback = SnodeEventCallback;
				cb.pathCheckFunction = SnodePathCheckFunction;
				cb.pathLookupFunction = SnodePathLookupFunction;
				_node = new Node(this,(void *)0,&cb,OSUtils::now());
			}

			// Make sure we can use the primary port, and hunt for one if configured to do so
			const int portTrials = (_primaryPort == 0) ? 256 : 1; // if port is 0, pick random
			for(int k=0;k<portTrials;++k) {
				if (_primaryPort == 0) {
					unsigned int randp = 0;
					Utils::getSecureRandom(&randp,sizeof(randp));
					_primaryPort = 20000 + (randp % 45500);
				}
				if (_trialBind(_primaryPort)) {
					_ports[0] = _primaryPort;
				} else {
					_primaryPort = 0;
				}
			}
			if (_ports[0] == 0) {
				Mutex::Lock _l(_termReason_m);
				_termReason = ONE_UNRECOVERABLE_ERROR;
				_fatalErrorMessage = "cannot bind to local control interface port";
				return _termReason;
			}

			// Attempt to bind to a secondary port chosen from our ZeroTier address.
			// This exists because there are buggy NATs out there that fail if more
			// than one device behind the same NAT tries to use the same internal
			// private address port number. Buggy NATs are a running theme.
			_ports[1] = (_secondaryPort == 0) ? 20000 + ((unsigned int)_node->address() % 45500) : _secondaryPort;
			for(int i=0;;++i) {
				if (i > 1000) {
					_ports[1] = 0;
					break;
				} else if (++_ports[1] >= 65536) {
					_ports[1] = 20000;
				}
				if (_trialBind(_ports[1]))
					break;
			}

#ifdef ZT_USE_MINIUPNPC
			if (_portMappingEnabled) {
				// If we're running uPnP/NAT-PMP, bind a *third* port for that. We can't
				// use the other two ports for that because some NATs do really funky
				// stuff with ports that are explicitly mapped that breaks things.
				if (_ports[1]) {
					_ports[2] = (_tertiaryPort == 0) ? _ports[1] : _tertiaryPort;
					for(int i=0;;++i) {
						if (i > 1000) {
							_ports[2] = 0;
							break;
						} else if (++_ports[2] >= 65536) {
							_ports[2] = 20000;
						}
						if (_trialBind(_ports[2]))
							break;
					}
					if (_ports[2]) {
						char uniqueName[64];
						OSUtils::ztsnprintf(uniqueName,sizeof(uniqueName),"ZeroTier/%.10llx@%u",_node->address(),_ports[2]);
						_portMapper = new PortMapper(_ports[2],uniqueName);
					}
				}
			}
#endif

#if NETWORK_CACHING
			// Join existing networks in networks.d
			{
				std::vector<std::string> networksDotD(OSUtils::listDirectory((_homePath + ZT_PATH_SEPARATOR_S "networks.d").c_str()));
				for(std::vector<std::string>::iterator f(networksDotD.begin());f!=networksDotD.end();++f) {
					std::size_t dot = f->find_last_of('.');
					if ((dot == 16)&&(f->substr(16) == ".conf"))
						_node->join(Utils::hexStrToU64(f->substr(0,dot).c_str()),(void *)0,(void *)0);
				}
			}
#endif

			// Main I/O loop
			_nextBackgroundTaskDeadline = 0;
			int64_t clockShouldBe = OSUtils::now();
			_lastRestart = clockShouldBe;
			int64_t lastTapMulticastGroupCheck = 0;
			int64_t lastBindRefresh = 0;
			int64_t lastUpdateCheck = clockShouldBe;
			int64_t lastMultipathModeUpdate = 0;
			int64_t lastCleanedPeersDb = 0;
			int64_t lastLocalInterfaceAddressCheck = (clockShouldBe - ZT_LOCAL_INTERFACE_CHECK_INTERVAL) + 15000; // do this in 15s to give portmapper time to configure and other things time to settle
			int64_t lastLocalConfFileCheck = OSUtils::now();
			for(;;) {
				_run_m.lock();
				if (!_run) {
					_run_m.unlock();
					_termReason_m.lock();
					_termReason = ONE_NORMAL_TERMINATION;
					_termReason_m.unlock();
					break;
				} else {
					_run_m.unlock();
				}

				const int64_t now = OSUtils::now();

				// Attempt to detect sleep/wake events by detecting delay overruns
				bool restarted = false;
				if ((now > clockShouldBe)&&((now - clockShouldBe) > 10000)) {
					_lastRestart = now;
					restarted = true;
				}

				// Refresh bindings in case device's interfaces have changed, and also sync routes to update any shadow routes (e.g. shadow default)
				if (((now - lastBindRefresh) >= (_multipathMode ? ZT_BINDER_REFRESH_PERIOD / 8 : ZT_BINDER_REFRESH_PERIOD))||(restarted)) {
					lastBindRefresh = now;
					unsigned int p[3];
					unsigned int pc = 0;
					for(int i=0;i<3;++i) {
						if (_ports[i])
							p[pc++] = _ports[i];
					}
					_binder.refresh(_phy,p,pc,explicitBind,*this);
				}
				// Update multipath mode (if needed)
				if (((now - lastMultipathModeUpdate) >= ZT_BINDER_REFRESH_PERIOD / 8)||(restarted)) {
					lastMultipathModeUpdate = now;
					_node->setMultipathMode(_multipathMode);
				}

				//
				generateEventMsgs();

				// Run background task processor in core if it's time to do so
				int64_t dl = _nextBackgroundTaskDeadline;
				if (dl <= now) {
					_node->processBackgroundTasks((void *)0,now,&_nextBackgroundTaskDeadline);
					dl = _nextBackgroundTaskDeadline;
				}

				// Sync multicast group memberships
				if ((now - lastTapMulticastGroupCheck) >= ZT_TAP_CHECK_MULTICAST_INTERVAL) {
					lastTapMulticastGroupCheck = now;
					std::vector< std::pair< uint64_t,std::pair< std::vector<MulticastGroup>,std::vector<MulticastGroup> > > > mgChanges;
					{
						Mutex::Lock _l(_nets_m);
						mgChanges.reserve(_nets.size() + 1);
						for(std::map<uint64_t,NetworkState>::const_iterator n(_nets.begin());n!=_nets.end();++n) {
							if (n->second.tap) {
								mgChanges.push_back(std::pair< uint64_t,std::pair< std::vector<MulticastGroup>,std::vector<MulticastGroup> > >(n->first,std::pair< std::vector<MulticastGroup>,std::vector<MulticastGroup> >()));
								n->second.tap->scanMulticastGroups(mgChanges.back().second.first,mgChanges.back().second.second);
							}
						}
					}
					for(std::vector< std::pair< uint64_t,std::pair< std::vector<MulticastGroup>,std::vector<MulticastGroup> > > >::iterator c(mgChanges.begin());c!=mgChanges.end();++c) {
						for(std::vector<MulticastGroup>::iterator m(c->second.first.begin());m!=c->second.first.end();++m)
							_node->multicastSubscribe((void *)0,c->first,m->mac().toInt(),m->adi());
						for(std::vector<MulticastGroup>::iterator m(c->second.second.begin());m!=c->second.second.end();++m)
							_node->multicastUnsubscribe(c->first,m->mac().toInt(),m->adi());
					}
				}

				// Sync information about physical network interfaces
				if ((now - lastLocalInterfaceAddressCheck) >= (_multipathMode ? ZT_LOCAL_INTERFACE_CHECK_INTERVAL / 8 : ZT_LOCAL_INTERFACE_CHECK_INTERVAL)) {
					lastLocalInterfaceAddressCheck = now;

					_node->clearLocalInterfaceAddresses();

#ifdef ZT_USE_MINIUPNPC
					if (_portMapper) {
						std::vector<InetAddress> mappedAddresses(_portMapper->get());
						for(std::vector<InetAddress>::const_iterator ext(mappedAddresses.begin());ext!=mappedAddresses.end();++ext)
							_node->addLocalInterfaceAddress(reinterpret_cast<const struct sockaddr_storage *>(&(*ext)));
					}
#endif

					std::vector<InetAddress> boundAddrs(_binder.allBoundLocalInterfaceAddresses());
					for(std::vector<InetAddress>::const_iterator i(boundAddrs.begin());i!=boundAddrs.end();++i)
						_node->addLocalInterfaceAddress(reinterpret_cast<const struct sockaddr_storage *>(&(*i)));
				}

				// Clean peers.d periodically
				if ((now - lastCleanedPeersDb) >= 3600000) {
					lastCleanedPeersDb = now;
					OSUtils::cleanDirectory((_homePath + ZT_PATH_SEPARATOR_S "peers.d").c_str(),now - 2592000000LL); // delete older than 30 days
				}

				const unsigned long delay = (dl > now) ? (unsigned long)(dl - now) : 100;
				clockShouldBe = now + (uint64_t)delay;
				_phy.poll(delay);
			}
		} catch (std::exception &e) {
			Mutex::Lock _l(_termReason_m);
			_termReason = ONE_UNRECOVERABLE_ERROR;
			_fatalErrorMessage = std::string("unexpected exception in main thread: ")+e.what();
		} catch ( ... ) {
			Mutex::Lock _l(_termReason_m);
			_termReason = ONE_UNRECOVERABLE_ERROR;
			_fatalErrorMessage = "unexpected exception in main thread: unknown exception";
		}

		{
			Mutex::Lock _l(_nets_m);
			for(std::map<uint64_t,NetworkState>::iterator n(_nets.begin());n!=_nets.end();++n)
				delete n->second.tap;
			_nets.clear();
		}

		delete _node;
		_node = (Node *)0;

		return _termReason;
	}

	virtual ReasonForTermination reasonForTermination() const
	{
		Mutex::Lock _l(_termReason_m);
		return _termReason;
	}

	virtual std::string fatalErrorMessage() const
	{
		Mutex::Lock _l(_termReason_m);
		return _fatalErrorMessage;
	}

	virtual std::string portDeviceName(uint64_t nwid) const
	{
		Mutex::Lock _l(_nets_m);
		std::map<uint64_t,NetworkState>::const_iterator n(_nets.find(nwid));
		if ((n != _nets.end())&&(n->second.tap))
			return n->second.tap->deviceName();
		else return std::string();
	}

	virtual std::string givenHomePath()
	{
		return _homePath;
	}

	void getRoutes(uint64_t nwid, void *routeArray, unsigned int *numRoutes)
	{
		Mutex::Lock _l(_nets_m);
		NetworkState &n = _nets[nwid];
		*numRoutes = *numRoutes < n.config.routeCount ? *numRoutes : n.config.routeCount;
		for(unsigned int i=0; i<*numRoutes; i++) {
			ZT_VirtualNetworkRoute *vnr = (ZT_VirtualNetworkRoute*)routeArray;
			memcpy(&vnr[i], &(n.config.routes[i]), sizeof(ZT_VirtualNetworkRoute));
		}
	}

	virtual Node *getNode()
	{
		return _node;
	}

	virtual void terminate()
	{
		_run_m.lock();
		_run = false;
		_run_m.unlock();
		_phy.whack();
	}

	virtual bool getNetworkSettings(const uint64_t nwid,NetworkSettings &settings) const
	{
		Mutex::Lock _l(_nets_m);
		std::map<uint64_t,NetworkState>::const_iterator n(_nets.find(nwid));
		if (n == _nets.end())
			return false;
		settings = n->second.settings;
		return true;
	}

	// =========================================================================
	// Internal implementation methods for control plane, route setup, etc.
	// =========================================================================

	// Checks if a managed IP or route target is allowed
	bool checkIfManagedIsAllowed(const NetworkState &n,const InetAddress &target)
	{
		if (!n.settings.allowManaged)
			return false;

		if (n.settings.allowManagedWhitelist.size() > 0) {
			bool allowed = false;
			for (InetAddress addr : n.settings.allowManagedWhitelist) {
				if (addr.containsAddress(target) && addr.netmaskBits() <= target.netmaskBits()) {
					allowed = true;
					break;
				}
			}
			if (!allowed) return false;
		}

		if (target.isDefaultRoute())
			return n.settings.allowDefault;
		switch(target.ipScope()) {
			case InetAddress::IP_SCOPE_NONE:
			case InetAddress::IP_SCOPE_MULTICAST:
			case InetAddress::IP_SCOPE_LOOPBACK:
			case InetAddress::IP_SCOPE_LINK_LOCAL:
				return false;
			case InetAddress::IP_SCOPE_GLOBAL:
				return n.settings.allowGlobal;
			default:
				return true;
		}
	}

	// Apply or update managed IPs for a configured network (be sure n.tap exists)
	void syncManagedStuff(NetworkState &n)
	{
		char ipbuf[64];
		// assumes _nets_m is locked
		std::vector<InetAddress> newManagedIps;
		newManagedIps.reserve(n.config.assignedAddressCount);
		for(unsigned int i=0;i<n.config.assignedAddressCount;++i) {
			const InetAddress *ii = reinterpret_cast<const InetAddress *>(&(n.config.assignedAddresses[i]));
			if (checkIfManagedIsAllowed(n,*ii))
				newManagedIps.push_back(*ii);
		}
		std::sort(newManagedIps.begin(),newManagedIps.end());
		newManagedIps.erase(std::unique(newManagedIps.begin(),newManagedIps.end()),newManagedIps.end());
		for(std::vector<InetAddress>::iterator ip(n.managedIps.begin());ip!=n.managedIps.end();++ip) {
			if (std::find(newManagedIps.begin(),newManagedIps.end(),*ip) == newManagedIps.end()) {
				if (!n.tap->removeIp(*ip))
					fprintf(stderr,"ERROR: unable to remove ip address %s" ZT_EOL_S, ip->toString(ipbuf));
			}
		}
		for(std::vector<InetAddress>::iterator ip(newManagedIps.begin());ip!=newManagedIps.end();++ip) {
			if (std::find(n.managedIps.begin(),n.managedIps.end(),*ip) == n.managedIps.end()) {
				if (!n.tap->addIp(*ip))
					fprintf(stderr,"ERROR: unable to add ip address %s" ZT_EOL_S, ip->toString(ipbuf));
			}
		}
		n.managedIps.swap(newManagedIps);

	}

	// =========================================================================
	// Handlers for Node and Phy<> callbacks
	// =========================================================================

	inline void phyOnDatagram(PhySocket *sock,void **uptr,const struct sockaddr *localAddr,const struct sockaddr *from,void *data,unsigned long len)
	{
		if ((len >= 16)&&(reinterpret_cast<const InetAddress *>(from)->ipScope() == InetAddress::IP_SCOPE_GLOBAL))
			_lastDirectReceiveFromGlobal = OSUtils::now();
		const ZT_ResultCode rc = _node->processWirePacket(
			(void *)0,
			OSUtils::now(),
			reinterpret_cast<int64_t>(sock),
			reinterpret_cast<const struct sockaddr_storage *>(from), // Phy<> uses sockaddr_storage, so it'll always be that big
			data,
			len,
			&_nextBackgroundTaskDeadline);
		if (ZT_ResultCode_isFatal(rc)) {
			char tmp[256];
			OSUtils::ztsnprintf(tmp,sizeof(tmp),"fatal error code from processWirePacket: %d",(int)rc);
			Mutex::Lock _l(_termReason_m);
			_termReason = ONE_UNRECOVERABLE_ERROR;
			_fatalErrorMessage = tmp;
			this->terminate();
		}
	}

	inline void phyOnTcpConnect(PhySocket *sock,void **uptr,bool success) {}
	inline void phyOnTcpAccept(PhySocket *sockL,PhySocket *sockN,void **uptrL,void **uptrN,const struct sockaddr *from) {}
	void phyOnTcpClose(PhySocket *sock,void **uptr) {}
	void phyOnTcpData(PhySocket *sock,void **uptr,void *data,unsigned long len) {}
	inline void phyOnTcpWritable(PhySocket *sock,void **uptr) {}
	inline void phyOnFileDescriptorActivity(PhySocket *sock,void **uptr,bool readable,bool writable) {}
	inline void phyOnUnixAccept(PhySocket *sockL,PhySocket *sockN,void **uptrL,void **uptrN) {}
	inline void phyOnUnixClose(PhySocket *sock,void **uptr) {}
	inline void phyOnUnixData(PhySocket *sock,void **uptr,void *data,unsigned long len) {}
	inline void phyOnUnixWritable(PhySocket *sock,void **uptr) {}

	inline int nodeVirtualNetworkConfigFunction(uint64_t nwid,void **nuptr,enum ZT_VirtualNetworkConfigOperation op,const ZT_VirtualNetworkConfig *nwc)
	{
		Mutex::Lock _l(_nets_m);
		NetworkState &n = _nets[nwid];

		switch(op) {

			case ZT_VIRTUAL_NETWORK_CONFIG_OPERATION_UP:
				if (!n.tap) {
					char friendlyName[128];
					OSUtils::ztsnprintf(friendlyName,sizeof(friendlyName),"ZeroTier One [%.16llx]",nwid);

					n.tap = new EthernetTap(
						_homePath.c_str(),
						MAC(nwc->mac),
						nwc->mtu,
						(unsigned int)ZT_IF_METRIC,
						nwid,
						friendlyName,
						StapFrameHandler,
						(void *)this);
					*nuptr = (void *)&n;
				}
				// After setting up tap, fall through to CONFIG_UPDATE since we also want to do this...

			case ZT_VIRTUAL_NETWORK_CONFIG_OPERATION_CONFIG_UPDATE:
				memcpy(&(n.config),nwc,sizeof(ZT_VirtualNetworkConfig));
				if (n.tap) { // sanity check
					syncManagedStuff(n);
					n.tap->setMtu(nwc->mtu);
				} else {
					_nets.erase(nwid);
					return -999; // tap init failed
				}
				break;

			case ZT_VIRTUAL_NETWORK_CONFIG_OPERATION_DOWN:
			case ZT_VIRTUAL_NETWORK_CONFIG_OPERATION_DESTROY:
				if (n.tap) { // sanity check
					*nuptr = (void *)0;
					delete n.tap;
					_nets.erase(nwid);
#if NETWORK_CACHING
					if (op == ZT_VIRTUAL_NETWORK_CONFIG_OPERATION_DESTROY) {
						char nlcpath[256];
						OSUtils::ztsnprintf(nlcpath,sizeof(nlcpath),"%s" ZT_PATH_SEPARATOR_S "networks.d" ZT_PATH_SEPARATOR_S "%.16llx.local.conf",_homePath.c_str(),nwid);
						OSUtils::rm(nlcpath);
					}
#endif
				} else {
					_nets.erase(nwid);
				}
				break;
		}
		return 0;
	}

	inline void nodeEventCallback(enum ZT_Event event,const void *metaData)
	{
		// Feed node events into lock-free queue for later dequeuing by the callback thread
		if (event <= ZT_EVENT_FATAL_ERROR_IDENTITY_COLLISION) {
			if (event == ZTS_EVENT_NODE_ONLINE) {
				struct zts_node_details *nd = new zts_node_details;
				nd->address = _node->address();
				postEvent(event, (void*)nd);
			}
			else {
				postEvent(event, (void*)0);
			}
		}
		switch(event) {
			case ZT_EVENT_FATAL_ERROR_IDENTITY_COLLISION: {
				Mutex::Lock _l(_termReason_m);
				_termReason = ONE_IDENTITY_COLLISION;
				_fatalErrorMessage = "identity/address collision";
				this->terminate();
			}	break;

			case ZT_EVENT_TRACE: {
				if (metaData) {
					::fprintf(stderr,"%s" ZT_EOL_S,(const char *)metaData);
					::fflush(stderr);
				}
			}	break;

			default:
				break;
		}
	}

	inline struct zts_network_details *prepare_network_details_msg(uint64_t nwid)
	{
		struct zts_network_details *nd = new zts_network_details;
		nd->nwid = nwid;
		return nd;
	}

	inline void generateEventMsgs()
	{
		// Force the ordering of callback messages, these messages are
		// only useful if the node and stack are both up and running
		if (!_node->online() || !lwip_is_up()) {
			return;
		}
		// Generate messages to be dequeued by the callback message thread
		Mutex::Lock _l(_nets_m);
		for(std::map<uint64_t,NetworkState>::iterator n(_nets.begin());n!=_nets.end();++n) {
			int mostRecentStatus = n->second.config.status;
			VirtualTap *tap = n->second.tap;
			uint64_t nwid = n->first;
			if (n->second.tap->_networkStatus == mostRecentStatus) {
				continue; // No state change
			}
			switch (mostRecentStatus) {
				case ZT_NETWORK_STATUS_NOT_FOUND:
					postEvent(ZTS_EVENT_NETWORK_NOT_FOUND, (void*)prepare_network_details_msg(nwid));
					break;
				case ZT_NETWORK_STATUS_CLIENT_TOO_OLD:
					postEvent(ZTS_EVENT_NETWORK_CLIENT_TOO_OLD, (void*)prepare_network_details_msg(nwid));
					break;
				case ZT_NETWORK_STATUS_REQUESTING_CONFIGURATION:
					postEvent(ZTS_EVENT_NETWORK_REQUESTING_CONFIG, (void*)prepare_network_details_msg(nwid));
					break;
				case ZT_NETWORK_STATUS_OK:
					if (tap->hasIpv4Addr() && lwip_is_netif_up(tap->netif)) {
						postEvent(ZTS_EVENT_NETWORK_READY_IP4, (void*)prepare_network_details_msg(nwid));
					}
					if (tap->hasIpv6Addr() && lwip_is_netif_up(tap->netif)) {
						postEvent(ZTS_EVENT_NETWORK_READY_IP6, (void*)prepare_network_details_msg(nwid));
					}
					// In addition to the READY messages, send one OK message
					postEvent(ZTS_EVENT_NETWORK_OK, (void*)prepare_network_details_msg(nwid));
					break;
				case ZT_NETWORK_STATUS_ACCESS_DENIED:
					postEvent(ZTS_EVENT_NETWORK_ACCESS_DENIED, (void*)prepare_network_details_msg(nwid));
					break;
				default:
					break;
			}
			n->second.tap->_networkStatus = mostRecentStatus;
		}

		// TODO: Add ZTS_EVENT_PEER_NEW
		ZT_PeerList *pl = _node->peers();
		struct zts_peer_details *pd;
		if (pl) {
			for(unsigned long i=0;i<pl->peerCount;++i) {
				if (!peerCache.count(pl->peers[i].address)) {
					// New peer, add status
					if (pl->peers[i].pathCount > 0) {
						pd = new zts_peer_details;
						memcpy(pd, &(pl->peers[i]), sizeof(struct zts_peer_details));
						postEvent(ZTS_EVENT_PEER_P2P, (void*)pd);
					}
					if (pl->peers[i].pathCount == 0) {
						pd = new zts_peer_details;
						memcpy(pd, &(pl->peers[i]), sizeof(struct zts_peer_details));
						postEvent(ZTS_EVENT_PEER_RELAY, (void*)pd);
					}
				}
				// Previously known peer, update status
				else {
					if (peerCache[pl->peers[i].address] == 0 && pl->peers[i].pathCount > 0) {
						pd = new zts_peer_details;
						memcpy(pd, &(pl->peers[i]), sizeof(struct zts_peer_details));
						postEvent(ZTS_EVENT_PEER_P2P, (void*)pd);
					}
					if (peerCache[pl->peers[i].address] > 0 && pl->peers[i].pathCount == 0) {
						pd = new zts_peer_details;
						memcpy(pd, &(pl->peers[i]), sizeof(struct zts_peer_details));
						postEvent(ZTS_EVENT_PEER_RELAY, (void*)pd);
					}
				}
				// Update our cache with most recently observed path count
				peerCache[pl->peers[i].address] = pl->peers[i].pathCount;
			}
		}
		_node->freeQueryResult((void *)pl);
	}

	inline int networkCount()
	{
		Mutex::Lock _l(_nets_m);
		return _nets.size();
	}

	inline void join(uint64_t nwid)
	{
		_node->join(nwid, NULL, NULL);
	}

	inline void leave(uint64_t nwid)
	{
		_node->leave(nwid, NULL, NULL);
	}

	inline void leaveAll()
	{
		Mutex::Lock _l(_nets_m);
		for(std::map<uint64_t,NetworkState>::iterator n(_nets.begin());n!=_nets.end();++n) {
			_node->leave(n->first, NULL, NULL);
		}
	}

	inline int getPeerStatus(uint64_t id)
	{
		ZT_PeerList *pl = _node->peers();
		int status = ZTS_EVENT_PEER_UNREACHABLE;
		if (pl) {
			for(unsigned long i=0;i<pl->peerCount;++i) {
				if (pl->peers[i].address == id) {
					status = pl->peers[i].pathCount > 0 ? ZTS_EVENT_PEER_P2P : ZTS_EVENT_PEER_RELAY;
					break;
				}
			}
		}
		_node->freeQueryResult((void *)pl);
		return status;
	}

	inline void nodeStatePutFunction(enum ZT_StateObjectType type,const uint64_t id[2],const void *data,int len)
	{
		char p[1024];
		FILE *f;
		bool secure = false;
		char dirname[1024];
		dirname[0] = 0;

		switch(type) {
			case ZT_STATE_OBJECT_IDENTITY_PUBLIC:
				OSUtils::ztsnprintf(p,sizeof(p),"%s" ZT_PATH_SEPARATOR_S "identity.public",_homePath.c_str());
				break;
			case ZT_STATE_OBJECT_IDENTITY_SECRET:
				OSUtils::ztsnprintf(p,sizeof(p),"%s" ZT_PATH_SEPARATOR_S "identity.secret",_homePath.c_str());
				secure = true;
				break;
			case ZT_STATE_OBJECT_PLANET:
				OSUtils::ztsnprintf(p,sizeof(p),"%s" ZT_PATH_SEPARATOR_S "planet",_homePath.c_str());
				break;
#if NETWORK_CACHING
			case ZT_STATE_OBJECT_NETWORK_CONFIG:
				OSUtils::ztsnprintf(dirname,sizeof(dirname),"%s" ZT_PATH_SEPARATOR_S "networks.d",_homePath.c_str());
				OSUtils::ztsnprintf(p,sizeof(p),"%s" ZT_PATH_SEPARATOR_S "%.16llx.conf",dirname,(unsigned long long)id[0]);
				secure = true;
				break;
#endif
#if PEER_CACHING
			case ZT_STATE_OBJECT_PEER:
				OSUtils::ztsnprintf(dirname,sizeof(dirname),"%s" ZT_PATH_SEPARATOR_S "peers.d",_homePath.c_str());
				OSUtils::ztsnprintf(p,sizeof(p),"%s" ZT_PATH_SEPARATOR_S "%.10llx.peer",dirname,(unsigned long long)id[0]);
				break;
#endif
			default:
				return;
		}

		if (len >= 0) {
			// Check to see if we've already written this first. This reduces
			// redundant writes and I/O overhead on most platforms and has
			// little effect on others.
			f = fopen(p,"rb");
			if (f) {
				char buf[65535];
				long l = (long)fread(buf,1,sizeof(buf),f);
				fclose(f);
				if ((l == (long)len)&&(memcmp(data,buf,l) == 0))
					return;
			}

			f = fopen(p,"wb");
			if ((!f)&&(dirname[0])) { // create subdirectory if it does not exist
				OSUtils::mkdir(dirname);
				f = fopen(p,"wb");
			}
			if (f) {
				if (fwrite(data,len,1,f) != 1)
					fprintf(stderr,"WARNING: unable to write to file: %s (I/O error)" ZT_EOL_S,p);
				fclose(f);
				if (secure)
					OSUtils::lockDownFile(p,false);
			} else {
				fprintf(stderr,"WARNING: unable to write to file: %s (unable to open)" ZT_EOL_S,p);
			}
		} else {
			OSUtils::rm(p);
		}
	}

	inline int nodeStateGetFunction(enum ZT_StateObjectType type,const uint64_t id[2],void *data,unsigned int maxlen)
	{
		char p[4096];
		switch(type) {
			case ZT_STATE_OBJECT_IDENTITY_PUBLIC:
				OSUtils::ztsnprintf(p,sizeof(p),"%s" ZT_PATH_SEPARATOR_S "identity.public",_homePath.c_str());
				break;
			case ZT_STATE_OBJECT_IDENTITY_SECRET:
				OSUtils::ztsnprintf(p,sizeof(p),"%s" ZT_PATH_SEPARATOR_S "identity.secret",_homePath.c_str());
				break;
			case ZT_STATE_OBJECT_PLANET:
				OSUtils::ztsnprintf(p,sizeof(p),"%s" ZT_PATH_SEPARATOR_S "planet",_homePath.c_str());
				break;
#if NETWORK_CACHING
			case ZT_STATE_OBJECT_NETWORK_CONFIG:
				OSUtils::ztsnprintf(p,sizeof(p),"%s" ZT_PATH_SEPARATOR_S "networks.d" ZT_PATH_SEPARATOR_S "%.16llx.conf",_homePath.c_str(),(unsigned long long)id[0]);
				break;
#endif
#if PEER_CACHING
			case ZT_STATE_OBJECT_PEER:
				OSUtils::ztsnprintf(p,sizeof(p),"%s" ZT_PATH_SEPARATOR_S "peers.d" ZT_PATH_SEPARATOR_S "%.10llx.peer",_homePath.c_str(),(unsigned long long)id[0]);
				break;
#endif
			default:
				return -1;
		}
		FILE *f = fopen(p,"rb");
		if (f) {
			int n = (int)fread(data,1,maxlen,f);
			fclose(f);
			if (n >= 0)
				return n;
		}
		return -1;
	}

	inline int nodeWirePacketSendFunction(const int64_t localSocket,const struct sockaddr_storage *addr,const void *data,unsigned int len,unsigned int ttl)
	{
		// Even when relaying we still send via UDP. This way if UDP starts
		// working we can instantly "fail forward" to it and stop using TCP
		// proxy fallback, which is slow.

		if ((localSocket != -1)&&(localSocket != 0)&&(_binder.isUdpSocketValid((PhySocket *)((uintptr_t)localSocket)))) {
			if ((ttl)&&(addr->ss_family == AF_INET)) _phy.setIp4UdpTtl((PhySocket *)((uintptr_t)localSocket),ttl);
			const bool r = _phy.udpSend((PhySocket *)((uintptr_t)localSocket),(const struct sockaddr *)addr,data,len);
			if ((ttl)&&(addr->ss_family == AF_INET)) _phy.setIp4UdpTtl((PhySocket *)((uintptr_t)localSocket),255);
			return ((r) ? 0 : -1);
		} else {
			return ((_binder.udpSendAll(_phy,addr,data,len,ttl)) ? 0 : -1);
		}
	}

	inline void nodeVirtualNetworkFrameFunction(uint64_t nwid,void **nuptr,uint64_t sourceMac,uint64_t destMac,unsigned int etherType,unsigned int vlanId,const void *data,unsigned int len)
	{
		NetworkState *n = reinterpret_cast<NetworkState *>(*nuptr);
		if ((!n)||(!n->tap))
			return;
		n->tap->put(MAC(sourceMac),MAC(destMac),etherType,data,len);
	}

	inline int nodePathCheckFunction(uint64_t ztaddr,const int64_t localSocket,const struct sockaddr_storage *remoteAddr)
	{
		// Make sure we're not trying to do ZeroTier-over-ZeroTier
		{
			Mutex::Lock _l(_nets_m);
			for(std::map<uint64_t,NetworkState>::const_iterator n(_nets.begin());n!=_nets.end();++n) {
				if (n->second.tap) {
					std::vector<InetAddress> ips(n->second.tap->ips());
					for(std::vector<InetAddress>::const_iterator i(ips.begin());i!=ips.end();++i) {
						if (i->containsAddress(*(reinterpret_cast<const InetAddress *>(remoteAddr)))) {
							return 0;
						}
					}
				}
			}
		}

		/* Note: I do not think we need to scan for overlap with managed routes
		 * because of the "route forking" and interface binding that we do. This
		 * ensures (we hope) that ZeroTier traffic will still take the physical
		 * path even if its managed routes override this for other traffic. Will
		 * revisit if we see recursion problems. */

		// Check blacklists
		const Hashtable< uint64_t,std::vector<InetAddress> > *blh = (const Hashtable< uint64_t,std::vector<InetAddress> > *)0;
		const std::vector<InetAddress> *gbl = (const std::vector<InetAddress> *)0;
		if (remoteAddr->ss_family == AF_INET) {
			blh = &_v4Blacklists;
			gbl = &_globalV4Blacklist;
		} else if (remoteAddr->ss_family == AF_INET6) {
			blh = &_v6Blacklists;
			gbl = &_globalV6Blacklist;
		}
		if (blh) {
			Mutex::Lock _l(_localConfig_m);
			const std::vector<InetAddress> *l = blh->get(ztaddr);
			if (l) {
				for(std::vector<InetAddress>::const_iterator a(l->begin());a!=l->end();++a) {
					if (a->containsAddress(*reinterpret_cast<const InetAddress *>(remoteAddr)))
						return 0;
				}
			}
		}
		if (gbl) {
			for(std::vector<InetAddress>::const_iterator a(gbl->begin());a!=gbl->end();++a) {
				if (a->containsAddress(*reinterpret_cast<const InetAddress *>(remoteAddr)))
					return 0;
			}
		}
		return 1;
	}

	inline int nodePathLookupFunction(uint64_t ztaddr,int family,struct sockaddr_storage *result)
	{
		const Hashtable< uint64_t,std::vector<InetAddress> > *lh = (const Hashtable< uint64_t,std::vector<InetAddress> > *)0;
		if (family < 0)
			lh = (_node->prng() & 1) ? &_v4Hints : &_v6Hints;
		else if (family == AF_INET)
			lh = &_v4Hints;
		else if (family == AF_INET6)
			lh = &_v6Hints;
		else return 0;
		const std::vector<InetAddress> *l = lh->get(ztaddr);
		if ((l)&&(l->size() > 0)) {
			memcpy(result,&((*l)[(unsigned long)_node->prng() % l->size()]),sizeof(struct sockaddr_storage));
			return 1;
		} else return 0;
	}

	inline void tapFrameHandler(uint64_t nwid,const MAC &from,const MAC &to,unsigned int etherType,unsigned int vlanId,const void *data,unsigned int len)
	{
		_node->processVirtualNetworkFrame((void *)0,OSUtils::now(),nwid,from.toInt(),to.toInt(),etherType,vlanId,data,len,&_nextBackgroundTaskDeadline);
	}

	bool shouldBindInterface(const char *ifname,const InetAddress &ifaddr)
	{
#if defined(__linux__) || defined(linux) || defined(__LINUX__) || defined(__linux)
		if ((ifname[0] == 'l')&&(ifname[1] == 'o')) return false; // loopback
		if ((ifname[0] == 'z')&&(ifname[1] == 't')) return false; // sanity check: zt#
		if ((ifname[0] == 't')&&(ifname[1] == 'u')&&(ifname[2] == 'n')) return false; // tun# is probably an OpenVPN tunnel or similar
		if ((ifname[0] == 't')&&(ifname[1] == 'a')&&(ifname[2] == 'p')) return false; // tap# is probably an OpenVPN tunnel or similar
#endif

#ifdef __APPLE__
		if ((ifname[0] == 'f')&&(ifname[1] == 'e')&&(ifname[2] == 't')&&(ifname[3] == 'h')) return false; // ... as is feth#
		if ((ifname[0] == 'l')&&(ifname[1] == 'o')) return false; // loopback
		if ((ifname[0] == 'z')&&(ifname[1] == 't')) return false; // sanity check: zt#
		if ((ifname[0] == 't')&&(ifname[1] == 'u')&&(ifname[2] == 'n')) return false; // tun# is probably an OpenVPN tunnel or similar
		if ((ifname[0] == 't')&&(ifname[1] == 'a')&&(ifname[2] == 'p')) return false; // tap# is probably an OpenVPN tunnel or similar
		if ((ifname[0] == 'u')&&(ifname[1] == 't')&&(ifname[2] == 'u')&&(ifname[3] == 'n')) return false; // ... as is utun#
#endif

		{
			Mutex::Lock _l(_localConfig_m);
			for(std::vector<std::string>::const_iterator p(_interfacePrefixBlacklist.begin());p!=_interfacePrefixBlacklist.end();++p) {
				if (!strncmp(p->c_str(),ifname,p->length()))
					return false;
			}
		}
		{
			// Check global blacklists
			const std::vector<InetAddress> *gbl = (const std::vector<InetAddress> *)0;
			if (ifaddr.ss_family == AF_INET) {
				gbl = &_globalV4Blacklist;
			} else if (ifaddr.ss_family == AF_INET6) {
				gbl = &_globalV6Blacklist;
			}
			if (gbl) {
				Mutex::Lock _l(_localConfig_m);
				for(std::vector<InetAddress>::const_iterator a(gbl->begin());a!=gbl->end();++a) {
					if (a->containsAddress(ifaddr))
						return false;
				}
			}
		}
		{
			Mutex::Lock _l(_nets_m);
			for(std::map<uint64_t,NetworkState>::const_iterator n(_nets.begin());n!=_nets.end();++n) {
				if (n->second.tap) {
					std::vector<InetAddress> ips(n->second.tap->ips());
					for(std::vector<InetAddress>::const_iterator i(ips.begin());i!=ips.end();++i) {
						if (i->ipsEqual(ifaddr))
							return false;
					}
				}
			}
		}

		return true;
	}

	bool _trialBind(unsigned int port)
	{
		struct sockaddr_in in4;
		struct sockaddr_in6 in6;
		PhySocket *tb;

		memset(&in4,0,sizeof(in4));
		in4.sin_family = AF_INET;
		in4.sin_port = Utils::hton((uint16_t)port);
		tb = _phy.udpBind(reinterpret_cast<const struct sockaddr *>(&in4),(void *)0,0);
		if (tb) {
			_phy.close(tb,false);
			tb = _phy.tcpListen(reinterpret_cast<const struct sockaddr *>(&in4),(void *)0);
			if (tb) {
				_phy.close(tb,false);
				return true;
			}
		}

		memset(&in6,0,sizeof(in6));
		in6.sin6_family = AF_INET6;
		in6.sin6_port = Utils::hton((uint16_t)port);
		tb = _phy.udpBind(reinterpret_cast<const struct sockaddr *>(&in6),(void *)0,0);
		if (tb) {
			_phy.close(tb,false);
			tb = _phy.tcpListen(reinterpret_cast<const struct sockaddr *>(&in6),(void *)0);
			if (tb) {
				_phy.close(tb,false);
				return true;
			}
		}

		return false;
	}
};

static int SnodeVirtualNetworkConfigFunction(ZT_Node *node,void *uptr,void *tptr,uint64_t nwid,void **nuptr,enum ZT_VirtualNetworkConfigOperation op,const ZT_VirtualNetworkConfig *nwconf)
{ return reinterpret_cast<OneServiceImpl *>(uptr)->nodeVirtualNetworkConfigFunction(nwid,nuptr,op,nwconf); }
static void SnodeEventCallback(ZT_Node *node,void *uptr,void *tptr,enum ZT_Event event,const void *metaData)
{ reinterpret_cast<OneServiceImpl *>(uptr)->nodeEventCallback(event,metaData); }
static void SnodeStatePutFunction(ZT_Node *node,void *uptr,void *tptr,enum ZT_StateObjectType type,const uint64_t id[2],const void *data,int len)
{ reinterpret_cast<OneServiceImpl *>(uptr)->nodeStatePutFunction(type,id,data,len); }
static int SnodeStateGetFunction(ZT_Node *node,void *uptr,void *tptr,enum ZT_StateObjectType type,const uint64_t id[2],void *data,unsigned int maxlen)
{ return reinterpret_cast<OneServiceImpl *>(uptr)->nodeStateGetFunction(type,id,data,maxlen); }
static int SnodeWirePacketSendFunction(ZT_Node *node,void *uptr,void *tptr,int64_t localSocket,const struct sockaddr_storage *addr,const void *data,unsigned int len,unsigned int ttl)
{ return reinterpret_cast<OneServiceImpl *>(uptr)->nodeWirePacketSendFunction(localSocket,addr,data,len,ttl); }
static void SnodeVirtualNetworkFrameFunction(ZT_Node *node,void *uptr,void *tptr,uint64_t nwid,void **nuptr,uint64_t sourceMac,uint64_t destMac,unsigned int etherType,unsigned int vlanId,const void *data,unsigned int len)
{ reinterpret_cast<OneServiceImpl *>(uptr)->nodeVirtualNetworkFrameFunction(nwid,nuptr,sourceMac,destMac,etherType,vlanId,data,len); }
static int SnodePathCheckFunction(ZT_Node *node,void *uptr,void *tptr,uint64_t ztaddr,int64_t localSocket,const struct sockaddr_storage *remoteAddr)
{ return reinterpret_cast<OneServiceImpl *>(uptr)->nodePathCheckFunction(ztaddr,localSocket,remoteAddr); }
static int SnodePathLookupFunction(ZT_Node *node,void *uptr,void *tptr,uint64_t ztaddr,int family,struct sockaddr_storage *result)
{ return reinterpret_cast<OneServiceImpl *>(uptr)->nodePathLookupFunction(ztaddr,family,result); }
static void StapFrameHandler(void *uptr,void *tptr,uint64_t nwid,const MAC &from,const MAC &to,unsigned int etherType,unsigned int vlanId,const void *data,unsigned int len)
{ reinterpret_cast<OneServiceImpl *>(uptr)->tapFrameHandler(nwid,from,to,etherType,vlanId,data,len); }

} // anonymous namespace

std::string OneService::platformDefaultHomePath()
{
	return OSUtils::platformDefaultHomePath();
}

OneService *OneService::newInstance(const char *hp,unsigned int port) { return new OneServiceImpl(hp,port); }
OneService::~OneService() {}

} // namespace ZeroTier
