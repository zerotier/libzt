package com.zerotier.one.events;

import android.net.NetworkInfo;

import com.zerotier.sdk.VirtualNetworkConfig;

/**
 * Created by Grant on 6/23/2015.
 */
public class NetworkInfoReplyEvent {
    private VirtualNetworkConfig vnc;

    public NetworkInfoReplyEvent(VirtualNetworkConfig vnc) {
        this.vnc = vnc;
    }

    public VirtualNetworkConfig getNetworkInfo() {
        return vnc;
    }
}
