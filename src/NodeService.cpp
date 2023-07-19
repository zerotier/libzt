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

#include "NodeService.hpp"

#include "../version.h"
#include "Events.hpp"
#include "InetAddress.hpp"
#include "Mutex.hpp"
#include "Node.hpp"
#include "Utilities.hpp"
#include "VirtualTap.hpp"

#if defined(__WINDOWS__)
#include <shlobj.h>
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <netioapi.h>
#define stat _stat
#endif

namespace ZeroTier {

static int SnodeVirtualNetworkConfigFunction(
    ZT_Node* node,
    void* uptr,
    void* tptr,
    uint64_t net_id,
    void** nuptr,
    enum ZT_VirtualNetworkConfigOperation op,
    const ZT_VirtualNetworkConfig* nwconf)
{
    ZTS_UNUSED_ARG(node);
    ZTS_UNUSED_ARG(tptr);
    return reinterpret_cast<NodeService*>(uptr)->nodeVirtualNetworkConfigFunction(net_id, nuptr, op, nwconf);
}

static void SnodeEventCallback(ZT_Node* node, void* uptr, void* tptr, enum ZT_Event event, const void* metaData)
{
    ZTS_UNUSED_ARG(node);
    ZTS_UNUSED_ARG(tptr);
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
    ZTS_UNUSED_ARG(node);
    ZTS_UNUSED_ARG(tptr);
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
    ZTS_UNUSED_ARG(node);
    ZTS_UNUSED_ARG(tptr);
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
    ZTS_UNUSED_ARG(node);
    ZTS_UNUSED_ARG(tptr);
    return reinterpret_cast<NodeService*>(uptr)->nodeWirePacketSendFunction(localSocket, addr, data, len, ttl);
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
    ZTS_UNUSED_ARG(node);
    ZTS_UNUSED_ARG(tptr);
    reinterpret_cast<NodeService*>(uptr)
        ->nodeVirtualNetworkFrameFunction(net_id, nuptr, sourceMac, destMac, etherType, vlanId, data, len);
}

static int SnodePathCheckFunction(
    ZT_Node* node,
    void* uptr,
    void* tptr,
    uint64_t ztaddr,
    int64_t localSocket,
    const struct sockaddr_storage* remoteAddr)
{
    ZTS_UNUSED_ARG(node);
    ZTS_UNUSED_ARG(tptr);
    return reinterpret_cast<NodeService*>(uptr)->nodePathCheckFunction(ztaddr, localSocket, remoteAddr);
}

static int SnodePathLookupFunction(
    ZT_Node* node,
    void* uptr,
    void* tptr,
    uint64_t ztaddr,
    int family,
    struct sockaddr_storage* result)
{
    ZTS_UNUSED_ARG(node);
    ZTS_UNUSED_ARG(tptr);
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
    ZTS_UNUSED_ARG(tptr);
    reinterpret_cast<NodeService*>(uptr)->tapFrameHandler(net_id, from, to, etherType, vlanId, data, len);
}

NodeService::NodeService()
    : _phy(this, false, true)
    , _node((Node*)0)
    , _nodeId(0x0)
    , _primaryPort()
    , _secondaryPort(0)
    , _tertiaryPort(0)
    , _randomPortRangeStart(0)
    , _randomPortRangeEnd(0)
    , _udpPortPickerCounter(0)
    , _lastDirectReceiveFromGlobal(0)
    , _lastRestart(0)
    , _nextBackgroundTaskDeadline(0)
    , _run(false)
    , _termReason(ONE_STILL_RUNNING)
    , _allowPortMapping(true)
#ifdef ZT_USE_MINIUPNPC
    , _portMapper((PortMapper*)0)
#endif
    , _allowSecondaryPort(true)
    , _allowNetworkCaching(true)
    , _allowPeerCaching(true)
    , _allowIdentityCaching(true)
    , _allowRootSetCaching(true)
    , _userDefinedWorld(false)
    , _nodeIsOnline(false)
    , _eventsEnabled(false)
    , _homePath("")
    , _events(NULL)
{
}

NodeService::~NodeService()
{
    _binder.closeAll(_phy);
#ifdef ZT_USE_MINIUPNPC
    delete _portMapper;
#endif
}

NodeService::ReasonForTermination NodeService::run()
{
    _run = true;
    try {
        // Create home path (if necessary)
        // By default, _homePath is empty and nothing is written to storage
        if (_homePath.length() > 0) {
            std::vector<std::string> hpsp(OSUtils::split(_homePath.c_str(), ZT_PATH_SEPARATOR_S, "", ""));
            std::string ptmp;
            if (_homePath[0] == ZT_PATH_SEPARATOR) {
                ptmp.push_back(ZT_PATH_SEPARATOR);
            }
            for (std::vector<std::string>::iterator pi(hpsp.begin()); pi != hpsp.end(); ++pi) {
                if (ptmp.length() > 0) {
                    ptmp.push_back(ZT_PATH_SEPARATOR);
                }
                ptmp.append(*pi);
                if ((*pi != ".") && (*pi != "..")) {
                    if (OSUtils::mkdir(ptmp) == false) {
                        Mutex::Lock _l(_termReason_m);
                        _termReason = ONE_UNRECOVERABLE_ERROR;
                        _fatalErrorMessage = "home path could not be created";
                        return _termReason;
                    }
                }
            }
        }

        // Set callbacks for ZT Node
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
            _node = new Node(this, (void*)0, &cb, OSUtils::now());
        }

        unsigned int minPort = (_randomPortRangeStart ? _randomPortRangeStart : 20000);
        unsigned int maxPort = (_randomPortRangeEnd ? _randomPortRangeEnd : 45500);

        // Make sure we can use the primary port, and hunt for one if
        // configured to do so
        const int portTrials = (_primaryPort == 0) ? 256 : 1;   // if port is 0, pick random
        for (int k = 0; k < portTrials; ++k) {
            if (_primaryPort == 0) {
                unsigned int randp = 0;
                Utils::getSecureRandom(&randp, sizeof(randp));
                _primaryPort = (randp % (maxPort - minPort + 1)) + minPort;
            }
            if (_trialBind(_primaryPort)) {
                _ports[0] = _primaryPort;
                break;
            }
            else {
                _primaryPort = 0;
            }
        }
        if (_ports[0] == 0) {
            Mutex::Lock _l(_termReason_m);
            _termReason = ONE_UNRECOVERABLE_ERROR;
            _fatalErrorMessage = "cannot bind to local control interface port";
            return _termReason;
        }

        // Attempt to bind to a secondary port.
        // This exists because there are buggy NATs out there that fail if more
        // than one device behind the same NAT tries to use the same internal
        // private address port number. Buggy NATs are a running theme.
        //
        // This used to pick the secondary port based on the node ID until we
        // discovered another problem: buggy routers and malicious traffic
        // "detection".  A lot of routers have such things built in these days
        // and mis-detect ZeroTier traffic as malicious and block it resulting
        // in a node that appears to be in a coma.  Secondary ports are now
        // randomized on startup.
        if (_allowSecondaryPort) {
            if (_secondaryPort) {
                _ports[1] = _secondaryPort;
            } else {
                _ports[1] = _getRandomPort(minPort, maxPort);
            }
        }
#ifdef ZT_USE_MINIUPNPC
        if (_allowPortMapping) {
            // If we're running uPnP/NAT-PMP, bind a *third* port for that.
            // We can't use the other two ports for that because some NATs
            // do really funky stuff with ports that are explicitly mapped
            // that breaks things.
            maxPort = (_randomPortRangeEnd ? _randomPortRangeEnd : 65536);

            if (_ports[1]) {
                if (_tertiaryPort) {
                    _ports[2] = _tertiaryPort;
                } else {
                    _ports[2] = minPort + (_ports[0] % 40000);
                    for(int i=0;;++i) {
                        if (i > 1000) {
                            _ports[2] = 0;
                            break;
                        }
                        else if (++_ports[2] >= maxPort) {
                            _ports[2] = minPort;
                        }
                        if (_trialBind(_ports[2])) {
                            _tertiaryPort = _ports[2];
                            break;
                        }
                    }
                    if (_ports[2]) {
                        char uniqueName[64] = { 0 };
                        OSUtils::ztsnprintf(
                            uniqueName,
                            sizeof(uniqueName),
                            "ZeroTier/%.10llx@%u",
                            _node->address(),
                            _ports[2]);
                        _portMapper = new PortMapper(_ports[2], uniqueName);
                    }
                }
            }
        }
#endif

        // Join existing networks in networks.d
        if (_allowNetworkCaching) {
            std::vector<std::string> networksDotD(
                OSUtils::listDirectory((_homePath + ZT_PATH_SEPARATOR_S "networks.d").c_str()));
            for (std::vector<std::string>::iterator f(networksDotD.begin()); f != networksDotD.end(); ++f) {
                std::size_t dot = f->find_last_of('.');
                if ((dot == 16) && (f->substr(16) == ".conf")) {
                    _node->join(Utils::hexStrToU64(f->substr(0, dot).c_str()), (void*)0, (void*)0);
                }
            }
        }
        // Main I/O loop
        _nextBackgroundTaskDeadline = 0;
        int64_t clockShouldBe = OSUtils::now();
        _lastRestart = clockShouldBe;
        int64_t lastTapMulticastGroupCheck = 0;
        int64_t lastBindRefresh = 0;
        int64_t lastCleanedPeersDb = 0;
        int64_t lastLocalInterfaceAddressCheck =
            (clockShouldBe - ZT_LOCAL_INTERFACE_CHECK_INTERVAL) + 15000;   // do this in 15s to give portmapper time to
        int64_t lastOnline = OSUtils::now();
        for (;;) {
            _run_m.lock();
            if (! _run) {
                _run_m.unlock();
                _termReason_m.lock();
                _termReason = ONE_NORMAL_TERMINATION;
                _termReason_m.unlock();
                break;
            }
            else {
                _run_m.unlock();
            }

            const int64_t now = OSUtils::now();

            // Attempt to detect sleep/wake events by detecting delay
            // overruns
            bool restarted = false;
            if ((now > clockShouldBe) && ((now - clockShouldBe) > 10000)) {
                _lastRestart = now;
                restarted = true;
            }

            // If secondary port is not configured to a constant value and we've been offline for a while,
            // bind a new secondary port. This is a workaround for a "coma" issue caused by buggy NATs that stop
            // working on one port after a while.
            if (_node->online()) {
                lastOnline = now;
            }
            else if ((_secondaryPort == 0) && ((now - lastOnline) > ZT_PATH_HEARTBEAT_PERIOD)) {
                _secondaryPort = _getRandomPort(minPort, maxPort);
                lastBindRefresh = 0;
            }

            // Refresh bindings in case device's interfaces have changed,
            // and also sync routes to update any shadow routes (e.g. shadow
            // default)
            if (((now - lastBindRefresh) >= ZT_BINDER_REFRESH_PERIOD) || (restarted)) {
                lastBindRefresh = now;
                unsigned int p[3] = { 0 };
                unsigned int pc = 0;
                for (int i = 0; i < 3; ++i) {
                    if (_ports[i]) {
                        p[pc++] = _ports[i];
                    }
                }
                _binder.refresh(_phy, p, pc, explicitBind, *this);
            }

            // Generate callback messages for user application
            generateSyntheticEvents();

            // Run background task processor in core if it's time to do so
            int64_t dl = _nextBackgroundTaskDeadline;
            if (dl <= now) {
                _node->processBackgroundTasks((void*)0, now, &_nextBackgroundTaskDeadline);
                dl = _nextBackgroundTaskDeadline;
            }

            // Sync multicast group memberships
            if ((now - lastTapMulticastGroupCheck) >= ZT_TAP_CHECK_MULTICAST_INTERVAL) {
                lastTapMulticastGroupCheck = now;
                std::vector<std::pair<uint64_t, std::pair<std::vector<MulticastGroup>, std::vector<MulticastGroup> > > >
                    mgChanges;
                {
                    Mutex::Lock _l(_nets_m);
                    mgChanges.reserve(_nets.size() + 1);
                    for (std::map<uint64_t, NetworkState>::const_iterator n(_nets.begin()); n != _nets.end(); ++n) {
                        if (n->second.tap) {
                            mgChanges.push_back(std::pair<
                                                uint64_t,
                                                std::pair<std::vector<MulticastGroup>, std::vector<MulticastGroup> > >(
                                n->first,
                                std::pair<std::vector<MulticastGroup>, std::vector<MulticastGroup> >()));
                            n->second.tap->scanMulticastGroups(
                                mgChanges.back().second.first,
                                mgChanges.back().second.second);
                        }
                    }
                }
                for (std::vector<
                         std::pair<uint64_t, std::pair<std::vector<MulticastGroup>, std::vector<MulticastGroup> > > >::
                         iterator c(mgChanges.begin());
                     c != mgChanges.end();
                     ++c) {
                    auto mgpair = c->second;
                    for (std::vector<MulticastGroup>::iterator m(mgpair.first.begin()); m != mgpair.first.end(); ++m) {
                        _node->multicastSubscribe((void*)0, c->first, m->mac().toInt(), m->adi());
                    }
                    for (std::vector<MulticastGroup>::iterator m(mgpair.second.begin()); m != mgpair.second.end();
                         ++m) {
                        _node->multicastUnsubscribe(c->first, m->mac().toInt(), m->adi());
                    }
                }
            }

            // Sync information about physical network interfaces
            if ((now - lastLocalInterfaceAddressCheck) >= ZT_LOCAL_INTERFACE_CHECK_INTERVAL) {
                lastLocalInterfaceAddressCheck = now;

                _node->clearLocalInterfaceAddresses();

#ifdef ZT_USE_MINIUPNPC
                if (_portMapper) {
                    std::vector<InetAddress> mappedAddresses(_portMapper->get());
                    for (std::vector<InetAddress>::const_iterator ext(mappedAddresses.begin());
                         ext != mappedAddresses.end();
                         ++ext)
                        _node->addLocalInterfaceAddress(reinterpret_cast<const struct sockaddr_storage*>(&(*ext)));
                }
#endif

                std::vector<InetAddress> boundAddrs(_binder.allBoundLocalInterfaceAddresses());
                for (std::vector<InetAddress>::const_iterator i(boundAddrs.begin()); i != boundAddrs.end(); ++i)
                    _node->addLocalInterfaceAddress(reinterpret_cast<const struct sockaddr_storage*>(&(*i)));
            }

            // Clean peers.d periodically
            if ((now - lastCleanedPeersDb) >= 3600000) {
                lastCleanedPeersDb = now;
                OSUtils::cleanDirectory(
                    (_homePath + ZT_PATH_SEPARATOR_S "peers.d").c_str(),
                    now - 2592000000LL);   // delete older than 30 days
            }

            const unsigned long delay = (dl > now) ? (unsigned long)(dl - now) : 100;
            clockShouldBe = now + (uint64_t)delay;
            _phy.poll(delay);
        }
    }
    catch (std::exception& e) {
        Mutex::Lock _l(_termReason_m);
        _termReason = ONE_UNRECOVERABLE_ERROR;
        _fatalErrorMessage = std::string("unexpected exception in main thread: ") + e.what();
    }
    catch (...) {
        Mutex::Lock _l(_termReason_m);
        _termReason = ONE_UNRECOVERABLE_ERROR;
        _fatalErrorMessage = "unexpected exception in main thread: unknown exception";
    }

