OSX + ZeroTier SDK
====

Welcome!

Imagine a flat, encrypted, no-configuration LAN for all of the instances of your OSX app. 

This short tutorial will show you how to enable ZeroTier functionality for your OSX app with little to no code modification. Check out our [ZeroTier SDK](https://www.zerotier.com/blog) page for more info on how the integration works.

***

## Via Traditional Linking (Everything bundled)

 - Use this if you'd like everything included in a single easy-to-use library.

```
make osx_shared_lib`
g++ app.cpp -o app libztosx.so
./app
```

## Via Traditional Linking (Service+Intercept model)

 - Use this model if you'd like multiple applications to talk to the same ZeroTierSDK service instance. Often the *intercept-model* is used when you don't have access to the source of an app and you'd like to re-direct its network calls.

Example:

    gcc app.c -o app libztintercept.so
    export ZT_NC_NETWORK=/tmp/sdk-test-home/nc_8056c2e21c000001

Start service

    ./zerotier-sdk-service -d -p8000 /tmp/sdk-test-home &

Run application

    ./app

## Via App Framework in XCode

***
**Step 1: Build OSX framework**

- `make osx_app_framework`
- This will output to `build/osx_app_framework/Release/ZeroTierSDK_OSX.framework`

**Step 2: Integrate SDK into project**

- Add the resultant framework package to your project
- Add `src` directory to *Build Settings -> Header Search Paths*
- Add `build/osx_app_framework/Release/` to *Build Settings -> Framework Search Paths*
- Add `ZeroTierSDK.frameworkOSX` to *General->Embedded Binaries*
- Add `src/SDK_XcodeWrapper.cpp` and `src/SDK_XcodeWrapper.hpp` to your project:
- Set `src/SDK_Apple-Bridging-Header.h` as your bridging-header in *Build Settings -> Objective-C Bridging-header*

**Step 3: Start the ZeroTier service**

Set up the ZeroTier service thread:

```
var service_thread : NSThread!
func zt_start_service() {
    let path = NSSearchPathForDirectoriesInDomains(NSSearchPathDirectory.DocumentDirectory, NSSearchPathDomainMask.UserDomainMask, true)
    start_service(".") // "." path will tell ZeroTier to write its config data to the same directory as the binary
}
```

and then start it:

```
dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), {
    self.service_thread = NSThread(target:self, selector:"zt_start_service", object:nil)
    self.service_thread.start()
});
```

**Step 4: Pick an API**

The following APIs are available for this integration:
- `Direct Call`: Consult [src/SDK_Apple-Bridging-Header.h](../../../../src/SDK_Apple-Bridging-Header.h).
- `Hook of BSD-like sockets`: Use BSD-like sockets as you normally would.
- `Proxy of NSStream`: Create NSStream. Configure stream for SOCKS5 Proxy (127.0.0.1:PORT). Start Proxy. Use stream.

**Step 5: Join a network!**

Simply call `zt_join_network("nwid")`

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

