Android + ZeroTier SDK 
====

Welcome!

Imagine a flat, encrypted, no-configuration LAN for all of the instances of your Android app. 

This short tutorial will show you how to enable ZeroTier functionality for your Android app with little to no code modification. Check out our [ZeroTier SDK](https://www.zerotier.com/blog) page for more info on how the integration works and [Shim Techniques](https://www.zerotier.com/blog) for a discussion of shims available for your app/technology.

In this example we aim to set up a minimal [Android Studio](https://developer.android.com/studio/index.html) project which contains all of the components necessary to enable ZeroTier for your app. If you'd rather skip all of these steps and grab the code, look in the [sdk/android](https://github.com/zerotier/ZeroTierOne/tree/dev/netcon/Android) folder in the source tree. Otherwise, let's get started!

**Step 1: Build Shared Library `libZeroTierOneJNI.so`**

Open `zerotiersdk/integrations/Android/proj` and build it. 

*Note: Building the project will take a while if you are building for all architectures, See note below on how to speed up this process.*

The resultant `zerotiersdk/integrations/Android/java/libs/YOUR_ARCH/libZeroTierOneJNI.so` will be what you want to import for your own project to provide the shim interface to your app. Select your architecture and copy the shared library into `YourProject/src/main/jniLibs/YOUR_ARCH/`

**Step 2: App Code Modifications**

Create new package called `Netcon` in your project and add a new file called `NetconWrapper.java` containing:

```
package Netcon;
public class NetconWrapper {
    public native void startOneService();
    static { System.loadLibrary("ZeroTierOneJNI”); } // Loads JNI code
}
```

And now, start the service:
```
new Thread(new Runnable() {
      public void run() {
        NetconWrapper wrapper = new NetconWrapper();
        wrapper.startOneService(); // Calls to JNI code
      }
}).start();
```
**Step 3: Pick a shim for your app**

If functional interposition isn't available for the API or library you've chosen to use, ZeroTier offers a SOCKS5 proxy server which can allow connectivity to your virtual network as long as your client API supports the SOCKS5 protocol. This proxy service will run alongside the tap service and can be turned on by compiling with the `-DUSE_SOCKS_PROXY` flag. By default, the proxy service is available at `0.0.0.0:1337`.

**Step 4: Add necessary app permissions**

In order for your application to write the auth keys to the internal storage you'll need to set a few permissions in your `AndroidManifest.xml` file:

```
<uses-permission android:name="android.permission.READ_EXTERNAL_STORAGE" />
<uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE" />
```

**Step 5: Join a network!**

Simply call `zt_join_network("XXXXXXXXXXXXXXXX")`

***
**Additional notes**

As mentioned above, you can reduce the amount of time required to build the ZeroTier JNI library by only building for the architectures you want. You can specify the architectures in `zerotiersdk/integrations/Android/java/jni/Application.mk`

If you change the method/class/package name for the Netcon glue code in `NetconWrapper.java` (Not recommended!), you must also change the name of the JNI implementation in the Netcon source to match the new java name. For example, if the glue code is contained in a package `Java.com.example.joseph.NetconProxyTest`, a JNI implementation name of `Java_com_example_joseph_netconproxytest_NetconWrapper_startOneService` would be required in the appropriate C/C++ source/header files.