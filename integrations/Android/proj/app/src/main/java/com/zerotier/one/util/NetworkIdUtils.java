package com.zerotier.one.util;

import java.math.BigInteger;

/**
 * Created by Grant on 5/26/2015.
 */
public class NetworkIdUtils {
    /**
     * Long will not parse a 64-bit hex string when the highest bit is 1 for some reasoon.  So
     * for converting network IDs in hex to a long, we must use big integer.
     *
     * @param hexString
     * @return
     */
    public static long hexStringToLong(String hexString) {
        BigInteger value = new BigInteger(hexString, 16);
        return value.longValue();
    }
}
