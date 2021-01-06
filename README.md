# ZeroTier SDK
Connect physical devices, virtual devices, and application instances as if everything is on a single LAN.
***

The ZeroTier SDK brings your network into user-space. We've paired our network hypervisor core with a network stack ([lwIP](https://savannah.nongnu.org/projects/lwip/)) to provide your application with an exclusive and private virtual network interface. All traffic on this interface is end-to-end encrypted between each peer and we provide an easy-to-use socket interface derived from [Berkeley Sockets](https://en.wikipedia.org/wiki/Berkeley_sockets). Since we aren't using the kernel's network stack that means, no drivers, no root, and no host configuration requirements. For a more in-depth discussion on the technical side of ZeroTier, check out our [Manual](https://www.zerotier.com/manual.shtml). For troubleshooting advice see our [Knowledgebase](https://zerotier.atlassian.net/wiki/spaces/SD/overview). If you need further assistance, create an account at [my.zerotier.com](https://my.zerotier.com) and join our community of users and professionals.

Downloads: [download.zerotier.com/dist/sdk](https://download.zerotier.com/dist/sdk)

<div style="page-break-after: always;"></div>

## Building on Linux, macOS
*Requires [CMake](https://cmake.org/download/), [Clang](https://releases.llvm.org/download.html) is recommended*
```
make update && make patch && make host_release CC=clang CXX=clang++
```

## Building on Windows
*Requires [CMake](https://cmake.org/download/) and [PowerShell](https://github.com/powershell/powershell)*

```
. ./dist.ps1
Build-Library -BuildType "Release" -Arch "Win32|x64|ARM|ARM64" -LanguageBinding "none|csharp"
```

*Note: To build both `release` and `debug` libraries for only your host's architecture use `make host`. Or optionally `make host_release` for release only. To build everything including things like iOS frameworks, Android packages, etc, use `make all`. Possible build targets can be seen by using `make list`. Resultant libraries will be placed in `./lib`, test and example programs will be placed in `./bin`*

Typical build output:

```
lib
├── release
|    └── linux-x86_64
|       ├── libzt.a
|       └── libzt.so
└── debug
    └── ...
bin
└── release
    └── linux-x86_64
        ├── client
        └── server
```

Example linking step:

```
clang++ -o yourApp yourApp.cpp -L./lib/release/linux-x86_64/ -lzt; ./yourApp
```

<div style="page-break-after: always;"></div>

## Starting ZeroTier

The next few sections explain how to use the network control interface portion of the API. These functions are non-blocking and will return an error code specified in the [Error Handling](#error-handling) section and will result in the generation of callback events detailed in the [Event Handling](#event-handling) section. It is your responsibility to handle these events. To start the service, simply call:

`zts_start(char *path, void (*userCallbackFunc)(struct zts_callback_msg*), int port)`

At this stage, if a cryptographic identity for this node does not already exist on your local storage medium, it will generate a new one and store it, the node's address (commonly referred to as `nodeId`) will be derived from this identity and will be presented to you upon receiving the `ZTS_EVENT_NODE_ONLINE` shown below. The first argument `path` is a path where you will direct ZeroTier to store its automatically-generated cryptographic identity files (`identity.public` and `identity.secret`), these files are your keys to communicating on the network. Keep them safe and keep them unique. If any two nodes are online using the same identities you will have a bad time. The second argument `userCallbackFunc` is a function that you specify to handle all generated events for the life of your program, see below:

```
#include "ZeroTierSockets.h"

...

bool networkReady = false;

void myZeroTierEventCallback(struct zts_callback_msg *msg)
{
    if (msg->eventCode == ZTS_EVENT_NODE_ONLINE) {
        printf("ZTS_EVENT_NODE_ONLINE, nodeId=%llx\n", msg->node->address);
        networkReady = true;
    }
    ...
}

int main()
{
    zts_start("configPath", &myZeroTierEventCallback, 9994);
    uint64_t nwid = 0x0123456789abcdef;
    while (!networkReady) { sleep(1); }
    zts_join(nwid);
    int fd = zts_socket(ZTS_AF_INET, ZTS_SOCK_STREAM, 0);
    ...
    return 0;
}

```

For more complete examples see `./examples/`

<div style="page-break-after: always;"></div>

After calling `zts_start()` you will receive one or more events specified in the [Node Events](#node-events) section. After receiving `ZTS_EVENT_NODE_ONLINE` you will be allowed to join or leave networks. You must authorize the node ID provided by the this callback event to join your network. This can be done manually or via our [Web API](https://my.zerotier.com/help/api). Note however that if you are using an Ad-hoc network, it has no controller and therefore requires no authorization.

At the end of your program or when no more network activity is anticipated, the user application can shut down the service with `zts_stop()`. However, it is safe to leave the service running in the background indefinitely as it doesn't consume much memory or CPU while at idle. `zts_stop()` is a non-blocking call and will itself issue a series of events indicating that various aspects of the ZeroTier service have successfully shut down.

It is worth noting that while `zts_stop()` will stop the service, the user-space network stack will continue operating in a headless hibernation mode. This is intended behavior due to the fact that the network stack we've chosen doesn't currently support the notion of shutdown since it was initially designed for embedded applications that are simply switched off. If you do need a way to shut everything down and free all resources you can call `zts_free()`, but please note that calling this function will prevent all subsequent `zts_start()` calls from succeeding and will require a full application restart if you want to run the service again. The events `ZTS_EVENT_NODE_ONLINE` and `ZTS_EVENT_NODE_OFFLINE` can be seen periodically throughout the lifetime of your application depending on the reliability of your underlying network link, these events are lagging indicators and are typically only triggered every thirty (30) seconds.

Lastly, the function `zts_restart()` is provided as a way to restart the ZeroTier service along with all of its virtual interfaces. The network stack will remain online and undisturbed during this call. Note that this call will temporarily block until the service has fully shut down, then will return and you may then watch for the appropriate startup callbacks mentioned above.

<div style="page-break-after: always;"></div>

## Joining a network

Joining a ZeroTier virtual network is as easy as calling `zts_join(uint64_t networkId)`. Similarly there is a `zts_leave(uint64_t networkId)`. Note that `zts_start()` must be called and a `ZTS_EVENT_NODE_ONLINE` event must have been received before these calls will succeed. After calling `zts_join()` any one of the events detailed in the [Network Events](#network-events) section may be generated.

<div style="page-break-after: always;"></div>

## Connecting and communicating with peers

Creating a standard socket connection generally works the same as it would using an ordinary socket interface, however with ZeroTier there is a subtle difference in how connections are established which may cause confusion. Since ZeroTier employs transport-triggered link provisioning a direct connection between peers will not exist until contact has been attempted by at least one peer. During this time before a direct link is available traffic will be handled via our free relay service. The provisioning of this direct link usually only takes a couple of seconds but it is important to understand that if you attempt something like s `zts_connect(...)` call during this time it may fail due to packet loss. Therefore it is advised to repeatedly call `zts_connect(...)` until it succeeds and to wait to send additional traffic until `ZTS_EVENT_PEER_DIRECT` has been received for the peer you are attempting to communicate with. All of the above is optional, but it will improve your experience.

`tl;dr: Try a few times and wait a few seconds`

As a mitigation for the above behavior, ZeroTier will by default cache details about how to contact a peer in the `peers.d` subdirectory of the config path you passed to `zts_start(...)`. In scenarios where paths do not often change, this can almost completely eliminate the issue and will make connections nearly instantaneous. If however you do not wish to cache these details you can disable it via `zts_set_peer_caching(false)`.

<div style="page-break-after: always;"></div>

## Event handling

As mentioned in previous sections, the control API works by use of non-blocking calls and the generation of a few dozen different event types. Depending on the type of event there may be additional contextual information attached to the `zts_callback_msg` object that you can use. This contextual information will be housed in one of the following structures which are defined in `include/ZeroTierSockets.h`:

```
struct zts_callback_msg
{
    int eventCode;
    struct zts_node_details *node;
    struct zts_network_details *network;
    struct zts_netif_details *netif;
    struct zts_virtual_network_route *route;
    struct zts_peer_details *peer;
    struct zts_addr_details *addr;
};
```

Here's an example of a callback function:

```
void myZeroTierEventCallback(struct zts_callback_msg *msg)
{
    if (msg->eventCode == ZTS_EVENT_NODE_ONLINE) {
        printf("ZTS_EVENT_NODE_ONLINE, node=%llx\n", msg->node->address);
        // You can join networks now!
    }
}
```

In this callback function you can perform additional non-blocking API calls or other work. While not returning control to the service isn't forbidden (the event messages are generated by a separate thread) it is recommended that you return control as soon as possible as not returning will prevent the user application from receiving additional callback event messages which may be time-sensitive.

<div style="page-break-after: always;"></div>

A typical ordering of messages may look like the following:

```
...
ZTS_EVENT_NODE_ONLINE       // Your node is ready to be used.
ZTS_EVENT_ADDR_ADDED_IP4    // Your node received an IP address assignment on a given network.
ZTS_EVENT_NETWORK_UPDATE    // Something about a network changed.
ZTS_EVENT_NETWORK_READY_IP4 // Your node has joined a network, has an address, and can send/receive traffic.
ZTS_EVENT_PEER_RELAY        // A peer was discovered but no direct path exists (yet.)
...
ZTS_EVENT_PEER_DIRECT       // One or more direct paths to a peer were discovered.
```

## Node Events

Accessible via `msg->node` as a `zts_node_details` object, this message type will contain information about the status of your node. *Possible values of `msg->eventCode`:*

```
ZTS_EVENT_NODE_OFFLINE             // Your node is offline.
ZTS_EVENT_NODE_ONLINE              // Your node is online and ready to communicate!
ZTS_EVENT_NODE_DOWN                // The node is down (for any reason.)
ZTS_EVENT_NODE_IDENTITY_COLLISION  // There is another node with the same identity causing a conflict.
ZTS_EVENT_NODE_UNRECOVERABLE_ERROR // Something went wrong internally.
ZTS_EVENT_NODE_NORMAL_TERMINATION  // Your node has terminated.
```

*Example contents of `msg->node`:*

```
id            : f746d550dd
version       : 1.4.6
primaryPort   : 9995
secondaryPort : 0
```

## Network Events

Accessible via `msg->network` as a `zts_network_details` object, this message type will contain information about the status of a particular network your node has joined. *Possible values of `msg->eventCode`:*

```
ZTS_EVENT_NETWORK_NOT_FOUND      // The network does not exist. The provided networkID may be incorrect.
ZTS_EVENT_NETWORK_CLIENT_TOO_OLD // This client is too old.
ZTS_EVENT_NETWORK_REQ_CONFIG     // Waiting for network config, this might take a few seconds.
ZTS_EVENT_NETWORK_OK             // Node successfully joined.
ZTS_EVENT_NETWORK_ACCESS_DENIED  // The network is private. Your node requires authorization.
ZTS_EVENT_NETWORK_READY_IP4      // Your node successfully received an IPv4 address.
ZTS_EVENT_NETWORK_READY_IP6      // Your node successfully received an IPv6 address.
ZTS_EVENT_NETWORK_DOWN           // For some reason the network is no longer available.
ZTS_EVENT_NETWORK_UPDATE         // The network's config has changed: mtu, name, managed route, etc.
```

*Example contents of `msg->network`:*

```
nwid                       : 8bd712bf36bdae5f
mac                        : ae53fa031fcf
name                       : cranky_hayes
type                       : 0
mtu                        : 2800
dhcp                       : 0
bridge                     : 0
broadcastEnabled           : 1
portError                  : 0
netconfRevision            : 34
routeCount                 : 1
multicastSubscriptionCount : 1
- mac=ffffffffffff, adi=ac1b2561
addresses:
- FC5D:69B6:E0F7:46D5:50DD::1
- 172.27.37.97
routes:
- target : 172.27.0.0
- via    : 0.0.0.0
  - flags  : 0
  - metric : 0
```

<div style="page-break-after: always;"></div>

## Peer Events

Accessible via `msg->peer` as a `zts_peer_details` object, this message type will contain information about a peer that was discovered by your node. These events are triggered when the reachability status of a peer has changed. *Possible values of `msg->eventCode`:*

```
ZTS_EVENT_PEER_DIRECT          // At least one direct path to this peer is known.
ZTS_EVENT_PEER_RELAY           // No direct path to this peer is known. It will be relayed, (high packet loss and jitter.)
ZTS_EVENT_PEER_UNREACHABLE     // Peer is not reachable by any means.
ZTS_EVENT_PEER_PATH_DISCOVERED // A new direct path to this peer has been discovered.
ZTS_EVENT_PEER_PATH_DEAD       // A direct path to this peer has expired.
```

*Example contents of `msg->peer`:*

```
peer      : a747d5502d
role      : 0
latency   : 4
version   : 1.4.6
pathCount : 2
 - 172.27.37.97
 - F75D:69B6:E0C7:47D5:51DB::1
```

## Address Events

Accessible via `msg->addr` as a `zts_addr_details` object, this message type will contain information about addresses assign to your node on a particular network. The information contained in these events is also available via `ZTS_EVENT_NETWORK_UPDATE` events. *Possible values of `msg->eventCode`:*

```
ZTS_EVENT_ADDR_ADDED_IP4   // A new IPv4 address was assigned to your node on the indicated network.
ZTS_EVENT_ADDR_REMOVED_IP4 // An IPv4 address assignment to your node was removed on the indicated network.
ZTS_EVENT_ADDR_ADDED_IP6   // A new IPv6 address was assigned to your node on the indicated network.
ZTS_EVENT_ADDR_REMOVED_IP6 // An IPv6 address assignment to your node was removed on the indicated network.
```

*Example contents of `msg->addr`:*

```
nwid : a747d5502d
addr : 172.27.37.97
```

<div style="page-break-after: always;"></div>

## Error handling

Calling a `zts_*` function will result in one of the following return codes. Only when `ZTS_ERR` is returned will `zts_errno` be set. Its values closely mirror those used in standard socket interfaces and are defined in `include/ZeroTierSockets.h`.

```
ZTS_ERR_OK        // No error
ZTS_ERR_SOCKET    // Socket error (see zts_errno for more information)
ZTS_ERR_SERVICE   // General ZeroTier internal error. Maybe you called something out of order?
ZTS_ERR_ARG       // An argument provided is invalid.
ZTS_ERR_NO_RESULT // Call succeeded but no result was available. Not necessarily an error.
ZTS_ERR_GENERAL   // General internal failure. Consider filing a bug report.
```

*NOTE: For Android/Java (or similar) which use JNI, the socket API's error codes are negative values encoded in the return values of function calls*
*NOTE: For protocol-level errors (such as dropped packets) or internal network stack errors, see the section `Statistics`*

<div style="page-break-after: always;"></div>

## Common pitfalls

  - If you have started a node but have not received a `ZTS_EVENT_NODE_ONLINE`:
    - You may need to view our [Router Config Tips](https://zerotier.atlassian.net/wiki/spaces/SD/pages/6815768/Router+Configuration+Tips) knowledgebase article. Sometimes this is due to firewall/NAT settings.

  - If you have received a `ZTS_EVENT_NODE_ONLINE` event and attempted to join a network but do not see your node ID in the network panel on [my.zerotier.com](my.zerotier.com) after some time:
    - You may have typed in your network ID incorrectly.
    - Used an improper integer representation for your network ID (e.g. `int` instead of `uint64_t`).

 - If you are unable to reliably connect to peers:
    - You should first read the section on [Connecting and communicating with peers](#connecting-and-communicating-with-peers).
    - If the previous step doesn't help move onto our knowledgebase article [Router Config Tips](https://zerotier.atlassian.net/wiki/spaces/SD/pages/6815768/Router+Configuration+Tips). Sometimes this can be a transport-triggered link issue, and sometimes it can be a firewall/NAT issue.

 - API calls seem to fail in nonsensical ways and you're tearing your hair out:
    - Be sure to read and understand the [API compatibility with host OS](#api-compatibility-with-host-os) section.
    - See the [Debugging](#debugging) section for more advice.

<div style="page-break-after: always;"></div>

## API compatibility with host OS

Since libzt re-implements a socket interface likely very similar to your host OS's own interface it may be tempting to mix and match host OS structures and functions with those of libzt. This may work on occasion, but you are tempting fate. Here are a few important guidelines:

If you are calling a `zts_*` function, use the appropriate `ZTS_*` constants:
```
zts_socket(ZTS_AF_INET6, ZTS_SOCK_DGRAM, 0); (CORRECT)
zts_socket(AF_INET6, SOCK_DGRAM, 0);         (INCORRECT)
```

If you are calling a `zts_*` function, use the appropriate `zts_*` structure:
```
struct zts_sockaddr_in in4;  <------ Note the zts_ prefix
    ...
zts_bind(fd, (struct sockaddr *)&in4, sizeof(struct zts_sockaddr_in)) < 0)
```

If you are calling a host OS function, use your host OS's constants (and structures!):
```
inet_ntop(AF_INET6, &(in6->sin6_addr), ...);         (CORRECT)
inet_ntop(ZTS_AF_INET6, &(in6->sin6_addr), ...);     (INCORRECT)
zts_inet_ntop(ZTS_AF_INET6, &(in6->sin6_addr), ...); (CORRECT)
```

If you are calling a host OS function but passing a `zts_*` structure, this can work sometimes but you should take care to pass the correct host OS constants:
```
struct zts_sockaddr_in6 in6;
    ...
inet_ntop(AF_INET6, &(in6->sin6_addr), dstStr, INET6_ADDRSTRLEN);
```

<div style="page-break-after: always;"></div>

## Thread model (advanced)

Both the **socket** and **control** interfaces are thread-safe but are implemented differently. The socket interface is implemented using a relatively performant core locking mechanism in lwIP. This can be disabled if you know what you're doing. The control interface is implemented by a single coarse-grained lock. This lock is not a performance bottleneck since it only applies to functions that manipulate the ZeroTier service and are called seldomly. Callback events are generated by a separate thread and are independent from the rest of the API's internal locking mechanism. Not returning from a callback event won't impact the rest of the API but it will prevent your application from receiving future events so it is in your application's best interest to perform as little work as possible in the callback function and promptly return control back to ZeroTier.

*Note: Internally, `libzt` will spawn a number of threads for various purposes: a thread for the core service, a thread for the network stack, a low priority thread to process callback events, and a thread for each network joined. The vast majority of work is performed by the core service and stack threads.*

<div style="page-break-after: always;"></div>

## Debugging

If you're experiencing odd behavior or something that looks like a bug I would suggest first reading and understanding the following sections:

* [Common pitfalls](#common-pitfalls)
* [API compatibility with host OS](#api-compatibility-with-host-os)
* [Thread model](#thread-model)

If the information in those sections hasn't helped, there are a couple of ways to get debug traces out of various parts of the library.

1) Build the library in debug mode with `make host_debug`. This will prevent the stripping of debug symbols from the library and will enable basic output traces from libzt.

2) If you believe your problem is in the network stack you can manually enable debug traces for individual modules in `src/lwipopts.h`. Toggle the `*_DEBUG` types from `LWIP_DBG_OFF` to `LWIP_DBG_ON`. And then rebuild. This will come with a significant performance cost.

3) Enabling network stack statistics. This is useful if you want to monitor the stack's receipt and handling of traffic as well as internal things like memory allocations and cache hits. Protocol and service statistics are available in debug builds of `libzt`. These statistics are detailed fully in the section of `include/ZeroTierSockets.h` that is guarded by `LWIP_STATS`.

  ```
  struct zts_stats_proto stats;
  if (zts_get_protocol_stats(ZTS_STATS_PROTOCOL_ICMP, &stats) == ZTS_ERR_OK) {
      printf("icmp.recv=%d\n", stats.recv); // Count of received pings
  }
  if (zts_get_protocol_stats(ZTS_STATS_PROTOCOL_TCP, &stats) == ZTS_ERR_OK) {
      printf("tcp.drop=%d\n", stats.drop); // Count of dropped TCP packets
  }
  ```

4) There are a series of additional events which can signal whether the network stack or its virtual network interfaces have been set up properly. See `ZTS_EVENT_STACK_*` and `ZTS_EVENT_NETIF_*`.

<div style="page-break-after: always;"></div>

## Licensing

ZeroTier is licensed under the BSL version 1.1. See [LICENSE.txt](./LICENSE.txt) and the ZeroTier pricing page for details. ZeroTier is free to use internally in businesses and academic institutions and for non-commercial purposes. Certain types of commercial use such as building closed-source apps and devices based on ZeroTier or offering ZeroTier network controllers and network management as a SaaS service require a commercial license.

A small amount of third party code is also included in ZeroTier and is not subject to our BSL license. See [AUTHORS.md](ext/ZeroTierOne/AUTHORS.md) for a list of third party code, where it is included, and the licenses that apply to it. All of the third party code in ZeroTier is liberally licensed (MIT, BSD, Apache, public domain, etc.). If you want a commercial license to use the ZeroTier SDK in your product contact us directly via [contact@zerotier.com](mailto:contact@zerotier.com)

