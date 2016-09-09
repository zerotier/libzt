iOS + ZeroTier SDK
====

Welcome!

Imagine a flat, encrypted, no-configuration LAN for all of the instances of your iOS app. 

This short tutorial will show you how to enable ZeroTier functionality for your iOS app with little to no code modification. Check out our [ZeroTier SDK](https://www.zerotier.com/blog) page for more info on how the integration works.

***
**Step 1: Build iOS framework**

- `make ios_app_framework`
- This will output to `build/ios_app_framework/Release-iphoneos/ZeroTierSDK_iOS.framework`

**Step 2: Integrate SDK into project**

- Add the resultant framework package to your project
- Add `src` directory to *Build Settings -> Header Search Paths*
- Add `build/ios_app_framework/Release-iphoneos/` to *Build Settings -> Framework Search Paths*
- Add `ZeroTierSDK.frameworkiOS` to *General->Embedded Binaries*
- Add `src/ZTSDK.swift`, `src/SDK_XcodeWrapper.cpp` and `src/SDK_XcodeWrapper.hpp` to your project:
- Set `src/SDK_Apple-Bridging-Header.h` as your bridging-header in *Build Settings -> Objective-C Bridging-header*

**Step 3: Start the ZeroTier service**

Now find a place in your code to set up the ZeroTier service thread:

`Start the service:

```
    zt.start_service(nil);
    zt.join_network(nwid);
```

Listen for incoming connections:

```
    let sock: Int32 = zt.socket(AF_INET, SOCK_STREAM, 0)
    let ztaddr: ZTAddress = ZTAddress(AF_INET, serverAddr, Int16(serverPort))
    let bind_err = zt.bind(sock, ztaddr)
    zt_listen(sock, 1);
    accepted_sock = zt.accept(sock, ztaddr)
```

Or, establish a connection:

```
    let sock: Int32 = zt.socket(AF_INET, SOCK_STREAM, 0)
    let ztaddr: ZTAddress = ZTAddress(AF_INET, serverAddr, Int16(serverPort))
    let connect_err = zt.connect(sock, ztaddr)
```

**Step 4: Pick an API**

The following APIs are available for this integration:
- `Direct Call`: Consult [src/SDK_Apple-Bridging-Header.h](../../../../src/SDK_Apple-Bridging-Header.h).
- `Hook of BSD-like sockets`: Use BSD-like sockets as you normally would. This likely won't work for calls used by a third-party library.
- `Proxy of NSStream`: Create NSStream. Configure stream for SOCKS5 Proxy (127.0.0.1:PORT). Start Proxy. Use stream. An example of how to use the proxy can be found in the example iOS/OSX projects.
