package SDK;
public class SDK {
    public native void startOneService();
    static { System.loadLibrary("ZeroTierOneJNI"); } // Loads JNI code
}