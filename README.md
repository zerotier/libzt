<div align="center">

<h1>ZeroTier Sockets</h1>
Part of the ZeroTier SDK
<img alt="" src="https://i.imgur.com/BwSHwE3.png" class="doxyhidden"> </img>

Peer-to-peer and cross-platform encrypted connections built right into your app or service. No drivers, no root, and no host configuration.

<br>

<a href="./examples">Examples</a> |
<a href="https://docs.zerotier.com/sockets/tutorial.html">Documentation</a> |
<a href="https://discuss.zerotier.com/">Community</a> |
<a href="https://github.com/zerotier/libzt/issues">Report a Bug</a>


<a href="https://www.twitter.com/zerotier"><img alt="@zerotier" src="https://img.shields.io/twitter/follow/zerotier?style=social"/></a>
<a href="https://old.reddit.com/r/zerotier"><img alt="r/zerotier" src="https://img.shields.io/reddit/subreddit-subscribers/zerotier?style=social"/></a>


<img alt="latest libzt version" src="https://img.shields.io/github/v/tag/zerotier/libzt?label=latest"/></a>
<a href="https://github.com/zerotier/libzt/commits/master"><img alt="Last Commit" src="https://img.shields.io/github/last-commit/zerotier/libzt"/></a>
<a href="https://github.com/zerotier/libzt/actions"><img alt="Build Status (master branch)" src="https://img.shields.io/github/workflow/status/zerotier/libzt/CMake/master"/></a>
</div>

| Language/Platform | Installation | Version | Example |
|:----------|:---------|:---|:---|
| C/C++  | [Build from source](#build-from-source) | <img alt="version" src="https://img.shields.io/github/v/tag/zerotier/libzt?label="/></a>| [C/C++](./examples/c)  |
| C#  | `Install-Package ZeroTier.Sockets` |<a href="https://www.nuget.org/packages/ZeroTier.Sockets/"><img src="https://img.shields.io/github/v/tag/zerotier/libzt?label=NuGet"/></a> |[C#](./examples/csharp)  |
| Python  | `pip install libzt`|<a href="https://pypi.org/project/libzt/"><img src="https://img.shields.io/pypi/v/libzt?label=PyPI"/></a> |[Python](./examples/python)  |
| Rust  | See: [crates.io/crates/libzt](https://crates.io/crates/libzt) | <img alt="version" src="https://img.shields.io/crates/v/libzt?color=blue"/>|[Rust](./examples/rust)  |
| Java  | `./build.sh host-jar` |<img src="https://img.shields.io/github/v/tag/zerotier/libzt?label="/> |[Java](./examples/java)  |
| Linux  | `brew install zerotier/tap/libzt` | <img alt="version" src="https://img.shields.io/github/v/tag/zerotier/libzt?label="/></a>| [C/C++](./examples/c)  |
| macOS  | `brew install zerotier/tap/libzt`| <img alt="version" src="https://img.shields.io/github/v/tag/zerotier/libzt?label=Homebrew"/></a>| [C/C++](./examples/c)  |
| iOS / iPadOS  | `./build.sh iphoneos-framework` | <img src="https://img.shields.io/github/v/tag/zerotier/libzt?label="/>| [Objective-C](./attic/objective-c), [Swift](./attic/swift)  |
| Android  |`./build.sh android-aar` | <img src="https://img.shields.io/github/v/tag/zerotier/libzt?label="/> | [Java](./examples/java)  |

<br>

<div align="left">

```
#include "ZeroTierSockets.h"

int main()
{
    zts_node_start();
    zts_net_join(net_id);
    int fd = zts_bsd_socket(ZTS_AF_INET, ZTS_SOCK_STREAM, 0);
    zts_bsd_connect(fd, ...);
    ...
}
```

# Build from source

```
git submodule update --init
```

This project uses [CMake](https://cmake.org/download/) as a build system generator. The scripts `build.*` simplify building and packaging for various targets. There are many targets and configurations not mentioned here.

|Platform| Build instructions | Notes |
|:---|:---|:---|
|Linux | `./build.sh host "release"`| [build.sh](./build.sh) |
|macOS | `./build.sh host "release"`| [build.sh](./build.sh) |
|Windows | `. .\build.ps1; Build-Host -BuildType "Release" -Arch "x64"` | [build.ps1](./build.ps1), *Requires [PowerShell](https://github.com/powershell/powershell)*|

 Using the `host` keyword will automatically detect the current machine type and build standard libzt for use in C/C++ (no additional language bindings.) See `./build.sh list` for additional target options. `libzt` depends on [cURL](https://github.com/curl/curl) for the optional portion of the API that interfaces with our hosted web offering ([my.zerotier.com](my.zerotier.com)). If you do not need this functionality you can omit it by passing `-DZTS_DISABLE_CENTRAL_API=1` to CMake.

Example output:

```
~/libzt/dist/macos-x64-host-release
├── bin
│   ├── client
│   ├── server
│   └── ...
└── lib
    ├── libzt.a
    └── libzt.dylib
```

Important directories:

|Directory| Purpose|
|:---|:---|
|`dist`| Contains finished targets (libraries, binaries, packages, etc.)|
|`cache`| Contains build system caches that can safely be deleted after use.|
|`pkg`| Contains project, script and spec files to generate packages.|

# Self-hosting (Optional)

We provide ways for your app or enterprise to function independently from any of our services if desired.

While we do operate a global network of redundant root servers, network controllers and an admin API/UI called [Central](https://my.zerotier.com), some use-cases require full control over the infrastructure and we try to make it as easy as possible to set up your own controllers and root servers: See [here](https://github.com/zerotier/ZeroTierOne/tree/master/controller) to learn more about how to set up your own network controller, and [here](https://www.zerotier.com/manual/#4_4) to learn more about setting up your own roots.

# Help

 - Documentation: [docs.zerotier.com](https://docs.zerotier.com/sockets/tutorial.html)
 - Examples: [examples/](./examples)
 - Bug reports: [Open a github issue](https://github.com/zerotier/libzt/issues).
 - General ZeroTier troubleshooting: [Knowledgebase](https://zerotier.atlassian.net/wiki/spaces/SD/overview).
 - Chat with us: [discuss.zerotier.com](https://discuss.zerotier.com)

# Licensing

ZeroTier and the ZeroTier SDK (libzt and libztcore) are licensed under the [BSL version 1.1](./LICENSE.txt). ZeroTier is free to use internally in businesses and academic institutions and for non-commercial purposes. Certain types of commercial use such as building closed-source apps and devices based on ZeroTier or offering ZeroTier network controllers and network management as a SaaS service require a commercial license. A small amount of third party code is also included in ZeroTier and is not subject to our BSL license. See [AUTHORS.md](ext/ZeroTierOne/AUTHORS.md) for a list of third party code, where it is included, and the licenses that apply to it. All of the third party code in ZeroTier is liberally licensed (MIT, BSD, Apache, public domain, etc.). If you want a commercial license to use the ZeroTier SDK in your product contact us directly via [contact@zerotier.com](mailto:contact@zerotier.com)
