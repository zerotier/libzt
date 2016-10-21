Hot-Swappable Network Stacks!
====

We've now enabled the ability for users to build the ZeroTier SDK with different network stacks with the mere flip of a compiler flag as well as running different stacks concurrently! This is perfect for embedded developers which may need a smaller code footprint and would like to use their own smaller or more specialized network stacks.

`SDK_LWIP=1` and `SDK_PICOTCP=1` will enable the lwIP and picoTCP network stacks respectively.

Currently our *lwIP* stack driver supports IPV4 and limited IPV6, whereas our *picoTCP* stack driver supports both IPV4 and IPV6 with no known issues.

To enable specific protocol versions use `SDK_IPV4=1` and `SDK_IPV6=1` in conjunction with the above stack selection flags.

## Integrating Your Own Custom Stack

If you don't know why this section exists, then I suggest turning back now. This is not for you. Now, let's get on with things, here's how you can integrate your own custom network stack if for some reason lwIP or picoTCP aren't cutting it for you.

The integration points are designated in the tap service code with the tags such as `SIP-0, SIP-1, ..., SIP-n`.

[More content to come]