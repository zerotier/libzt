OSX + ZeroTier SDK
====

Welcome!

Imagine a flat, encrypted, no-configuration LAN for all of the instances of your OSX app. 

This short tutorial will show you how to enable ZeroTier functionality for your iOS app with little to no code modification. Check out our [ZeroTier SDK](https://www.zerotier.com/blog) page for more info on how the integration works and [Shim Techniques](https://www.zerotier.com/blog) for a discussion of shims available for your app/technology.

In this example we aim to set up a minimal XCode project which contains all of the components necessary to enable ZeroTier for your app. If you'd rather skip all of these steps and grab the code, look in the [sdk/OSX](https://github.com/zerotier/ZeroTierSDK/tree/dev/sdk/iOS) folder of the source tree. Otherwise, let's get started!

**Step 1: Add ZeroTier source and Netcon-iOS XCode project to yours**
- Place a copy of the ZeroTierOne source in a folder at the same level as your project
- Add `ZeroTierSDK/src/tests/iOS/Netcon-iOS.xcodeproj` to your project

**Step 2: Add ZeroTier binaries to your app**
- Add `ZeroTierSDK.frameworkiOS` to *General->Embedded Binaries*
- Add `libServiceSetup.a` and `ZeroTierSDK.framework` to *Build Phases->Link Binary With Libraries*

**Step 3: Configure your project**
- Add `$(SRCROOT)/../ZeroTierOne/src` to *Build Settings->Header Search Paths* for your project
- Add `-D__IOS__` to *Build Settings->Other C Flags*
- Add `zerotiersdk/tests/iOS/Netcon-iOS/NetconWrapper.cpp` and `zerotiersdk/tests/iOS/Netcon-iOS/NetconWrapper.hpp` to your project:
- Add contents of `ZeroTierOne/netcon/tests/iOS/Netcon-iOS/SDK-iOS-Bridging-Header.h` to your project’s bridging header.

*Note: You should have been prompted to create a bridging header for your project, if you haven't make sure you do this and add the native function prototypes manually from the bridging header we provide.*

**Step 4: App Code Modifications**

After you've linked the two projects you need to find a place in your code to set up the ZeroTier service thread:

```
var service_thread : NSThread!
func ztnc_start_service() {
    let path = NSSearchPathForDirectoriesInDomains(NSSearchPathDirectory.DocumentDirectory, NSSearchPathDomainMask.UserDomainMask, true)
    start_service(path[0])
}
```

...and then start it. If you enabled the proxy service via `-DUSE_SOCKS_PROXY` it will start automatically and be reachable at `0.0.0.0:1337`: 

```
dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), {
    self.service_thread = NSThread(target:self, selector:"ztnc_start_service", object:nil)
    self.service_thread.start()
});
```

**Step 5: Pick a shim for your app**

This integration allows for the following shim combinations:
- `Hook of BSD-like sockets`: Use BSD-like sockets as you normally would.
- `Proxy of NSStream`: Create NSStream. Configure stream for SOCKS5 Proxy. Use stream.
- `Changeling of BSD-like sockets`: Call `start_changeling()` and then use BSD-like sockets as you normally would.
- `Direct Call`: Consult [SDK-iOS-Bridging-Header.h](netcon/iOS/Netcon-iOS/Netcon-iOS-Bridging-Header.h).

If functional interposition isn't available for the API or library you've chosen to use, ZeroTier offers a SOCKS5 proxy server which can allow connectivity to your virtual network as long as your client API supports the SOCKS5 protocol. This proxy service will run alongside the tap service and can be turned on by compiling with the `-DUSE_SOCKS_PROXY` flag in *Build Settings->Other C Flags*. By default, the proxy service is available at `0.0.0.0:1337`.

**Step 6: Join a network!**

Simply call `zt_join_network("XXXXXXXXXXXXXXXX")`

***
**NSStream and SOCKS Proxy:**

As an example, here's how one would configure a NSStream object to redirect all network activity to the ZeroTier SOCKS proxy server:

```
// BEGIN proxy configuration
let myDict:NSDictionary = [NSStreamSOCKSProxyHostKey : "0.0.0.0",
                           NSStreamSOCKSProxyPortKey : 1337,
                           NSStreamSOCKSProxyVersionKey : NSStreamSOCKSProxyVersion5]

inputStream!.setProperty(myDict, forKey: NSStreamSOCKSProxyConfigurationKey)
outputStream!.setProperty(myDict, forKey: NSStreamSOCKSProxyConfigurationKey)
// END proxy configuration
```









## Linking into an application on Mac OSX

Example:

    gcc myapp.c -o myapp libztintercept.so
    export ZT_NC_NETWORK=/tmp/netcon-test-home/nc_8056c2e21c000001

Start service

    ./zerotier-netcon-service -d -p8000 /tmp/netcon-test-home

Run application

    ./myapp

