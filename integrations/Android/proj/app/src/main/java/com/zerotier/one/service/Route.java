package com.zerotier.one.service;

import com.zerotier.one.util.InetAddressUtils;

import java.math.BigInteger;
import java.net.InetAddress;
import java.nio.ByteBuffer;

/**
 * Created by Grant on 6/13/2015.
 */
public class Route {
    InetAddress address;
    int prefix;

    public Route(InetAddress address, int prefix) {
        this.address = address;
        this.prefix = prefix;
    }

    public boolean belongsToRoute(InetAddress otherAddr) {
        InetAddress route = InetAddressUtils.addressToRoute(otherAddr, prefix);

        return address.equals(route);
    }

    public boolean equals(Route r) {
        return address.equals(r.address) && prefix == r.prefix;
    }
}
