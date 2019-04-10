<p align="center">
  <img src="https://raw.githubusercontent.com/zerotier/ZeroTierOne/master/artwork/ZeroTierIcon.png" width="128" height="128" />
  <br>
  ZeroTier SDK
</p>

The ZeroTier SDK is composed of two libraries: `libztcore` which is the  platform-agnostic network hypervisor, and `libzt` which is the network hypervisor paired with a userspace network stack. `libzt` is a superset of `libztcore` and is distinguished by the fact that it exposes a standard [socket API](https://en.wikipedia.org/wiki/Berkeley_sockets) and simple network control API. The full source for these products can be found at [github.com/zerotier/libzt](https://github.com/zerotier/libzt) for the SDK and [github.com/zerotier/ZeroTierOne](https://github.com/zerotier/ZeroTierOne) for the desktop client. With these libraries the network stack and virtual link are exclusive to your app and traffic is fully encrypted via the [Salsa20](https://en.wikipedia.org/wiki/Salsa20) cipher. For a more in-depth discussion on the technical side of ZeroTier, check out our [Manual](https://www.zerotier.com/manual.shtml?pk_campaign=github_libzt)

The ZeroTier source code is open source and is licensed under the GNU GPL v3 (not LGPL). If you'd like to embed it in a closed-source commercial product or appliance, please e-mail contact@zerotier.com to discuss commercial licensing. Otherwise it can be used for free.

<div style="page-break-after: always;"></div>

# Getting started

Before we dive into the technicals, the first thing to understand is that there are two API families to choose from and each is intended for a very different purpose:

`libzt` Intended for convenience and simplicity, derives from [Berkley Sockets](https://en.wikipedia.org/wiki/Berkeley_sockets).
 - socket API: `zts_socket(), zts_connect(), zts_bind(), ...`
 - control API: `zts_start(), zts_join(), zts_leave(), ....` 

`libztcore` Intended for raw performance. If your goal is simply moving frames as quickly as possible and you're willing to put in some extra work, is what you're looking. The API is described in `include/ZeroTierOne.h`. For an example of how this API is used, see the living documentation that is `src/Service.cpp`.
 - core API: `ZT_VirtualNetworkFrameFunction(), ZT_WirePacketSendFunction(), ...`

*NOTE: The remainder of this document will focus on the usage of the socket and control C API exposed in `include/ZeroTier.h` for `libzt`. For more information on the `libztcore` API see `include/ZeroTierOne.h`. We also provide bindings, frameworks, and packages for other languages and platforms in the `ports` directory and example applications using them in the `examples` directory.*

<div style="page-break-after: always;"></div>

# Starting the service

The next few sections explain how to use the control API. These functions are non-blocking and will return an error code specified in `include/ZeroTierConstants.h` and will result in the generation of callback events. It is your responsibility to handle these events.

To start the service, simply call:

`zts_start(char *path, void (*userCallbackFunc)(struct zts_callback_msg*), int port)`

At this stage, if a cryptographic identity for this node does not already exist, it will generate a new one and store it on disk, the node's address (commonly referred to as `nodeId`) will be derived from this identity and will be presented to you upon receiving the `ZTS_EVENT_NODE_ONLINE` shown below.

*NOTE: The first argument `path` is a path where you will allow ZeroTier to store its automatically-generated cryptographic identity files (`identity.public` and `identity.secret`), these files are your keys to communicating on the network. Keep them safe and keep them unique. If any two nodes are online using the same identities you will have a bad time. The second argument `userCallbackFunc` is a function that you specify to handle all generated events for the life of your program (see below):*

```
void myZeroTierEventCallback(struct zts_callback_msg *msg)
{
    if (msg->eventCode == ZTS_EVENT_NODE_ONLINE) {
        printf("ZTS_EVENT_NODE_ONLINE, nodeId=%llx\n", msg->node->address);
        // You can join networks now!
    }
    // ...
}
```

After calling `zts_start()` you will receive one or more of the following events:

```
ZTS_EVENT_NODE_OFFLINE
ZTS_EVENT_NODE_ONLINE
ZTS_EVENT_NODE_DOWN
ZTS_EVENT_NODE_IDENTITY_COLLISION
ZTS_EVENT_NODE_UNRECOVERABLE_ERROR
ZTS_EVENT_NODE_NORMAL_TERMINATION
```

After receiving `ZTS_EVENT_NODE_ONLINE` you will be allowed to join or leave networks.

At the end of your program or when no more network activity is anticipated, the user application can shut down the service with `zts_stop()`. However, it is safe to leave the service running in the background indefinitely as it doesn't consume much memory or CPU while at idle. `zts_stop()` is a non-blocking call and will itself issue a series of events indicating that various aspects of the ZeroTier service have successfully shut down.

It is worth noting that while `zts_stop()` will stop the service, but the user-space network stack will continue operating in a headless hibernation mode. This is intended behavior due to the fact that the network stack we've chosen doesn't currently support the notion of shutdown since it was initially designed for embedded applications that are simply switched off. If you do need a way to shut everything down and free all resources you can call `zts_free()`, but please note that calling this function will prevent all subsequent `zts_start()` calls from succeeding and will require a full application restart if you want to run the service again. The events `ZTS_EVENT_NODE_ONLINE` and `ZTS_EVENT_NODE_OFFLINE` can be seen periodically throughout the lifetime of your application depending on the reliability of your underlying network link, these events are lagging indicators and are typically only triggered every thirty (30) seconds.

Lastly, the function `zts_restart()` is provided as a way to restart the ZeroTier service along with all of its virtual interfaces. The network stack will remain online and undisturbed during this call. Note that this call will temporarily block until the service has fully shut down, then will return and you may then watch for the appropriate startup callbacks mentioned above.

<div style="page-break-after: always;"></div>

# Joining a network

Joining a ZeroTier virtual network is as easy as calling `zts_join(uint64_t networkId)`. Similarly there is a `zts_leave(uint64_t networkId)`. Note that `zts_start()` must be called and a `ZTS_EVENT_NODE_ONLINE` event must be received before these calls will succeed. After calling `zts_join()` any one of the following events may be generated:

```
ZTS_EVENT_NETWORK_NOT_FOUND
ZTS_EVENT_NETWORK_CLIENT_TOO_OLD
ZTS_EVENT_NETWORK_REQUESTING_CONFIG
ZTS_EVENT_NETWORK_OK
ZTS_EVENT_NETWORK_ACCESS_DENIED
ZTS_EVENT_NETWORK_READY_IP4
ZTS_EVENT_NETWORK_READY_IP6
ZTS_EVENT_NETWORK_DOWN
```

`ZTS_EVENT_NETWORK_READY_IP4` and `ZTS_EVENT_NETWORK_READY_IP6` are combinations of a few different events. They signal that the network was found, joined successfully, an IP address was assigned and the network stack's interface is ready to process traffic of the indicated type. After this point you should be able to communicate with peers on the network.

<div style="page-break-after: always;"></div>

# Communicating with peers

After successfully starting the service and joining a network, communicating with other nodes (peers) on that network is as easy as it would ordinarily be without ZeroTier. However, one thing to be aware of is the difference between relay and P2P modes. In the event that a direct connection cannot be established between your nodes, ZeroTier offers a free relaying service, this means that your nodes are reachable almost instantaneously but at a temporary performance cost. One should wait to send large amounts of traffic until a `ZTS_EVENT_PEER_P2P` is received for the node that you're interested in talking to. This event usually only takes a few seconds to appear after data has initially been sent. Similarly if after some time ZeroTier determines that a previously known path to one of your nodes is no longer available you will see a `ZTS_EVENT_PEER_RELAY` event.

One can use `zts_get_peer_status(uint64_t peerId)` to query the current reachability state of another node. This function will actually **return** the previously mentioned event values, plus an additional one called `ZTS_EVENT_PEER_UNREACHABLE` if no known direct path exists between the calling node and the remote node.

<div style="page-break-after: always;"></div>

# Handling events

As mentioned in previous sections, the control API works by use of non-blocking calls and the generation of a few dozen different event types. Depending on the type of event there may be additional contextual information attached to the `zts_callback_msg` object that you can use. This contextual information will be housed in one of the following structures which are defined in `include/ZeroTier.h`:

```
struct zts_callback_msg
{
    int eventCode;
    struct zts_node_details *node;
    struct zts_network_details *network;
    struct zts_netif_details *netif;
    struct zts_virtual_network_route *route;
    struct zts_physical_path *path;
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
ZTS_EVENT_NETIF_UP --- network=a09acf023be465c1, mac=73b7abcfc207, mtu=10000
ZTS_EVENT_ADDR_NEW_IP4 --- addr=11.7.7.184 (on network=a09acf023be465c1)
ZTS_EVENT_ADDR_NEW_IP6 --- addr=fda0:9acf:233:e4b0:7099:9309:4c9b:c3c7 (on network=a09acf023be465c1)
ZTS_EVENT_NODE_ONLINE, node=c4c7ba3cf
ZTS_EVENT_NETWORK_READY_IP4 --- network=a09acf023be465c1
ZTS_EVENT_NETWORK_READY_IP6 --- network=a09acf023be465c1
ZTS_EVENT_PEER_P2P --- node=74d0f5e89d
ZTS_EVENT_PEER_P2P --- node=9d219039f3
ZTS_EVENT_PEER_P2P --- node=a09acf0233
```

## Node Events

These events pertain to the state of the current node. This message type will arrive with a `zts_node_details` object accessible via `msg->node`. Additionally, one can query the status of the node with `zts_get_node_status()`, this will **return** the status as an integer value only.

```
ZTS_EVENT_NODE_OFFLINE
ZTS_EVENT_NODE_ONLINE
ZTS_EVENT_NODE_DOWN
ZTS_EVENT_NODE_IDENTITY_COLLISION
ZTS_EVENT_NODE_UNRECOVERABLE_ERROR
ZTS_EVENT_NODE_NORMAL_TERMINATION
```


## Network Events

These events pertain to the state of the indicated network. This event type will arrive with a `zts_network_details` object accessible via `msg->network`. If for example you want to know the number of assigned routes for your network you can use `msg->network->num_routes`. Similarly for the MTU, use `msg->network->mtu`. Additionally, one can query the status of the network with `zts_get_network_status(uint64_t networkId)`, this will **return** the status as an integer value only.

```
ZTS_EVENT_NETWORK_NOT_FOUND
ZTS_EVENT_NETWORK_CLIENT_TOO_OLD
ZTS_EVENT_NETWORK_REQUESTING_CONFIG
ZTS_EVENT_NETWORK_OK
ZTS_EVENT_NETWORK_ACCESS_DENIED
ZTS_EVENT_NETWORK_READY_IP4
ZTS_EVENT_NETWORK_READY_IP6
ZTS_EVENT_NETWORK_DOWN
```

## Peer Events

These events are triggered when the reachability status of a peer has changed, this can happen at any time. This event type will arrive with a `zts_peer_details` object for additional context. Additionally, one can query the status of the network with `zts_get_peer_status(uint64_t peerId)`, this will **return** the status as an integer value only.

```
ZTS_EVENT_PEER_P2P
ZTS_EVENT_PEER_RELAY
ZTS_EVENT_PEER_UNREACHABLE
```

## Path Events

These events are triggered when a direct path to a peer has been discovered or is now considered too old to be used. You will see these in conjunction with peer events. This event type will arrive with a `zts_physical_path` object for additional context.

```
ZTS_EVENT_PATH_DISCOVERED
ZTS_EVENT_PATH_ALIVE
ZTS_EVENT_PATH_DEAD
```

## Route Events

This event type will arrive with a `zts_virtual_network_route` object for additional context.

```
ZTS_EVENT_ROUTE_ADDED
ZTS_EVENT_ROUTE_REMOVED
```

## Address Events

These events are triggered when new addresses are assigned to the node on a particular virtual network. This event type will arrive with a `zts_addr_details` object for additional context.

```
ZTS_EVENT_ADDR_ADDED_IP4
ZTS_EVENT_ADDR_REMOVED_IP4
ZTS_EVENT_ADDR_ADDED_IP6
ZTS_EVENT_ADDR_REMOVED_IP6
```
## Network Stack Events (debugging)

These events aren't very important to the application developer but are important for debugging. These signal whether the userspace networking stack was brought up successfully. You can ignore these in most cases. This event type will arrive with no additional contextual information.

```
ZTS_EVENT_STACK_UP
ZTS_EVENT_STACK_DOWN
```

## Netif Events (debugging)

These events aren't very important to the application developer but are important for debugging. These signal whether the userspace networking stack was brought up successfully. You can ignore these in most cases. This event type will arrive with a `zts_netif_details` object for additional context.

```
ZTS_EVENT_NETIF_UP
ZTS_EVENT_NETIF_DOWN
ZTS_EVENT_NETIF_REMOVED
ZTS_EVENT_NETIF_LINK_UP
ZTS_EVENT_NETIF_LINK_DOWN
```
<div style="page-break-after: always;"></div>

# Errors

Just as there are two APIs (socket and control), there are two sets of error codes. The control API (`zts_start()`, `zts_join()`, etc) errors defined in `include/ZeroTierConstants.h` are:

 - `ZTS_ERR_OK`: Everything is ok
 - `ZTS_ERR_INVALID_ARG`: An argument provided by the user application is invalid (e.g. out of range, NULL, etc)
 - `ZTS_ERR_SERVICE`: The service isn't initialized or is for some reason currently unavailable. Try again. 
 - `ZTS_ERR_INVALID_OP`: For some reason this API operation is not permitted or doesn't make sense at this time.
 - `ZTS_ERR_NO_RESULT`: The call succeeded, but no object or relevant result was available
 - `ZTS_ERR_GENERAL`: General internal failure (memory allocation, null reference, etc)

The socket API error codes are defined in `doc/errno.h`

*NOTE: For Android/Java (or similar) which use JNI, the socket API's error codes are negative values*

*NOTE: For protocol-level errors (such as dropped packets) or internal network stack errors, see the section `Statistics`*

<div style="page-break-after: always;"></div>

# Thread model

The control API for `libzt` is thread safe and can be called at any time from any thread. There is a single internal lock guarding access to this API. The socket API is similar in this regard. Callback events are generated by a separate thread and are independent from the rest of the API's internal locking mechanism. Not returning from a callback event won't impact the rest of the API but it will prevent your application from receiving future events so it is in your application's best interest to perform as little work as possible in the callback function and promptly return control back to ZeroTier.

*Note: Internally, `libzt` will spawn a number of threads for various purposes: a thread for the core service, a thread for the network stack, a low priority thread to process callback events, and a thread for each network joined. The vast majority of work is performed by the core service and stack threads.*

<div style="page-break-after: always;"></div>

# Statistics

Protocol and service statistics are available in debug builds of `libzt`. These statistics are detailed fully in the section of `include/ZeroTier.h` that is guarded by `LWIP_STATS`. The protocol constants are defined in `include/ZeroTierConstants.h`. An example usage is as follows:

C++ example:
```
struct zts_stats_proto stats;

// Get count of received pings
if (zts_get_protocol_stats(ZTS_STATS_PROTOCOL_ICMP, &stats) == ZTS_ERR_OK) {
    printf("icmp.recv=%d\n", stats.recv);
}

// Get count of dropped TCP packets
if (zts_get_protocol_stats(ZTS_STATS_PROTOCOL_TCP, &stats) == ZTS_ERR_OK) {
    printf("tcp.drop=%d\n", stats.drop);
}
```

Java Example:

```
import com.zerotier.libzt.ZeroTierProtoStats;

...

// Get received pings
ZeroTierProtoStats stats = new ZeroTierProtoStats();
ZeroTier.get_protocol_stats(ZeroTier.STATS_PROTOCOL_ICMP, stats);
System.out.println("icmp.recv="+stats.recv);
```

<div style="page-break-after: always;"></div>

# Network Controller Mode

The library form of ZeroTier can act as a network controller and in `libzt` this is controlled via the `zts_controller_*` API calls specified in `include/ZeroTier.h`. Currently controller mode is not available in the `iOS` and `macOS` framework builds.

<div style="page-break-after: always;"></div>

# C Example

```
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ZeroTier.h"

bool node_ready = false;
bool network_ready = false;

void myZeroTierEventCallback(struct zts_callback_msg *msg)
{
    switch (msg->eventCode)
    {
        case ZTS_EVENT_NODE_ONLINE:
            printf("ZTS_EVENT_NODE_ONLINE, nodeId=%llx\n", msg->node->address);
            node_ready = true;
            break;
        case ZTS_EVENT_NODE_OFFLINE:
            printf("ZTS_EVENT_NODE_OFFLINE\n");
            node_ready = false;
            break;
        case ZTS_EVENT_NETWORK_READY_IP4:
            printf("ZTS_EVENT_NETWORK_READY_IP4, networkId=%llx\n", msg->network->nwid);
            network_ready = true;
            break;
        case ZTS_EVENT_PEER_P2P:
            printf("ZTS_EVENT_PEER_P2P, nodeId=%llx\n", msg->peer->address);
            break;
        case ZTS_EVENT_PEER_RELAY:
            printf("ZTS_EVENT_PEER_RELAY, nodeId=%llx\n", msg->peer->address);
            break;
        default:
            break;
    }
}

int main() 
{
    char *str = "welcome to the machine";
    char *remoteIp = "11.7.7.223";
    int remotePort = 8082;
    int fd, err = 0;
    struct zts_sockaddr_in addr;	
    addr.sin_family = ZTS_AF_INET;
    addr.sin_addr.s_addr = inet_addr(remoteIp);
    addr.sin_port = htons(remotePort);

    // Set up ZeroTier service and wait for callbacks
    int port = 9994;
    int nwid = 0x0123456789abcdef;
    zts_start("test/path", &myZeroTierEventCallback, port);
    printf("Waiting for node to come online...\n");
    while (!node_ready) { sleep(1); }
    zts_join(nwid);
    printf("Joined virtual network. Requesting configuration...\n");
    while (!network_ready) { sleep(1); }

    // Socket API example
    if ((fd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("error creating socket\n");
    }
    if ((err = zts_connect(fd, (const struct sockaddr *)&addr, sizeof(addr))) < 0) {
        printf("error connecting to remote host\n");
    }
    if ((err = zts_write(fd, str, strlen(str))) < 0) {
        printf("error writing to socket\n");
    }
    zts_close(fd);
    zts_stop();
    return 0;
}
```

<div style="page-break-after: always;"></div>

# Java Example

Starting ZeroTier:

```
MyZeroTierEventListener listener = new MyZeroTierEventListener();
ZeroTier.start(getApplicationContext().getFilesDir() + "/zerotier", listener, myPort);
// Wait for EVENT_NODE_ONLINE
while (listener.isOnline == false) {
    try {
        Thread.sleep(interval);
    } catch (Exception e) { }
}
ZeroTier.join(myNetworkId);
// Wait for EVENT_NETWORK_READY_IP4/6
while (listener.isNetworkReady == false) {
    try {
        Thread.sleep(interval);
    } catch (Exception e) { }
}
// Now you can use the socket API!
```

An example event listener:

```
package com.example.exampleandroidapp;

import com.zerotier.libzt.ZeroTier;
import com.zerotier.libzt.ZeroTierEventListener;
import com.zerotier.libzt.ZeroTierPeerDetails;

public class MyZeroTierEventListener implements ZeroTierEventListener
{
    public void onZeroTierEvent(long id, int eventCode)
    {
        if (eventCode == ZeroTier.EVENT_NODE_ONLINE) {
            System.out.println("EVENT_NODE_ONLINE: nodeId=" + Long.toHexString(ZeroTier.get_node_id()));
            isOnline = true;
        }
        if (eventCode == ZeroTier.EVENT_NODE_OFFLINE) {
            System.out.println("EVENT_NODE_OFFLINE");
        }
        if (eventCode == ZeroTier.EVENT_NETWORK_READY_IP4) {
            System.out.println("ZTS_EVENT_NETWORK_READY_IP4: nwid=" + Long.toHexString(id));
            if (id == myNetworkId) {
                isNetworkReady = true;
            }
        }
        if (eventCode == ZeroTier.EVENT_PEER_P2P) {
            System.out.println("EVENT_PEER_P2P: id=" + Long.toHexString(id));
        }
        if (eventCode == ZeroTier.EVENT_PEER_RELAY) {
            System.out.println("EVENT_PEER_RELAY: id=" + Long.toHexString(id));
        }
        // ...
    }
}
```
