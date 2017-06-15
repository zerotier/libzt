# libzt
*Embed ZeroTier directly into your app*
***

<a href="https://www.zerotier.com"><img src="https://github.com/zerotier/ZeroTierOne/raw/master/artwork/AppIcon_87x87.png" align="left" hspace="20" vspace="6"></a>

**ZeroTier** makes it easy to securely connect devices, servers, cloud VMs, containers, and apps everywhere and manage them at scale. Now, with **libzt** you can bake this ability directly into your app or service using your preferred language or framework. We provide a BSD socket-like API to make the integration simple.

<hr>

[![irc](https://img.shields.io/badge/IRC-%23zerotier%20on%20freenode-orange.svg)](https://webchat.freenode.net/?channels=zerotier)

Pre-Built Binaries Here: [zerotier.com/download.shtml](https://zerotier.com/download.shtml?pk_campaign=github_ZeroTierSDK).

*** 

### Example

```
#include "libzt.h"

char *str = "welcome to the machine"; 
char *nwid = "c7cd7c9e1b0f52a2"; // network to join
char *pasth = "zt1" // path where this node's keys and configs will be stored

zts_simple_start(path, nwid);
int fd = zts_socket(AF_INET, SOCK_STREAM, 0);
zts_connect(fd, (const struct sockaddr *)addr, sizeof(addr));
zts_write(fd, str, strlen(str));
zts_close(fd);
```

Bindings for various [LANGUAGES](examples)

***

### Building (linux, macos, bsd, win, ios)

 Complete instructions here [PLATFORMS](BUILDING.md)
 All build targets will output to `build/`

 - Static Library (linux, mac, win, bsd): `make static_lib`
 - iOS App Framework: `make ios_app_framework`
 - macOS App Framework: `make macos_app_framework`

***

### Testing and Debugging
 - See [TESTING.md](TESTING.md)