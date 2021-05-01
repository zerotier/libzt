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
    /* Convenience structures for exposing lower level operational details to the user.
    These structures do not have the same memory layout or content as those found in
    include/ZeroTierSockets.h */

    /// <summary>
    /// Structure containing information about the local Node.
    /// </summary>
    public class NodeInfo {
        // Node ID
        public ulong Id { get; set; }

        /**
         * The port used by the service to send and receive
         * all encapsulated traffic
         */
        public ushort PrimaryPort { get; set; }
        public ushort SecondaryPort { get; set; }
        public ushort TertiaryPort { get; set; }

        /**
         * ZT version
         */
        public byte VersionMajor { get; set; }
        public byte VersionMinor { get; set; }
        public byte VersionRev { get; set; }
    }
}
