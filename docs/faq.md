Frequently Asked Questions
======


#### Why would I integrate ZeroTier into my app?

The ZeroTier SDK is designed specifically for the developer that doesn't want to work with or deal with the headaches of writing a networking layer for their app. Integrating ZeroTier into your application will prevent you from having to figure out how to do the complex networking that your app might require. It will provide you with a secure, easy-to-use, P2P connectivity solution.

##### Use in Mobile Apps
If you're writing a game and you'd like to give it the ability talk to other instances in a secure and fast manner, you can embed our library into your app and choose from a couple different APIs. If you're developing for iOS, you'll want to add our [iOS Framework](https://github.com/zerotier/ZeroTierSDK/tree/master/integrations/apple/example_app/iOS) to your project. If you're developing for Android, you'll need to add our [JNI library](https://github.com/zerotier/ZeroTierSDK/tree/master/integrations/android) to your project. Once your app starts up, a separate thread will start which contains the ZeroTier service and a network stack dedicated entirely to your app. Each of these integrations give your app two main options for talking over a ZeroTier network. The first is a "direct call" which is means you'll call functions such as `zt_socket(), zt_connect(), ...` which are reimplementations of the traditional [socket API](https://en.wikipedia.org/wiki/Berkeley_sockets). Alternatively we provide a SOCKS5 proxy server in a separate thread which you can turn on via `zt_start_proxy_server(...)`. And if you've already implemented your networking layer using the traditional socket API and you aren't using third-party libraries that need to make network calls, you can `zt_start_intercept()`. The intercept will essentially hijack your network calls and route them to our reimplementations. This has the advantage that you literally don't have to change a single line of your networking code to use ZeroTier. (NOTE, the intercept is *not* available on Android).

##### Use in Desktop Apps
Desktop apps are a bit easier than mobile apps to integrate with ZeroTier and you'll have even more API options. The exact details vary slightly among the various [platforms/OSes we support](https://github.com/zerotier/ZeroTierSDK/tree/master/integrations), but generally you can either link the ZeroTier library into your application (same as mobile), you can use the built-in SOCKS5 proxy (same as mobile), you can use the intercept (so far tested on OSX, Linux), or you can use `LD_PRELOAD` to dynamically load just the intercept into each of your apps and all will talk to a single `zerotier-sdk-service` running on the local host. This last option is a good option if you don't have access to the source code of the application and thus cannot use the direct call or SOCKS5 proxy APIs.

***







#### I want technical details on how this all works under the hood.
 - Cool. Read [this](technical.md) and let us know [here](zerotier.com/community/) if you have any questions.

***







#### I don't have access to my an app's source but I still want to embed ZeroTier into it. Is this possible?
 - Yes! Since you can't build your app with our library you'll have to use the **Intercept** mode. This means that you will dynamically-load our library at runtime and it'll connect to the locally-running ZeroTier service and function just the same.

***






#### How do switch network stacks?

We currently provide a driver for [picoTCP](http://www.picotcp.com/) and [lwIP](http://savannah.nongnu.org/projects/lwip/), and we recommend their useage in that order. Each one has its own pros and cons, if you experience strange behavior it might be worth it to test your app on a different stack. Use `SDK_PICOTCP=1` or `SDK_LWIP=1`. For more info, see: [Network Stacks](network_stacks.md)

***






#### Suitable for games?

Yes. We think this solution is well suited for low-latency multiplayer games where reliability and ease of use are important.

The ZeroTier protocol is inherently P2P and only falls back to a relay in the event that your direct link is interrupted. It's in our best interest to automatically find the quickest route for your data and to *not* handle your data. This has the obvious benefits of reduced latency for your game, but also provides you better security and control of your data and reduces our costs. It seems non-sensical to do it any other way. ZeroTier is not a "cloud" that you send all of your data to.

We've just begun work on a native [Unity 3D](https://unity3d.com/) plugin to enable your Unity game to communicate over ZeroTier networks. You can check it out [here](../integrations/Unity3D)

***






#### Embedded Applications / IoT

We foresee the largest application of the ZeroTier SDK to be embedded devices that require lightweight, efficient and reliable networking layers that are also secure and effortless to provision. We've specifically engineered the core service and the API library to be as lightweight and portable as possible. We'd like to see people retake control of their data and security by skipping the the "cloud" without adding complexity.

***






#### Controlling traffic?

The SDK's interception of network calls is currently all or nothing. If engaged, the intercept library intercepts all network I/O calls and redirects them through the new path. A network-containerized application cannot communicate over the regular network connection of its host or container or with anything else except other hosts on its ZeroTier virtual LAN. Support for optional "fall-through" to the host IP stack for outgoing connections outside the virtual network and for gateway routes within the virtual network is planned. (It will be optional since in some cases total network isolation might be considered a nice security feature.)

***




