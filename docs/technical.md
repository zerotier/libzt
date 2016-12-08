Under the Hood
======

*Note: It is not necessary for you to understand anything in this document, this is merely for those curious about the inner workings of the intercept, tap service, stack driver, and network stack.*

In this document we will walk you through what happens when your ZeroTier-enabled app does things like accepting a connection, receiving data, etc. But before we get into specific cases it's worth understanding the three major components which work together to produce this magic: 

 - **Intercept Library**: Mates to your app and allows us to re-implement classic network calls such as `socket()`, `bind()`, etc
 - **Tap Service / Stack Driver**: Mediates communication between the Intercept Library and the Network Stack. 
 - **Network Stack**: Handles incoming ethernet frames and generates outgoing frames.

So now that we've clarified what these components do, here's the general idea:

You make a network call, the intercept routes that to our tap service which uses the user-space network stack to generate ethernet frames which is then sent out onto your ZeroTier virtual network.

***






### Creating a socket

Your app requests a socket:

```
socket()
```

Our library's implementation of `socket()` is executed instead of the kernel's. We automatically establish an `AF_UNIX` socket connection with the **tap service**. This is how your app will communicate with ZeroTier. An `RPC_SOCKET` message is sent to the **tap service**. The **tap service** receives the `RPC_SOCKET` message and requests the allocation of a new `Connection` object from the **stack driver** which represents the new socket. The **tap service** then repurposes the socket used for the RPC message and returns its file descriptor to your app for it to use as the new socket.

From your app's perspective nothing out of the ordinary has happened. It called `socket()`, and got a file descriptor back.
***









### Establishing a connection

You app connects to a remote host:

```
connect()
```

An `RPC_CONNECT` call is sent to the **tap service**, it is unpacked by the **stack driver** in `pico_handleConnect()`. A `pico_socket_connect()` call is made to the **network stack**. Once it establishes a connection (or fails), it sends a return value back to the app.

```
phyOnUnixData()
pico_handleConnect()
pico_socket_connect()
```
***








### Accepting a connection

Your app places a socket into a listen state:

```
listen()
```
An `RPC_LISTEN` call is sent to the **tap service** and **stack driver**

You app accepts a connection:

```
accept()
```

The **network stack** raises a `PICO_SOCK_EV_CONN` event which calls `pico_cb_socket_activity()`. From here we request a new `pico_socket` be created to represent the new connection. This is done via `pico_socket_accept()`. Once we have a valid `pico_socket`, we create an `AF_UNIX` socket pair. We associate one end with the newly-created `pico_socket` via a `Connection` object. And we send the other end of the socket pair to the app.

Our library's implementation of `accept()` will read and return the new file descriptor representing one end of the socket pair. From your app's prespective this is a normal file descriptor.
***








### Receiving data

The **tap service** monitors incoming packets, when one destined for us is detected it notifies the **stack driver** via `put()`. Then `pico_rx()` is called, its job is to re-encapsulate the ethernet frame and copy it onto the guarded `pico_frame_rxbuf`. This buffer is guarded because it is accessed via the **tap service** thread and the **network stack** thread.

```
wire
 put()
  pico_rx() ---> <pico_frame_rxbuf>
```

Periodically the **network stack** thread will call `pico_eth_poll()`, this is responsible for reading the frames from the aformentioned `pico_frame_rxbuf` and feeding it into the stack via `pico_stack_recv()`.

```
pico_eth_poll()
 <pico_frame_rxbuf> ---> pico_stack_recv()
```

After some time has passed and the **network stack** has processed the incoming frames a `PICO_SOCK_EV_RD` event will be triggered which calls `pico_cb_socket_activity()`, and ultimately `pico_cb_tcp_read()`. This is where we copy the incoming data from the `pico_socket` to the `Connection`'s `rxbuf` (different from `pico_frame_rxbuf`). We then notify the **tap service** that the `PhySocket` (a wrapped file descriptor with one end visible to the application) associated with this `Connection` has data in its `rxbuf` that needs to be written so the app can read it.

```  
pico_cb_socket_activity()
    pico_cb_tcp_read() ---> <rxbuf>
    setNotifyWritable(TRUE)
```

After some (more) time, the **tap service** thread will call `pico_handleRead()`, this will copy the data from the `rxbuf` to the `AF_UNIX` socket which links the service and your application. 

```
pico_handleRead()
 streamSend(): <rxbuf> ---> PhySock
```

After this point it's up to your application to read the data via a conventional `read()`, `recv()`, or `recvfrom()` call.

```
read()
```
***







### Sending data

Your app performs a `write()`, `send()`, or `sendto()` call.

```
write()
```

The other end of the `AF_UNIX` socket which was written to is monitored by the **tap service** thread. Once data is read from the socket, it will call `phyOnUnixData()` which will copy the buffer contents onto the `Connection`'s `txbuf`. Then `pico_handleWrite()` will be called. The **stack driver** will determine how much of the buffer can safely be sent to the **network stack** (up to `ZT_MAX_MTU` which is currently set at 2800 bytes). A call is made to `pico_socket_write()` which copies the data from the `txbuf` to the `pico_socket`.

```
  phyOnUnixData()
   handleWrite() 
    pico_socket_write(): <txbuf> ---> picosock
```

Periodically a `PICO_SOCK_EV_WR` event will be raised by the **network stack**, this will call `pico_cb_socket_activity()` and ultimately `pico_cb_tcp_write()` where a `pico_socket_write()` call will be made to copy any remaining `txbuf` contents into the stack.

```
pico_cb_tcp_write()
```

After some time, the **network stack** will emit an ethernet frame via `pico_eth_send()`, we then copy the frame into the **tap service** where it will then be sent onto the network.

```
pico_eth_send()
 tap->_handler()
```
***

