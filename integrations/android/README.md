Android + ZeroTier SDK
====

Welcome!

Imagine a flat, encrypted, no-configuration LAN for all of the instances of your Android app. This short tutorial will show you how to enable ZeroTier functionality for your Android app with little to no code modification. Check out our [ZeroTier SDK](https://www.zerotier.com/blog) page for more info on how the integration works. In this example we aim to set up a minimal [Android Studio](https://developer.android.com/studio/index.html) project which contains all of the components necessary to enable ZeroTier for your app.

*NOTE: For Android JNI libraries to build you'll need to install [Android Studio](https://developer.android.com/studio/index.html) the [Android NDK](https://developer.android.com/ndk/index.html). Currently only Android NDK r10e is supported and can be found [here for OSX](http://dl.google.com/android/repository/android-ndk-r10e-darwin-x86_64.zip) and [here for Linux](http://dl.google.com/android/repository/android-ndk-r10e-linux-x86_64.zip). You'll need to tell our project where you put it by putting the path in [this file](android_jni_lib/proj/local.properties), you'll need to install the Android Build-Tools (this can typically be done through the editor the first time you start it up), and finally you should probably upgrade your Gradle plugin if it asks you to. If you don't have these things installed and configured we will detect that and just skip those builds automatically.*

If you want to skip the following steps and just take a look at the project, go [here](example_app).

***

**Step 1: App Code Modifications**
 - In your project, create a new package called `ZeroTier` and class file within called `ZTSDK.java` and copy contents from `src/SDK_JavaWrapper.java`
 
 - Start the service

    ```
    String nwid = "8056c2e21c000001";
    // Set up service
    final ZTSDK zt = new ZTSDK();
    final String homeDir = getApplicationContext().getFilesDir() + "/zerotier";
    new Thread(new Runnable() {
        public void run() {
            // Calls to JNI code
            zt.start_service(homeDir);
        }
    }).start();
    while(!zt.running()) { }
    ```

 - Join network and start doing network stuff!

    ```
    zt.join_network(nwid);
    int sock = zt.socket(zt.AF_INET, zt.SOCK_STREAM, 0);
    int err = zt.connect(sock, "10.9.9.203", 8080, nwid);
    // zt.recvfrom(), zt.write(), etc...
    ```

**Step 2: App permissions**

 - In order for your application to write the auth keys and network files to the internal storage you'll need to set a few permissions in your `AndroidManifest.xml` file at the same scope level as `<application>`:

    ```
    <uses-permission android:name="android.permission.READ_EXTERNAL_STORAGE" />
    <uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE" />
    ```

**Step 3: Set NDK/SDK paths**
 - Specify your SDK/NDK path in `android_jni_lib/proj/local.properties`. For example:

    ```
    sdk.dir=/Users/Name/Library/Android/sdk
    ndk.dir=/Users/Name/Library/Android/ndk-r10e
    ```

**Step 4: Select build targets**
 - Specify the target architectures you want to build in [Application.mk](android_jni_lib/java/jni/Application.mk). By default it will build `arm64-v8a`, `armeabi`, `armeabi-v7a`, `mips`, `mips64`, `x86`, and `x86_64`. For each architecture you wish to support a different shared library will need to be built. This is all taken care of automatically by the build script.

**Step 4: Build Shared Library**
 - `make android_jni_lib`
 - The resultant `build/android_jni_lib/ARCH/libZeroTierOneJNI.so` is what you want to import into your own project to provide our API implementation to your app. Select your architecture and copy the shared library `libZeroTierOneJNI.so` into your project's JNI directory, possibly `/src/main/jniLibs/ARCH/libZeroTierOneJNI.so`.
 - Selecting only the architectures you need will *significantly* reduce overall build time.