    {
        Mutex::Lock _l(_nets_m);
        for (std::map<uint64_t, NetworkState>::iterator n(_nets.begin()); n != _nets.end(); ++n) {
            delete n->second.tap;
        }
        _nets.clear();
    }

    switch (_termReason) {
        case ONE_NORMAL_TERMINATION:
            nodeEventCallback(ZT_EVENT_DOWN, NULL);
            break;
        case ONE_UNRECOVERABLE_ERROR:
            nodeEventCallback(ZT_EVENT_FATAL_ERROR_IDENTITY_COLLISION, NULL);
            break;
        default:
            break;
    }
    delete _node;
    _node = (Node*)0;
    return _termReason;
}

NodeService::ReasonForTermination NodeService::reasonForTermination() const
{
    Mutex::Lock _l(_termReason_m);
    return _termReason;
}

std::string NodeService::fatalErrorMessage() const
{
    Mutex::Lock _l(_termReason_m);
    return _fatalErrorMessage;
}

void NodeService::terminate()
{
    _run_m.lock();
    _run = false;
    _run_m.unlock();
    _nodeId = 0x0;
    _primaryPort = 0;
    _homePath.clear();
    _allowNetworkCaching = true;
    _allowPeerCaching = true;
    _allowIdentityCaching = true;
    _allowRootSetCaching = true;
    memset(_publicIdStr, 0, ZT_IDENTITY_STRING_BUFFER_LENGTH);
    memset(_secretIdStr, 0, ZT_IDENTITY_STRING_BUFFER_LENGTH);
    _interfacePrefixBlacklist.clear();
    _events->disable();
    _phy.whack();
}

