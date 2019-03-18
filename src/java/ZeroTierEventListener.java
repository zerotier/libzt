package com.zerotier.libzt;

public interface ZeroTierEventListener {

    /*
     * Called when an even occurs in the native section of the ZeroTier library service
     */
    public void onZeroTierEvent(long nwid, int eventCode);
}