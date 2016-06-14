package com.zerotier.one.events;

import com.zerotier.sdk.VirtualNetworkConfig;

/**
 * Created by Grant on 6/23/2015.
 */
public class NetworkListReplyEvent {
    private VirtualNetworkConfig[] networks;

    public NetworkListReplyEvent(VirtualNetworkConfig[] networks) {
        this.networks = networks;
    }

    public VirtualNetworkConfig[] getNetworkList() {
        return networks;
    }
}
