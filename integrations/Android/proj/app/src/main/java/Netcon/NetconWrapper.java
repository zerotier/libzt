package Netcon;

/**
 * Created by Joseph on 3/14/16.
 */
public class NetconWrapper {
    public native int loadsymbols();

    // From OneServiceSetup.cpp
    public native void startOneService();

    static {
        System.loadLibrary("ZeroTierOneJNI");
    }
}
