## Building from source

The npm install script will attempt to statically link to `libzt.a`.

You first need `Cmake` to build the fPIC-version of the `libzt.a` library.

To build both `release` and `debug` libraries for only your host's architecture use `make host`. Or optionally `make host_release` for release only. To build everything including things like iOS frameworks, Android packages, etc, use `make all`. Possible build targets can be seen by using `make list`. Resultant libraries will be placed in `/lib`:

```
brew install cmake
(cd ../.. ; make clean; make update && make patch && make host_release CC=clang CXX=clang++)
npm install
npm start
```

Typical build output:

```
lib
├── release
|    └── linux-x86_64
|       ├── libzt.a
|       └── libzt.so
|    └── macos-x86_64
|       ├── libzt.a
└── debug
    └── ...
```

## Licensing

ZeroTier is licensed under the BSL version 1.1. See [LICENSE.txt](./LICENSE.txt) and the ZeroTier pricing page for details. ZeroTier is free to use internally in businesses and academic institutions and for non-commercial purposes. Certain types of commercial use such as building closed-source apps and devices based on ZeroTier or offering ZeroTier network controllers and network management as a SaaS service require a commercial license.

A small amount of third party code is also included in ZeroTier and is not subject to our BSL license. See [AUTHORS.md](ext/ZeroTierOne/AUTHORS.md) for a list of third party code, where it is included, and the licenses that apply to it. All of the third party code in ZeroTier is liberally licensed (MIT, BSD, Apache, public domain, etc.). If you want a commercial license to use the ZeroTier SDK in your product contact us directly via [contact@zerotier.com](mailto:contact@zerotier.com)

