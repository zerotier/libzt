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
 * ZeroTier Node Service
 */

#ifndef ZTS_NODE_SERVICE_HPP
#define ZTS_NODE_SERVICE_HPP

#include "Binder.hpp"
#include "Constants.hpp"
#include "Events.hpp"
#include "InetAddress.hpp"
#include "Mutex.hpp"
#include "Node.hpp"
#include "PortMapper.hpp"
#include "VirtualTap.hpp"
#include "ZeroTierSockets.h"

#include <string>
#include <vector>

#define ZTS_SERVICE_THREAD_NAME        "ZTServiceThread"
#define ZTS_EVENT_CALLBACK_THREAD_NAME "ZTEventCallbackThread"
// Interface metric for ZeroTier taps -- this ensures that if we are on WiFi and
// also bridged via ZeroTier to the same LAN traffic will (if the OS is sane)
// prefer WiFi.
#define ZT_IF_METRIC 5000
// How often to check for new multicast subscriptions on a tap device
#define ZT_TAP_CHECK_MULTICAST_INTERVAL 5000
// How often to check for local interface addresses
#define ZT_LOCAL_INTERFACE_CHECK_INTERVAL 60000

#ifdef __WINDOWS__
#include <Windows.h>
#endif

namespace ZeroTier {

/**
 * ZeroTier node service
 */
class NodeService {
  public:
	/**
	 * Returned by node main if/when it terminates
	 */
	enum ReasonForTermination {
		/**
		 * Instance is still running
		 */
		ONE_STILL_RUNNING = 0,

		/**
		 * Normal shutdown
		 */
		ONE_NORMAL_TERMINATION = 1,

		/**
		 * A serious unrecoverable error has occurred
		 */
		ONE_UNRECOVERABLE_ERROR = 2,

		/**
		 * Your identity has collided with another
		 */
		ONE_IDENTITY_COLLISION = 3
	};

	/**
	 * Local settings for each network
	 */
	struct NetworkSettings {
		/**
		 * Allow this network to configure IP addresses and routes?
		 */
		bool allowManaged;

		/**
		 * Whitelist of addresses that can be configured by this network.
		 * If empty and allowManaged is true, allow all
		 * private/pseudoprivate addresses.
		 */
		std::vector<InetAddress> allowManagedWhitelist;

		/**
		 * Allow configuration of IPs and routes within global (Internet) IP
		 * space?
		 */
		bool allowGlobal;

		/**
		 * Allow overriding of system default routes for "full tunnel"
		 * operation?
		 */
		bool allowDefault;
	};

	Phy<NodeService*> _phy;
	Node* _node;
	unsigned int _primaryPort = 0;
	unsigned int _secondaryPort = 0;
	unsigned int _tertiaryPort = 0;
	volatile unsigned int _udpPortPickerCounter;

	std::map<uint64_t, unsigned int> peerCache;

	// Local configuration and memo-ized information from it
	Hashtable<uint64_t, std::vector<InetAddress> > _v4Hints;
	Hashtable<uint64_t, std::vector<InetAddress> > _v6Hints;
	Hashtable<uint64_t, std::vector<InetAddress> > _v4Blacklists;
	Hashtable<uint64_t, std::vector<InetAddress> > _v6Blacklists;
	std::vector<InetAddress> _globalV4Blacklist;
	std::vector<InetAddress> _globalV6Blacklist;
	std::vector<InetAddress> _allowManagementFrom;
	std::vector<std::string> _interfacePrefixBlacklist;
	Mutex _localConfig_m;

	std::vector<InetAddress> explicitBind;

	/*
	 * To attempt to handle NAT/gateway craziness we use three local UDP
	 * ports:
	 *
	 * [0] is the normal/default port, usually 9993
	 * [1] is a port derived from our ZeroTier address
	 * [2] is a port computed from the normal/default for use with
	 * uPnP/NAT-PMP mappings
	 *
	 * [2] exists because on some gateways trying to do regular NAT-t
	 * interferes destructively with uPnP port mapping behavior in very
	 * weird buggy ways. It's only used if uPnP/NAT-PMP is enabled in this
	 * build.
	 */
	unsigned int _ports[3] = { 0 };
	Binder _binder;

