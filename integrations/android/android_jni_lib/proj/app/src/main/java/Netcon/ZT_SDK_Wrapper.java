package Netcon;

/**
 * Created by Joseph Henry on 3/14/16.
 */
public class ZT_SDK_Wrapper {
    public native int loadsymbols();

    // From SDK_ServiceSetup.cpp
    public native void startOneService();

    static {
        System.loadLibrary("ZeroTierOneJNI");
    }
}
