"""ZeroTier low-level socket interface"""

import libzt

class EventCallbackClass(libzt.PythonDirectorCallbackClass):
    """ZeroTier event callback class"""
    pass

def handle_error(err):
    """Convert libzt error code to exception"""
    if err == libzt.ZTS_ERR_SOCKET:
        raise Exception("ZTS_ERR_SOCKET (" + str(err) + ")")
    if err == libzt.ZTS_ERR_SERVICE:
        raise Exception("ZTS_ERR_SERVICE (" + str(err) + ")")
    if err == libzt.ZTS_ERR_ARG:
        raise Exception("ZTS_ERR_ARG (" + str(err) + ")")
    # ZTS_ERR_NO_RESULT isn't strictly an error
    # if (err == libzt.ZTS_ERR_NO_RESULT):
    #   raise Exception('ZTS_ERR_NO_RESULT (' + err + ')')
    if err == libzt.ZTS_ERR_GENERAL:
        raise Exception("ZTS_ERR_GENERAL (" + str(err) + ")")

# This implementation of errno is NOT thread safe
# That is, this value is shared among all lower-level socket calls
# and may change for any reason at any time if you have multiple
# threads making socket calls.
def errno():
    """Return errno value of low-level socket layer"""
    return libzt.cvar.zts_errno

def start(path, callback, port):
    """Start the ZeroTier service"""
    libzt.zts_start(path, callback, port)

def stop():
    """Stop the ZeroTier service"""
    libzt.zts_stop()

def restart():
    """[debug] Restarts the ZeroTier service and network stack"""
    libzt.zts_restart()

def free():
    """Permenantly shuts down the network stack"""
    libzt.zts_free()

def join(network_id):
    """Join a ZeroTier network"""
    libzt.zts_join(network_id)

def leave(network_id):
    """Leave a ZeroTier network"""
    libzt.zts_leave(network_id)

def zts_orbit(moon_world_id, moon_seed):
    """Orbit a moon"""
    return libzt.zts_orbit(moon_world_id, moon_seed)

def zts_deorbit(moon_world_id):
    """De-orbit a moon"""
    return libzt.zts_deorbit(moon_world_id)


