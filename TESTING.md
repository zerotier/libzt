## Testing via [selftest.cpp](test/selftest.cpp)

### Step 1. Enabling debug output

 - `make static_lib SDK_DEBUG=1`: For debugging libzt
 - `make static_lib ZT_DEBUG=1`: For debugging the ZeroTier core protocol (you usually won't need this)

### Step 2. Build the test programs:

 - `make tests`

 This will output `selftest` and `echotest` to `build/$PLATFORM/`

 *Note, the following examples assume your testing environment is `linux`, you'll see this in the build output path. If this is not true, change it to `darwin`, `freebsd`, or `win` depending on what you're running.*


### Step 3. Define your test configuration in `test/selftest.conf`:

This is essentially just a listing of libzt-based app identities and host machine identities. We will be conducting `library-to-remote-library`, `library-to-remote-host`, and `remote-host-to-library` tests over the network. For this reason we need to define who should be talking to who.

A simple test configutation might look like the following. This will create an `alice` and `bob` test personality, that is, we will run one instance of the library as a server (alice), and one instance of the `echotest` on the same host machine. `echotest` is merely a program to record timings for transmitted data and also generate data for the library to receive). Additionally we will be running a library as a client `bob` on another remote host as well as another instance of `echotest` on that same machine. In this configuration the following will happen:

 - `alice` libzt will tx/rx to/from `bob` libzt
 - `alice` libzt will send X bytes to `bob`'s `echotest` to test maximum TX rate
 - `alice` libzt will request X bytes from `bob`'s `echotest` to test maximum RX rate
 - `bob` libzt will send X bytes to `alice`'s `echotest` to test maximum TX rate
 - `bob` libzt will request X bytes from `alice`'s `echotest` to test maximum RX rate

```
# Tests will use ports starting from 'port' to 'port+n' where 'n' is the number of tests


# Alice
name alice
mode server
nwid 17d7094b2c2c7319
test comprehensive
port 7000
path test/alice
ipv4 172.30.30.10
ipv6 fd12:d719:4b6c:6c53:f799:13c4:07e0:abb8
echo_ipv4 172.30.30.1
echo_ipv6 fd11:d759:136e:2b53:6791:9328:31ce:618a
;

# Bob
name bob
mode client
nwid 17d7094b2c2c7319
test comprehensive
port 7000
path test/bob
ipv4 172.30.30.20
ipv6 fd11:d759:136e:2b53:6791:9328:31ce:618a
echo_ipv4 172.30.30.2
echo_ipv6 fd12:d719:4b6c:6c53:f799:13c4:07e0:abb8
```


Build outputs are as follows:

```
build
 |
 |--darwin
 |  |-libzt.a
 |  |-selftest
 |  |-echotest
 |
 |--linux
 |  |-libzt.a
 |  |-selftest
 |  |-echotest
 |
 |--freebsd
 |  |-libzt.a
 |  |-selftest
 |  |-echotest
 |
 |--win
    |-libzt.a
    |-selftest
    |-echotest
```

The self test will be performed over the network in the following configuration (addresses and ports are subject to change depending on what you define in your `test/*.conf` files):
![Image](docs/test_diagram.png)

### Test sets:
 
 - Test set A: Tests for correctness, error handling, blocking behaviour, on-system performance, etc
 - Test set B: Tests RX performance (from non-libzt app)
 - Test set C: Tests TX performance (to non-libzt app)

### Types of tests (defined in `selftest.cpp`)

#### Simple Tests:

 - Simple tests merely test one aspect of the library. For instance, its role as an IPv4 server, or IPv6 client.

#### Sustained Tests

 - Sustained tests will test the library's ability to support long-duration connections and data transfers.

#### Slam Tests

 - Slam tests will test the library's ability to handle many repeated API calls or repeated common sequences of API calls that a typical application may make. For instance, it will try to create as many sockets as possible, or try to create a socket, bind to an address, listen, and accept over and over. This is useful for detecting memory leaks and architectural limitations in the stack drivers.

#### Comprehensive Tests

 - A comprehensive test will test each aspect of the library one time.

#### Random Tests

 - Makes random API calls with random (or plausible arguments/data) to test for proper error handling

#### Performance Tests

 - Test's the library's performance characteristics

#### Correctness Tests
 
 - Tests's the library's error handling, address treatment, and blocking/non-blocking behaviour.
