Docker + ZeroTier SDK
====

Welcome!

Imagine a flat, encrypted, no-configuration LAN for all of your Docker containers. 

This short tutorial will show you how to enable ZeroTier functionality for your Docker software container with little to no configuration. In this example we aim to build a Docker container with ZeroTier’s Network Container service bundled right in so that it’s effortless to hook any number of your services in the container up to your virtual network.

**Step 1: Build the ZeroTier service binaries**

From the ZeroTier source directory,  `make sdk` Optionally, if you'd like to see some debug output during execution, use `make sdk SDK_DEBUG=1`

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

**Step 3: Start your container**

`docker run -d -it redis_test /bin/bash`

**Step 4: From your container, set up environment variables**

Set our application pre-load with `export LD_PRELOAD=./libztintercept.so`. This dynamically loads our intercept library into your application which allows us to re-direct its network calls to our virtual network.

Tell the ZeroTier SDK service which network to connect to with `export ZT_NC_NETWORK=/var/lib/zerotier-one/nc_XXXXXXXXXXXXXXXX`.

**Step 5: Run your new ZeroTier-enabled service**

At this point, simply run your application as you normally would. It will be automatically intercepted and linked to the ZeroTier service (and hence your virtual networks!)

`/usr/bin/redis-server --port 6379`

***
**Additional info**
If you'd like to know the IP address your service can be reached at on this particular virtual network, use the following:
`zerotier-cli -D/var/lib/zerotier-one/nc_XXXXXXXXXXXXXXXX listnetworks`


***
`docker exec -it 'container id' /bin/bash`

