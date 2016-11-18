ZeroTier SDK
======

ZeroTier-enabled apps and devices.

Secure virtual network access embedded directly into applications, games, and devices. Imagine starting an instance of your application or game and having it automatically be a member of your virtual network without having to rewrite your networking layer. Check out our [Integrations](integrations/) to learn how to integrate this into your application, device, or ecosystem.

## How do I use it?

There are generally two ways one might want to use the service. 

 - The first approach is a *compile-time static linking* of our service directly into your application. With this option you can bundle our entire functionality right into your app with no need to communicate with a service externally, it'll all be handled automatically. This is most typical for mobile applications, games, etc.

 - The second is a service-oriented approach where our network call "intercept" is *dynamically-linked* into your applications upon startup and will communicate to a single ZeroTier service on the host. This can be useful if you've already compiled your applications and can't perform a static linking.

![Image](docs/img/methods.png)

## How does it work?

We've designed a background tap service that pairs the ZeroTier protocol with swappable user-space network stacks. We've provided drivers for [Lightweight IP (lwIP)](http://savannah.nongnu.org/projects/lwip/) and [picoTCP](http://www.picotcp.com/). The aim is to give you a new way to bring your applications onto your virtual network. For a more in-depth explanation of our technology take a look at our [SDK Primer](docs/zt_sdk_primer.md)

## APIs

**Hook/Intercept**
- Uses dynamic loading of our library to allow function interposition or "hooking" to re-implement traditional socket API functions like `socket()`, `connect()`, `bind()`, etc.

**SOCKS5 Proxy**
- Provides an integrated SOCKS5 server alongside the ZeroTier service to proxy connections from an application to resources on a ZeroTier network. For instance, a developer which has built an iOS app using the NSStreams API could add ZeroTier to their application and simply use the SOCKS5 support build into NSStreams to reach resources on their network. An Android developer could do the same using the SOCKS5 support provided in the `Socket` API.

**Direct Call**
- Directly call the `zt_` API specified in [SDK.h](src/SDK.h). For this to work, just use one of the provided headers that specify the interface for your system/architecture and then either dynamically-load our library into your app or compile it right in. 


***
## Important Build Flags

- `SDK_IPV4=1` - Enable IPv4 support
- `SDK_IPV6=1` - Enable IPv6 support

- `SDK_DEBUG=1` - Enables SDK debugging

- `SDK_PICOTCP=1` - Enable the use of `picoTCP` (recommended)
- `SDK_PICOTCP_DEBUG=1` - Enables debug output for the `picoTCP` network stack

- `SDK_LWIP=1` - Enable the use of `lwIP` (deprecated)
- `SDK_LWIP_DEBUG=1` - Enables debug output for the `lwIP` library.

***

### Apple 
 - For everything: `make apple`

##### iOS
 - [Embedding within an app](apple/example_app/iOS) `make ios_app_framework` -> `build/ios_app_framework/*`
 - Unity3D plugin `make ios_unity3d_bundle` -> `build/ios_unity3d_bundle/*`

##### OSX
 - [Linking into an app at compiletime](../docs/osx_zt_sdk.md) `make osx_shared_lib` -> `build/libztosx.so`
 - [Embedding within an app with Xcode](apple/example_app/OSX) `make osx_app_framework` -> `build/osx_app_framework/*`
 - [Dynamic-linking into an app/service at runtime](../docs/osx_zt_sdk.md) `make osx_service_and_intercept` -> { `build/zerotier-sdk-service` + `build/libztintercept.so` }
 - [Intercept library](../docs/osx_zt_sdk.md) `make osx_sdk_service` -> `build/zerotier-sdk-service`
 - [SDK Service](../docs/osx_zt_sdk.md) `make osx_intercept` -> `build/libztintercept.so`
 - [Unity3D plugin](apple/ZeroTierSDK_Apple) `make osx_unity3d_bundle`

***
### Linux
 - For everything: `make linux`
 - [Dynamic-linking into an app/service at runtime](../docs/linux_zt_sdk.md) `make linux_shared_lib`
 - Service and Intercept `make linux_service_and_intercept` -> { `build/zerotier-sdk-service` + `build/libztintercept.so` }
 - [Using the SDK with Docker](docker)

### Android 
 - For everything: `make android`
 
 - [Embedding within an app](android) `make android_jni_lib` -> `build/android_jni_lib/YOUR_ARCH/libZeroTierOneJNI.so`
 - [Unity 3D plugin](../docs/android_unity3d_zt_sdk.md) `make android_unity3d_plugin` -> `build/android_unity3d_plugin/*`

***
### Windows
 - Not yet.


***
![Image](docs/img/api_diagram.png)
