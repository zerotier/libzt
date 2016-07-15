ZeroTier Integrations
====

If you want everything built at once, type `make all` and go play outside for a few minutes, we'll copy all of the targets into the `build` directory for you along with specific instructions contained in a `README.md` on how to use each binary. You can then use `make -s check` to check the build status of each binary target.

*NOTE for Apple platforms: In order to build iOS/OSX Frameworks and Bundles you will need XCode command line tools `xcode-select --install`*

*NOTE: For Android JNI libraries to build you'll need to install [Android Studio](https://developer.android.com/studio/index.html) and the [Android NDK](https://developer.android.com/ndk/index.html), and you'll need to tell our project where you put it by putting the path in [this file](Android/proj/local.properties), if you don't have these things installed and configured we will detect that and just skip those builds automatically. Additionally, you can specify the target architectures you want to build in [Application.mk](android/java/jni/Application.mk). By default it will build `arm64-v8a`, `armeabi`, `armeabi-v7a`, `mips`, `mips64`, `x86`, and `x86_64`*

Below are the specific instructions for each integration requiring *little to no modification to your code*. Remember, with a full build we'll put a copy of the appropriate integration instructions in the resultant binary's folder for you anyway.

For more support on these integrations, or if you'd like help creating a new integration, stop by our [community section](https://www.zerotier.com/community/)!

***
## Build flags

- `SDK_DEBUG` - Turns on SDK activity/warning/error output. Levels of verbosity can be adjusted in `src/SDK_Debug.h`

- `SDK_DEBUG_LOGFILE` - Used in conjunction with `SDK_DEBUG`, this will write all SDK debug chatter to a log file. To use this, set `make SDK_DEBUG_LOGFILE=1` then `export ZT_SDK_LOGFILE=debug.log`. 

- `SDK_LWIP_DEBUG` - Turns on debug output for the lwIP library.

- `SDK_BUNDLED` - Builds the SDK as a single target including an API shim, the lwIP library, and the ZeroTier service.

- `SDK_SERVICE`

- `SDK_SOCKS_PROXY` - Enables the SOCK5 Proxy. 

- `SDK_UNITY_3D` - For use when building a target for a native plugin for Unity3D.


***
## Current Integrations

### Apple `make apple`
##### iOS
 - [Embedding within an app](../docs/ios_zt_sdk.md) `make ios_app_framework`
 - [Unity3D plugin](../docs/unity3d_ios_zt_sdk.md) `make ios_unity3d_bundle`

##### OSX
 - [Embedding within an app](../docs/osx_zt_sdk.md) `make osx_app_framework`
 - [Dynamic-linking into an app/service at runtime](../docs/osx_zt_sdk.md) `make osx_shared_lib`
 - [Unity3D plugin](../docs/unity3d_osx_zt_sdk.md) `make osx_unity3d_bundle`

***
### Linux
 - [Dynamic-linking into an app/service at runtime](../docs/linux_zt_sdk.md) `make linux_shared_lib`
 - [Using the SDK with Docker](../docs/docker_linux_zt_sdk.md) `make linux_shared_lib`

### Android `make android`
 - [Embedding within an app](../docs/android_zt_sdk.md) `make android_jni_lib`
 - [Unity 3D plugin](../docs/unity3d_android_zt_sdk.md) `make android_unity3d_plugin`

***
### Windows
 - Not yet.
