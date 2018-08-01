# libzt
Library version of [ZeroTier](https://github.com/zerotier/ZeroTierOne)
***

<a href="https://www.zerotier.com/?pk_campaign=github_libzt"><img src="https://raw.githubusercontent.com/zerotier/ZeroTierOne/master/artwork/ZeroTierIcon.png" width="128" height="128" align="left" hspace="20" vspace="9"></a>

**libzt** makes it easy to securely connect devices, servers, cloud VMs, containers, and apps everywhere and manage them at scale. We provide a socket-like API supporting `SOCK_STREAM`, `SOCK_DGRAM`, and `SOCK_RAW`. There's no need for system-wide virtual interfaces. This connection is exclusive to your app and fully encrypted via the [Salsa20](https://en.wikipedia.org/wiki/Salsa20) cipher. For a more in-depth discussion on the technical side of ZeroTier, check out our [Manual](https://www.zerotier.com/manual.shtml?pk_campaign=github_libzt)

<hr>

[![irc](https://img.shields.io/badge/IRC-%23zerotier%20on%20freenode-orange.svg)](https://webchat.freenode.net/?channels=zerotier)

 - Pre-built library binaries can be found at: [https://download.zerotier.com/RELEASES/1.2.12/dist/libzt/](https://download.zerotier.com/RELEASES/1.2.12/dist/libzt/?pk_campaign=github_libzt)
 - Bindings for popular languages like [Scala](examples/scala), [Swift](examples/swift), [Java](examples/java), [Python](examples/python), etc. can be found [here](examples/)

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
	char *remoteIp = "10.8.8.42";
	int remotePort = 8080;
	int fd, err = 0;
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(remoteIp);
	addr.sin_port = htons(remotePort);

	zts_startjoin("path", 0xc7cd7c981b0f52a2); // config path, network ID

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

For an example using only the [Virtual Layer 2](https://www.zerotier.com/manual.shtml#2_2?pk_campaign=github_libzt), see [examples/layer2](examples/layer2/layer2.cpp)

***

### Build

We recommend using [CMake](https://cmake.org/) and [clang](https://en.wikipedia.org/wiki/Clang).

```
git submodule init
git submodule update
make patch
cmake -H. -Bbuild 
```

Then

```
cmake --build build -DCMAKE_BUILD_TYPE=Release
```

or 

```
cmake --build build --config Release
```

Builds are placed in `bin\lib`

***

### Commercial License

If you want a commercial license to use libzt in your product contact us directly via `contact@zerotier.com`

