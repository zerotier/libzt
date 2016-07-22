Android + ZeroTier SDK
====

Welcome!

Imagine a flat, encrypted, no-configuration LAN for all of the instances of your Android app. This short tutorial will show you how to enable ZeroTier functionality for your Android app with little to no code modification. Check out our [ZeroTier SDK](https://www.zerotier.com/blog) page for more info on how the integration works. In this example we aim to set up a minimal [Android Studio](https://developer.android.com/studio/index.html) project which contains all of the components necessary to enable ZeroTier for your app.

*NOTE: For Android JNI libraries to build you'll need to install [Android Studio](https://developer.android.com/studio/index.html) the [Android NDK](https://developer.android.com/ndk/index.html). Currently only Android NDK r10e is supported and can be found [here for OSX](http://dl.google.com/android/repository/android-ndk-r10e-darwin-x86_64.zip) and [here for Linux](http://dl.google.com/android/repository/android-ndk-r10e-linux-x86_64.zip). You'll need to tell our project where you put it by putting the path in [this file](android/proj/local.properties), you'll need to install the Android Build-Tools (this can typically be done through the editor the first time you start it up), and finally you should probably upgrade your Gradle plugin if it asks you to. If you don't have these things installed and configured we will detect that and just skip those builds automatically.*

If you want to skip these steps and just take a look at the project, go [here](example_app).

***
**Step 1: Select build targets**
 - Specify the target architectures you want to build in [Application.mk](android/java/jni/Application.mk). By default it will build `arm64-v8a`, `armeabi`, `armeabi-v7a`, `mips`, `mips64`, `x86`, and `x86_64`.

**Step 2: Build Shared Library**
 - `make android_jni_lib`
 - The resultant `build/android_jni_lib_YOUR_ARCH/libZeroTierOneJNI.so` is what you want to import into your own project to provide the API to your app. Select your architecture and copy the shared library into your project's JNI directory, possibly `/src/main/jniLibs/YOUR_ARCH/`. Selecting only the architectures you need will *significantly* reduce overall build time.

**Step 3: App permissions**

 - In order for your application to write the auth keys and network files to the internal storage you'll need to set a few permissions in your `AndroidManifest.xml` file at the same scope level as `<application>`:

```
<uses-permission android:name="android.permission.READ_EXTERNAL_STORAGE" />
<uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE" />
```

**Step 4: App Code Modifications**
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

 - Start the service

```
final SDK zt = new SDK();
final String homeDir = getApplicationContext().getFilesDir() + "/zerotier";

new Thread(new Runnable() {
    public void run() {
        // Calls to JNI code
        zt.zt_start_service(homeDir);
    }
}).start();
```

 - Join network and perform network call

```
while(!zt.zt_running()) { }
zt.zt_join_network("XXXXXXXXXXXXXXXX");

// Create ZeroTier socket
int sock = zt.zt_socket(zt.AF_INET, zt.SOCK_STREAM, 0);

// Connect to remote host
int err = zt.zt_connect(sock, "10.9.9.203", 8080);
```

***

*Note for the curious on JNI naming conventions: In order to reference a symbol in the JNI library you need to structure the package and class in your Android Studio project in a very particular way. For example, in the ZeroTierSDK we define a function called `Java_ZeroTier_SDK_zt_1start_1service`, the name can be broken down as follows: `Java_PACKAGENAME_CLASSNAME_zt_1start_1service`, so as we've defined it, you must create a package called `ZeroTier` and add a class called `SDK`.*