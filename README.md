## **ZeroTier SDK**: Embed ZeroTier directly into your app

<a href="https://www.zerotier.com"><img src="https://github.com/zerotier/ZeroTierOne/raw/master/artwork/AppIcon_87x87.png" align="left" hspace="10" vspace="6"></a>

**ZeroTier** makes it easy to securely connect devices, servers, cloud VMs, containers, and apps everywhere and manage them at scale. Now, with the SDK you can bake this ability directly into your application or service using your preferred language. We provide a BSD socket-like API to make the integration go as smoothly as possible.[![irc](https://img.shields.io/badge/IRC-%23zerotier%20on%20freenode-orange.svg)](https://webchat.freenode.net/?channels=zerotier)

<br>

## Example

```
string str = "welcome to the machine";
zts_start("./zt");
while(!zts_running())
	sleep(1);
zts_join("e5cd7a7b1c0fa971");
while(!zts_has_address(nwid))
	sleep(1);
if((fd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	printf("error creating ZeroTier socket");
}
if((err = zts_connect(fd, (const struct sockaddr *)addr, sizeof(addr))) < 0) {
	printf("error connecting to remote host (%d)\n", err);
}
int wrote = zts_write(fd, str.c_str(), str.length());
zts_close(fd);
```

Bindings also exist for [many popular languages]().

## Build Targets
### Static Library
 - `make static_lib SDK_IPV4=1`: Will output to `build/`

### Tests
 - `make tests`: Will output to `build/tests/`

Then run the unit test suite with whatever configuration you need. For instance:

To run a single-test IPv4 client/server test:

  - Host 1: `./build/selftest zt1 c7cd7c9e1b0f52a2 simple 4 server 8787`
  - Host 2: `./build/selftest zt2 c7cd7c9e1b0f52a2 simple 4 client 10.9.9.40 8787`

To run a multi-message IPv4 client/server test:
  - Host 1: `./build/test/unit zt2 c7cd7c9e1b0f52a2 simple 4 server 8787 n_bytes 100 50`
  - Host 2: `./build/test/unit zt2 c7cd7c9e1b0f52a2 simple 4 client 10.9.9.40 8787 n_bytes 100 50`

  - For more unit test examples, see the [testing]() page  
  

## IP version flags
 - `SDK_IPV4=1`
 - `SDK_IPV6=1`

## Using Language Bindings
 - `SDK_LANG_JNI=1`: Enable JNI bindings for Java (produces a shared library)
 - `SDK_LANG_CSHARP=1`
 - `SDK_LANG_PYTHON=1`
 - `SDK_LANG_GO=1`

## Debugging flags
 - `SDK_DEBUG=1` - Enable SDK library debugging
 - `ZT_DEBUG=1` - Enable core ZeroTier service debugging