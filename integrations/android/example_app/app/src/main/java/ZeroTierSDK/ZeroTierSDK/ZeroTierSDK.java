package ZeroTierSDK.ZeroTierSDK;
public class ZeroTierSDK {
    public native void startOneService();
    static { System.loadLibrary("ZeroTierOneJNI"); } // Loads JNI code
}