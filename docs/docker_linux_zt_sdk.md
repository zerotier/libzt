Docker + ZeroTier SDK
====

Welcome!

Imagine a flat, encrypted, no-configuration LAN for all of your Docker containers. 

This short tutorial will show you how to enable ZeroTier functionality for your Docker software container with little to no configuration. In this example we aim to build a Docker container with ZeroTier’s Network Container service bundled right in so that it’s effortless to hook any number of your services in the container up to your virtual network. Alternatively, you can check out a docker project directory [here](docker_demo).


**Step 1: Build ZeroTier shared library**

`make shared_lib`, to see debug output, use `make shared_lib SDK_DEBUG=1`

**Step 2: Build your Docker image**

`docker build --tag=redis_test .`

The example dockerfile below incorperates a few important elements:
 
1) The ZeroTier service binaries
2) Whatever ZeroTier identity keys you plan on using (if you don't already have keys you wish to use, fret not! A new identity will be generated automatically).
3) The service we've chosen to use. In this case, redis.
```
FROM fedora:23
# Install apps
RUN yum -y update
RUN yum -y install redis-3.0.4-1.fc23.x86_64
RUN yum clean all
# Add ZT files
RUN mkdir -p /var/lib/zerotier-one/networks.d
ADD sdk_identity.public /var/lib/zerotier-one/identity.public
ADD sdk_identity.secret /var/lib/zerotier-one/identity.secret
ADD *.conf /var/lib/zerotier-one/networks.d/
ADD *.conf /
ADD *.name /
EXPOSE 9993/udp 6379/udp
# Install LWIP library used by service
ADD liblwip.so /var/lib/zerotier-one/liblwip.so
# Install syscall intercept library
ADD libztintercept.so /
RUN cp libztintercept.so lib/libztintercept.so
RUN ln -sf /lib/libztintercept.so /lib/libztintercept
ADD zerotier-cli /
Add zerotier-sdk-service /
# Install test scripts
ADD sdk_entrypoint.sh /sdk_entrypoint.sh
RUN chmod -v +x /sdk_entrypoint.sh
# Start ZeroTier-One
CMD ["./sdk_entrypoint.sh"]
```

**Step 3: Start container**

`docker run -d -it redis_test /bin/bash`

**Step 4: From container, set up environment variables**

Set our application pre-load with `export LD_PRELOAD=./libztintercept.so`. This dynamically loads our intercept library into your application which allows us to re-direct its network calls to our virtual network.

Tell the ZeroTier Network Containers service which network to connect to with `export ZT_NC_NETWORK=/var/lib/zerotier-one/nc_XXXXXXXXXXXXXXXX`.

**Step 5: Run your new ZeroTier-enabled service**

At this point, simply run your application as you normally would. It will be automatically intercepted and linked to the ZeroTier service (and hence your virtual networks!)

`/usr/bin/redis-server --port 6379`

***
**Additional info**
If you'd like to know the IP address your service can be reached at on this particular virtual network, use the following:
`zerotier-cli -D/var/lib/zerotier-one/nc_XXXXXXXXXXXXXXXX listnetworks`

## Installing in a Docker container (or any other container engine)

If it's not immediately obvious, installation into a Docker container is easy. Just install `zerotier-sdk-service`, `libztintercept.so`, and `liblwip.so` into the container at an appropriate locations. We suggest putting it all in `/var/lib/zerotier-one` since this is the default ZeroTier home and will eliminate the need to supply a path to any of ZeroTier's services or utilities. Then, in your Docker container entry point script launch the service with *-d* to run it in the background, set the appropriate environment variables as described above, and launch your container's main application.

The only bit of complexity is configuring which virtual network to join. ZeroTier's service automatically joins networks that have `.conf` files in `ZTHOME/networks.d` even if the `.conf` file is empty. So one way of doing this very easily is to add the following commands to your Dockerfile or container entry point script:

    mkdir -p /var/lib/zerotier-one/networks.d
    touch /var/lib/zerotier-one/networks.d/8056c2e21c000001.conf

Replace 8056c2e21c000001 with the network ID of the network you want your container to automatically join. It's also a good idea in your container's entry point script to add a small loop to wait until the container's instance of ZeroTier generates an identity and comes online. This could be something like:

    /var/lib/zerotier-one/zerotier-sdk-service -d
    while [ ! -f /var/lib/zerotier-one/identity.secret ]; do
      sleep 0.1
    done
    # zerotier-sdk-service is now running and has generated an identity

(Be sure you don't bundle the identity into the container, otherwise every container will try to be the same device and they will "fight" over the device's address.)

Now each new instance of your container will automatically join the specified network on startup. Authorizing the container on a private network still requires a manual authorization step either via the ZeroTier Central web UI or the API. We're working on some ideas to automate this via bearer token auth or similar since doing this manually or with scripts for large deployments is tedious.
