import libzt

class EventCallbackClass(libzt.PythonDirectorCallbackClass):
	pass

# Convert libzt error code to exception
def handle_error(err):
	if (err == libzt.ZTS_ERR_SOCKET):
		raise Exception('ZTS_ERR_SOCKET (' + str(err) + ')')
	if (err == libzt.ZTS_ERR_SERVICE):
		raise Exception('ZTS_ERR_SERVICE (' + str(err) + ')')
	if (err == libzt.ZTS_ERR_ARG):
		raise Exception('ZTS_ERR_ARG (' + str(err) + ')')
	# ZTS_ERR_NO_RESULT isn't strictly an error
	#if (err == libzt.ZTS_ERR_NO_RESULT):
	#	raise Exception('ZTS_ERR_NO_RESULT (' + err + ')')
	if (err == libzt.ZTS_ERR_GENERAL):
		raise Exception('ZTS_ERR_GENERAL (' + str(err) + ')')

# This implementation of errno is NOT thread safe
# That is, this value is shared among all lower-level socket calls
# and may change for any reason at any time if you have multiple
# threads making socket calls.
def errno():
	return libzt.cvar.zts_errno

# Start the ZeroTier service
def start(path, callback, port):
	libzt.zts_start(path, callback, port)

# Stop the ZeroTier service
def stop():
	libzt.zts_stop()

# [debug] Restarts the ZeroTier service and network stack
def restart():
	libzt.zts_restart()

# Permenantly shuts down the network stack.
def free():
	libzt.zts_free()

# Join a ZeroTier network
def join(networkId):
	libzt.zts_join(networkId)

# Leave a ZeroTier network
def leave(networkId):
	libzt.zts_leave(networkId)

# Orbit a moon
def zts_orbit(moonWorldId, moonSeed):
    return libzt.zts_orbit(moonWorldId, moonSeed)

# De-orbit a moon
def zts_deorbit(moonWorldId):
    return libzt.zts_deorbit(moonWorldId)

