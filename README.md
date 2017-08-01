# libzt
*Embed ZeroTier directly into your app*
***

<a href="https://www.zerotier.com?pk_campaign=github_libzt"><img src="https://github.com/zerotier/ZeroTierOne/raw/master/artwork/AppIcon_87x87.png" align="left" hspace="20" vspace="6"></a>

**ZeroTier** makes it easy to securely connect devices, servers, cloud VMs, containers, and apps everywhere and manage them at scale. Now, with **libzt** you can bake this ability directly into your app or service using your preferred language or framework. We provide a BSD socket-like API supporting `SOCK_STREAM`, `SOCK_DGRAM`, and `SOCK_RAW` to make the integration simple. There's also no longer any need for system-wide virtual interfaces. This connection is exclusive to your app and fully encrypted via the [Salsa20](https://en.wikipedia.org/wiki/Salsa20) cipher.

<hr>

[![irc](https://img.shields.io/badge/IRC-%23zerotier%20on%20freenode-orange.svg)](https://webchat.freenode.net/?channels=zerotier)

Pre-Built Binaries Here: [zerotier.com/download.shtml](https://zerotier.com/download.shtml?pk_campaign=github_libzt).

*** 

### Example

```
#include "libzt.h"

char *str = "welcome to the machine"; // test msg 
char *nwid = "c7cd7c9e1b0f52a2";      // network to join
char *path = "zt1";                   // path where this node's keys and configs will be stored
char *ip = "10.8.8.42";               // host on ZeroTier network
int port = 8080;                      // resource's port

struct sockaddr_in addr;
addr.sin_family = AF_INET;
addr.sin_addr.s_addr = inet_addr(ip);
addr.sin_port = hton(port);	

zts_simple_start(path, nwid);
int fd = zts_socket(AF_INET, SOCK_STREAM, 0);
zts_connect(fd, (const struct sockaddr *)addr, sizeof(addr));
zts_write(fd, str, strlen(str));
zts_close(fd);
```

Bindings for various [languages](examples)

For an example using only the Virtual Layer 2, see [test/layer2.cpp](test/layer2.cpp)

***

### Building (linux, macos, bsd, win, ios)

 ```
 git submodule init
 git submodule update
 make static_lib
 ```
 
 All targets will output to `build/`. Complete instructions [here](BUILDING.md)

***

### Testing and Debugging
 - See [TESTING.md](TESTING.md)

### Licensing
 - For a [BSD]() license, build using the `lwIP` network stack with `STACK_LWIP=1`
 - For a [GPL]() license, build using the `picoTCP` network stack with `STACK_PICO=1`

 Regardless of which network stack you build with, the socket API will remain the same.