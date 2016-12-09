Walkthrough
======

In this document we'll run through a simple example which should demonstrate the concept of the ZeroTier SDK. For this tutorial you'll need two devices (or at least the ability to run a VM or something like [Docker](https://www.docker.com/)). We will demonstrate a simple TCP server application intercepted on linux. This is only one of *many* ways the SDK can be used, but it'll at least convey the core concept how how the intercept works.

#### On your first device:
 - Download ZeroTier at [zerotier.com](https://www.zerotier.com/product-one.shtml)
 - Install it on a device/computer
 - Create an account and new virtual network at [my.zerotier.com](https://my.zerotier.com/)
 - Join your device to the network and assign it an address `zerotier-cli join <nwid>`
 - Use `zerotier-cli listnetworks` to verify that you've joined the network.
***



#### On your second device:

##### Build the SDK
```
make linux SDK_PICOTCP=1 SDK_IPV4=1 SDK_DEBUG=1; make -s check; ls -lG build
```

##### Build test apps

```
make tests
```

##### Start the SDK service in the background
```
./zerotier-cli -U -p8000 /netpath &
```

##### Set environment variables 
```
export ZT_NC_NETWORK=/netpath/nc_XXXXXXXXXXXXXXXX
export LD_PRELOAD=./libztintercept.so
```

Where `netpath` can be any path you'd like the client's keys and configuration to be stored and `XXXXXXXXXXXXXXXX` is the 16-digit network ID.

##### Start your app
```
./build/tests/linux.tcpserver4.out 8001
```

Now, on your first device, `./build/tests/linux.tcpclient4.out <ip> 8001` where `<ip>` is the ip address that you assigned to your first device.

Now, you'll note that your new TCP server is automatically intercepted and available at on port 8001. The `tcpclient4` sample app running on the device with a normal instance of ZeroTier running will be able to connect directly to your intercepted `tcpserver4` app on the second machine running the SDK.
***


You've just uplifted your app onto your private ZeroTier network. 

