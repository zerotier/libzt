package com.zerotier.one.events;

/**
 * Created by Grant on 6/23/2015.
 */
public class RequestNetworkInfoEvent {
    private long networkId;

    public RequestNetworkInfoEvent(long nwid) {
        networkId = nwid;
    }

    public long getNetworkId() {
        return networkId;
    }
}