void NodeService::syncManagedStuff(NetworkState& n)
{
    char ipbuf[64] = { 0 };
    // assumes _nets_m is locked
    std::vector<InetAddress> newManagedIps;
    newManagedIps.reserve(n.config.assignedAddressCount);
    for (unsigned int i = 0; i < n.config.assignedAddressCount; ++i) {
        const InetAddress* ii = reinterpret_cast<const InetAddress*>(&(n.config.assignedAddresses[i]));
        newManagedIps.push_back(*ii);
    }
    std::sort(newManagedIps.begin(), newManagedIps.end());
    newManagedIps.erase(std::unique(newManagedIps.begin(), newManagedIps.end()), newManagedIps.end());
    for (std::vector<InetAddress>::iterator ip(n.managedIps.begin()); ip != n.managedIps.end(); ++ip) {
        if (std::find(newManagedIps.begin(), newManagedIps.end(), *ip) == newManagedIps.end()) {
            if (! n.tap->removeIp(*ip)) {
                fprintf(stderr, "ERROR: unable to remove ip address %s" ZT_EOL_S, ip->toString(ipbuf));
            }
            else {
                zts_addr_info_t* ad = new zts_addr_info_t();
                ad->net_id = n.tap->_net_id;
                if ((*ip).isV4()) {
                    struct sockaddr_in* in4 = (struct sockaddr_in*)&(ad->addr);
                    memcpy(&(in4->sin_addr.s_addr), (*ip).rawIpData(), 4);
                    in4->sin_family = ZTS_AF_INET;
                    sendEventToUser(ZTS_EVENT_ADDR_REMOVED_IP4, (void*)ad);
                }
                if ((*ip).isV6()) {
                    struct sockaddr_in6* in6 = (struct sockaddr_in6*)&(ad->addr);
                    memcpy(&(in6->sin6_addr.s6_addr), (*ip).rawIpData(), 16);
                    in6->sin6_family = ZTS_AF_INET6;
                    sendEventToUser(ZTS_EVENT_ADDR_REMOVED_IP6, (void*)ad);
                }
            }
        }
    }
    for (std::vector<InetAddress>::iterator ip(newManagedIps.begin()); ip != newManagedIps.end(); ++ip) {
        if (std::find(n.managedIps.begin(), n.managedIps.end(), *ip) == n.managedIps.end()) {
            if (! n.tap->addIp(*ip)) {
                fprintf(stderr, "ERROR: unable to add ip address %s" ZT_EOL_S, ip->toString(ipbuf));
            }
            else {
                zts_addr_info_t* ad = new zts_addr_info_t();
                ad->net_id = n.tap->_net_id;
                if ((*ip).isV4()) {
                    struct sockaddr_in* in4 = (struct sockaddr_in*)&(ad->addr);
                    memcpy(&(in4->sin_addr.s_addr), (*ip).rawIpData(), 4);
                    in4->sin_family = ZTS_AF_INET;
                    sendEventToUser(ZTS_EVENT_ADDR_ADDED_IP4, (void*)ad);
                }
                if ((*ip).isV6()) {
                    struct sockaddr_in6* in6 = (struct sockaddr_in6*)&(ad->addr);
                    memcpy(&(in6->sin6_addr.s6_addr), (*ip).rawIpData(), 16);
                    in6->sin6_family = ZTS_AF_INET6;
                    sendEventToUser(ZTS_EVENT_ADDR_ADDED_IP6, (void*)ad);
                }
            }
        }
    }
    n.managedIps.swap(newManagedIps);
}

void NodeService::phyOnDatagram(
    PhySocket* sock,
    void** uptr,
    const struct sockaddr* localAddr,
    const struct sockaddr* from,
    void* data,
    unsigned long len)
{
    ZTS_UNUSED_ARG(uptr);
    ZTS_UNUSED_ARG(localAddr);
    if ((len >= 16) && (reinterpret_cast<const InetAddress*>(from)->ipScope() == InetAddress::IP_SCOPE_GLOBAL))
        _lastDirectReceiveFromGlobal = OSUtils::now();
    const ZT_ResultCode rc = _node->processWirePacket(
        (void*)0,
        OSUtils::now(),
        reinterpret_cast<int64_t>(sock),
        reinterpret_cast<const struct sockaddr_storage*>(from),   // Phy<> uses sockaddr_storage, so
                                                                  // it'll always be that big
        data,
        len,
        &_nextBackgroundTaskDeadline);
    if (ZT_ResultCode_isFatal(rc)) {
        char tmp[256] = { 0 };
        OSUtils::ztsnprintf(tmp, sizeof(tmp), "fatal error code from processWirePacket: %d", (int)rc);
        Mutex::Lock _l(_termReason_m);
        _termReason = ONE_UNRECOVERABLE_ERROR;
        _fatalErrorMessage = tmp;
        this->terminate();
    }
}

int NodeService::nodeVirtualNetworkConfigFunction(
    uint64_t net_id,
    void** nuptr,
    enum ZT_VirtualNetworkConfigOperation op,
    const ZT_VirtualNetworkConfig* nwc)
{
    Mutex::Lock _l(_nets_m);
    NetworkState& n = _nets[net_id];

    switch (op) {
        case ZT_VIRTUAL_NETWORK_CONFIG_OPERATION_UP:
            if (! n.tap) {
                n.tap = new VirtualTap(
                    _homePath.c_str(),
                    MAC(nwc->mac),
                    nwc->mtu,
                    (unsigned int)ZT_IF_METRIC,
                    net_id,
                    StapFrameHandler,
                    (void*)this);
                *nuptr = (void*)&n;
                n.tap->setUserEventSystem(_events);
            }
            // After setting up tap, fall through to CONFIG_UPDATE since we
            // also want to do this...
        case ZT_VIRTUAL_NETWORK_CONFIG_OPERATION_CONFIG_UPDATE:
            memcpy(&(n.config), nwc, sizeof(ZT_VirtualNetworkConfig));
            if (n.tap) {   // sanity check
                syncManagedStuff(n);
                n.tap->setMtu(nwc->mtu);
            }
            else {
                _nets.erase(net_id);
                return -999;   // tap init failed
            }
            if (op == ZT_VIRTUAL_NETWORK_CONFIG_OPERATION_CONFIG_UPDATE) {
                sendEventToUser(ZTS_EVENT_NETWORK_UPDATE, (void*)&n);
            }
            break;
        case ZT_VIRTUAL_NETWORK_CONFIG_OPERATION_DOWN:
        case ZT_VIRTUAL_NETWORK_CONFIG_OPERATION_DESTROY:
            sendEventToUser(ZTS_EVENT_NETWORK_DOWN, (void*)&n);
            if (n.tap) {   // sanity check
                *nuptr = (void*)0;
                delete n.tap;
                _nets.erase(net_id);
                if (_allowNetworkCaching) {
                    if (op == ZT_VIRTUAL_NETWORK_CONFIG_OPERATION_DESTROY) {
                        char nlcpath[256] = { 0 };
                        OSUtils::ztsnprintf(
                            nlcpath,
                            sizeof(nlcpath),
                            "%s" ZT_PATH_SEPARATOR_S "networks.d" ZT_PATH_SEPARATOR_S "%.16llx.local.conf",
                            _homePath.c_str(),
                            net_id);
                        OSUtils::rm(nlcpath);
                    }
                }
            }
            else {
                _nets.erase(net_id);
            }
            break;
    }
    return 0;
}

void NodeService::nodeEventCallback(enum ZT_Event event, const void* metaData)
{
    ZTS_UNUSED_ARG(metaData);

    int event_code = 0;
    _nodeIsOnline = (event == ZT_EVENT_ONLINE) ? true : false;
    _nodeId = _node ? _node->address() : 0x0;

    switch (event) {
        case ZT_EVENT_UP:
            event_code = ZTS_EVENT_NODE_UP;
            break;
        case ZT_EVENT_ONLINE:
            event_code = ZTS_EVENT_NODE_ONLINE;
            break;
        case ZT_EVENT_OFFLINE:
            event_code = ZTS_EVENT_NODE_OFFLINE;
            break;
        case ZT_EVENT_DOWN:
            event_code = ZTS_EVENT_NODE_DOWN;
            break;
        case ZT_EVENT_FATAL_ERROR_IDENTITY_COLLISION: {
            Mutex::Lock _l(_termReason_m);
            _termReason = ONE_IDENTITY_COLLISION;
            event_code = ZTS_EVENT_NODE_FATAL_ERROR;
            this->terminate();
        } break;
        default:
            break;
    }
    if (event_code) {
        sendEventToUser(event_code, NULL);
    }
}

