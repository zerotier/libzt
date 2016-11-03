Hot-Swappable Network Stacks!
====

We've now enabled the ability for users to build the ZeroTier SDK with different network stacks with the mere flip of a compiler flag as well as running different stacks concurrently! This is perfect for embedded developers which may need a smaller code footprint and would like to use their own smaller or more specialized network stacks.

`SDK_LWIP=1` and `SDK_PICOTCP=1` will enable the lwIP and picoTCP network stacks respectively.

Currently our *lwIP* stack driver supports IPV4 and limited IPV6, whereas our *picoTCP* stack driver supports both IPV4 and IPV6 with no known issues.

To enable specific protocol versions use `SDK_IPV4=1` and `SDK_IPV6=1` in conjunction with the above stack selection flags.

Also, to enable debug for the SDK use `SDK_DEBUG=1`, to enable debug for the *lwIP* stack use `SDK_LWIP_DEBUG=1`. 

## Integrating Your Own Custom Stack

If you don't know why this section exists, then I suggest turning back now. This is not for you. Otherwise, let's get on with things, here's how you can integrate your own custom network stack if for some reason lwIP or picoTCP aren't cutting it for you:

Investigate the structure of `src/tap.cpp`, this file contains calls to functions implemented in the stack driver code (located in `src/stack_drivers`).

Each stack is different but generally you'll need to provide:
 - An initialization function to configure and bring up the stack's `interface` (or similar).
 - An I/O polling loop section where you'll execute your timer calls, and check for inbound and outbound frames.
 - A low-level input and output function to handle feeding ethernet frames into and out of the stack in its own unique way.
 - Calls to your stack's API which roughly correspond with `handleRead()`, `handleWrite()`, `handleSocket()`, `handleConnect()`, etc
 - In those calls you'll need to handle the creation, management, and destruction of your stack's "connection" objects, whatever that may be.