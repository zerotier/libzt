Notes / Limitations
====

## Limitations and Compatibility

The beta version of the SDK **only supports IPv4**. There is no IPv6 support and no support for ICMP (or RAW sockets). That means network-containerizing *ping* won't work.

The virtual TCP/IP stack will respond to *incoming* ICMP ECHO requests, which means that you can ping it from another host on the same ZeroTier virtual network. This is useful for testing.


#### Controlling traffic

**Network Containers are currently all or nothing.** If engaged, the intercept library intercepts all network I/O calls and redirects them through the new path. A network-containerized application cannot communicate over the regular network connection of its host or container or with anything else except other hosts on its ZeroTier virtual LAN. Support for optional "fall-through" to the host IP stack for outgoing connections outside the virtual network and for gateway routes within the virtual network is planned. (It will be optional since in some cases total network isolation might be considered a nice security feature.)