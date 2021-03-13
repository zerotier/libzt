# Python example

This example demonstrates how to use the ZeroTier socket interface provided by libzt in a Python application. The API is designed to be a drop-in replacement for the Python [Low-level networking interface](https://docs.python.org/3/library/socket.html).

Note: Only `AF_INET` and `AF_INET6` address families are supported.

### Install

```
pip install libzt
```

### Run
```
python3 example.py server id-path/bob 0123456789abcdef 9997 8080
python3 example.py client id-path/alice 0123456789abcdef 9996 11.22.33.44 8080
```

*Where `9996` and `9997` are arbitrary ports that you allow ZeroTier to use for encrypted UDP traffic, port `8080` is an arbitrary port used by the client/server socket code, and `11.22.33.44` should be whatever IP address the network assigns your node.*

### Implementation Details

- See [src/bindings/python](../../src/bindings/python)
