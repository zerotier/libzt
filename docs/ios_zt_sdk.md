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

Start the service:

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
    let accepted_sock: Int32 = zt.accept(sock, ztaddr)
```

Or, establish a connection:

```
    let sock: Int32 = zt.socket(AF_INET, SOCK_STREAM, 0)
    let ztaddr: ZTAddress = ZTAddress(AF_INET, serverAddr, Int16(serverPort))
    let connect_err: Int32 = zt.connect(sock, ztaddr)
```

**Alternative APIs**

CLick [here](../../../../docs/api_discussion.md) to learn more about alternative APIs such as the Intercept and SOCKS5 Proxy.