package ZeroTier;
public class SDK {
    public native void startOneService();
    static { System.loadLibrary("ZeroTierOneJNI"); } // Loads JNI code
}