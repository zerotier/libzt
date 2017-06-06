## Testing (using src/selftest.cpp)

After building the static library, you can run:

 - `make tests`

 This will output `selftest` to `build/tests/`. Using this, you can run the tests below. Note, the following examples assume your testing environment is `linux`, you'll see this in the build output path. If this is not true, change it to `darwin`, or `win` depending on what you're running.

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

***

### Simple Tests

Simple tests merely test one aspect of the library. For instance, it's role as an IPv4 server, or IPv6 client.

To run a single-test IPv4 client/server test. Where `$PLATFORM` is `linux`, `darwin` or `win`:

  - Host 1: `./build/linux/selftest zt1 c7cd7c9e1b0f52a2 simple 4 server 10.9.9.40 8787`
  - Host 2: `./build/linux/selftest zt2 c7cd7c9e1b0f52a2 simple 4 client 10.9.9.40 8787`

To run a multi-message IPv4 client/server test:
  - Host 1: `./build/linux/selftest zt2 c7cd7c9e1b0f52a2 simple 4 server 10.9.9.40 8787 n_bytes 100 50`
  - Host 2: `./build/linux/selftest zt2 c7cd7c9e1b0f52a2 simple 4 client 10.9.9.40 8787 n_bytes 100 50`

### Sustained Tests

Sustained tests will test the library's ability to support long-duration connections and data transfers.

### Slam Tests

Slam tests will test the library's ability to handle many repeated API calls or repeated common sequences of API calls that a typical application may make.

### Comprehensive Tests

A comprehensive test will test each aspect of the library one time.

On `host-1`, run: 
 - `./build/linux/selftest test/bob.conf`

On `host-2`, run:
 - `./build/linux/selftest test/alice.conf`

### Random Tests

Makes random API calls with random (or plausible arguments/data) to test for proper error handling

### Performance Tests

Test's the library's performance characteristics

### Correctness Tests

Tests's the library's error handling, address treatment, and blocking/non-blocking behaviour.




