Useful things to know
 =====

### IDENTITIES and AUTHORIZATION:

Upon the first execution of this code, a new identity will be generated and placed in the location given in the first argument to zts_start(path, ...). If you accidentally duplicate the identity files and use them simultaneously in a different node instance **you will experience undefined behavior** and it is likely that nothing will work.

You must authorize the node ID provided by the `ZTS_EVENT_NODE_ONLINE` callback to join your network, otherwise nothing will happen. This can be done manually or via our web API: https://my.zerotier.com/help/api

An exception to the above rule is if you are using an Ad-hoc network, it has no controller and therefore requires no authorization.


### ESTABLISHING A CONNECTION:

Creating a standard socket connection generally works the same as it would using an ordinary socket interface, however with libzt there is a subtle difference in how connections are established which may cause confusion:

The underlying virtual ZT layer creates what are called "transport-triggered links" between nodes. That is, links are not established until an attempt to communicate with a peer has taken place. The side effect is that the first few packets sent from a libzt instance are usually relayed via our free infrastructure and it isn't until a root server has passed contact information to both peers that a direct connection will be established. Therefore, it is required that multiple connection attempts be undertaken when initially communicating with a peer. After a transport-triggered link is established libzt will inform you via `ZTS_EVENT_PEER_DIRECT` for a specific peer ID. No action is required on your part for this callback event.

*Note: In these initial moments before `ZTS_EVENT_PEER_DIRECT` has been received for a specific peer, traffic may be slow, jittery and there may be high packet loss. This will subside within a couple of seconds.*


### ERROR HANDLING:

libzt's API is actually composed of two categories of functions with slightly different error reporting mechanisms.

- Category 1: Control functions (`zts_start`, `zts_join`, `zts_get_peer_status`, etc). Errors returned by these functions can be any of the following:

```
ZTS_ERR_OK            // No error
ZTS_ERR_SOCKET        // Socket error, see zts_errno
ZTS_ERR_SERVICE       // You probably did something at the wrong time
ZTS_ERR_ARG           // Invalid argument
ZTS_ERR_NO_RESULT     // No result (not necessarily an error)
ZTS_ERR_GENERAL       // Consider filing a bug report
```

- Category 2: Sockets (`zts_socket`, `zts_bind`, `zts_connect`, `zts_listen`, etc). Errors returned by these functions can be the same as the above. With the added possibility of `zts_errno` being set. Much like standard errno this will provide a more specific reason for an error's occurrence. See `ZeroTierSockets.h` for values.


### API COMPATIBILITY WITH HOST OS:

While the ZeroTier socket interface can coexist with your host OS's own interface in the same file with no type and naming conflicts, try not to mix and match host OS/libzt structures, functions, or constants. It may look similar and may even work some of the time but there enough differences that it will cause headaches:

If you are calling a `zts_*` function, use the appropriate `ZTS_*` constants:

```
zts_socket(ZTS_AF_INET6, ZTS_SOCK_DGRAM, 0); (CORRECT)
zts_socket(AF_INET6, SOCK_DGRAM, 0);         (INCORRECT)
```

If you are calling a `zts_*` function, use the appropriate `zts_*` structure:

```
struct zts_sockaddr_in in4;  <------ Note the zts_* prefix
  ...
zts_bind(fd, (struct zts_sockaddr *)&in4, sizeof(struct zts_sockaddr_in)) < 0)
```