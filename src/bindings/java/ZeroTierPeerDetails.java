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

package com.zerotier.sockets;

import com.zerotier.sockets.*;

/**
 * This class encapsulates details about a Peer on a ZeroTier network
 */
class ZeroTierPeerDetails {
    /**
     * ZeroTier address (40 bits)
     */
    public long address;

    /**
     * Remote major version or -1 if not known
     */
    public int versionMajor;

    /**
     * Remote minor version or -1 if not known
     */
    public int versionMinor;

    /**
     * Remote revision or -1 if not known
     */
    public int versionRev;

    /**
     * Last measured latency in milliseconds or -1 if unknown
     */
    public int latency;

    /**
     * What trust hierarchy role does this device have?
     */
    public int role;

    /**
     * Number of paths (size of paths[])
     */
    public int pathCount;

    /**
     * Known network paths to peer
     */
    public ZeroTierSocketAddress[] paths = new ZeroTierSocketAddress[ZeroTierNative.ZTS_MAX_PEER_NETWORK_PATHS];
}
