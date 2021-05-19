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
 * Class that must be implemented to receive ZeroTier event notifications.
 */
public interface ZeroTierEventListener {
    /*
     * Called when an even occurs within ZeroTier
     */
    public void onZeroTierEvent(long id, int eventCode);
}
