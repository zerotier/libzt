## Testing via [selftest.cpp](test/selftest.cpp)

### Enabling debug output
 - `SDK_DEBUG=1` - For debugging libzt
 - `ZT_DEBUG=1` - For debugging the ZeroTier core protocol

After building the static library, you can run:

 - `make tests`

 This will output `selftest` to `build/$PLATFORM/`. Using this, you can run the tests below. Note, the following examples assume your testing environment is `linux`, you'll see this in the build output path. If this is not true, change it to `darwin`, `freebsd`, or `win` depending on what you're running.

 Simply add your `host-1` and `host-2` address, port, and network information to `test/alice.conf` and `test/bob.conf`, this way you can use the selftest shorthand shown below. The file contain examples of what you should do.

Build outputs are as follows:

```
build
 |
 |--darwin
 |  |-libzt.a
 |  |-selftest
 |
 |--linux
    |-libzt.a
    |-selftest
```

### Simple Tests

Simple tests merely test one aspect of the library. For instance, its role as an IPv4 server, or IPv6 client.

To run a single-test IPv4 client/server test:

  - host-1: `./build/linux/selftest zt1 c7cd7c9e1b0f52a2 simple 4 server 10.9.9.40 8787`
  - host-2: `./build/linux/selftest zt2 c7cd7c9e1b0f52a2 simple 4 client 10.9.9.40 8787`

To run a multi-message IPv4 client/server test:

  - host-1: `./build/linux/selftest zt1 c7cd7c9e1b0f52a2 simple 4 server 10.9.9.40 8787 n_bytes 100 50`
  - host-2: `./build/linux/selftest zt2 c7cd7c9e1b0f52a2 simple 4 client 10.9.9.40 8787 n_bytes 100 50`

### Sustained Tests

Sustained tests will test the library's ability to support long-duration connections and data transfers.

 - host-1: `./build/linux/selftest sustained test/bob.conf`
 - host-2: `./build/linux/selftest sustained test/alice.conf`

### Slam Tests

Slam tests will test the library's ability to handle many repeated API calls or repeated common sequences of API calls that a typical application may make. For instance, it will try to create as many sockets as possible, or try to create a socket, bind to an address, listen, and accept over and over. This is useful for detecting memory leaks and architectural limitations in the stack drivers.

 - host-1: `./build/linux/selftest slam test/bob.conf`
 - host-2: `./build/linux/selftest slam test/alice.conf`

### Comprehensive Tests

A comprehensive test will test each aspect of the library one time.

 - host-1: `./build/linux/selftest comprehensive test/bob.conf`
 - host-2: `./build/linux/selftest comprehensive test/alice.conf`

### Random Tests

Makes random API calls with random (or plausible arguments/data) to test for proper error handling

 - host-1: `./build/linux/selftest random test/bob.conf`

### Performance Tests

Test's the library's performance characteristics

 - host-1: `./build/linux/selftest performance test/bob.conf`
 - host-2: `./build/linux/selftest performance test/alice.conf`

### Correctness Tests

Tests's the library's error handling, address treatment, and blocking/non-blocking behaviour.

 - host-1: `./build/linux/selftest correctness test/bob.conf`
 - host-2: `./build/linux/selftest correctness test/alice.conf`



