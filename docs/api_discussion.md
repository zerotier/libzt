ZTSDK API Options
====

*This document is under construction*

## APIs

**Hook/Intercept**
- Uses dynamic loading of our library to allow function interposition or "hooking" to re-implement traditional socket API functions like `socket()`, `connect()`, `bind()`, etc.

**SOCKS5 Proxy**
- Provides an integrated SOCKS5 server alongside the ZeroTier service to proxy connections from an application to resources on a ZeroTier network. For instance, a developer which has built an iOS app using the NSStreams API could add ZeroTier to their application and simply use the SOCKS5 support build into NSStreams to reach resources on their network. An Android developer could do the same using the SOCKS5 support provided in the `Socket` API.

**Direct Call**
- Directly call the `zt_` API specified in [SDK.h](src/SDK.h). For this to work, just use one of the provided headers that specify the interface for your system/architecture and then either dynamically-load our library into your app or compile it right in. 

***
![Image](docs/img/api_diagram.png)


The following APIs are available for this integration:
- `Direct Call`: Consult [src/SDK_Apple-Bridging-Header.h](../../../../src/SDK_Apple-Bridging-Header.h).
- `Hook of BSD-like sockets`: Use BSD-like sockets as you normally would.
- `Proxy of NSStream`: Create NSStream. Configure stream for SOCKS5 Proxy (127.0.0.1:PORT). Start Proxy. Use stream. An example of how to use the proxy can be found in the example iOS/OSX projects.