void NodeService::sendEventToUser(unsigned int zt_event_code, const void* obj, unsigned int len)
{
    if (! _events) {
        return;
    }

    // Convert raw ZT object into ZTS counterpart

    void* objptr = NULL;

    switch (zt_event_code) {
        case ZTS_EVENT_NODE_UP:
        case ZTS_EVENT_NODE_ONLINE:
        case ZTS_EVENT_NODE_OFFLINE:
        case ZTS_EVENT_NODE_DOWN:
        case ZTS_EVENT_NODE_FATAL_ERROR: {
            zts_node_info_t* nd = new zts_node_info_t;
            nd->node_id = _nodeId;
            nd->ver_major = ZEROTIER_ONE_VERSION_MAJOR;
            nd->ver_minor = ZEROTIER_ONE_VERSION_MINOR;
            nd->ver_rev = ZEROTIER_ONE_VERSION_REVISION;
            nd->port_primary = _primaryPort;
            nd->port_secondary = _secondaryPort;
            nd->port_tertiary = _tertiaryPort;
            objptr = (void*)nd;
            break;
        }
        case ZTS_EVENT_NETWORK_NOT_FOUND:
        case ZTS_EVENT_NETWORK_CLIENT_TOO_OLD:
        case ZTS_EVENT_NETWORK_REQ_CONFIG:
        case ZTS_EVENT_NETWORK_ACCESS_DENIED:
        case ZTS_EVENT_NETWORK_DOWN: {
            NetworkState* ns = (NetworkState*)obj;
            zts_net_info_t* nd = new zts_net_info_t();
            nd->net_id = ns->config.nwid;
            objptr = (void*)nd;
            break;
        }
        case ZTS_EVENT_NETWORK_UPDATE:
        case ZTS_EVENT_NETWORK_READY_IP4:
        case ZTS_EVENT_NETWORK_READY_IP6:
        case ZTS_EVENT_NETWORK_OK: {
            NetworkState* ns = (NetworkState*)obj;
            zts_net_info_t* nd = new zts_net_info_t();
            nd->net_id = ns->config.nwid;
            nd->mac = ns->config.mac;
            strncpy(nd->name, ns->config.name, sizeof(ns->config.name));
            nd->status = (zts_network_status_t)ns->config.status;
            nd->type = (zts_net_info_type_t)ns->config.type;
            nd->mtu = ns->config.mtu;
            nd->dhcp = ns->config.dhcp;
            nd->bridge = ns->config.bridge;
            nd->broadcast_enabled = ns->config.broadcastEnabled;
            nd->port_error = ns->config.portError;
            nd->netconf_rev = ns->config.netconfRevision;
            // Copy and convert address structures
            nd->assigned_addr_count = ns->config.assignedAddressCount;
            for (unsigned int i = 0; i < ns->config.assignedAddressCount; i++) {
                native_ss_to_zts_ss(&(nd->assigned_addrs[i]), &(ns->config.assignedAddresses[i]));
            }
            nd->route_count = ns->config.routeCount;
            for (unsigned int i = 0; i < ns->config.routeCount; i++) {
                native_ss_to_zts_ss(&(nd->routes[i].target), &(ns->config.routes[i].target));
                native_ss_to_zts_ss(&(nd->routes[i].via), &(ns->config.routes[i].via));
                nd->routes[i].flags = ns->config.routes[i].flags;
                nd->routes[i].metric = ns->config.routes[i].metric;
            }
            nd->multicast_sub_count = ns->config.multicastSubscriptionCount;
            memcpy(nd->multicast_subs, &(ns->config.multicastSubscriptions), sizeof(ns->config.multicastSubscriptions));
            objptr = (void*)nd;
            break;
        }
        case ZTS_EVENT_ADDR_ADDED_IP4:
            objptr = (void*)obj;
            break;
        case ZTS_EVENT_ADDR_ADDED_IP6:
            objptr = (void*)obj;
            break;
        case ZTS_EVENT_ADDR_REMOVED_IP4:
            objptr = (void*)obj;
            break;
        case ZTS_EVENT_ADDR_REMOVED_IP6:
            objptr = (void*)obj;
            break;
        case ZTS_EVENT_STORE_IDENTITY_PUBLIC:
            objptr = (void*)obj;
            break;
        case ZTS_EVENT_STORE_IDENTITY_SECRET:
            objptr = (void*)obj;
            break;
        case ZTS_EVENT_STORE_PLANET:
            objptr = (void*)obj;
            break;
        case ZTS_EVENT_STORE_PEER:
            objptr = (void*)obj;
            break;
        case ZTS_EVENT_STORE_NETWORK:
            objptr = (void*)obj;
            break;
        case ZTS_EVENT_PEER_DIRECT:
        case ZTS_EVENT_PEER_RELAY:
        case ZTS_EVENT_PEER_UNREACHABLE:
        case ZTS_EVENT_PEER_PATH_DISCOVERED:
        case ZTS_EVENT_PEER_PATH_DEAD: {
            zts_peer_info_t* pd = new zts_peer_info_t();
            ZT_Peer* peer = (ZT_Peer*)obj;
            memcpy(pd, peer, sizeof(zts_peer_info_t));
            for (unsigned int j = 0; j < peer->pathCount; j++) {
                native_ss_to_zts_ss(&(pd->paths[j].address), &(peer->paths[j].address));
            }
            objptr = (void*)pd;
            break;
        }
        default:
            break;
    }

    // Send event

    if (objptr) {
        _events->enqueue(zt_event_code, objptr, len);
    }
}

void NodeService::generateSyntheticEvents()
{
    // Force the ordering of callback messages, these messages are
    // only useful if the node and stack are both up and running
    if (! _node->online() || ! zts_lwip_is_up()) {
        return;
    }
    // Generate messages to be dequeued by the callback message thread
    Mutex::Lock _l(_nets_m);
    for (std::map<uint64_t, NetworkState>::iterator n(_nets.begin()); n != _nets.end(); ++n) {
        auto netState = n->second;
        int mostRecentStatus = netState.config.status;
        VirtualTap* tap = netState.tap;
        // uint64_t net_id = n->first;
        if (netState.tap->_networkStatus == mostRecentStatus) {
            continue;   // No state change
        }
        switch (mostRecentStatus) {
            case ZT_NETWORK_STATUS_NOT_FOUND:
                sendEventToUser(ZTS_EVENT_NETWORK_NOT_FOUND, (void*)&netState);
                break;
            case ZT_NETWORK_STATUS_CLIENT_TOO_OLD:
                sendEventToUser(ZTS_EVENT_NETWORK_CLIENT_TOO_OLD, (void*)&netState);
                break;
            case ZT_NETWORK_STATUS_REQUESTING_CONFIGURATION:
                sendEventToUser(ZTS_EVENT_NETWORK_REQ_CONFIG, (void*)&netState);
                break;
            case ZT_NETWORK_STATUS_OK:
                if (tap->hasIpv4Addr() && zts_lwip_is_netif_up(tap->netif4)) {
                    sendEventToUser(ZTS_EVENT_NETWORK_READY_IP4, (void*)&netState);
                }
                if (tap->hasIpv6Addr() && zts_lwip_is_netif_up(tap->netif6)) {
                    sendEventToUser(ZTS_EVENT_NETWORK_READY_IP6, (void*)&netState);
                }
                // In addition to the READY messages, send one OK message
                sendEventToUser(ZTS_EVENT_NETWORK_OK, (void*)&netState);
                break;
            case ZT_NETWORK_STATUS_ACCESS_DENIED:
                sendEventToUser(ZTS_EVENT_NETWORK_ACCESS_DENIED, (void*)&netState);
                break;
            default:
                break;
        }
        netState.tap->_networkStatus = mostRecentStatus;
    }
    ZT_PeerList* pl = _node->peers();
    if (pl) {
        for (unsigned long i = 0; i < pl->peerCount; ++i) {
            if (! peerCache.count(pl->peers[i].address)) {
                // New peer, add status
                if (pl->peers[i].pathCount > 0) {
                    sendEventToUser(ZTS_EVENT_PEER_DIRECT, (void*)&(pl->peers[i]));
                }
                if (pl->peers[i].pathCount == 0) {
                    sendEventToUser(ZTS_EVENT_PEER_RELAY, (void*)&(pl->peers[i]));
                }
            }
            else {   // Previously known peer, update status
                if (peerCache[pl->peers[i].address] < pl->peers[i].pathCount) {
                    sendEventToUser(ZTS_EVENT_PEER_PATH_DISCOVERED, (void*)&(pl->peers[i]));
                }
                if (peerCache[pl->peers[i].address] > pl->peers[i].pathCount) {
                    sendEventToUser(ZTS_EVENT_PEER_PATH_DEAD, (void*)&(pl->peers[i]));
                }
                if (peerCache[pl->peers[i].address] == 0 && pl->peers[i].pathCount > 0) {
                    sendEventToUser(ZTS_EVENT_PEER_DIRECT, (void*)&(pl->peers[i]));
                }
                if (peerCache[pl->peers[i].address] > 0 && pl->peers[i].pathCount == 0) {
                    sendEventToUser(ZTS_EVENT_PEER_RELAY, (void*)&(pl->peers[i]));
                }
            }
            // Update our cache with most recently observed path count
            peerCache[pl->peers[i].address] = pl->peers[i].pathCount;
        }
    }
    _node->freeQueryResult((void*)pl);
}

int NodeService::join(uint64_t net_id)
{
    if (! net_id) {
        return ZTS_ERR_ARG;
    }
    _node->join(net_id, NULL, NULL);
    return ZTS_ERR_OK;
}

int NodeService::leave(uint64_t net_id)
{
    if (! net_id) {
        return ZTS_ERR_ARG;
    }
    _node->leave(net_id, NULL, NULL);
    return ZTS_ERR_OK;
}

