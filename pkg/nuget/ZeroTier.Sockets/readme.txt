-------------------------------------------------------------------------------
      ZeroTier.Sockets (libzt) --- Encrypted P2P SD-WAN networking layer
-------------------------------------------------------------------------------

With ZeroTier's SDK you can embed a ZeroTier node into your application and
communicate with other ZeroTier nodes (or non-ZeroTier devices bridged onto a
ZeroTier network) securely within your own software-defined virtual network.

C# API:

 - Guide: https://github.com/zerotier/libzt/tree/master/examples/csharp

This package presents a managed .NET-style Socket API. It is designed as
a drop-in replacement for System.Net.Sockets. This is the easiest and most
idiomatic way to use ZeroTier in a C# application.

Self-hosting:

ZeroTier operates a hosted service (https://my.zerotier.com) that lets you
manage your virtual networks. If however you would like to self-host we make
that possible too:

 - https://github.com/zerotier/ZeroTierOne/tree/master/controller

Other API options:

We offer two other API layers depending on your use case. The first is a lower-
level BSD-style socket API. This API is similar to BSD-style sockets
(zts_bsd_socket(), zts_bsd_listen(), zts_bsd_bind(), etc.) The second is a Highly-
performant virtual Ethernet layer. It can be used for any transport protocol
and is only recommended for those who have advanced or specialty applications.

Bug?

You should let us know so we can fix it. Please open a github issue here:

 - https://www.github.com/zerotier/libzt

-------------------------------------------------------------------------------
 LINKS
-------------------------------------------------------------------------------

 - Documentation and bug reports: https://www.github.com/zerotier/libzt
 - Website: https://www.zerotier.com
 - Community: https://discuss.zerotier.com
