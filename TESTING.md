## Testing (using src/selftest.cpp)

After you build the static library, you can run:

 - `make tests`: 

 This will output `selftest` to `build/tests/`.

 Using this, you can run the following tests:

***

### Simple Tests

Simple tests merely test one aspect of the library. For instance, it's role as an IPv4 server, or IPv6 client.

To run a single-test IPv4 client/server test. Where `$PLATFORM` is `linux`, `darwin` or `win`:

  - Host 1: `./build/$PLATFORM/test/selftest zt1 c7cd7c9e1b0f52a2 simple 4 server 10.9.9.40 8787`
  - Host 2: `./build/$PLATFORM/test/selftest zt2 c7cd7c9e1b0f52a2 simple 4 client 10.9.9.40 8787`

To run a multi-message IPv4 client/server test:
  - Host 1: `./build/$PLATFORM/test/test/unit zt2 c7cd7c9e1b0f52a2 simple 4 server 10.9.9.40 8787 n_bytes 100 50`
  - Host 2: `./build/$PLATFORM/test/test/unit zt2 c7cd7c9e1b0f52a2 simple 4 client 10.9.9.40 8787 n_bytes 100 50`

### Sustained Tests

Sustained tests will test the library's ability to support long-duration connections and data transfers.

### Slam Tests

Slam tests will test the library's ability to handle many repeated API calls or repeated common sequences of API calls that a typical application may make.

### Comprehensive Tests

A comprehensive test will test each aspect of the library one time.

On `host-1`, run: 
 - `./build/test/linux/selftest test/bob.conf`

On `host-2`, run:
 - `./build/test/linux/selftest test/alice.conf`

### Random Tests

Makes random API calls with random (or plausible arguments/data) to test for proper error handling

### Performance Tests

Test's the library's performance characteristics

### Correctness Tests

Tests's the library's error handling, address treatment, and blocking/non-blocking behaviour.