void NodeService::obtainLock() const
{
    _nets_m.lock();
}

void NodeService::releaseLock() const
{
    _nets_m.unlock();
}

bool NodeService::networkIsReady(uint64_t net_id) const
{
    if (! net_id) {
        return ZTS_ERR_ARG;
    }
    Mutex::Lock _l(_nets_m);
    std::map<uint64_t, NetworkState>::const_iterator n(_nets.find(net_id));
    if (n == _nets.end()) {
        return false;
    }
    auto netState = n->second;
    return netState.config.assignedAddressCount > 0;
}

int NodeService::addressCount(uint64_t net_id) const
{
    std::map<uint64_t, NetworkState>::const_iterator n(_nets.find(net_id));
    if (n == _nets.end()) {
        return ZTS_ERR_NO_RESULT;
    }
    return n->second.config.assignedAddressCount;
}

int NodeService::routeCount(uint64_t net_id) const
{
    std::map<uint64_t, NetworkState>::const_iterator n(_nets.find(net_id));
    if (n == _nets.end()) {
        return ZTS_ERR_NO_RESULT;
    }
    return n->second.config.routeCount;
}

int NodeService::multicastSubCount(uint64_t net_id) const
{
    std::map<uint64_t, NetworkState>::const_iterator n(_nets.find(net_id));
    if (n == _nets.end()) {
        return ZTS_ERR_NO_RESULT;
    }
    return n->second.config.multicastSubscriptionCount;
}

int NodeService::pathCount(uint64_t peer_id) const
{
    return ZTS_ERR_NO_RESULT;   // TODO
}

int NodeService::getAddrAtIdx(uint64_t net_id, unsigned int idx, char* dst, unsigned int len)
{
    std::map<uint64_t, NetworkState>::const_iterator n(_nets.find(net_id));
    if (n == _nets.end()) {
        return 0;
    }
    auto netState = n->second;
    if (idx >= netState.config.assignedAddressCount) {
        return ZTS_ERR_ARG;
    }
    struct sockaddr* sa = (struct sockaddr*)&(netState.config.assignedAddresses[idx]);

    if (sa->sa_family == AF_INET) {
        struct sockaddr_in* in4 = (struct sockaddr_in*)sa;
        inet_ntop(AF_INET, &(in4->sin_addr), dst, ZTS_INET6_ADDRSTRLEN);
    }
    if (sa->sa_family == AF_INET6) {
        struct sockaddr_in6* in6 = (struct sockaddr_in6*)sa;
        inet_ntop(AF_INET6, &(in6->sin6_addr), dst, ZTS_INET6_ADDRSTRLEN);
    }
    return ZTS_ERR_OK;
}

int NodeService::getRouteAtIdx(
    uint64_t net_id,
    unsigned int idx,
    char* target,
    char* via,
    unsigned int len,
    uint16_t* flags,
    uint16_t* metric)
{
    // We want to use strlen later so let's ensure there's no junk first.
    memset(target, 0, len);
    memset(via, 0, len);
    std::map<uint64_t, NetworkState>::const_iterator n(_nets.find(net_id));
    if (n == _nets.end()) {
        return 0;
    }
    auto netState = n->second;
    if (idx >= netState.config.routeCount) {
        return ZTS_ERR_ARG;
    }
    // target
    struct sockaddr* sa = (struct sockaddr*)&(netState.config.routes[idx].target);
    if (sa->sa_family == AF_INET) {
        struct sockaddr_in* in4 = (struct sockaddr_in*)sa;
        inet_ntop(AF_INET, &(in4->sin_addr), target, ZTS_INET6_ADDRSTRLEN);
    }
    if (sa->sa_family == AF_INET6) {
        struct sockaddr_in6* in6 = (struct sockaddr_in6*)sa;
        inet_ntop(AF_INET6, &(in6->sin6_addr), target, ZTS_INET6_ADDRSTRLEN);
    }
    // via
    struct sockaddr* sa_via = (struct sockaddr*)&(netState.config.routes[idx].via);
    if (sa_via->sa_family == AF_INET) {
        struct sockaddr_in* in4 = (struct sockaddr_in*)sa_via;
        inet_ntop(AF_INET, &(in4->sin_addr), via, ZTS_INET6_ADDRSTRLEN);
    }
    if (sa_via->sa_family == AF_INET6) {
        struct sockaddr_in6* in6 = (struct sockaddr_in6*)sa_via;
        inet_ntop(AF_INET6, &(in6->sin6_addr), via, ZTS_INET6_ADDRSTRLEN);
    }
    if (strlen(via) == 0) {
        strncpy(via, "0.0.0.0", 7);
        // TODO: Double check
    }
    *flags = netState.config.routes[idx].flags;
    *metric = netState.config.routes[idx].metric;
    return ZTS_ERR_OK;
}

int NodeService::getMulticastSubAtIdx(uint64_t net_id, unsigned int idx, uint64_t* mac, uint32_t* adi)
{
    std::map<uint64_t, NetworkState>::const_iterator n(_nets.find(net_id));
    if (n == _nets.end()) {
        return 0;
    }
    auto netState = n->second;
    if (idx >= netState.config.multicastSubscriptionCount) {
        return ZTS_ERR_ARG;
    }
    *mac = netState.config.multicastSubscriptions[idx].mac;
    *adi = netState.config.multicastSubscriptions[idx].adi;
    return ZTS_ERR_OK;
}

int NodeService::getPathAtIdx(uint64_t peer_id, unsigned int idx, char* path, unsigned int len)
{
    return ZTS_ERR_NO_RESULT;   // TODO
}

int NodeService::getFirstAssignedAddr(uint64_t net_id, unsigned int family, struct zts_sockaddr_storage* addr)
{
    if (net_id == 0 || ((family != ZTS_AF_INET) && (family != ZTS_AF_INET6)) || ! addr) {
        return ZTS_ERR_ARG;
    }
    Mutex::Lock _l(_nets_m);
    std::map<uint64_t, NetworkState>::const_iterator n(_nets.find(net_id));
    if (n == _nets.end()) {
        return ZTS_ERR_NO_RESULT;
    }
    auto netState = n->second;
    if (netState.config.assignedAddressCount == 0) {
        return ZTS_ERR_NO_RESULT;
    }
    for (unsigned int i = 0; i < netState.config.assignedAddressCount; i++) {
        struct sockaddr* sa = (struct sockaddr*)&(netState.config.assignedAddresses[i]);
        // Family values may vary across platforms, thus the following
        if (sa->sa_family == AF_INET && family == ZTS_AF_INET) {
            native_ss_to_zts_ss(addr, &(netState.config.assignedAddresses[i]));
            return ZTS_ERR_OK;
        }
        if (sa->sa_family == AF_INET6 && family == ZTS_AF_INET6) {
            native_ss_to_zts_ss(addr, &(netState.config.assignedAddresses[i]));
            return ZTS_ERR_OK;
        }
    }
    return ZTS_ERR_NO_RESULT;
}

int NodeService::getAllAssignedAddr(uint64_t net_id, struct zts_sockaddr_storage* addr, unsigned int* count)
{
    if (net_id == 0 || ! addr || ! count || *count != ZTS_MAX_ASSIGNED_ADDRESSES) {
        return ZTS_ERR_ARG;
    }
    Mutex::Lock _l(_nets_m);
    std::map<uint64_t, NetworkState>::const_iterator n(_nets.find(net_id));
    if (n == _nets.end()) {
        return ZTS_ERR_NO_RESULT;
    }
    memset(addr, 0, sizeof(struct zts_sockaddr_storage) * ZTS_MAX_ASSIGNED_ADDRESSES);
    auto netState = n->second;
    if (netState.config.assignedAddressCount == 0) {
        return ZTS_ERR_NO_RESULT;
    }
    for (unsigned int i = 0; i < netState.config.assignedAddressCount; i++) {
        native_ss_to_zts_ss(&addr[i], &(netState.config.assignedAddresses[i]));
    }
    *count = netState.config.assignedAddressCount;
    return ZTS_ERR_OK;
}

int NodeService::addrIsAssigned(uint64_t net_id, unsigned int family)
{
    if (net_id == 0) {
        return ZTS_ERR_ARG;
    }
    struct zts_sockaddr_storage addr;   // unused
    return getFirstAssignedAddr(net_id, family, &addr) != ZTS_ERR_NO_RESULT;
}

int NodeService::networkHasRoute(uint64_t net_id, unsigned int family)
{
    Mutex::Lock _l(_nets_m);
    std::map<uint64_t, NetworkState>::const_iterator n(_nets.find(net_id));
    if (n == _nets.end()) {
        return ZTS_ERR_NO_RESULT;
    }
    auto netState = n->second;
    for (unsigned int i = 0; i < netState.config.routeCount; i++) {
        struct sockaddr* sa = (struct sockaddr*)&(netState.config.routes[i].target);
        if (sa->sa_family == AF_INET && family == ZTS_AF_INET) {
            return true;
        }
        if (sa->sa_family == AF_INET6 && family == ZTS_AF_INET6) {
            return true;
        }
    }
    return false;
}

