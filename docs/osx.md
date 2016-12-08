OSX + ZeroTier SDK
====

Welcome!

Imagine a flat, encrypted, no-configuration LAN for all of the instances of your OSX app. 

***

## Via Traditional Linking (Service and Intercept)

 - This will allow the interception of traditional socket API calls such as `socket()`, `connect()`, `bind()`, etc.

Build against our library:

    gcc app.c -o app libztintercept.so
    export ZT_NC_NETWORK=/tmp/sdk-test-home/nc_8056c2e21c000001

Start service

    ./zerotier-sdk-service -d -p8000 /tmp/sdk-test-home &

Run application

    ./app

## Via `DYLD_LIBRARY_PATH` (Injecting code dynamically at runtime)

As of the release of El Capitan, Apple requires one to turn off System Integrity Protection for code injection from external libraries. This method of intercepting network calls is now deprecated and we are investigating new methods for future releases of the SDK. If you still wish to use this method just `export DYLD_LIBRARY_PATH=./path/to/libztintercept.so:$DYLD_LIBRARY_PATH`. Also set `ZT_NC_NETWORK` appropriately.


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
- Add `src/wrappers/swift/ZTSDK.swift`, `src/wrappers/swift/XcodeWrapper.cpp`, and `src/wrappers/swift/XcodeWrapper.hpp` to your project:
- Set `src/wrappers/swift/Apple-Bridging-Header.h` as your bridging-header in *Build Settings -> Objective-C Bridging-header*

**Step 3: Start the ZeroTier service**

Start the service:

```
    zt.start_service("."); // Where the ZeroTier config files for this app will be stored
    zt.join_network(nwid);
```

Listen for incoming connections:

```
    let sock: Int32 = zt.socket(AF_INET, SOCK_STREAM, 0)
    let ztaddr: ZTAddress = ZTAddress(AF_INET, serverAddr, Int16(serverPort))
    let bind_err: Int32 = zt.bind(sock, ztaddr)
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

Click [here](../../../../docs/api_discussion.md) to learn more about alternative APIs such as the Intercept and SOCKS5 Proxy.