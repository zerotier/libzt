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

/**
 * This class provides an fdset-like behavior to ZeroTierSocket
 */
class ZeroTierFileDescriptorSet {
    byte[] fds_bits = new byte[1024];

    /**
     * Clear bit
     */
    public void CLR(int fd)
    {
        fds_bits[fd] = 0x00;
    }

    /**
     * Check if bit is set
     */
    public boolean ISSET(int fd)
    {
        return fds_bits[fd] == 0x01;
    }

    /**
     * Set bit
     */
    public void SET(int fd)
    {
        fds_bits[fd] = 0x01;
    }

    /**
     * Zero-out all bits
     */
    public void ZERO()
    {
        for (int i = 0; i < 1024; i++) {
            fds_bits[i] = 0x00;
        }
    }
}