int NodeService::orbit(uint64_t moon_roots_id, uint64_t moon_seed)
{
    if (! moon_roots_id || ! moon_seed) {
        return ZTS_ERR_ARG;
    }
    Mutex::Lock _lr(_run_m);
    if (! _run) {
        return ZTS_ERR_SERVICE;
    }
    return _node->orbit(NULL, moon_roots_id, moon_seed);
}

int NodeService::deorbit(uint64_t moon_roots_id)
{
    if (! moon_roots_id) {
        return ZTS_ERR_ARG;
    }
    Mutex::Lock _lr(_run_m);
    if (! _run) {
        return ZTS_ERR_SERVICE;
    }
    return _node->deorbit(NULL, moon_roots_id);
}

uint64_t NodeService::getNodeId()
{
    Mutex::Lock _lr(_run_m);
    if (! _run) {
        return 0x0;
    }
    return _node ? _node->address() : 0x0;
}

int NodeService::setIdentity(const char* keypair, unsigned int len)
{
    if (keypair == NULL || len < ZT_IDENTITY_STRING_BUFFER_LENGTH) {
        return ZTS_ERR_ARG;
    }
    // Double check user-provided keypair
    Identity id;
    if ((strlen(keypair) > 32) && (keypair[10] == ':')) {
        if (! id.fromString(keypair)) {
            return id.locallyValidate();
        }
    }
    Mutex::Lock _lr(_run_m);
    if (_run) {
        return ZTS_ERR_SERVICE;
    }
    Mutex::Lock _ls(_store_m);
    memcpy(_secretIdStr, keypair, len);
    return ZTS_ERR_OK;
}

int NodeService::getIdentity(char* keypair, unsigned int* len)
{
    if (keypair == NULL || *len < ZT_IDENTITY_STRING_BUFFER_LENGTH) {
        return ZTS_ERR_ARG;
    }
    if (_node) {
        _node->identity().toString(true, keypair);
    }
    else {
        return ZTS_ERR_GENERAL;
    }
    *len = strnlen(keypair, ZT_IDENTITY_STRING_BUFFER_LENGTH);
    return ZTS_ERR_OK;
}

void NodeService::nodeStatePutFunction(
    enum ZT_StateObjectType type,
    const uint64_t id[2],
    const void* data,
    unsigned int len)
{
    char p[1024] = { 0 };
    FILE* f;
    bool secure = false;
    char dirname[1024] = { 0 };
    dirname[0] = 0;

    Mutex::Lock _ls(_store_m);

    switch (type) {
        case ZT_STATE_OBJECT_IDENTITY_PUBLIC:
            sendEventToUser(ZTS_EVENT_STORE_IDENTITY_PUBLIC, data, len);
            memcpy(_publicIdStr, data, len);
            if (_homePath.length() > 0 && _allowIdentityCaching) {
                OSUtils::ztsnprintf(p, sizeof(p), "%s" ZT_PATH_SEPARATOR_S "identity.public", _homePath.c_str());
            }
            else {
                return;
            }
            break;
        case ZT_STATE_OBJECT_IDENTITY_SECRET:
            sendEventToUser(ZTS_EVENT_STORE_IDENTITY_SECRET, data, len);
            memcpy(_secretIdStr, data, len);
            if (_homePath.length() > 0 && _allowIdentityCaching) {
                OSUtils::ztsnprintf(p, sizeof(p), "%s" ZT_PATH_SEPARATOR_S "identity.secret", _homePath.c_str());
                secure = true;
            }
            else {
                return;
            }
            break;
        case ZT_STATE_OBJECT_PLANET:
            sendEventToUser(ZTS_EVENT_STORE_PLANET, data, len);
            memcpy(_rootsData, data, len);
            if (_homePath.length() > 0 && _allowRootSetCaching) {
                OSUtils::ztsnprintf(p, sizeof(p), "%s" ZT_PATH_SEPARATOR_S "roots", _homePath.c_str());
            }
            else {
                return;
            }
            break;
        case ZT_STATE_OBJECT_NETWORK_CONFIG:
            if (_homePath.length() > 0 && _allowNetworkCaching) {
                OSUtils::ztsnprintf(dirname, sizeof(dirname), "%s" ZT_PATH_SEPARATOR_S "networks.d", _homePath.c_str());
                OSUtils::ztsnprintf(
                    p,
                    sizeof(p),
                    "%s" ZT_PATH_SEPARATOR_S "%.16llx.conf",
                    dirname,
                    (unsigned long long)id[0]);
                secure = true;
            }
            else {
                return;
            }
            break;
        case ZT_STATE_OBJECT_PEER:
            if (_homePath.length() > 0 && _allowPeerCaching) {
                OSUtils::ztsnprintf(dirname, sizeof(dirname), "%s" ZT_PATH_SEPARATOR_S "peers.d", _homePath.c_str());
                OSUtils::ztsnprintf(
                    p,
                    sizeof(p),
                    "%s" ZT_PATH_SEPARATOR_S "%.10llx.peer",
                    dirname,
                    (unsigned long long)id[0]);
            }
            else {
                return;
            }
            break;
        default:
            return;
    }

    if (len >= 0) {
        // Check to see if we've already written this first. This reduces
        // redundant writes and I/O overhead on most platforms and has
        // little effect on others.
        f = fopen(p, "rb");
        if (f) {
            char buf[65535] = { 0 };
            long l = (long)fread(buf, 1, sizeof(buf), f);
            fclose(f);
            if ((l == (long)len) && (memcmp(data, buf, l) == 0)) {
                return;
            }
        }

        f = fopen(p, "wb");
        if ((! f) && (dirname[0])) {   // create subdirectory if it does not exist
            OSUtils::mkdir(dirname);
            f = fopen(p, "wb");
        }
        if (f) {
            if (fwrite(data, len, 1, f) != 1)
                fprintf(
                    stderr,
                    "WARNING: unable to write to file: %s (I/O "
                    "error)" ZT_EOL_S,
                    p);
            fclose(f);
            if (secure) {
                OSUtils::lockDownFile(p, false);
            }
        }
        else {
            fprintf(
                stderr,
                "WARNING: unable to write to file: %s (unable to "
                "open)" ZT_EOL_S,
                p);
        }
    }
    else {
        OSUtils::rm(p);
    }
}

int NodeService::nodeStateGetFunction(
    enum ZT_StateObjectType type,
    const uint64_t id[2],
    void* data,
    unsigned int maxlen)
{
    char p[4096] = { 0 };
    unsigned int keylen = 0;
    switch (type) {
        case ZT_STATE_OBJECT_IDENTITY_PUBLIC:
            keylen = strlen(_publicIdStr);
            if (keylen > 0 && keylen <= maxlen) {
                memcpy(data, _publicIdStr, keylen);
                return keylen;
            }
            if (_homePath.length() > 0) {
                OSUtils::ztsnprintf(p, sizeof(p), "%s" ZT_PATH_SEPARATOR_S "identity.public", _homePath.c_str());
            }
            break;
        case ZT_STATE_OBJECT_IDENTITY_SECRET:
            keylen = strlen(_secretIdStr);
            if (keylen > 0 && keylen <= maxlen) {
                memcpy(data, _secretIdStr, keylen);
                return keylen;
            }
            if (_homePath.length() > 0) {
                OSUtils::ztsnprintf(p, sizeof(p), "%s" ZT_PATH_SEPARATOR_S "identity.secret", _homePath.c_str());
            }
            break;
        case ZT_STATE_OBJECT_PLANET:
            if (_userDefinedWorld) {
                memcpy(data, _rootsData, _rootsDataLen);
                return _rootsDataLen;
            }
            OSUtils::ztsnprintf(p, sizeof(p), "%s" ZT_PATH_SEPARATOR_S "roots", _homePath.c_str());
            break;
        case ZT_STATE_OBJECT_NETWORK_CONFIG:
            OSUtils::ztsnprintf(
                p,
                sizeof(p),
                "%s" ZT_PATH_SEPARATOR_S "networks.d" ZT_PATH_SEPARATOR_S "%.16llx.conf",
                _homePath.c_str(),
                (unsigned long long)id[0]);
            break;
        case ZT_STATE_OBJECT_PEER:
            OSUtils::ztsnprintf(
                p,
                sizeof(p),
                "%s" ZT_PATH_SEPARATOR_S "peers.d" ZT_PATH_SEPARATOR_S "%.10llx.peer",
                _homePath.c_str(),
                (unsigned long long)id[0]);
            break;
        default:
            return -1;
    }
    FILE* f = fopen(p, "rb");
    if (f) {
        int n = (int)fread(data, 1, maxlen, f);
        fclose(f);
        if (n >= 0) {
            return n;
        }
    }
    return -1;
}

