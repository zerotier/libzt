package com.zerotier.one.events;

import com.zerotier.one.ui.JoinNetworkFragment;

/**
 * Created by Grant on 6/23/2015.
 */
public class JoinNetworkEvent {
    private long networkId;

    public JoinNetworkEvent(long nwid) {
        networkId = nwid;
    }

    public long getNetworkId() {
        return networkId;
    }
}
