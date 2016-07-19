ZeroTier Integrations
====

If you want everything built at once, type `make all` and go play outside for a few minutes, we'll copy all of the targets into the `build` directory for you along with specific instructions contained in a `README.md` on how to use each binary. You can then use `make -s check` to check the build status of each binary target.

*NOTE for Apple platforms: In order to build iOS/OSX Frameworks and Bundles you will need XCode command line tools `xcode-select --install`*

*NOTE: For Android JNI libraries to build you'll need to install [Android Studio](https://developer.android.com/studio/index.html) the [Android NDK](https://developer.android.com/ndk/index.html). Currently only Android NDK r10e is supported and can be found [here for OSX](http://dl.google.com/android/repository/android-ndk-r10e-darwin-x86_64.zip) and [here for Linux](http://dl.google.com/android/repository/android-ndk-r10e-linux-x86_64.zip). You'll need to tell our project where you put it by putting the path in [this file](android/proj/local.properties), you'll need to install the Android Build-Tools (this can typically be done through the editor the first time you start it up), and finally you should probably upgrade your Gradle plugin if it asks you to. If you don't have these things installed and configured we will detect that and just skip those builds automatically. Additionally, you can specify the target architectures you wish to build for by editing [Application.mk](android/android_jni_lib/java/jni/Application.mk). By default it will build `arm64-v8a`, `armeabi`, `armeabi-v7a`, `mips`, `mips64`, `x86`, and `x86_64`*

Below are the specific instructions for each integration requiring *little to no modification to your code*. Remember, with a full build we'll put a copy of the appropriate integration instructions in the resultant binary's folder for you anyway.

For more support on these integrations, or if you'd like help creating a new integration, stop by our [community section](https://www.zerotier.com/community/)!

***
## Important Build flags

- `SDK_DEBUG` - Turns on SDK activity/warning/error output. Levels of verbosity can be adjusted in `src/SDK_Debug.h`
- `SDK_DEBUG_LOGFILE` - Used in conjunction with `SDK_DEBUG`, this will write all SDK debug chatter to a log file. To use this, set `make SDK_DEBUG_LOGFILE=1` then `export ZT_SDK_LOGFILE=debug.log`. 
- `SDK_LWIP_DEBUG` - Turns on debug output for the lwIP library.
- `SDK_BUNDLED` - Builds the SDK as a single bundled target including a the RPC mechanism, the lwIP library, and the ZeroTier service.
- `SDK_SOCKS_PROXY` - Enables the SOCK5 Proxy. This flag is enabled by default on must builds, especially mobile.

***
## Current Integrations

### Apple `make apple`
##### iOS
 - [Embedding within an app](apple/example_app/iOS) `make ios_app_framework`
 - Unity3D plugin `make ios_unity3d_bundle`

##### OSX
 - [Embedding within an app](apple/example_app/OSX) `make osx_app_framework`
 - [Dynamic-linking into an app/service at runtime](../docs/osx_zt_sdk.md) `make osx_shared_lib`
 - [Unity3D plugin](apple/ZeroTierSDK_Apple) `make osx_unity3d_bundle`

***
### Linux
 - [Dynamic-linking into an app/service at runtime](../docs/linux_zt_sdk.md) `make linux_shared_lib`
 - [Using the SDK with Docker](docker)

### Android `make android`
 - [Embedding within an app](android) `make android_jni_lib`
 - [Unity 3D plugin](../docs/android_unity3d_zt_sdk.md) `make android_unity3d_plugin`

***
### Windows
 - Not yet.