	// Time we last received a packet from a global address
	uint64_t _lastDirectReceiveFromGlobal;

	// Last potential sleep/wake event
	uint64_t _lastRestart;

	// Deadline for the next background task service function
	volatile int64_t _nextBackgroundTaskDeadline;

	// Configured networks
	struct NetworkState {
		NetworkState() : tap((VirtualTap*)0)
		{
			// Real defaults are in network 'up' code in network event
			// handler
			settings.allowManaged = true;
			settings.allowGlobal = false;
			settings.allowDefault = false;
		}

		VirtualTap* tap;
		ZT_VirtualNetworkConfig config;   // memcpy() of raw config from core
		std::vector<InetAddress> managedIps;
		NetworkSettings settings;
	};
	std::map<uint64_t, NetworkState> _nets;

	/** Lock to control access to network configuration data */
	Mutex _nets_m;
	/** Lock to control access to storage data */
	Mutex _store_m;
	/** Lock to control access to service run state */
	Mutex _run_m;
	// Set to false to force service to stop
	volatile bool _run;
	/** Lock to control access to termination reason */
	Mutex _termReason_m;
	// Termination status information
	ReasonForTermination _termReason;

	std::string _fatalErrorMessage;

	// uPnP/NAT-PMP port mapper if enabled
	bool _portMappingEnabled;   // local.conf settings
#ifdef ZT_USE_MINIUPNPC
	PortMapper* _portMapper;
#endif

	/** Whether we allow caching network configs to storage */
	uint8_t _allowNetworkCaching;

	/** Whether we allow caching peer hints to storage */
	uint8_t _allowPeerCaching;

	char _publicIdStr[ZT_IDENTITY_STRING_BUFFER_LENGTH] = { 0 };
	char _secretIdStr[ZT_IDENTITY_STRING_BUFFER_LENGTH] = { 0 };
	char _planetData[ZTS_STORE_DATA_LEN] = { 0 };

	/** Whether the node has successfully come online */
	bool _nodeIsOnline;

	/** Whether we allow the NodeService to generate events for the user */
	bool _eventsEnabled;

	/** Primary port defined by the user */
	uint16_t _userProvidedPort;

	/** Storage path defined by the user */
	std::string _homePath;

	/** System to ingest events from this class and emit them to the user */
	Events* _events;

	/**
	 * Constructor
	 */
	NodeService();

	/**
	 * Destructor
	 */
	~NodeService();

	ReasonForTermination run();

	ReasonForTermination reasonForTermination() const;

	std::string fatalErrorMessage() const;

	void getRoutes(uint64_t net_id, void* routeArray, unsigned int* numRoutes);

	void terminate();

	void uninitialize();

	int getNetworkSettings(const uint64_t net_id, NetworkSettings& settings) const;

	int checkIfManagedIsAllowed(const NetworkState& n, const InetAddress& target);

	void syncManagedStuff(NetworkState& n);

	void phyOnDatagram(
	    PhySocket* sock,
	    void** uptr,
	    const struct sockaddr* localAddr,
	    const struct sockaddr* from,
	    void* data,
	    unsigned long len);

	int nodeVirtualNetworkConfigFunction(
	    uint64_t net_id,
	    void** nuptr,
	    enum ZT_VirtualNetworkConfigOperation op,
	    const ZT_VirtualNetworkConfig* nwc);

	void nodeEventCallback(enum ZT_Event event, const void* metaData);
	zts_net_info_t* prepare_network_details_msg(const NetworkState& n);

	void generateEventMsgs();

	/** Join a network */
	int join(uint64_t net_id);

	/** Leave a network */
	int leave(uint64_t net_id);

	/** Return number of networks joined */
	int networkCount();

