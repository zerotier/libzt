Android + ZeroTier SDK
====

Welcome!

Imagine a flat, encrypted, no-configuration LAN for all of the instances of your Android app.

This short tutorial will show you how to enable ZeroTier functionality for your Android app with little to no code modification. Check out our [ZeroTier SDK](https://www.zerotier.com/blog) page for more info on how the integration works and [Shim Techniques](https://www.zerotier.com/blog) for a discussion of shims available for your app/technology.

In this example we aim to set up a minimal [Android Studio](https://developer.android.com/studio/index.html) project which contains all of the components necessary to enable ZeroTier for your app. If you'd rather skip all of these steps and grab the code, look in the [sdk/android](https://github.com/zerotier/ZeroTierOne/tree/dev/sdk/integrations/android/example_app) folder in the source tree. Otherwise, let's get started!

*NOTE: For Android JNI libraries to build you'll need to install [Android Studio](https://developer.android.com/studio/index.html) the [Android NDK](https://developer.android.com/ndk/index.html). Currently only Android NDK r10e is supported and can be found [here for OSX](http://dl.google.com/android/repository/android-ndk-r10e-darwin-x86_64.zip) and [here for Linux](http://dl.google.com/android/repository/android-ndk-r10e-linux-x86_64.zip). You'll need to tell our project where you put it by putting the path in [this file](Android/proj/local.properties), you'll need to install the Android Build-Tools (this can typically be done through the editor the first time you start it up), and finally you should probably upgrade your Gradle plugin if it asks you to. If you don't have these things installed and configured we will detect that and just skip those builds automatically.*

**Step 1: Select build targets**
 - Specify the target architectures you want to build in [Application.mk](android/java/jni/Application.mk). By default it will build `arm64-v8a`, `armeabi`, `armeabi-v7a`, `mips`, `mips64`, `x86`, and `x86_64`.

**Step 2: Build Shared Library**
 - `make android_jni_lib`
 - The resultant `build/android_jni_lib_YOUR_ARCH/libZeroTierOneJNI.so` is what you want to import into your own project to provide the API to your app. Select your architecture and copy the shared library into your project's JNI directory, possibly `/src/main/jniLibs/YOUR_ARCH/`. Selecting only the architectures you need will *significantly* reduce overall build time.

**Step 3: App Code Modifications**
 - Create new package called `ZeroTierSDK` in your project and add a new file called `ZeroTierSDK.java` containing:

```
package ZeroTier;
public class ZeroTierSDK {
    public native void startOneService(String homeDir);
    public native void joinNetwork(String nwid);
    public native void leaveNetwork(String nwid);
    public native boolean isRunning();
    static { System.loadLibrary("ZeroTierOneJNI"); } // Loads JNI code
}
```

 - And now, start the service in your app with:

```
new Thread(new Runnable() {
      public void run() {
        ZeroTierSDK wrapper = new ZeroTierSDK();
        wrapper.startOneService(); // Calls to JNI code
      }
}).start();
```

**Step 4: App permissions**

 - In order for your application to write the auth keys and network files to the internal storage you'll need to set a few permissions in your `AndroidManifest.xml` file at the same scope level as `<application>`:

```
<uses-permission android:name="android.permission.READ_EXTERNAL_STORAGE" />
<uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE" />
```

**Step 5: Pick an API**

 - If functional interposition isn't available for the API or library you've chosen to use, ZeroTier offers a SOCKS5 proxy server which can allow connectivity to your virtual network as long as your client API supports the SOCKS5 protocol. This proxy service will run alongside the tap service and can be turned on by compiling with the `-DUSE_SOCKS_PROXY` flag. By default, the proxy service is available at `0.0.0.0:1337`.

**Step 6: Join a network!**

 - Simply call `wrapper.joinNetwork("XXXXXXXXXXXXXXXX")`



***

*Note for the curious on JNI naming conventions: In order to reference a symbol in the JNI library you need to structure the package and class in your Android Studio project in a very particular way. For example, in the ZeroTierSDK we define a function called `Java_ZeroTier_SDK_startOneService`, the name can be broken down as follows: `Java_PACKAGENAME_CLASSNAME_startOneService`, so as we've defined it, you must create a package called `ZeroTier` and add a class called `SDK`.* 