int NodeService::nodeWirePacketSendFunction(
    const int64_t localSocket,
    const struct sockaddr_storage* addr,
    const void* data,
    unsigned int len,
    unsigned int ttl)
{
    // Even when relaying we still send via UDP. This way if UDP starts
    // working we can instantly "fail forward" to it and stop using TCP
    // proxy fallback, which is slow.

    if ((localSocket != -1) && (localSocket != 0) && (_binder.isUdpSocketValid((PhySocket*)((uintptr_t)localSocket)))) {
        if ((ttl) && (addr->ss_family == AF_INET))
            _phy.setIp4UdpTtl((PhySocket*)((uintptr_t)localSocket), ttl);
        const bool r = _phy.udpSend((PhySocket*)((uintptr_t)localSocket), (const struct sockaddr*)addr, data, len);
        if ((ttl) && (addr->ss_family == AF_INET))
            _phy.setIp4UdpTtl((PhySocket*)((uintptr_t)localSocket), 255);
        return ((r) ? 0 : -1);
    }
    else {
        return ((_binder.udpSendAll(_phy, addr, data, len, ttl)) ? 0 : -1);
    }
}

void NodeService::nodeVirtualNetworkFrameFunction(
    uint64_t net_id,
    void** nuptr,
    uint64_t sourceMac,
    uint64_t destMac,
    unsigned int etherType,
    unsigned int vlanId,
    const void* data,
    unsigned int len)
{
    ZTS_UNUSED_ARG(vlanId);
    ZTS_UNUSED_ARG(net_id);
    NetworkState* n = reinterpret_cast<NetworkState*>(*nuptr);
    if ((! n) || (! n->tap)) {
        return;
    }
    n->tap->put(MAC(sourceMac), MAC(destMac), etherType, data, len);
}

int NodeService::nodePathCheckFunction(
    uint64_t ztaddr,
    const int64_t localSocket,
    const struct sockaddr_storage* remoteAddr)
{
    ZTS_UNUSED_ARG(localSocket);
    // Make sure we're not trying to do ZeroTier-over-ZeroTier
    {
        Mutex::Lock _l(_nets_m);
        for (std::map<uint64_t, NetworkState>::const_iterator n(_nets.begin()); n != _nets.end(); ++n) {
            if (n->second.tap) {
                std::vector<InetAddress> ips(n->second.tap->ips());
                for (std::vector<InetAddress>::const_iterator i(ips.begin()); i != ips.end(); ++i) {
                    if (i->containsAddress(*(reinterpret_cast<const InetAddress*>(remoteAddr)))) {
                        return 0;
                    }
                }
            }
        }
    }

    /* Note: I do not think we need to scan for overlap with managed routes
     * because of the "route forking" and interface binding that we do. This
     * ensures (we hope) that ZeroTier traffic will still take the physical
     * path even if its managed routes this for other traffic. Will
     * revisit if we see recursion problems. */

    // Check blacklists
    const Hashtable<uint64_t, std::vector<InetAddress> >* blh =
        (const Hashtable<uint64_t, std::vector<InetAddress> >*)0;
    const std::vector<InetAddress>* gbl = (const std::vector<InetAddress>*)0;
    if (remoteAddr->ss_family == AF_INET) {
        blh = &_v4Blacklists;
        gbl = &_globalV4Blacklist;
    }
    else if (remoteAddr->ss_family == AF_INET6) {
        blh = &_v6Blacklists;
        gbl = &_globalV6Blacklist;
    }
    if (blh) {
        Mutex::Lock _l(_localConfig_m);
        const std::vector<InetAddress>* l = blh->get(ztaddr);
        if (l) {
            for (std::vector<InetAddress>::const_iterator a(l->begin()); a != l->end(); ++a) {
                if (a->containsAddress(*reinterpret_cast<const InetAddress*>(remoteAddr))) {
                    return 0;
                }
            }
        }
    }
    if (gbl) {
        for (std::vector<InetAddress>::const_iterator a(gbl->begin()); a != gbl->end(); ++a) {
            if (a->containsAddress(*reinterpret_cast<const InetAddress*>(remoteAddr))) {
                return 0;
            }
        }
    }
    return 1;
}

int NodeService::nodePathLookupFunction(uint64_t ztaddr, unsigned int family, struct sockaddr_storage* result)
{
    const Hashtable<uint64_t, std::vector<InetAddress> >* lh = (const Hashtable<uint64_t, std::vector<InetAddress> >*)0;
    if (family < 0) {
        lh = (_node->prng() & 1) ? &_v4Hints : &_v6Hints;
    }
    else if (family == AF_INET) {
        lh = &_v4Hints;
    }
    else if (family == AF_INET6) {
        lh = &_v6Hints;
    }
    else {
        return 0;
    }
    const std::vector<InetAddress>* l = lh->get(ztaddr);
    if ((l) && (l->size() > 0)) {
        memcpy(result, &((*l)[(unsigned long)_node->prng() % l->size()]), sizeof(struct sockaddr_storage));
        return 1;
    }
    else {
        return 0;
    }
}

void NodeService::tapFrameHandler(
    uint64_t net_id,
    const MAC& from,
    const MAC& to,
    unsigned int etherType,
    unsigned int vlanId,
    const void* data,
    unsigned int len)
{
    _node->processVirtualNetworkFrame(
        (void*)0,
        OSUtils::now(),
        net_id,
        from.toInt(),
        to.toInt(),
        etherType,
        vlanId,
        data,
        len,
        &_nextBackgroundTaskDeadline);
}

