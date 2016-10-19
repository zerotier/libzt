Hot-Swappable Network Stacks!
====

We've now enabled the ability for users to build the ZeroTier SDK with different network stacks with the mere flip of a compiler flag as well as running different stacks concurrently!

`SDK_LWIP=1` and `SDK_PICOTCP=1` will enable the lwIP and picoTCP network stacks respectively.

Currently our lwIP stack driver supports IPV4 and limited IPV6, whereas our picoTCP stack driver supports both IPV4 and IPV6 with no known issues.

To enable specific protocol versions use `SDK_IPV4=1` and `SDK_IPV6=1` in cnojunction with the above stack selection flags.