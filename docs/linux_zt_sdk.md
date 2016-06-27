Linux + ZeroTier SDK
====

Welcome!

Imagine a flat, encrypted, no-configuration LAN for all of your Linux applications.

This short tutorial will show you how to enable ZeroTier functionality for Linux applications using static-linking and dynamic linking methods.

## Use with Docker

See [doc/docker_linux_zt_sdk.md](doc/docker_linux_zt_sdk.md)

## Dynamic Linking 

To build the service host, IP stack, and intercept library, from the base of the ZeroTier One tree run:

    make shared_lib

This will build a binary called `zerotier-sdk-service` and a library called `libztintercept.so`. It will also build the IP stack as `build/liblwip.so`. 

The `zerotier-sdk-service` binary is almost the same as a regular ZeroTier One build except instead of creating virtual network ports using Linux's `/dev/net/tun` interface, it creates instances of a user-space TCP/IP stack for each virtual network and provides RPC access to this stack via a Unix domain socket. The latter is a library that can be loaded with the Linux `LD_PRELOAD` environment variable or by placement into `/etc/ld.so.preload` on a Linux system or container. Additional magic involving nameless Unix domain socket pairs and interprocess socket handoff is used to emulate TCP sockets with extremely low overhead and in a way that's compatible with select, poll, epoll, and other I/O event mechanisms.

The intercept library does nothing unless the `ZT_NC_NETWORK` environment variable is set. If on program launch (or fork) it detects the presence of this environment variable, it will attempt to connect to a running `zerotier-sdk-service` at the specified Unix domain socket path.

Unlike `zerotier-one`, `zerotier-sdk-service` does not need to be run with root privileges and will not modify the host's network configuration in any way. It can be run alongside `zerotier-one` on the same host with no ill effect, though this can be confusing since you'll have to remember the difference between "real" host interfaces (tun/tap) and network containerized endpoints. The latter are completely unknown to the kernel and will not show up in `ifconfig`.


## Static Linking






## Starting the SDK Service

A simple test can be performed in user space (no root) in your own home directory.

First, build the SDK service and intercept library as described above. Then create a directory to act as a temporary ZeroTier home for your test SDK service instance. You'll need to move the `liblwip.so` binary that was built with `make shared_lib` into there, since the service must be able to find it there and load it.

    mkdir /tmp/sdk-test-home
    cp -f build/liblwip.so /tmp/sdk-test-home

Now you can run the service (no sudo needed, and `-d` tells it to run in the background):

    ./zerotier-sdk-service -d -p8000 /tmp/sdk-test-home

As with ZeroTier One in its normal incarnation, you'll need to join a network for anything interesting to happen:

    ./zerotier-cli -D/tmp/sdk-test-home join 8056c2e21c000001

If you don't want to use [Earth](https://www.zerotier.com/public.shtml) for this test, replace 8056c2e21c000001 with a different network ID. The `-D` option tells `zerotier-cli` not to look in `/var/lib/zerotier-one` for information about a running instance of the ZeroTier system service but instead to look in `/tmp/sdk-test-home`.

Now type:

    ./zerotier-cli -D/tmp/sdk-test-home listnetworks

Try it a few times until you see that you've successfully joined the network and have an IP address. Instead of a *zt#* device, a path to a Unix domain socket will be listed for the network's port.

Now you will want to have ZeroTier One (the normal `zerotier-one` build, not the SDK) running somewhere else, such as on another Linux system or VM. Technically you could run it on the *same* Linux system and it wouldn't matter at all, but many people find this intensely confusing until they grasp just what exactly is happening here.

On the other Linux system, join the same network if you haven't already (8056c2e21c000001 if you're using Earth) and wait until you have an IP address. Then try pinging the IP address your SDK service instance received. You should see ping replies.

Back on the host that's running `zerotier-sdk-service`, type `ip addr list` or `ifconfig` (ifconfig is technically deprecated so some Linux systems might not have it). Notice that the IP address of the network containers endpoint is not listed and no network device is listed for it either. That's because as far as the Linux kernel is concerned it doesn't exist.

What are you pinging? What is happening here?

The `zerotier-sdk-service` binary has joined a *virtual* network and is running a *virtual* TCP/IP stack entirely in user space. As far as your system is concerned it's just another program exchanging UDP packets with a few other hosts on the Internet and nothing out of the ordinary is happening at all. That's why you never had to type *sudo*. It didn't change anything on the host.

Now you can run an application .

    export LD_PRELOAD=`pwd`/build/libztintercept.so
    export ZT_NC_NETWORK=/tmp/sdk-test-home/nc_8056c2e21c000001
    node tests/httpserver.js

Also note that the "pwd" in LD_PRELOAD assumes you are in the ZeroTier source root and have built the SDK there. If not, substitute the full path to *libztintercept.so*. If you want to remove those environment variables later, use "unset LD_PRELOAD" and "unset ZT_NC_NETWORK".

If you don't have node.js installed, an alternative test using python would be:

    python -m SimpleHTTPServer 80

If you are running Python 3, use `-m http.server`.

If all went well a small static HTTP server is now serving up the current directory, but only inside the network container. Going to port 80 on your machine won't work. To reach it, go to the other system where you joined the same network with a conventional ZeroTier instance and try:

    curl http://SDKSERVICE.INSTANCE.IP/

Replace *SDKSERVICE.INSTANCE.IP* with the IP address that *zerotier-sdk-service* was assigned on the virtual network. (This is the same IP you pinged in your first test.) If everything works, you should get back a copy of ZeroTier One's main README.md file.


#### Dynamic linking compatibility test results

The following applications have been tested and confirmed to work for the beta release:

Fedora 23:

    httpstub.c
    nginx 1.8.0
    http 2.4.16, 2.4.17
    darkhttpd 1.11
    python 2.7.10 (python -m SimpleHTTPServer)
    python 3.4.3 (python -m http.server)
    redis 3.0.4
    node 6.0.0-pre
    sshd

CentOS 7:

    httpstub.c
    nginx 1.6.3
    httpd 2.4.6 (debug mode -X)
    darkhttpd 1.11
    node 4.2.2
    redis 2.8.19
    sshd

Ubuntu 14.04.3:

    httpstub.c
    nginx 1.4.6
    python 2.7.6 (python -m SimpleHTTPServer)
    python 3.4.0 (python -m http.server)
    node 5.2.0
    redis 2.8.4
    sshd

It is *likely* to work with other things but there are no guarantees.
