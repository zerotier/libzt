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

using System.Net;
using System.Collections.Concurrent;
using System.Collections.Generic;

using ZeroTier;

namespace ZeroTier.Core
{
    public class NetworkInfo {
        public ulong Id { get; set; }
        public ulong MACAddress;
        public string Name;
        public int Status;
        public int Type;
        public uint MTU;
        public int DHCP;
        public bool Bridge;
        public bool BroadcastEnabled;
        internal bool transportReady;   // Synthetic value

        public bool IsPrivate
        {
            get {
                return Type == Constants.NETWORK_TYPE_PRIVATE;
            }
        }

        public bool IsPublic
        {
            get {
                return Type == Constants.NETWORK_TYPE_PUBLIC;
            }
        }

        internal ConcurrentDictionary<string, IPAddress> _addrs = new ConcurrentDictionary<string, IPAddress>();

        public ICollection<IPAddress> Addresses
        {
            get {
                return _addrs.Values;
            }
        }

        internal ConcurrentDictionary<string, ZeroTier.Core.RouteInfo> _routes =
            new ConcurrentDictionary<string, ZeroTier.Core.RouteInfo>();

        public ICollection<ZeroTier.Core.RouteInfo> Routes
        {
            get {
                return _routes.Values;
            }
        }
    }
}
