How Data is Sent and Received by an App using the SDK
======

*Note: It is not necessary for you to understand anything in this document, this is merely for those curious about the inner workings of the intercept, tap service, stack driver, and network stack.*

## Receiving data

The **tap service** monitors incoming packets, when one destined for us is detected it notifies the **stack driver** via `put()`. Then `pico_rx()` is called, its job is to re-encapsulate the ethernet frame and copy it onto the guarded `pico_frame_rxbuf`. This buffer is guarded because it is accessed via the **tap service** thread and the **network stack** thread.

```
SERVICE_THREAD
wire
 put()
  pico_rx() ---> <pico_frame_rxbuf>
```

Periodically the stack thread will call `pico_eth_poll()`, this is responsible for reading the frames from the aformentioned RX I/O frame buffer and feeding it into the stack via `pico_stack_recv()`.

```
STACK_THREAD 
pico_eth_poll()
 <pico_frame_rxbuf> ---> pico_stack_recv
```

After some time has passed and the stack has processed the incoming frames a `PICO_SOCK_EV_RD` event will be triggered which calls `pico_cb_socket_activity()`, and ultimately `pico_cb_tcp_read()`. This is where we copy the incoming data from the `pico_socket` to the `Connection`'s `rxbuf`. We then notify the ZeroTier tap service that the `PhySocket` (a wrapped file descriptor with one end visible to the application) associated with with this `Connection` has data in the `rxbuf` that needs to be written to it.  

```  
STACK_THREAD
pico_cb_socket_activity()
    pico_cb_tcp_read() ---> conn->rxbuf
    setNotifyWritable=TRUE
```

After some (more) time, the ZeroTier tap service thread will call `pico_handleRead()`, this will copy the data from the `rxbuf` to the `AF_UNIX` socket which links the service and your application. After this point it's up to you application to read the data via a conventional `read()`, `recv()`, or `recvfrom()` call.

```
SERVICE_THREAD
pico_handleRead()x
 streamSend(): conn->rxbuf --- conn->sock
```

...


APP_THREAD
 read()

***

## Sending data

APP_THREAD
 write(): <DATA> ---> fd

SERVICE_THREAD
 I/O loop PhySocket
  phyOnUnixData()
   handleWrite() 
    pico_socket_write(): conn->txbuf ---> conn->picosock

STACK_THREAD
 pico_cb_socket_activity
  pico_cb_tcp_write()
   pico_socket_write()

