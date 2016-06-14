package com.zerotier.one.events;

/**
 * Created by Grant on 6/23/2015.
 */
public class LeaveNetworkEvent {
    long networkId;

    public LeaveNetworkEvent(long nwid) {
        networkId = nwid;
    }

    public long getNetworkId() {
        return networkId;
    }
}
