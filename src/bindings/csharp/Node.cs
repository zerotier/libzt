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

using System;
using System.Net;
using System.Net.Sockets;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Runtime.InteropServices;

using ZeroTier;

// Prototype of callback used by ZeroTier to signal events to C# application
[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
public delegate void CSharpCallbackWithStruct(IntPtr msgPtr);

namespace ZeroTier.Core
{
    public delegate void ZeroTierManagedEventCallback(ZeroTier.Core.Event nodeEvent);

    public class Node {
        static ulong _id = 0x0;
        static ushort _secondaryPort;
        static ushort _tertiaryPort;
        static int _versionMajor;
        static int _versionMinor;
        static int _versionRev;
        static bool _isOnline = false;
        static bool _hasBeenFreed = false;
        static string _configFilePath;
        static ushort _primaryPort;

        static ZeroTierManagedEventCallback _managedCallback;
        static CSharpCallbackWithStruct _unmanagedCallback;

        static ConcurrentDictionary<ulong, ZeroTier.Core.NetworkInfo> _networks =
            new ConcurrentDictionary<ulong, NetworkInfo>();
        static ConcurrentDictionary<ulong, ZeroTier.Core.PeerInfo> _peers = new ConcurrentDictionary<ulong, PeerInfo>();

        public Node()
        {
            ClearNode();
        }

        void ClearNode()
        {
            _id = 0x0;
            _primaryPort = 0;
            _secondaryPort = 0;
            _tertiaryPort = 0;
            _versionMajor = 0;
            _versionMinor = 0;
            _versionRev = 0;
            _isOnline = false;
            _configFilePath = string.Empty;
            _networks.Clear();
            _peers.Clear();
        }

        public int InitFromStorage(string configFilePath)
        {
            if (String.IsNullOrEmpty(configFilePath)) {
                throw new ArgumentNullException("configFilePath");
            }
            int res = Constants.ERR_OK;
            if ((res = zts_init_from_storage(configFilePath)) == Constants.ERR_OK) {
                _configFilePath = configFilePath;
            }
            return res;
        }

        public int InitSetEventHandler(ZeroTierManagedEventCallback managedCallback)
        {
            if (managedCallback == null) {
                throw new ArgumentNullException("managedCallback");
            }
            _unmanagedCallback = OnZeroTierEvent;
            int res = Constants.ERR_OK;
            if ((res = zts_init_set_event_handler(_unmanagedCallback)) == Constants.ERR_OK) {
                _managedCallback = new ZeroTierManagedEventCallback(managedCallback);
            }
            return res;
        }

        public int InitSetPort(UInt16 port)
        {
            int res = Constants.ERR_OK;
            if ((res = zts_init_set_port(port)) == Constants.ERR_OK) {
                _primaryPort = port;
            }
            return res;
        }

        public int InitSetRandomPortRange(UInt16 startPort, UInt16 endPort)
        {
            return zts_init_set_random_port_range(startPort, endPort);
        }

        public int InitSetRoots(byte[] roots_data, int len)
        {
            IntPtr unmanagedPointer = Marshal.AllocHGlobal(roots_data.Length);
            Marshal.Copy(roots_data, 0, unmanagedPointer, roots_data.Length);
            int res = zts_init_set_roots(unmanagedPointer, len);
            Marshal.FreeHGlobal(unmanagedPointer);
            return res;
        }

        public int InitAllowNetworkCaching(bool allowed)
        {
            return zts_init_allow_net_cache(Convert.ToByte(allowed));
        }

        public int InitAllowSecondaryPort(bool allowed)
        {
            return zts_init_allow_secondary_port(Convert.ToByte(allowed));
        }

        public int InitAllowPortMapping(bool allowed)
        {
            return zts_init_allow_port_mapping(Convert.ToByte(allowed));
        }

        public int InitAllowPeerCaching(bool allowed)
        {
            return zts_init_allow_peer_cache(Convert.ToByte(allowed));
        }

        void OnZeroTierEvent(IntPtr msgPtr)
        {
            zts_event_msg_t msg = (zts_event_msg_t)Marshal.PtrToStructure(msgPtr, typeof(zts_event_msg_t));
            ZeroTier.Core.Event newEvent = null;

            // Node

            if (msg.node != IntPtr.Zero) {
                zts_node_info_t details = (zts_node_info_t)Marshal.PtrToStructure(msg.node, typeof(zts_node_info_t));
                newEvent = new ZeroTier.Core.Event();
                newEvent.Code = msg.event_code;
                _id = details.node_id;
                _primaryPort = details.primary_port;
                _secondaryPort = details.secondary_port;
                _tertiaryPort = details.tertiary_port;
                _versionMajor = details.ver_major;
                _versionMinor = details.ver_minor;
                _versionRev = details.ver_rev;
                _isOnline = Convert.ToBoolean(zts_node_is_online());

                if (msg.event_code == Constants.EVENT_NODE_UP) {
                    newEvent.Name = "EVENT_NODE_UP";
                }
                if (msg.event_code == Constants.EVENT_NODE_ONLINE) {
                    newEvent.Name = "EVENT_NODE_ONLINE";
                }
                if (msg.event_code == Constants.EVENT_NODE_OFFLINE) {
                    newEvent.Name = "EVENT_NODE_OFFLINE";
                }
                if (msg.event_code == Constants.EVENT_NODE_DOWN) {
                    newEvent.Name = "EVENT_NODE_DOWN";
                }
                if (msg.event_code == Constants.ZTS_EVENT_NODE_FATAL_ERROR) {
                    newEvent.Name = "EVENT_NODE_FATAL_ERROR";
                }
            }

            // Network

            if (msg.network != IntPtr.Zero) {
                zts_net_info_t net_info = (zts_net_info_t)Marshal.PtrToStructure(msg.network, typeof(zts_net_info_t));
                newEvent = new ZeroTier.Core.Event();
                newEvent.Code = msg.event_code;

                // Update network info as long as we aren't tearing down the network
                if (msg.event_code != Constants.EVENT_NETWORK_DOWN) {
                    ulong networkId = net_info.net_id;
                    NetworkInfo ni = _networks.GetOrAdd(networkId, new NetworkInfo());

                    newEvent.NetworkInfo = ni;
                    newEvent.NetworkInfo.Id = net_info.net_id;
                    newEvent.NetworkInfo.MACAddress = net_info.mac;
                    newEvent.NetworkInfo.Name = System.Text.Encoding.UTF8.GetString(net_info.name);
                    newEvent.NetworkInfo.Status = net_info.status;
                    newEvent.NetworkInfo.Type = net_info.type;
                    newEvent.NetworkInfo.MTU = net_info.mtu;
                    newEvent.NetworkInfo.DHCP = net_info.dhcp;
                    newEvent.NetworkInfo.Bridge = Convert.ToBoolean(net_info.bridge);
                    newEvent.NetworkInfo.BroadcastEnabled = Convert.ToBoolean(net_info.broadcast_enabled);

                    zts_core_lock_obtain();

                    // Get assigned addresses

                    ConcurrentDictionary<string, IPAddress> newAddrsDict =
                        new ConcurrentDictionary<string, IPAddress>();
                    IntPtr addrBuffer = Marshal.AllocHGlobal(ZeroTier.Constants.INET6_ADDRSTRLEN);
                    int addr_count = zts_core_query_addr_count(networkId);

                    for (int idx = 0; idx < addr_count; idx++) {
                        zts_core_query_addr(networkId, idx, addrBuffer, ZeroTier.Constants.INET6_ADDRSTRLEN);
                        // Convert buffer to managed string
                        string str = Marshal.PtrToStringAnsi(addrBuffer);
                        IPAddress addr = IPAddress.Parse(str);
                        newAddrsDict[addr.ToString()] = addr;
                    }

                    // Update addresses in NetworkInfo object

                    // TODO: This update block works but could use a re-think, I think.
                    // Step 1. Remove addresses not present in new concurrent dict.
                    if (! ni._addrs.IsEmpty) {
                        foreach (string key in ni._addrs.Keys) {
                            if (! newAddrsDict.Keys.Contains(key)) {
                                ni._addrs.TryRemove(key, out _);
                            }
                        }
                    }
                    else {
                        ni._addrs = newAddrsDict;
                    }
                    // Step 2. Add addresses not present in existing concurrent dict.
                    foreach (string key in newAddrsDict.Keys) {
                        if (! ni._addrs.Keys.Contains(key)) {
                            ni._addrs[key] = newAddrsDict[key];
                        }
                    }

                    Marshal.FreeHGlobal(addrBuffer);
                    addrBuffer = IntPtr.Zero;

                    // Get managed routes

                    ConcurrentDictionary<string, RouteInfo> newRoutesDict =
                        new ConcurrentDictionary<string, RouteInfo>();
                    IntPtr targetBuffer = Marshal.AllocHGlobal(ZeroTier.Constants.INET6_ADDRSTRLEN);
                    IntPtr viaBuffer = Marshal.AllocHGlobal(ZeroTier.Constants.INET6_ADDRSTRLEN);

                    int route_count = zts_core_query_route_count(networkId);

                    ushort flags = 0, metric = 0;

                    for (int idx = 0; idx < route_count; idx++) {
                        zts_core_query_route(
                            networkId,
                            idx,
                            targetBuffer,
                            viaBuffer,
                            ZeroTier.Constants.INET6_ADDRSTRLEN,
                            ref flags,
                            ref metric);

                        // Convert buffer to managed string

                        try {
                            string targetStr = Marshal.PtrToStringAnsi(targetBuffer);
                            IPAddress targetAddr = IPAddress.Parse(targetStr);
                            string viaStr = Marshal.PtrToStringAnsi(viaBuffer);
                            IPAddress viaAddr = IPAddress.Parse(viaStr);
                            RouteInfo route = new RouteInfo(targetAddr, viaAddr, flags, metric);
                            // Add to NetworkInfo object
                            newRoutesDict[targetStr] = route;
                        }
                        catch {
                            Console.WriteLine("error while parsing route");
                        }
                    }

                    // TODO: This update block works but could use a re-think, I think.
                    // Step 1. Remove routes not present in new concurrent dict.
                    if (! ni._routes.IsEmpty) {
                        foreach (string key in ni._routes.Keys) {
                            if (! newRoutesDict.Keys.Contains(key)) {
                                ni._routes.TryRemove(key, out _);
                            }
                        }
                    }
                    else {
                        ni._routes = newRoutesDict;
                    }
                    // Step 2. Add routes not present in existing concurrent dict.
                    foreach (string key in newRoutesDict.Keys) {
                        if (! ni._routes.Keys.Contains(key)) {
                            ni._routes[key] = newRoutesDict[key];
                        }
                    }

                    Marshal.FreeHGlobal(targetBuffer);
                    Marshal.FreeHGlobal(viaBuffer);
                    targetBuffer = IntPtr.Zero;
                    viaBuffer = IntPtr.Zero;

                    // Get multicast subscriptions

                    zts_core_lock_release();

                    // Update synthetic "readiness" value
                    ni.transportReady = (route_count > 0) && (addr_count > 0) ? true : false;
                }   // EVENT_NETWORK_DOWN

                if (msg.event_code == Constants.EVENT_NETWORK_NOT_FOUND) {
                    newEvent.Name = "EVENT_NETWORK_NOT_FOUND " + net_info.net_id.ToString("x16");
                }
                if (msg.event_code == Constants.EVENT_NETWORK_REQ_CONFIG) {
                    newEvent.Name = "EVENT_NETWORK_REQ_CONFIG " + net_info.net_id.ToString("x16");
                }
                if (msg.event_code == Constants.EVENT_NETWORK_ACCESS_DENIED) {
                    newEvent.Name = "EVENT_NETWORK_ACCESS_DENIED " + net_info.net_id.ToString("x16");
                }
                if (msg.event_code == Constants.EVENT_NETWORK_READY_IP4) {
                    newEvent.Name = "EVENT_NETWORK_READY_IP4 " + net_info.net_id.ToString("x16");
                }
                if (msg.event_code == Constants.EVENT_NETWORK_READY_IP6) {
                    newEvent.Name = "EVENT_NETWORK_READY_IP6 " + net_info.net_id.ToString("x16");
                }
                if (msg.event_code == Constants.EVENT_NETWORK_DOWN) {
                    newEvent.Name = "EVENT_NETWORK_DOWN " + net_info.net_id.ToString("x16");
                }
                if (msg.event_code == Constants.EVENT_NETWORK_CLIENT_TOO_OLD) {
                    newEvent.Name = "EVENT_NETWORK_CLIENT_TOO_OLD " + net_info.net_id.ToString("x16");
                }
                if (msg.event_code == Constants.EVENT_NETWORK_REQ_CONFIG) {
                    newEvent.Name = "EVENT_NETWORK_REQ_CONFIG " + net_info.net_id.ToString("x16");
                }
                if (msg.event_code == Constants.EVENT_NETWORK_OK) {
                    newEvent.Name = "EVENT_NETWORK_OK " + net_info.net_id.ToString("x16");
                }
                if (msg.event_code == Constants.EVENT_NETWORK_ACCESS_DENIED) {
                    newEvent.Name = "EVENT_NETWORK_ACCESS_DENIED " + net_info.net_id.ToString("x16");
                }
                if (msg.event_code == Constants.EVENT_NETWORK_READY_IP4_IP6) {
                    newEvent.Name = "EVENT_NETWORK_READY_IP4_IP6 " + net_info.net_id.ToString("x16");
                }
                if (msg.event_code == Constants.EVENT_NETWORK_UPDATE) {
                    newEvent.Name = "EVENT_NETWORK_UPDATE " + net_info.net_id.ToString("x16");
                }
            }

            // Route

            if (msg.route != IntPtr.Zero) {
                zts_route_info_t route_info =
                    (zts_route_info_t)Marshal.PtrToStructure(msg.route, typeof(zts_route_info_t));
                newEvent = new ZeroTier.Core.Event();
                newEvent.Code = msg.event_code;
                // newEvent.RouteInfo = default;   // new RouteInfo();

                if (msg.event_code == Constants.EVENT_ROUTE_ADDED) {
                    newEvent.Name = "EVENT_ROUTE_ADDED";
                }
                if (msg.event_code == Constants.EVENT_ROUTE_REMOVED) {
                    newEvent.Name = "EVENT_ROUTE_REMOVED";
                }
            }

            // Peer

            if (msg.peer != IntPtr.Zero) {
                zts_peer_info_t peer_info = (zts_peer_info_t)Marshal.PtrToStructure(msg.peer, typeof(zts_peer_info_t));
                newEvent = new ZeroTier.Core.Event();
                newEvent.Code = msg.event_code;
                // newEvent.PeerInfo = default;   // new PeerInfo();

                if (peer_info.role == Constants.PEER_ROLE_PLANET) {
                    newEvent.Name = "PEER_ROLE_PLANET";
                }
                if (msg.event_code == Constants.EVENT_PEER_DIRECT) {
                    newEvent.Name = "EVENT_PEER_DIRECT";
                }
                if (msg.event_code == Constants.EVENT_PEER_RELAY) {
                    newEvent.Name = "EVENT_PEER_RELAY";
                }
                // newEvent = new ZeroTier.Core.Event(msg.event_code,"EVENT_PEER_UNREACHABLE");
                if (msg.event_code == Constants.EVENT_PEER_PATH_DISCOVERED) {
                    newEvent.Name = "EVENT_PEER_PATH_DISCOVERED";
                }
                if (msg.event_code == Constants.EVENT_PEER_PATH_DEAD) {
                    newEvent.Name = "EVENT_PEER_PATH_DEAD";
                }
            }

            // Address

            if (msg.addr != IntPtr.Zero) {
                zts_addr_info_t unmanagedDetails =
                    (zts_addr_info_t)Marshal.PtrToStructure(msg.addr, typeof(zts_addr_info_t));
                newEvent = new ZeroTier.Core.Event();
                newEvent.Code = msg.event_code;
                // newEvent.AddressInfo = default;   // new AddressInfo();

                if (msg.event_code == Constants.EVENT_ADDR_ADDED_IP4) {
                    newEvent.Name = "EVENT_ADDR_ADDED_IP4";
                }
                if (msg.event_code == Constants.EVENT_ADDR_ADDED_IP6) {
                    newEvent.Name = "EVENT_ADDR_ADDED_IP6";
                }
                if (msg.event_code == Constants.EVENT_ADDR_REMOVED_IP4) {
                    newEvent.Name = "EVENT_ADDR_REMOVED_IP4";
                }
                if (msg.event_code == Constants.EVENT_ADDR_REMOVED_IP6) {
                    newEvent.Name = "EVENT_ADDR_REMOVED_IP6";
                }
            }

            // Storage

            if (msg.cache != IntPtr.Zero) {
                newEvent = new ZeroTier.Core.Event();
                newEvent.Code = msg.event_code;
                // newEvent.AddressInfo = default;   // new AddressInfo();

                if (msg.event_code == Constants.EVENT_STORE_IDENTITY_SECRET) {
                    newEvent.Name = "EVENT_STORE_IDENTITY_SECRET";
                }
                if (msg.event_code == Constants.EVENT_STORE_IDENTITY_PUBLIC) {
                    newEvent.Name = "EVENT_STORE_IDENTITY_PUBLIC";
                }
                if (msg.event_code == Constants.EVENT_STORE_PLANET) {
                    newEvent.Name = "EVENT_STORE_PLANET";
                }
                if (msg.event_code == Constants.EVENT_STORE_PEER) {
                    newEvent.Name = "EVENT_STORE_PEER";
                }
                if (msg.event_code == Constants.EVENT_STORE_NETWORK) {
                    newEvent.Name = "EVENT_STORE_NETWORK";
                }
            }

            // Pass the converted Event to the managed callback (visible to user)
            if (newEvent != null) {
                _managedCallback(newEvent);
            }
        }

        public List<NetworkInfo> Networks
        {
            get {
                return new List<NetworkInfo>(_networks.Values);
            }
        }

        public List<PeerInfo> Peers
        {
            get {
                return new List<PeerInfo>(_peers.Values);
            }
        }

        public bool IsNetworkTransportReady(ulong networkId)
        {
            try {
                return _networks[networkId].transportReady
                       && (_networks[networkId].Status == Constants.NETWORK_STATUS_OK);
            }
            catch (KeyNotFoundException) {
                return false;
            }
        }

        public List<IPAddress> GetNetworkAddresses(ulong networkId)
        {
            try {
                return new List<IPAddress>(_networks[networkId].Addresses);
            }
            catch (KeyNotFoundException) {
            }
            return new List<IPAddress>();
        }

        public List<RouteInfo> GetNetworkRoutes(ulong networkId)
        {
            try {
                return new List<RouteInfo>(_networks[networkId].Routes);
            }
            catch (KeyNotFoundException) {
            }
            return new List<RouteInfo>();
        }

        /*
                public NetworkInfo GetNetwork(ulong networkId)
                {
                    try {
                        return _networks[networkId];
                        } catch (KeyNotFoundException) {

                        }
                        return null;
                }
        */

        public int Start()
        {
            if (_hasBeenFreed == true) {
                throw new ObjectDisposedException(
                    "ZeroTier Node has previously been freed. Restart application to create new instance.");
            }
            return zts_node_start();
        }

        public int Free()
        {
            _id = 0x0;
            _hasBeenFreed = true;
            ClearNode();
            return zts_node_free();
        }

        public int Stop()
        {
            _id = 0x0;
            ClearNode();
            return zts_node_stop();
        }

        public int Join(ulong networkId)
        {
            /* The NetworkInfo object will only be added to _networks and populated when a
            response from the controller is received */
            return zts_net_join(networkId);
        }

        public int Leave(ulong networkId)
        {
            int res = Constants.ERR_OK;
            if (zts_net_leave(networkId) == Constants.ERR_OK) {
                _networks.TryRemove(networkId, out _);
            }
            return res;
        }

        public bool Online
        {
            get {
                return _isOnline;
            }
        }

        public ulong Id
        {
            get {
                return _id;
            }
        }

        public string IdString
        {
            get {
                return _id.ToString("x16").TrimStart('0');
            }
        }

        public string IdStr
        {
            get {
                return string.Format("{0}", _id.ToString("x16"));
            }
        }

        public ushort PrimaryPort
        {
            get {
                return _primaryPort;
            }
        }

        public ushort SecondaryPort
        {
            get {
                return _secondaryPort;
            }
        }

        public ushort TertiaryPort
        {
            get {
                return _tertiaryPort;
            }
        }

        public string Version
        {
            get {
                return string.Format("{0}.{1}.{2}", _versionMajor, _versionMinor, _versionRev);
            }
        }

        // id

        [DllImport("libzt", CharSet = CharSet.Ansi, EntryPoint = "CSharp_zts_id_new")] static extern int
        zts_id_new(string arg1, IntPtr arg2);

        [DllImport("libzt", CharSet = CharSet.Ansi, EntryPoint = "CSharp_zts_id_pair_is_valid")]
        static extern int zts_id_pair_is_valid(string arg1, int arg2);

        // init

        [DllImport("libzt", EntryPoint = "CSharp_zts_init_allow_net_cache")]
        static extern int zts_init_allow_net_cache(int arg1);

        [DllImport("libzt", EntryPoint = "CSharp_zts_init_allow_peer_cache")]
        static extern int zts_init_allow_peer_cache(int arg1);

        [DllImport("libzt", CharSet = CharSet.Ansi, EntryPoint = "CSharp_zts_init_from_storage")]
        static extern int zts_init_from_storage(string arg1);

        [DllImport("libzt", CharSet = CharSet.Ansi, EntryPoint = "CSharp_zts_init_from_memory")]
        static extern int zts_init_from_memory(string arg1, ushort arg2);

        [DllImport("libzt", EntryPoint = "CSharp_zts_init_set_event_handler")]
        static extern int zts_init_set_event_handler(CSharpCallbackWithStruct arg1);

        [DllImport("libzt", CharSet = CharSet.Ansi, EntryPoint = "CSharp_zts_init_blacklist_if")]
        static extern int zts_init_blacklist_if(string arg1, int arg2);

        [DllImport("libzt", EntryPoint = "CSharp_zts_init_set_roots")]
        static extern int zts_init_set_roots(IntPtr roots_data, int len);

        [DllImport("libzt", EntryPoint = "CSharp_zts_init_set_port")]
        static extern int zts_init_set_port(ushort arg1);

        [DllImport("libzt", EntryPoint = "CSharp_zts_init_set_random_port_range")]
        static extern int zts_init_set_random_port_range(ushort arg1, ushort arg2);

        [DllImport("libzt", EntryPoint = "CSharp_zts_init_allow_secondary_port")]
        static extern int zts_init_allow_secondary_port(int arg1);

        [DllImport("libzt", EntryPoint = "CSharp_zts_init_allow_port_mapping")]
        static extern int zts_init_allow_port_mapping(int arg1);

        // Core query API

        [DllImport("libzt", EntryPoint = "CSharp_zts_core_lock_obtain")]
        static extern int zts_core_lock_obtain();

        [DllImport("libzt", EntryPoint = "CSharp_zts_core_lock_release")]
        static extern int zts_core_lock_release();

        [DllImport("libzt", EntryPoint = "CSharp_zts_core_query_addr_count")]
        static extern int zts_core_query_addr_count(ulong net_id);

        [DllImport("libzt", EntryPoint = "CSharp_zts_core_query_addr")]
        static extern int zts_core_query_addr(ulong net_id, int idx, IntPtr dst, int len);

        [DllImport("libzt", EntryPoint = "CSharp_zts_core_query_route_count")]
        static extern int zts_core_query_route_count(ulong net_id);

        [DllImport("libzt", EntryPoint = "CSharp_zts_core_query_route")]
        static extern int zts_core_query_route(
            ulong net_id,
            int idx,
            IntPtr target,
            IntPtr via,
            int len,
            ref ushort flags,
            ref ushort metric);

        [DllImport("libzt", EntryPoint = "CSharp_zts_core_query_path_count")]
        static extern int zts_core_query_path_count(ulong peer_id);

        [DllImport("libzt", EntryPoint = "CSharp_zts_core_query_path")]
        static extern int zts_core_query_path(ulong peer_id, int idx, IntPtr dst, int len);

        [DllImport("libzt", EntryPoint = "CSharp_zts_core_query_mc_count")]
        static extern int zts_core_query_mc_count(ulong net_id);

        [DllImport("libzt", EntryPoint = "CSharp_zts_core_query_mc")]
        static extern int zts_core_query_mc(ulong net_id, int idx, ref ulong mac, ref uint adi);

        // addr

        [DllImport("libzt", EntryPoint = "CSharp_zts_addr_is_assigned")]
        static extern int zts_addr_is_assigned(ulong arg1, int arg2);

        [DllImport("libzt", EntryPoint = "CSharp_zts_addr_get")]
        static extern int zts_addr_get(ulong arg1, int arg2, IntPtr arg3);

        [DllImport("libzt", CharSet = CharSet.Ansi, EntryPoint = "CSharp_zts_addr_get_str")]
        static extern int zts_addr_get_str(ulong arg1, int arg2, IntPtr arg3, int arg4);

        [DllImport("libzt", EntryPoint = "CSharp_zts_addr_get_all")]
        static extern int zts_addr_get_all(ulong arg1, IntPtr arg2, IntPtr arg3);

        [DllImport("libzt", EntryPoint = "CSharp_zts_addr_compute_6plane")]
        static extern int zts_addr_compute_6plane(ulong arg1, ulong arg2, IntPtr arg3);

        [DllImport("libzt", EntryPoint = "CSharp_zts_addr_compute_rfc4193")]
        static extern int zts_addr_compute_rfc4193(ulong arg1, ulong arg2, IntPtr arg3);

        [DllImport("libzt", CharSet = CharSet.Ansi, EntryPoint = "CSharp_zts_addr_compute_rfc4193_str")]
        static extern int zts_addr_compute_rfc4193_str(ulong arg1, ulong arg2, string arg3, int arg4);

        [DllImport("libzt", CharSet = CharSet.Ansi, EntryPoint = "CSharp_zts_addr_compute_6plane_str")]
        static extern int zts_addr_compute_6plane_str(ulong arg1, ulong arg2, string arg3, int arg4);

        /*
                [DllImport("libzt", EntryPoint="CSharp_zts_get_6plane_addr")]
                static extern int zts_get_6plane_addr(IntPtr arg1, ulong arg2, ulong arg3);

                [DllImport("libzt", EntryPoint="CSharp_zts_get_rfc4193_addr")]
                static extern int zts_get_rfc4193_addr(IntPtr arg1, ulong arg2, ulong arg3);
        */

        // net

        [DllImport("libzt", EntryPoint = "CSharp_zts_net_compute_adhoc_id")]
        public static extern ulong zts_net_compute_adhoc_id(ushort arg1, ushort arg2);

        [DllImport("libzt", EntryPoint = "CSharp_zts_net_join")]
        static extern int zts_net_join(ulong arg1);

        [DllImport("libzt", EntryPoint = "CSharp_zts_net_leave")]
        static extern int zts_net_leave(ulong arg1);

        [DllImport("libzt", EntryPoint = "CSharp_zts_net_transport_is_ready")]
        static extern int zts_net_transport_is_ready(ulong arg1);

        [DllImport("libzt", EntryPoint = "CSharp_zts_net_get_mac")]
        public static extern ulong zts_net_get_mac(ulong arg1);

        [DllImport("libzt", CharSet = CharSet.Ansi, EntryPoint = "CSharp_zts_net_get_mac_str")]
        static extern int zts_net_get_mac_str(ulong arg1, string arg2, int arg3);

        [DllImport("libzt", EntryPoint = "CSharp_zts_net_get_broadcast")]
        static extern int zts_net_get_broadcast(ulong arg1);

        [DllImport("libzt", EntryPoint = "CSharp_zts_net_get_mtu")]
        static extern int zts_net_get_mtu(ulong arg1);

        [DllImport("libzt", CharSet = CharSet.Ansi, EntryPoint = "CSharp_zts_net_get_name")]
        static extern int zts_net_get_name(ulong arg1, string arg2, int arg3);

        [DllImport("libzt", EntryPoint = "CSharp_zts_net_get_status")]
        static extern int zts_net_get_status(ulong arg1);

        [DllImport("libzt", EntryPoint = "CSharp_zts_net_get_type")]
        static extern int zts_net_get_type(ulong arg1);

        // route

        [DllImport("libzt", EntryPoint = "CSharp_zts_route_is_assigned")]
        static extern int zts_route_is_assigned(ulong arg1, int arg2);

        // node

        [DllImport("libzt", EntryPoint = "CSharp_zts_node_start")]
        static extern int zts_node_start();

        [DllImport("libzt", EntryPoint = "CSharp_zts_node_is_online")]
        static extern int zts_node_is_online();

        [DllImport("libzt", EntryPoint = "CSharp_zts_node_get_id")]
        public static extern ulong zts_node_get_id();

        [DllImport("libzt", CharSet = CharSet.Ansi, EntryPoint = "CSharp_zts_node_get_id_pair")]
        static extern int zts_node_get_id_pair(string arg1, IntPtr arg2);

        [DllImport("libzt", EntryPoint = "CSharp_zts_node_get_port")]
        static extern int zts_node_get_port();

        [DllImport("libzt", EntryPoint = "CSharp_zts_node_stop")]
        static extern int zts_node_stop();

        [DllImport("libzt", EntryPoint = "CSharp_zts_node_free")]
        static extern int zts_node_free();

        // moon

        [DllImport("libzt", EntryPoint = "CSharp_zts_moon_orbit")]
        static extern int zts_moon_orbit(ulong arg1, ulong arg2);

        [DllImport("libzt", EntryPoint = "CSharp_zts_moon_deorbit")]
        static extern int zts_moon_deorbit(ulong arg1);

        // util

        [DllImport("libzt", EntryPoint = "CSharp_zts_util_delay")]
        static extern void zts_util_delay(int arg1);

        [DllImport("libzt", EntryPoint = "CSharp_zts_errno_get")]
        static extern int zts_errno_get();

        [StructLayout(LayoutKind.Sequential)]
        struct zts_node_info_t {
            public ulong node_id;
            public ushort primary_port;
            public ushort secondary_port;
            public ushort tertiary_port;
            public byte ver_major;
            public byte ver_minor;
            public byte ver_rev;
        }

        [StructLayout(LayoutKind.Sequential)]
        struct zts_net_info_t {
            public ulong net_id;
            public ulong mac;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 128)]
            public byte[] name;
            public int status;
            public int type;
            public uint mtu;
            public int dhcp;
            public int bridge;
            public int broadcast_enabled;
            public int port_error;
            public ulong netconf_rev;
            // address, routes, and multicast subs are retrieved using zts_core_ API
        }

        [StructLayout(LayoutKind.Sequential)]
        struct zts_route_info_t {
            IntPtr target;
            IntPtr via;
            ushort flags;
            ushort metric;
        }

        [StructLayout(LayoutKind.Sequential)]
        struct zts_addr_info_t {
            ulong nwid;
            IntPtr addr;
        }

        [StructLayout(LayoutKind.Sequential)]
        struct zts_peer_info_t {
            public ulong address;
            public int ver_major;
            public int ver_minor;
            public int ver_rev;
            public int latency;
            public byte role;
            // TODO
        }

        [StructLayout(LayoutKind.Sequential)]
        struct zts_event_msg_t {
            public short event_code;
            public IntPtr node;
            public IntPtr network;
            public IntPtr netif;
            public IntPtr route;
            public IntPtr peer;
            public IntPtr addr;
            public IntPtr cache;
            public int len;
        }

        public static int ErrNo
        {
            get {
                return zts_errno_get();
            }
        }
    }
}