# Pythonic class that wraps low-level sockets
class socket():

	_fd        = -1 # native layer file descriptor
	_family    = -1
	_type      = -1
	_proto     = -1
	_connected = False
	_closed    = True
	_bound     = False

	def __init__(self, sock_family=-1, sock_type=-1, sock_proto=-1, sock_fd=None):
		self._fd = sock_fd
		self._family = sock_family
		self._type = sock_type
		self._family = sock_family
		# Only create native socket if no fd was provided. We may have
		# accepted a connection
		if (sock_fd == None):
			self._fd = libzt.zts_socket(sock_family, sock_type, sock_proto)

	def has_dualstack_ipv6():
		return True

	@property
	def family(self):
		return _family

	@property
	def type(self):
		return _type

	@property
	def proto(self):
		return _proto

	# Intentionally not supported
	def socketpair(self, family, type, proto):
		raise NotImplementedError("socketpair(): libzt does not support AF_UNIX sockets")

	# Convenience function to create a connection to a remote host
	def create_connection(self, remote_address):
		# TODO: implement timeout
		conn = socket(libzt.ZTS_AF_INET, libzt.ZTS_SOCK_STREAM, 0)
		conn.connect(remote_address)
		return conn

	# Convenience function to create a listening socket
	def create_server(self, local_address, family=libzt.ZTS_AF_INET, backlog=None):
		# TODO: implement reuse_port
		conn = socket(libzt.ZTS_AF_INET, libzt.ZTS_SOCK_STREAM, 0)
		conn.bind(local_address)
		conn.listen(backlog)
		return conn

	def fromfd(self, fd, family, type, proto=0):
		raise NotImplementedError("fromfd(): Not supported. OS File descriptors aren't used in libzt.")

	def fromshare(self, data):
		raise NotImplementedError("libzt does not support this (yet?)")

	def close(self, fd):
		raise NotImplementedError("close(fd): Not supported OS File descriptors aren't used in libzt.")

	def getaddrinfo(self, host, port, family=0, type=0, proto=0, flags=0):
		raise NotImplementedError("libzt does not support this (yet?)")

	def getfqdn(self, name):
		raise NotImplementedError("libzt does not support this (yet?)")

	def gethostbyname(self, hostname):
		raise NotImplementedError("libzt does not support this (yet?)")

	def gethostbyname_ex(self, hostname):
		raise NotImplementedError("libzt does not support this (yet?)")

	def gethostname(self):
		raise NotImplementedError("libzt does not support this (yet?)")

	def gethostbyaddr(self, ip_address):
		raise NotImplementedError("libzt does not support this (yet?)")

	def getnameinfo(self, sockaddr, flags):
		raise NotImplementedError("libzt does not support this (yet?)")

	def getprotobyname(self, protocolname):
		raise NotImplementedError("libzt does not support this (yet?)")

	def getservbyname(self, servicename, protocolname):
		raise NotImplementedError("libzt does not support this (yet?)")

	def getservbyport(self, port, protocolname):
		raise NotImplementedError("libzt does not support this (yet?)")

	def ntohl(x):
		raise NotImplementedError("libzt does not support this (yet?)")

	def ntohs(x):
		raise NotImplementedError("libzt does not support this (yet?)")

	def htonl(x):
		raise NotImplementedError("libzt does not support this (yet?)")

	def htons(x):
		raise NotImplementedError("libzt does not support this (yet?)")

	def inet_aton(ip_string):
		raise NotImplementedError("libzt does not support this (yet?)")

	def inet_ntoa(packed_ip):
		raise NotImplementedError("libzt does not support this (yet?)")

	def inet_pton(address_family, ip_string):
		raise NotImplementedError("libzt does not support this (yet?)")

	def inet_ntop(address_family, packed_ip):
		raise NotImplementedError("libzt does not support this (yet?)")

	def CMSG_LEN(length):
		raise NotImplementedError("libzt does not support this (yet?)")

	def CMSG_SPACE(length):
		raise NotImplementedError("libzt does not support this (yet?)")

	def getdefaulttimeout(self):
		raise NotImplementedError("libzt does not support this (yet?)")

	def setdefaulttimeout(self, timeout):
		raise NotImplementedError("libzt does not support this (yet?)")

	def sethostname(self, name):
		raise NotImplementedError("libzt does not support this (yet?)")

	def if_nameindex(self):
		raise NotImplementedError("libzt does not support this (yet?)")

	def if_nametoindex(self, if_name):
		raise NotImplementedError("if_nametoindex(): libzt does not name interfaces.")

	def if_indextoname(self, if_index):
		raise NotImplementedError("if_indextoname(): libzt does not name interfaces.")

	# Accept connection on the socket
	def accept(self):
		new_conn_fd, addr, port = libzt.zts_py_accept(self._fd)
		if (new_conn_fd < 0):
			handle_error(new_conn_fd)
			return None
		return ztsocket(self._family, self._type, self._proto, new_conn_fd), addr

	# Bind the socket to a local interface address
	def bind(self, local_address):
		err = libzt.zts_py_bind(self._fd, self._family, self._type, local_address)
		if (err < 0):
			handle_error(err)

	# Close the socket
	def close(self):
		err = libzt.zts_py_close(self._fd)
		if (err < 0):
			handle_error(err)

	# Connect the socket to a remote address
	def connect(self, remote_address):
		err = libzt.zts_py_connect(self._fd, self._family, self._type, remote_address)
		if (err < 0):
			handle_error(err)

	# Connect to remote host but return low-level result code, and errno on failure
	# This uses a non-thread-safe implementation of errno
	def connect_ex(self, remote_address):
		err = libzt.zts_py_connect(self._fd, self._family, self._type, remote_address)
		if (err < 0):
			return errno()
		return err

	def detach(self):
		raise NotImplementedError("detach(): Not supported. OS File descriptors aren't used in libzt.")

	def dup(self):
		raise NotImplementedError("libzt does not support this (yet?)")

	def fileno(self):
		raise NotImplementedError("libzt does not support this (yet?)")

	def get_inheritable(self):
		raise NotImplementedError("libzt does not support this (yet?)")

	def getpeername(self):
		raise NotImplementedError("libzt does not support this (yet?)")

	def getsockname(self):
		raise NotImplementedError("libzt does not support this (yet?)")

	def getsockopt(self, level, optname, buflen):
		raise NotImplementedError("libzt does not support this (yet?)")

	# Get whether this socket is in blocking or non-blocking mode
	def getblocking(self):
		return libzt.zts_py_getblocking(self._fd)

	def gettimeout(self):
		raise NotImplementedError("libzt does not support this (yet?)")

	def ioctl(self, control, option):
		raise NotImplementedError("libzt does not support this (yet?)")

	# Put the socket in a listening state (with an optional backlog argument)
	def listen(self, backlog):
		err = libzt.zts_py_listen(self._fd, backlog)
		if (err < 0):
			handle_error(err)

	def makefile(mode='r', buffering=None, *, encoding=None, errors=None, newline=None):
		raise NotImplementedError("libzt does not support this (yet?)")

	# Read data from the socket
	def recv(self, n_bytes, flags=0):
		err, data = libzt.zts_py_recv(self._fd, n_bytes, flags)
		if (err < 0):
			handle_error(err)
			return None
		return data

	def recvfrom(self, bufsize, flags):
		raise NotImplementedError("libzt does not support this (yet?)")

	def recvmsg(self, bufsize, ancbufsize, flags):
		raise NotImplementedError("libzt does not support this (yet?)")

	def recvmsg_into(self, buffers, ancbufsize, flags):
		raise NotImplementedError("libzt does not support this (yet?)")

	def recvfrom_into(self, buffer, nbytes, flags):
		raise NotImplementedError("libzt does not support this (yet?)")

	def recv_into(self, buffer, nbytes, flags):
		raise NotImplementedError("libzt does not support this (yet?)")

	# Write data to the socket
	def send(self, data, flags=0):
		err = libzt.zts_py_send(self._fd, data, len(data), flags)
		if (err < 0):
			handle_error(err)
		return err

	def sendall(self, bytes, flags):
		raise NotImplementedError("libzt does not support this (yet?)")

	def sendto(self, bytes, address):
		raise NotImplementedError("libzt does not support this (yet?)")

	def sendto(self, bytes, flags, address):
		raise NotImplementedError("libzt does not support this (yet?)")

	def sendmsg(self, buffers, ancdata, flags, address):
		raise NotImplementedError("libzt does not support this (yet?)")

	def sendmsg_afalg(self, msg, *, op, iv, assoclen, flags):
		raise NotImplementedError("sendmsg_afalg(): libzt does not support AF_ALG")

	def send_fds(self, sock, buffers, fds, flags, address):
		raise NotImplementedError("libzt does not support this (yet?)")

	def recv_fds(self, sock, bufsize, maxfds, flags):
		raise NotImplementedError("libzt does not support this (yet?)")

	def sendfile(self, file, offset=0, count=None):
		raise NotImplementedError("libzt does not support this (yet?)")

	def set_inheritable(self, inheritable):
		raise NotImplementedError("libzt does not support this (yet?)")

	# Set whether this socket is in blocking or non-blocking mode
	def setblocking(self, flag):
		libzt.zts_py_setblocking(self._fd, flag)

	def settimeout(self, value):
		raise NotImplementedError("libzt does not support this (yet?)")

	def setsockopt(self, level, optname, value):
		# TODO: value: buffer
		# TODO: value: int
		# TODO: value: None -> optlen required
		raise NotImplementedError("libzt does not support this (yet?)")

	# Shut down one or more aspects (rx/tx) of the socket
	def shutdown(self, how):
		libzt.shutdown(self._fd, how)
