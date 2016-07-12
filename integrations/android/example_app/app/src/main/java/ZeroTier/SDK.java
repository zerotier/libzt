package ZeroTier;
public class SDK {
    public native void startOneService(String homeDir);
    public native void joinNetwork(String nwid);
    public native void leaveNetwork(String nwid);
    public native boolean isRunning();

    static { System.loadLibrary("ZeroTierOneJNI"); } // Loads JNI code
}