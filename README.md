# libzt
Library version of [ZeroTier](https://github.com/zerotier/ZeroTierOne)
***

<a href="https://www.zerotier.com/?pk_campaign=github_libzt"><img src="https://raw.githubusercontent.com/zerotier/ZeroTierOne/master/artwork/ZeroTierIcon.png" width="128" height="128" align="left" hspace="20" vspace="9"></a>

**libzt** makes it easy to securely connect devices, servers, cloud VMs, containers, and apps everywhere and manage them at scale. We provide a socket-like API supporting `SOCK_STREAM`, `SOCK_DGRAM`, and `SOCK_RAW`. There's no need for system-wide virtual interfaces. This connection is exclusive to your app and fully encrypted via the [Salsa20](https://en.wikipedia.org/wiki/Salsa20) cipher. For a more in-depth discussion on the technical side of ZeroTier, check out our [Manual](https://www.zerotier.com/manual.shtml?pk_campaign=github_libzt)

<hr>

[![irc](https://img.shields.io/badge/IRC-%23zerotier%20on%20freenode-orange.svg)](https://webchat.freenode.net/?channels=zerotier)

| Platform  | Static | Shared | Package | Example project/code | Library build instructions
| --------- | --- | --- | --- | --- | --- |
| macOS     | [libzt.a](https://download.zerotier.com/RELEASES/1.2.12/dist/libzt/macos/libzt-1.2.0r1-macOS-10.13.6-x64-release.a) | [libzt.dylib](https://download.zerotier.com/RELEASES/1.2.12/dist/libzt/macos/libzt-1.2.0r1-macOS-10.13.6-x64-release.dylib) | | | see below |
| iOS       | [libzt.a](https://download.zerotier.com/RELEASES/1.2.12/dist/libzt/ios/libzt-1.2.0r1-ios-arm64-static-release.tar.tar.gz) | | [zt.framework](https://download.zerotier.com/RELEASES/1.2.12/dist/libzt/ios/libzt-1.2.0r1-ios-arm64-framework.tar.gz) | [examples/swift](examples/swift) | [packages/iOS](packages/iOS) |
| Windows   | [libzt.lib](https://download.zerotier.com/RELEASES/1.2.12/dist/libzt/win/libzt-1.2.0r1-win10-x86-release.lib) (x86), [libzt.lib](https://download.zerotier.com/RELEASES/1.2.12/dist/libzt/win/libzt-1.2.0r1-win10-x64-release.lib) (x64) | [libzt.dll](https://download.zerotier.com/RELEASES/1.2.12/dist/libzt/win/libzt-1.2.0r1-win10-x86-release.dll) (x86), [libzt.dll](https://download.zerotier.com/RELEASES/1.2.12/dist/libzt/win/libzt-1.2.0r1-win10-x64-release.dll) (x64) | | [examples/cpp/ExampleWindowsCppApp](examples/cpp/ExampleWindowsCppApp), [examples/csharp/ExampleWindowsCSharpApp](examples/csharp/ExampleWindowsCSharpApp) | see below |
| Android   |  |  | [libzt.aar](https://download.zerotier.com/RELEASES/1.2.12/dist/libzt/android/libzt-1.2.0r1-android-armeabi-v7a.aar) | [examples/android/ExampleAndroidApp](examples/android/ExampleAndroidApp) | [packages/android](packages/android)|
| Linux     | see below | see below | | | see below |

C API: [libzt.h](include/libzt.h)
Java JNI API: [ZeroTier.java](packages/android/app/src/main/java/ZeroTier.java)

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
git submodule update --init && make patch
cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Release
```

or possibly:

```
cmake -H. -Bbuild --config Release
```

Then

```
cmake --build build 
```


Builds are placed in `bin\lib`

***

### Commercial License

If you want a commercial license to use libzt in your product contact us directly via `contact@zerotier.com`

