package com.zerotier.one.util;

import android.util.Log;

import java.net.InetAddress;
import java.net.UnknownHostException;

/**
 * Created by Grant on 6/19/2015.
 */
public class IPPacketUtils {
    private static final String TAG = "IPPacketUtils";

    public static InetAddress getSourceIP(byte[] data) {
        byte[] addrBuffer = new byte[4];
        System.arraycopy(data, 12, addrBuffer, 0, 4);
        try {
            return InetAddress.getByAddress(addrBuffer);
        } catch (UnknownHostException e) {
            Log.e(TAG, "Error creating InetAddress: " + e.getMessage());
        }
        return null;
    }

    public static InetAddress getDestIP(byte[] data) {
        byte[] addrBuffer = new byte[4];
        System.arraycopy(data, 16, addrBuffer, 0, 4);
        try {
            return InetAddress.getByAddress(addrBuffer);
        } catch (UnknownHostException e) {
            Log.e(TAG, "Error creating InetAddress: " + e.getMessage());
        }
        return null;
    }

    /**
     * Calculates the 1's complement checksum of a region of bytes
     *
     * @param buffer
     * @param startValue
     * @param startByte
     * @param endByte
     * @return
     */
    public static long calculateChecksum(byte[] buffer, long startValue, int startByte, int endByte) {

        int length = endByte - startByte;
        int i = startByte;
        long sum = startValue;
        long data;

        // Handle all pairs
        while (length > 1) {
            data = (((buffer[i] << 8) & 0xFF00) | ((buffer[i + 1]) & 0xFF));
            sum += data;
            // 1's complement carry bit correction in 16-bits (detecting sign extension)
            if ((sum & 0xFFFF0000) > 0) {
                sum = sum & 0xFFFF;
                sum += 1;
            }

            i += 2;
            length -= 2;
        }

        // Handle remaining byte in odd length buffers
        if (length > 0) {
            sum += (buffer[i] << 8 & 0xFF00);
            // 1's complement carry bit correction in 16-bits (detecting sign extension)
            if ((sum & 0xFFFF0000) > 0) {
                sum = sum & 0xFFFF;
                sum += 1;
            }
        }

        // Final 1's complement value correction to 16-bits
        sum = ~sum;
        sum = sum & 0xFFFF;

        return sum;
    }
}
