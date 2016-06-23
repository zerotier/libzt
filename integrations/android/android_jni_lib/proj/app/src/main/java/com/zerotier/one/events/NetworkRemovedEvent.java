package com.zerotier.one.events;

/**
 * Created by Grant on 6/23/2015.
 */
public class NetworkRemovedEvent {
    private long networkId;

    public NetworkRemovedEvent(long nwid) {
        networkId = nwid;
    }

    public long getNetworkId() {
        return networkId;
    }
}
