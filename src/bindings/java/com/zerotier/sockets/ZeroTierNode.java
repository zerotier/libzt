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
import java.net.InetAddress;
import java.net.UnknownHostException;

/**
 * Class that provides a control interface for nodes and networks by
 * wrapping the ZeroTierNative class.
 */
public class ZeroTierNode {
    /**
     * Start the ZeroTier node
     *
     * @return Standard
     */
    public int start()
    {
        return ZeroTierNative.zts_node_start();
    }

    /**
     * Stop the ZeroTier node
     *
     * @return return
     */
    public int stop()
    {
        return ZeroTierNative.zts_node_stop();
    }

    /**
     * (Optional) Instruct ZeroTier to read and write identities and cache from a
     * storage location. Note that this is an initialization method that should
     * be called before {@code start()}.
     *
     * @param storagePath Where to read and write files
     *
     * @return return
     */
    public int initFromStorage(String storagePath)
    {
        return ZeroTierNative.zts_init_from_storage(storagePath);
    }

    /**
     * (Optional) Whether caching of peer address information to storage is allowed. This
     * is true by default if {@code initFromStorage()} is used. Note that this is an
     * initialization method that can only be called before {@code start()}.
     *
     * @param allowed Whether caching or storage of this item is allowed
     *
     * @return return
     */
    public int initAllowPeerCache(boolean allowed)
    {
        return ZeroTierNative.zts_init_allow_peer_cache(allowed ? 1 : 0);
    }

    /**
     * (Optional) Whether caching of network configuration data to storage is allowed. This
     * is true by default if {@code initFromStorage()} is used. Note that this is an
     * initialization method that can only be called before {@code start()}.
     *
     * @param allowed Whether caching or storage of this item is allowed
     *
     * @return return
     */
    public int initAllowNetworkCache(boolean allowed)
    {
        return ZeroTierNative.zts_init_allow_net_cache(allowed ? 1 : 0);
    }

    /**
     * (Optional) Whether caching of identity key pairs to storage is allowed. This
     * is true by default if {@code initFromStorage()} is used. Note that this is an
     * initialization method that can only be called before {@code start()}.
     *
     * @param allowed Whether caching or storage of this item is allowed
     *
     * @return return
     */
    public int initAllowIdCache(boolean allowed)
    {
        return ZeroTierNative.zts_init_allow_id_cache(allowed ? 1 : 0);
    }

    /**
     * (Optional) Whether caching of root topology to storage is allowed. This
     * is true by default if {@code initFromStorage()} is used. Note that this is an
     * initialization method that can only be called before {@code start()}.
     *
     * @param allowed Whether caching or storage of this item is allowed
     *
     * @return return
     */
    public int initAllowRootsCache(boolean allowed)
    {
        return ZeroTierNative.zts_init_allow_roots_cache(allowed ? 1 : 0);
    }

    /**
     * (Optional) Set the ZeroTier service port. Note that this is an
     * initialization method that can only be called before {@code start()}.
     *
     * @param port Port number
     *
     * @return return
     */
    public int initSetPort(short port)
    {
        return ZeroTierNative.zts_init_set_port(port);
    }

    /**
     * (Optional) Set the event handler function. Note that this is an
     * initialization method that can only be called before {@code start()}.
     *
     * @param handler Function to handle internal ZeroTier events
     *
     * @return return
     */
    public int initSetEventHandler(ZeroTierEventListener handler)
    {
        return ZeroTierNative.zts_init_set_event_handler(handler);
    }

    /**
     * Return whether the ZeroTier node is currently online (able to reach a root)
     *
     * @return return
     */
    public boolean isOnline()
    {
        return ZeroTierNative.zts_node_is_online() == 1;
    }

    /**
     * Join a network
     *
     * @param networkId Network to join
     *
     * @return return
     */
    public int join(long networkId)
    {
        return ZeroTierNative.zts_net_join(networkId);
    }

    /**
     * Leave a network
     *
     * @param networkId Network to leave
     *
     * @return return
     */
    public int leave(long networkId)
    {
        return ZeroTierNative.zts_net_leave(networkId);
    }

    /**
     * Return whether the given network is ready to process traffic
     *
     * @param networkId Network to test
     *
     * @return return
     */
    public boolean isNetworkTransportReady(long networkId)
    {
        return ZeroTierNative.zts_net_transport_is_ready(networkId) == 1;
    }

    /**
     * Get the identity of this node (public key)
     *
     * @return 64-bit integer representing the 10-digit hexadecimal node ID
     */
    public long getId()
    {
        return ZeroTierNative.zts_node_get_id();
    }

    /**
     * Get the first-assigned IPv4 address
     *
     * @param networkId Network to get assigned address for
     *
     * @return  address
     */
    public InetAddress getIPv4Address(long networkId)
    {
        try {
            return InetAddress.getByName(ZeroTierNative.zts_addr_get_str(networkId, ZeroTierNative.ZTS_AF_INET));
        }
        catch (Exception e) {
            return null;
        }
    }

    /**
     * Get the first-assigned IPv6 address
     *
     * @param networkId Network to get assigned address for
     *
     * @return  address
     */
    public InetAddress getIPv6Address(long networkId)
    {
        try {
            return InetAddress.getByName(ZeroTierNative.zts_addr_get_str(networkId, ZeroTierNative.ZTS_AF_INET6));
        }
        catch (Exception e) {
            return null;
        }
    }

    /**
     * Get the first-assigned IPv6 address
     *
     * @param networkId Network to get assigned address for
     *
     * @return  address
     */
    public String getMACAddress(long networkId)
    {
        return ZeroTierNative.zts_net_get_mac_str(networkId);
    }
}
