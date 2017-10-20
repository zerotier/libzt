# libzt
*Embed ZeroTier directly into your app*
***

<a href="https://www.zerotier.com/?pk_campaign=github_libzt"><img src="https://raw.githubusercontent.com/zerotier/ZeroTierOne/master/artwork/ZeroTierIcon.png" width="128" height="128" align="left" hspace="20" vspace="9"></a>

A library version of [ZeroTier](https://github.com/zerotier/ZeroTierOne), **libzt** makes it easy to securely connect devices, servers, cloud VMs, containers, and apps everywhere and manage them at scale. Now you can bake this ability directly into your app or service using your preferred language. We provide a POSIX-like socket API supporting `SOCK_STREAM`, `SOCK_DGRAM`, and `SOCK_RAW` to make the integration simple. There's no need for system-wide virtual interfaces. This connection is exclusive to your app and fully encrypted via the [Salsa20](https://en.wikipedia.org/wiki/Salsa20) cipher. For a more in-depth discussion on the technical side of ZeroTier, check out our [Manual](https://www.zerotier.com/manual.shtml?pk_campaign=github_libzt)

<hr>

[![irc](https://img.shields.io/badge/IRC-%23zerotier%20on%20freenode-orange.svg)](https://webchat.freenode.net/?channels=zerotier)

 - Pre-Built binaries will soon be placed here: [zerotier.com/download.shtml](https://zerotier.com/download.shtml?pk_campaign=github_libzt).
 - Bindings for popular languages like [Scala](examples/bindings/scala), [Swift](examples/bindings/swift), [Java](examples/bindings/java), [Python](examples/bindings/python), etc. can be found [here](examples/bindings)

*** 

### C++ Example

```
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "libzt.h"

int main() 
{
	char *str = "welcome to the machine";
	char *nwid = "c7cd7c981b0f52a2";      // network
	char *path = "config_path";           // where this instance's keys and configs are stored
	char *ip = "10.8.8.42";               // remote address
	int port = 8080;                      // remote port

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip);
	addr.sin_port = htons(port);	

	zts_startjoin(path, nwid);

	int fd, err = 0;
	if ((fd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("error creating socket\n");
	}
	if ((err = zts_connect(fd, (const struct sockaddr *)&addr, sizeof(addr))) < 0) {
		printf("error connecting to remote host\n");
	}
	if ((err = zts_write(fd, str, strlen(str))) < 0) {
		printf("error writing to socket\n");
	}
	if ((err = zts_close(fd)) < 0) {
		printf("error closing socket\n");
	}
	zts_stop();
	return 0;
}
```

For an example using only the [Virtual Layer 2](https://www.zerotier.com/manual.shtml#2_2?pk_campaign=github_libzt), see [test/layer2.cpp](test/layer2.cpp)

***

### Building (linux, macos, bsd, win, ios)

 ```
 git submodule init
 git submodule update
 make static_lib
 make tests
 ```
 
 All targets will output to `build/`. Complete instructions [here](BUILDING.md)

***

### Testing and Debugging
 - See [TESTING.md](TESTING.md)


### Contributing

Please make pull requests against the `dev` branch. The `master` branch is release, and `edge` is for unstable and work in progress changes and is not likely to work.

### License
 - **Commercial**, with BSD license, build using `lwIP` network stack with `STACK_LWIP=1`, then contact us directly via `contact@zerotier.com` to discuss commercial licensing.
 - **Personal**, with GPL license, build using `picoTCP` network stack with `STACK_PICO=1`, everything is free. Have fun.

 Regardless of which network stack you build with, the socket API will remain the same.