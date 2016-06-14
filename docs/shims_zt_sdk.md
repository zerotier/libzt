Shims
====

If you're here, you're wondering how you're going to connect your app up to the ZeroTier service. We've tried our best to simplify this process for as many situations as we could. Once you get past the initial hurdle of integrating your first shim, we think you'll be pleased with the result. How we accomplish this depends entirely on the structure and design of your app. Let's begin!

A shim is a piece of code in the form of a `Hook`, `Proxy`, `Direct Call`, or `Changeling` which we "inject" into your application. A shim augments your app to provide a private interface to ZeroTier networks with no change to your app's code or structure. A shim will forward network-related activity directly to and from a local running instance of ZeroTier.

We suggest selecting one of the architectures you'd like to support and trying out one of our sample projects. If you need any help or have a feature request, be sure to let us know [here](https://www.zerotier.com/community/)!

***
#### iOS

**Integration Option 1:** Using `ZeroTierSDK.framework` (instructions [here](ios_zt_sdk.md))
- By including our framework in your app, you'll get a full instance of the ZeroTier service and suite of shim mechanisms built right into your app and ready to use. This option allows for `Hook of BSD-like sockets`, `Proxy of NSStream`, `Changeling of BSD-like sockets`, and `Direct Call`.

*Note: `CFSocket` is not currently supported on `iOS`*

***
#### OSX

**Integration Option 1:** Using `ZeroTierSDK.framework` (instructions [here](osx_zt_sdk.md))
- By including our framework in your app, you'll get a full instance of the ZeroTier service and suite of shim mechanisms built right into your app and ready to use. This option allows for `Hook of BSD-like sockets and CFSocket`, `Proxy of NSStream`, `Changeling of BSD-like sockets`, and `Direct Call`.

**Integration Option 2:** Statically-link `libztintercept.so`
- You can build our shared library and link it into your application. This option allows for `Hook of BSD-like sockets and CFSocket`, `Proxy of NSStream`, `Changeling of BSD-like sockets`, and `Direct Call`.

***
#### Android

**Integration Option 1:** Using `libZeroTierJNI.so` (instructions [here](android_zt_sdk.md))
- This is very similar to the approach used for `iOS`, in this case you'll build a shared library for the architectures you wish to support and then add it to your project. This option allows for `Proxy of Socket`, and `Direct Call`.

***
#### Linux

**Integration Option 1:** Using `LD_PRELOAD`
- This option allows for `Hook of BSD-like sockets`, `Proxy` and `Direct Call`.

***

#### Windows
*Note: We haven't yet put development effort towards a shim for Windows but it's in the pipeline*

***  

### Further explanation of each shim type:

**Hook**
- Uses dynamic loading of our library to allow function interposition or "hooking" to re-implement traditional socket API functions like `socket()`, `connect()`, `bind()`, etc.

**SOCKS5 Proxy**
- Provides an integrated SOCKS5 server alongside the ZeroTier service to proxy connections from an application to resources on a ZeroTier network. For instance, a developer which has built an iOS app using the NSStreams API could add ZeroTier to their application and simply use the SOCKS5 support build into NSStreams to reach resources on their network. An Android developer could do the same using the SOCKS5 support provided in the `Socket` API.

**Direct Call**
- Directly call the `zt_` API specified in [SDK.h](../src/SDK.h). For this to work, just use one of the provided headers that specify the interface for your system/architecture and then either dynamically-load our library into your app or compile it right in. 

**Changeling**
- This method is still experimental but the idea is to link `libztkq.so` into your app. You call `start_changeling()`. This will set up a separate thread to monitor all files for the process using kqueue. When an event is detected which indicates something is attempting to connect out or something is accepting a connection, we'll perform a sort of "hot-swap" of that socket for a socket that has been administered by ZeroTier.