class socket:
    """Pythonic class that wraps low-level sockets"""
    _fd = -1  # native layer file descriptor
    _family = -1
    _type = -1
    _proto = -1
    _connected = False
    _closed = True
    _bound = False

    def __init__(self, sock_family=-1, sock_type=-1, sock_proto=-1, sock_fd=None):
        self._fd = sock_fd
        self._family = sock_family
        self._type = sock_type
        self._family = sock_family
        # Only create native socket if no fd was provided. We may have
        # accepted a connection
        if sock_fd is None:
            self._fd = libzt.zts_socket(sock_family, sock_type, sock_proto)

    def has_dualstack_ipv6(self):
        """Return whether libzt supports dual stack sockets: yes"""
        return True

    @property
    def family(self):
        """Return family of socket"""
        return self._family

    @property
    def type(self):
        """Return type of socket"""
        return self._type

    @property
    def proto(self):
        """Return protocol of socket"""
        return self._proto

    def socketpair(self, sock_family, sock_type, sock_proto):
        """Intentionally not supported"""
        raise NotImplementedError(
            "socketpair(): libzt does not support AF_UNIX sockets"
        )

    def create_connection(self, remote_address):
        """Convenience function to create a connection to a remote host"""
        # TODO: implement timeout
        conn = socket(libzt.ZTS_AF_INET, libzt.ZTS_SOCK_STREAM, 0)
        conn.connect(remote_address)
        return conn

    def create_server(self, local_address, sock_family=libzt.ZTS_AF_INET, backlog=None):
        """Convenience function to create a listening socket"""
        # TODO: implement reuse_port
        conn = socket(sock_family, libzt.ZTS_SOCK_STREAM, 0)
        conn.bind(local_address)
        conn.listen(backlog)
        return conn

    def fromfd(self, fd, sock_family, sock_type, sock_proto=0):
        """libzt does not support this (yet)"""
        raise NotImplementedError(
            "fromfd(): Not supported. OS File descriptors aren't used in libzt."
        )

    def fromshare(self, data):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def getaddrinfo(self, host, port, sock_family=0, sock_type=0, sock_proto=0, flags=0):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def getfqdn(self, name):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def gethostbyname(self, hostname):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def gethostbyname_ex(self, hostname):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def gethostname(self):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def gethostbyaddr(self, ip_address):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def getnameinfo(self, sockaddr, flags):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def getprotobyname(self, protocolname):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def getservbyname(self, servicename, protocolname):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def getservbyport(self, port, protocolname):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def ntohl(x):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def ntohs(x):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def htonl(x):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def htons(x):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def inet_aton(ip_string):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def inet_ntoa(packed_ip):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def inet_pton(address_family, ip_string):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def inet_ntop(address_family, packed_ip):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def CMSG_LEN(length):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def CMSG_SPACE(length):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def getdefaulttimeout(self):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def setdefaulttimeout(self, timeout):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def sethostname(self, name):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def if_nameindex(self):
        """libzt does not support this"""
        raise NotImplementedError("if_nameindex(): libzt does not name interfaces.")

    def if_nametoindex(self, if_name):
        """libzt does not support this"""
        raise NotImplementedError("if_nametoindex(): libzt does not name interfaces.")

    def if_indextoname(self, if_index):
        """libzt does not support this"""
        raise NotImplementedError("if_indextoname(): libzt does not name interfaces.")

    def accept(self):
        """Accept connection on the socket"""
        new_conn_fd, addr, port = libzt.zts_py_accept(self._fd)
        if new_conn_fd < 0:
            handle_error(new_conn_fd)
            return None
        return socket(self._family, self._type, self._proto, new_conn_fd), addr

    def bind(self, local_address):
        """Bind the socket to a local interface address"""
        err = libzt.zts_py_bind(self._fd, self._family, self._type, local_address)
        if err < 0:
            handle_error(err)

    def close(self):
        """Close the socket"""
        err = libzt.zts_py_close(self._fd)
        if err < 0:
            handle_error(err)

    def connect(self, remote_address):
        """Connect the socket to a remote address"""
        err = libzt.zts_py_connect(self._fd, self._family, self._type, remote_address)
        if err < 0:
            handle_error(err)

    def connect_ex(self, remote_address):
        """Connect to remote host but return low-level result code, and errno on failure
        This uses a non-thread-safe implementation of errno
        """
        err = libzt.zts_py_connect(self._fd, self._family, self._type, remote_address)
        if err < 0:
            return errno()
        return err

    def detach(self):
        """libzt does not support this"""
        raise NotImplementedError(
            "detach(): Not supported. OS File descriptors aren't used in libzt.")

    def dup(self):
        """libzt does not support this"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def fileno(self):
        """libzt does not support this"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def get_inheritable(self):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def getpeername(self):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def getsockname(self):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def getsockopt(self, level, optname, buflen):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def getblocking(self):
        """Get whether this socket is in blocking or non-blocking mode"""
        return libzt.zts_py_getblocking(self._fd)

    def gettimeout(self):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def ioctl(self, control, option):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def listen(self, backlog):
        """Put the socket in a listening state (with an optional backlog argument)"""
        err = libzt.zts_py_listen(self._fd, backlog)
        if err < 0:
            handle_error(err)

    def makefile(mode="r", buffering=None, *, encoding=None, errors=None, newline=None):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def recv(self, n_bytes, flags=0):
        """Read data from the socket"""
        err, data = libzt.zts_py_recv(self._fd, n_bytes, flags)
        if err < 0:
            handle_error(err)
            return None
        return data

    def recvfrom(self, bufsize, flags):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def recvmsg(self, bufsize, ancbufsize, flags):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def recvmsg_into(self, buffers, ancbufsize, flags):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def recvfrom_into(self, buffer, n_bytes, flags):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def recv_into(self, buffer, n_bytes, flags):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def send(self, data, flags=0):
        """Write data to the socket"""
        err = libzt.zts_py_send(self._fd, data, len(data), flags)
        if err < 0:
            handle_error(err)
        return err

    def sendall(self, n_bytes, flags):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def sendto(self, n_bytes, flags, address):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def sendmsg(self, buffers, ancdata, flags, address):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def sendmsg_afalg(self, msg, *, op, iv, assoclen, flags):
        """libzt does not support this (yet)"""
        raise NotImplementedError("sendmsg_afalg(): libzt does not support AF_ALG")

    def send_fds(self, sock, buffers, fds, flags, address):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def recv_fds(self, sock, bufsize, maxfds, flags):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def sendfile(self, file, offset=0, count=None):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def set_inheritable(self, inheritable):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def setblocking(self, flag):
        """Set whether this socket is in blocking or non-blocking mode"""
        libzt.zts_py_setblocking(self._fd, flag)

    def settimeout(self, value):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    # TODO: value: buffer
    # TODO: value: int
    # TODO: value: None -> optlen required
    def setsockopt(self, level, optname, value):
        """libzt does not support this (yet)"""
        raise NotImplementedError("libzt does not support this (yet?)")

    def shutdown(self, how):
        """Shut down one or more aspects (rx/tx) of the socket"""
        libzt.zts_shutdown(self._fd, how)
