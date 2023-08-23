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

import com.zerotier.sockets.ZeroTierNative;
import java.net.InetAddress;

/**
 * Convenience class for holding address information. Used internally by JNI layer.
 * You (as a consumer of this library) should probably not use this as it is non-standard
 * and very likely to be removed at some point.
 */
class ZeroTierSocketAddress {
    private byte[] _ip6 = new byte[16];
    private byte[] _ip4 = new byte[4];

    private int _family;
    private int _port;   // Also reused for netmask or prefix

    public ZeroTierSocketAddress()
    {
    }

    /**
     * Constructor
     */
    public ZeroTierSocketAddress(String ipStr, int port)
    {
        if (ipStr.contains(":")) {
            _family = ZeroTierNative.ZTS_AF_INET6;
            try {
                InetAddress ip = InetAddress.getByName(ipStr);
                _ip6 = ip.getAddress();
            }
            catch (Exception e) {
            }
        }
        else if (ipStr.contains(".")) {
            _family = ZeroTierNative.ZTS_AF_INET;
            try {
                InetAddress ip = InetAddress.getByName(ipStr);
                _ip4 = ip.getAddress();
            }
            catch (Exception e) {
            }
        }
        _port = port;
    }

    /**
     * Convert to string (ip portion only)
     */
    public String ipString()
    {
        if (_family == ZeroTierNative.ZTS_AF_INET) {
            try {
                InetAddress inet = InetAddress.getByAddress(_ip4);
                return "" + inet.getHostAddress();
            }
            catch (Exception e) {
                System.out.println(e);
            }
        }
        if (_family == ZeroTierNative.ZTS_AF_INET6) {
            try {
                InetAddress inet = InetAddress.getByAddress(_ip6);
                return "" + inet.getHostAddress();
            }
            catch (Exception e) {
                System.out.println(e);
            }
        }
        return "";
    }

    /**
     * Convert to string (ip and port)
     */
    public String toString()
    {
        return ipString() + ":" + _port;
    }

    /**
     * Convert to string (ip+netmask and port)
     */
    public String toCIDR()
    {
        return ipString() + "/" + _port;
    }

    /**
     * Get port
     */
    public int getPort()
    {
        return _port;
    }

    /**
     * Get netmask
     */
    public int getNetmask()
    {
        return _port;
    }

    /**
     * Get prefix (stored in port)
     */
    public int getPrefix()
    {
        return _port;
    }
}
