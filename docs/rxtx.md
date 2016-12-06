How Data is Sent and Received by an App using the SDK
======

*Note: It is not necessary for you to understand anything in this document, this is merely for those curious about the inner workings of the intercept, tap service, stack driver, and network stack.*


## Establishing a connection


## Acception a connection


## Receiving data

The **tap service** monitors incoming packets, when one destined for us is detected it notifies the **stack driver** via `put()`. Then `pico_rx()` is called, its job is to re-encapsulate the ethernet frame and copy it onto the guarded `pico_frame_rxbuf`. This buffer is guarded because it is accessed via the **tap service** thread and the **network stack** thread.

```
wire
 put()
  pico_rx() ---> <pico_frame_rxbuf>
```

Periodically the **network stack** thread will call `pico_eth_poll()`, this is responsible for reading the frames from the aformentioned `pico_frame_rxbuf` and feeding it into the stack via `pico_stack_recv()`.

```
pico_eth_poll()
 <pico_frame_rxbuf> ---> pico_stack_recv
```

After some time has passed and the **network stack** has processed the incoming frames a `PICO_SOCK_EV_RD` event will be triggered which calls `pico_cb_socket_activity()`, and ultimately `pico_cb_tcp_read()`. This is where we copy the incoming data from the `pico_socket` to the `Connection`'s `rxbuf` (different from `pico_frame_rxbuf`). We then notify the **tap service** that the `PhySocket` (a wrapped file descriptor with one end visible to the application) associated with this `Connection` has data in its `rxbuf` that needs to be written so the app can read it.

```  
pico_cb_socket_activity()
    pico_cb_tcp_read() ---> conn->rxbuf
    setNotifyWritable=TRUE
```

After some (more) time, the **tap service** thread will call `pico_handleRead()`, this will copy the data from the `rxbuf` to the `AF_UNIX` socket which links the service and your application. 

```
pico_handleRead()
 streamSend(): conn->rxbuf --- conn->sock
```

After this point it's up to your application to read the data via a conventional `read()`, `recv()`, or `recvfrom()` call.

```
read()
```


***

## Sending data

Your app performs a `write()`, `send()`, or `sendto()` call.

```
write()
```

The other end of the `AF_UNIX` socket which was written to is monitored by the **tap service** thread. Once data is read from the socket, it will call `phyOnUnixData()` which will copy the buffer contents onto the `Connection`'s `txbuf`. Then `pico_handleWrite()` will be called. The **stack driver** will determine how much of the buffer can safely be sent to the **network stack** (up to `ZT_MAX_MTU` which is currently set at 2800 bytes). A call is made to `pico_socket_write()` which copies the data from the `txbuf` to the `pico_socket`.

```
 I/O loop PhySocket
  phyOnUnixData()
   handleWrite() 
    pico_socket_write(): conn->txbuf ---> conn->picosock
```

Periodically a `PICO_SOCK_EV_WR` event will be raised by the **network stack**, this will call `pico_cb_socket_activity()` and ultimately `pico_cb_tcp_write()` where a `pico_socket_write()` call will be made to copy any remaining `txbuf` contents into the stack.

```
pico_cb_tcp_write()
```

After some time, the stack will emit an ethernet frame via `pico_eth_send()`, we then copy the frame into the **tap service** where it will then be sent onto the network.

```
pico_eth_send()
 tap->_handler()
```