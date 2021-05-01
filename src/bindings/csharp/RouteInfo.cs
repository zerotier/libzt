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

using ZeroTier;

namespace ZeroTier.Core
{
    public class RouteInfo {
        public RouteInfo(IPAddress target, IPAddress via, ushort flags, ushort metric)
        {
            _target = target;
            _via = via;
            _flags = flags;
            _metric = metric;
        }

        public IPAddress _target;
        public IPAddress _via;
        public ushort _flags;
        public ushort _metric;

        public IPAddress Target
        {
            get {
                return _target;
            }
        }
        public IPAddress Via
        {
            get {
                return _via;
            }
        }
        public ushort Flags
        {
            get {
                return _flags;
            }
        }
        public ushort Metric
        {
            get {
                return _metric;
            }
        }
    }
}
