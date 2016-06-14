package com.zerotier.one.util;

import android.util.Log;

import java.math.BigInteger;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;

/**
 * Created by Grant on 6/13/2015.
 */
public class InetAddressUtils {
    public static final String TAG = "InetAddressUtils";

    public static byte[] addressToNetmask(InetAddress addr, int prefix) {
        int numAddressBytes = addr.getAddress().length;
        if(numAddressBytes > 4) {
            throw new UnsupportedOperationException("IPv6 is not yet supported");
        }
        int numAddressBits = numAddressBytes * 8;

        byte[] maskBytes = new byte[numAddressBytes];
        for(int i = 0; i < numAddressBytes; ++i) {
            maskBytes[i] = (byte)255;
        }
        int mask = ByteBuffer.wrap(maskBytes).getInt();

        mask = mask >> (numAddressBits - prefix);
        mask = mask << (numAddressBits - prefix);

        return ByteBuffer.allocate(4).putInt(mask).array();
    }

    public static InetAddress addressToRoute(InetAddress addr, int prefix) {
        byte[] maskBytes = addressToNetmask(addr, prefix);
        byte[] routeBytes = new byte[maskBytes.length];
        InetAddress route = null;
        try {
            for(int i = 0; i < maskBytes.length; ++i) {
                routeBytes[i] = (byte)(addr.getAddress()[i] & maskBytes[i]);
            }
            route = InetAddress.getByAddress(routeBytes);

        } catch (UnknownHostException e) {
            Log.e(TAG, "Uknown Host Exception calculating route");
        }

        return route;
    }

}