	/** Orbit a moon */
	int orbit(void* tptr, uint64_t moonWorldId, uint64_t moonSeed);

	/** De-orbit a moon */
	int deorbit(void* tptr, uint64_t moonWorldId);

	uint64_t getNodeId();

	int getIdentity(char* keypair, uint16_t* len);

	int setIdentity(const char* keypair, uint16_t len);

	void nodeStatePutFunction(
	    enum ZT_StateObjectType type,
	    const uint64_t id[2],
	    const void* data,
	    int len);

	int nodeStateGetFunction(
	    enum ZT_StateObjectType type,
	    const uint64_t id[2],
	    void* data,
	    unsigned int maxlen);

	int nodeWirePacketSendFunction(
	    const int64_t localSocket,
	    const struct sockaddr_storage* addr,
	    const void* data,
	    unsigned int len,
	    unsigned int ttl);

	void nodeVirtualNetworkFrameFunction(
	    uint64_t net_id,
	    void** nuptr,
	    uint64_t sourceMac,
	    uint64_t destMac,
	    unsigned int etherType,
	    unsigned int vlanId,
	    const void* data,
	    unsigned int len);

	int nodePathCheckFunction(
	    uint64_t ztaddr,
	    const int64_t localSocket,
	    const struct sockaddr_storage* remoteAddr);

	int nodePathLookupFunction(uint64_t ztaddr, int family, struct sockaddr_storage* result);

	void tapFrameHandler(
	    uint64_t net_id,
	    const MAC& from,
	    const MAC& to,
	    unsigned int etherType,
	    unsigned int vlanId,
	    const void* data,
	    unsigned int len);

	int shouldBindInterface(const char* ifname, const InetAddress& ifaddr);

	int _trialBind(unsigned int port);

	/** Return whether the NodeService is running */
	int isRunning();

	/** Return whether the node is online */
	int nodeIsOnline();

	/** Instruct the NodeService on where to look for identity files and caches */
	int setHomePath(const char* homePath);

	/** Set the NodeService's primary port */
	int setPrimaryPort(unsigned short primaryPort);

	/** Get the NodeService's primary port */
	unsigned short getPrimaryPort();

	/** Set the event system instance used to convey messages to the user */
	int setUserEventSystem(Events* events);

	/** Set the planet definition */
	int setPlanet(const char* data, int len);

	/** Add Interface prefix to blacklist (prevents ZeroTier from using that interface) */
	int addInterfacePrefixToBlacklist(const char* prefix, int len);

	/** Return the MAC Address of the node in the given network */
	uint64_t getMACAddress(uint64_t net_id);

	int getNetworkName(uint64_t net_id, char* dst, int len);

	int allowPeerCaching(int allowed);
	int allowNetworkCaching(int allowed);
	int getNetworkBroadcast(uint64_t net_id);
	int getNetworkMTU(uint64_t net_id);
	int getNetworkType(uint64_t net_id);
	int getNetworkStatus(uint64_t net_id);

	int getFirstAssignedAddr(uint64_t net_id, int family, struct zts_sockaddr_storage* addr);

	int getAllAssignedAddr(uint64_t net_id, struct zts_sockaddr_storage* addr, int* count);

	int networkHasRoute(uint64_t net_id, int family);

	int addrIsAssigned(uint64_t net_id, int family);

