# ZeroTier SDK (libzt, libztcore)
Library edition of [ZeroTier](https://github.com/zerotier/ZeroTierOne)
***

<a href="https://www.zerotier.com/"><img src="https://raw.githubusercontent.com/zerotier/ZeroTierOne/master/artwork/ZeroTierIcon.png" width="128" height="128" align="left" hspace="20" vspace="9"></a>

The ZeroTier SDK is composed of two libraries: `libztcore` which is the  platform-agnostic network hypervisor, and `libzt` which is the network hypervisor paired with a userspace network stack. `libzt` is a superset of `libztcore` and is distinguished by the fact that it exposes a standard socket API and simple network control API. With these libraries the stack and virtual link are exclusive to your app and traffic is fully encrypted via the [Salsa20](https://en.wikipedia.org/wiki/Salsa20) cipher. For a more in-depth discussion on the technical side of ZeroTier, check out our [Manual](https://www.zerotier.com/manual.shtml)

*** 

<br>

## Downloads / Installation

 Tarballs:

  - [libzt-release.tar.gz](https://download.zerotier.com/dist/sdk/libzt-1.3.0-release.tar.gz) // [libzt-debug.tar.gz](https://download.zerotier.com/dist/sdk/libzt-1.3.0-debug.tar.gz) // [libzt-source.tar.gz](https://download.zerotier.com/dist/sdk/libzt-1.3.0-source.tar.gz)

Homebrew

```
brew install libzt
```

***

<br>

## Example

 - Complete example: [test/simple.cpp](test/simple.cpp)
 - Slightly more thorough example: [test/example.cpp](test/example.cpp)

```
#include "ZeroTier.h"

void myZeroTierEventCallback(struct zts_callback_msg *msg)
{
    switch (msg->eventCode)
    {
        //
    }
}

int main()
{
    zts_start("yourConfig/key/path", &myZeroTierEventCallback, 9994);
    zts_join(0x0123456789abcdef);
    zts_socket(ZTS_AF_INET, ZTS_SOCK_STREAM, 0);
    zts_connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    zts_write(fd, "welcome to the machine", 22);
    zts_close(fd);
    zts_stop();
    return 0;
}

...
```

After you've created a virtual network and added its `nwid` to the sample code, run:

```
clang++ example.cpp -o example -lzt
./example
```

The complete API specification can be found here: [API.md](API.md)

***

## Build from source

Build scripts use a combination of make, and cmake. To retrieve sources for all submodules, patch them, and build all targets (debug and release) for your host machine, issue the following:

```
make update
make patch
make all
```

All build targets can be seen by using `make list`.

Resultant libraries will be placed in `lib`, test and example programs will be placed in `bin`.

***

## Commercial License

If you want a commercial license to use the ZeroTier SDK in your product contact us directly via `contact@zerotier.com`