int NodeService::shouldBindInterface(const char* ifname, const InetAddress& ifaddr)
{
#if defined(__linux__) || defined(linux) || defined(__LINUX__) || defined(__linux)
    if ((ifname[0] == 'l') && (ifname[1] == 'o')) {
        return false;   // loopback
    }
    if ((ifname[0] == 'z') && (ifname[1] == 't')) {
        return false;   // sanity check: zt#
    }
    if ((ifname[0] == 't') && (ifname[1] == 'u') && (ifname[2] == 'n')) {
        return false;   // tun# is probably an OpenVPN tunnel or similar
    }
    if ((ifname[0] == 't') && (ifname[1] == 'a') && (ifname[2] == 'p')) {
        return false;   // tap# is probably an OpenVPN tunnel or similar
    }
#endif

#ifdef __APPLE__
    if ((ifname[0] == 'f') && (ifname[1] == 'e') && (ifname[2] == 't') && (ifname[3] == 'h')) {
        return false;   // ... as is feth#
    }
    if ((ifname[0] == 'l') && (ifname[1] == 'o')) {
        return false;   // loopback
    }
    if ((ifname[0] == 'z') && (ifname[1] == 't')) {
        return false;   // sanity check: zt#
    }
    if ((ifname[0] == 't') && (ifname[1] == 'u') && (ifname[2] == 'n')) {
        return false;   // tun# is probably an OpenVPN tunnel or similar
    }
    if ((ifname[0] == 't') && (ifname[1] == 'a') && (ifname[2] == 'p')) {
        return false;   // tap# is probably an OpenVPN tunnel or similar
    }
    if ((ifname[0] == 'u') && (ifname[1] == 't') && (ifname[2] == 'u') && (ifname[3] == 'n')) {
        return false;   // ... as is utun#
    }
#endif

    {
        Mutex::Lock _l(_localConfig_m);
        for (std::vector<std::string>::const_iterator p(_interfacePrefixBlacklist.begin());
             p != _interfacePrefixBlacklist.end();
             ++p) {
            if (! strncmp(p->c_str(), ifname, p->length())) {
                return false;
            }
        }
    }
    {
        // Check global blacklists
        const std::vector<InetAddress>* gbl = (const std::vector<InetAddress>*)0;
        if (ifaddr.ss_family == AF_INET) {
            gbl = &_globalV4Blacklist;
        }
        else if (ifaddr.ss_family == AF_INET6) {
            gbl = &_globalV6Blacklist;
        }
        if (gbl) {
            Mutex::Lock _l(_localConfig_m);
            for (std::vector<InetAddress>::const_iterator a(gbl->begin()); a != gbl->end(); ++a) {
                if (a->containsAddress(ifaddr)) {
                    return false;
                }
            }
        }
    }
    {
        Mutex::Lock _l(_nets_m);
        for (std::map<uint64_t, NetworkState>::const_iterator n(_nets.begin()); n != _nets.end(); ++n) {
            if (n->second.tap) {
                std::vector<InetAddress> ips(n->second.tap->ips());
                for (std::vector<InetAddress>::const_iterator i(ips.begin()); i != ips.end(); ++i) {
                    if (i->ipsEqual(ifaddr)) {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

unsigned int NodeService::_getRandomPort(unsigned int minPort, unsigned int maxPort)
{
    unsigned int randp = 0;
    Utils::getSecureRandom(&randp,sizeof(randp));
    randp = (randp % (maxPort - minPort + 1)) + minPort;
    for(int i=0;;++i) {
        if (i > 1000) {
            return 0;
        }
        else if (++randp >= maxPort) {
            randp = minPort;
        }
        if (_trialBind(randp)) {
            break;
        }
    }
    return randp;
}

int NodeService::_trialBind(unsigned int port)
{
    struct sockaddr_in in4;
    struct sockaddr_in6 in6;
    PhySocket* tb;

    memset(&in4, 0, sizeof(in4));
    in4.sin_family = AF_INET;
    in4.sin_port = Utils::hton((uint16_t)port);
    tb = _phy.udpBind(reinterpret_cast<const struct sockaddr*>(&in4), (void*)0, 0);
    if (tb) {
        _phy.close(tb, false);
        return true;
    }

    memset(&in6, 0, sizeof(in6));
    in6.sin6_family = AF_INET6;
    in6.sin6_port = Utils::hton((uint16_t)port);
    tb = _phy.udpBind(reinterpret_cast<const struct sockaddr*>(&in6), (void*)0, 0);
    if (tb) {
        _phy.close(tb, false);
        return true;
    }
    return false;
}

int NodeService::isRunning() const
{
    return _run;
}

int NodeService::nodeIsOnline() const
{
    return _nodeIsOnline;
}

int NodeService::setHomePath(const char* homePath)
{
    if (! homePath) {
        return ZTS_ERR_ARG;
    }
    Mutex::Lock _lr(_run_m);
    if (_run) {
        return ZTS_ERR_SERVICE;
    }
    _homePath = std::string(homePath);
    return ZTS_ERR_OK;
}

int NodeService::setPrimaryPort(unsigned short primaryPort)
{
    Mutex::Lock _lr(_run_m);
    if (_run) {
        return ZTS_ERR_SERVICE;
    }
    _primaryPort = primaryPort;
    return ZTS_ERR_OK;
}

int NodeService::setRandomPortRange(unsigned short startPort, unsigned short endPort)
{
    Mutex::Lock _lr(_run_m);
    if (_run) {
        return ZTS_ERR_SERVICE;
    }
    _randomPortRangeStart = startPort;
    _randomPortRangeEnd = endPort;
    return ZTS_ERR_OK;
}

unsigned short NodeService::getPrimaryPort() const
{
    return _primaryPort;
}

int NodeService::allowPortMapping(unsigned int allowed)
{
    Mutex::Lock _lr(_run_m);
    if (_run) {
        return ZTS_ERR_SERVICE;
    }
    _allowPortMapping = allowed;
    return ZTS_ERR_OK;
}

int NodeService::allowSecondaryPort(unsigned int allowed)
{
    Mutex::Lock _lr(_run_m);
    if (_run) {
        return ZTS_ERR_SERVICE;
    }
    _allowSecondaryPort = allowed;
    return ZTS_ERR_OK;
}

int NodeService::setUserEventSystem(Events* events)
{
    Mutex::Lock _lr(_run_m);
    if (_run) {
        return ZTS_ERR_SERVICE;
    }
    _events = events;
    return ZTS_ERR_OK;
}

void NodeService::enableEvents()
{
    Mutex::Lock _lr(_run_m);
    if (! _events) {
        return;
    }
    _events->enable();
}

int NodeService::setRoots(const void* rootsData, unsigned int len)
{
    if (! rootsData || len <= 0 || len > ZTS_STORE_DATA_LEN) {
        return ZTS_ERR_ARG;
    }
    Mutex::Lock _lr(_run_m);
    if (_run) {
        return ZTS_ERR_SERVICE;
    }
    Mutex::Lock _ls(_store_m);
    memcpy(_rootsData, rootsData, len);
    _rootsDataLen = len;
    _userDefinedWorld = true;
    return ZTS_ERR_OK;
}

int NodeService::setLowBandwidthMode(bool enabled)
{
    Mutex::Lock _lr(_run_m);
    if (_run) {
        return ZTS_ERR_SERVICE;
    }
    _node->setLowBandwidthMode(enabled);
    return ZTS_ERR_OK;
}

int NodeService::addInterfacePrefixToBlacklist(const char* prefix, unsigned int len)
{
    if (! prefix || len == 0 || len > 15) {
        return ZTS_ERR_ARG;
    }
    Mutex::Lock _lr(_run_m);
    if (_run) {
        return ZTS_ERR_SERVICE;
    }
    Mutex::Lock _l(_localConfig_m);
    _interfacePrefixBlacklist.push_back(std::string(prefix));
    return ZTS_ERR_OK;
}

uint64_t NodeService::getMACAddress(uint64_t net_id) const
{
    if (net_id == 0) {
        return ZTS_ERR_ARG;
    }
    Mutex::Lock _lr(_run_m);
    if (! _run) {
        return ZTS_ERR_SERVICE;
    }
    Mutex::Lock _ln(_nets_m);
    std::map<uint64_t, NetworkState>::const_iterator n(_nets.find(net_id));
    if (n == _nets.end()) {
        return ZTS_ERR_NO_RESULT;
    }
    return n->second.config.mac;
}

int NodeService::getNetworkName(uint64_t net_id, char* dst, unsigned int len) const
{
    if (net_id == 0 || ! dst || len != ZTS_MAX_NETWORK_SHORT_NAME_LENGTH) {
        return ZTS_ERR_ARG;
    }
    Mutex::Lock _lr(_run_m);
    if (! _run) {
        return ZTS_ERR_SERVICE;
    }
    if (! _nodeIsOnline) {
        return ZTS_ERR_SERVICE;
    }
    Mutex::Lock _ln(_nets_m);
    std::map<uint64_t, NetworkState>::const_iterator n(_nets.find(net_id));
    if (n == _nets.end()) {
        return ZTS_ERR_NO_RESULT;
    }
    auto netState = n->second;
    strncpy(dst, netState.config.name, ZTS_MAX_NETWORK_SHORT_NAME_LENGTH);
    return ZTS_ERR_OK;
}

int NodeService::allowPeerCaching(unsigned int allowed)
{
    Mutex::Lock _lr(_run_m);
    if (_run) {
        return ZTS_ERR_SERVICE;
    }
    _allowPeerCaching = allowed;
    return ZTS_ERR_OK;
}

int NodeService::allowNetworkCaching(unsigned int allowed)
{
    Mutex::Lock _lr(_run_m);
    if (_run) {
        return ZTS_ERR_SERVICE;
    }
    _allowNetworkCaching = allowed;
    return ZTS_ERR_OK;
}

int NodeService::allowIdentityCaching(unsigned int allowed)
{
    Mutex::Lock _lr(_run_m);
    if (_run) {
        return ZTS_ERR_SERVICE;
    }
    _allowIdentityCaching = allowed;
    return ZTS_ERR_OK;
}

int NodeService::allowRootSetCaching(unsigned int allowed)
{
    Mutex::Lock _lr(_run_m);
    if (_run) {
        return ZTS_ERR_SERVICE;
    }
    _allowRootSetCaching = allowed;
    return ZTS_ERR_OK;
}
int NodeService::getNetworkBroadcast(uint64_t net_id)
{
    if (net_id == 0) {
        return ZTS_ERR_ARG;
    }
    Mutex::Lock _lr(_run_m);
    if (! _run) {
        return ZTS_ERR_SERVICE;
    }
    Mutex::Lock _ln(_nets_m);
    std::map<uint64_t, NetworkState>::const_iterator n(_nets.find(net_id));
    if (n == _nets.end()) {
        return ZTS_ERR_NO_RESULT;
    }
    return n->second.config.broadcastEnabled;
}

int NodeService::getNetworkMTU(uint64_t net_id)
{
    Mutex::Lock _lr(_run_m);
    if (! _run) {
        return ZTS_ERR_SERVICE;
    }
    Mutex::Lock _ln(_nets_m);
    std::map<uint64_t, NetworkState>::const_iterator n(_nets.find(net_id));
    if (n == _nets.end()) {
        return ZTS_ERR_NO_RESULT;
    }
    return n->second.config.mtu;
}

int NodeService::getNetworkType(uint64_t net_id)
{
    Mutex::Lock _lr(_run_m);
    if (! _run) {
        return ZTS_ERR_SERVICE;
    }
    Mutex::Lock _ln(_nets_m);
    std::map<uint64_t, NetworkState>::const_iterator n(_nets.find(net_id));
    if (n == _nets.end()) {
        return ZTS_ERR_NO_RESULT;
    }
    return n->second.config.type;
}

int NodeService::getNetworkStatus(uint64_t net_id)
{
    Mutex::Lock _lr(_run_m);
    if (! _run) {
        return ZTS_ERR_SERVICE;
    }
    Mutex::Lock _ln(_nets_m);
    std::map<uint64_t, NetworkState>::const_iterator n(_nets.find(net_id));
    if (n == _nets.end()) {
        return ZTS_ERR_NO_RESULT;
    }
    return n->second.config.status;
}

}   // namespace ZeroTier