	void phyOnTcpConnect(PhySocket* sock, void** uptr, bool success)
	{
	}
	void phyOnTcpAccept(
	    PhySocket* sockL,
	    PhySocket* sockN,
	    void** uptrL,
	    void** uptrN,
	    const struct sockaddr* from)
	{
	}
	void phyOnTcpClose(PhySocket* sock, void** uptr)
	{
	}
	void phyOnTcpData(PhySocket* sock, void** uptr, void* data, unsigned long len)
	{
	}
	void phyOnTcpWritable(PhySocket* sock, void** uptr)
	{
	}
	void phyOnFileDescriptorActivity(PhySocket* sock, void** uptr, bool readable, bool writable)
	{
	}
	void phyOnUnixAccept(PhySocket* sockL, PhySocket* sockN, void** uptrL, void** uptrN)
	{
	}
	void phyOnUnixClose(PhySocket* sock, void** uptr)
	{
	}
	void phyOnUnixData(PhySocket* sock, void** uptr, void* data, unsigned long len)
	{
	}
	void phyOnUnixWritable(PhySocket* sock, void** uptr)
	{
	}
};

static int SnodeVirtualNetworkConfigFunction(
    ZT_Node* node,
    void* uptr,
    void* tptr,
    uint64_t net_id,
    void** nuptr,
    enum ZT_VirtualNetworkConfigOperation op,
    const ZT_VirtualNetworkConfig* nwconf)
{
	return reinterpret_cast<NodeService*>(uptr)
	    ->nodeVirtualNetworkConfigFunction(net_id, nuptr, op, nwconf);
}

static void
SnodeEventCallback(ZT_Node* node, void* uptr, void* tptr, enum ZT_Event event, const void* metaData)
{
	reinterpret_cast<NodeService*>(uptr)->nodeEventCallback(event, metaData);
}

static void SnodeStatePutFunction(
    ZT_Node* node,
    void* uptr,
    void* tptr,
    enum ZT_StateObjectType type,
    const uint64_t id[2],
    const void* data,
    int len)
{
	reinterpret_cast<NodeService*>(uptr)->nodeStatePutFunction(type, id, data, len);
}

static int SnodeStateGetFunction(
    ZT_Node* node,
    void* uptr,
    void* tptr,
    enum ZT_StateObjectType type,
    const uint64_t id[2],
    void* data,
    unsigned int maxlen)
{
	return reinterpret_cast<NodeService*>(uptr)->nodeStateGetFunction(type, id, data, maxlen);
}

static int SnodeWirePacketSendFunction(
    ZT_Node* node,
    void* uptr,
    void* tptr,
    int64_t localSocket,
    const struct sockaddr_storage* addr,
    const void* data,
    unsigned int len,
    unsigned int ttl)
{
	return reinterpret_cast<NodeService*>(uptr)
	    ->nodeWirePacketSendFunction(localSocket, addr, data, len, ttl);
}

static void SnodeVirtualNetworkFrameFunction(
    ZT_Node* node,
    void* uptr,
    void* tptr,
    uint64_t net_id,
    void** nuptr,
    uint64_t sourceMac,
    uint64_t destMac,
    unsigned int etherType,
    unsigned int vlanId,
    const void* data,
    unsigned int len)
{
	reinterpret_cast<NodeService*>(uptr)->nodeVirtualNetworkFrameFunction(
	    net_id,
	    nuptr,
	    sourceMac,
	    destMac,
	    etherType,
	    vlanId,
	    data,
	    len);
}

static int SnodePathCheckFunction(
    ZT_Node* node,
    void* uptr,
    void* tptr,
    uint64_t ztaddr,
    int64_t localSocket,
    const struct sockaddr_storage* remoteAddr)
{
	return reinterpret_cast<NodeService*>(uptr)->nodePathCheckFunction(
	    ztaddr,
	    localSocket,
	    remoteAddr);
}

static int SnodePathLookupFunction(
    ZT_Node* node,
    void* uptr,
    void* tptr,
    uint64_t ztaddr,
    int family,
    struct sockaddr_storage* result)
{
	return reinterpret_cast<NodeService*>(uptr)->nodePathLookupFunction(ztaddr, family, result);
}

static void StapFrameHandler(
    void* uptr,
    void* tptr,
    uint64_t net_id,
    const MAC& from,
    const MAC& to,
    unsigned int etherType,
    unsigned int vlanId,
    const void* data,
    unsigned int len)
{
	reinterpret_cast<NodeService*>(uptr)
	    ->tapFrameHandler(net_id, from, to, etherType, vlanId, data, len);
}

}   // namespace ZeroTier

#endif
