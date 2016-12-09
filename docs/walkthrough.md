Walkthrough
======

In this document we'll run through a simple example which should demonstrate the concept of the ZeroTierSDK. For this tutorial you'll need two devices (or at least the ability to run a VM or something like Docker).

### Create your virtual network
 - Download ZeroTier at (zerotier.com)[https://www.zerotier.com/]
 - Install it on a device/computer
 - Create an account and new virtual network at (my.zerotier.com)[https://my.zerotier.com/]
 - Join your device to the network and assign it an address `zerotier-cli join <nwid>`
 - Use `zerotier-cli listnetworks` to verify that you've joined the network.

### On your second device, Build the SDK
 - On Linux: `make linux SDK_PICOTCP=1 SDK_IPV4=1 SDK_DEBUG=1; make -s check; ls -lG build`

### Build test apps

```
make tests
```

### Start the SDK service
```
./zerotier-cli -U -p8000 /network/homepath &
```

### Set environment variables 
```
export ZT_NC_NETWORK=/network/homepath/nc_XXXXXXXXXXXXXXXX
export LD_PRELOAD=./libztintercept.so
```

### Start your app
```
./build/tests/linux.tcpserver4.out 8001
```

*Now, you'll note that your new TCP server is automatically intercepted and available at on port 80001*
