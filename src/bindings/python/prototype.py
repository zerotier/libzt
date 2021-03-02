import libzt

import time
import struct
import pprint
pp = pprint.PrettyPrinter(width=41, compact=True)

class zerotier():
	# Create a socket
	def socket(sock_family, sock_type, sock_proto=0):
		return ztsocket(sock_family, sock_type, sock_proto)
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

# ZeroTier pythonic low-level socket class
class ztsocket():

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

	# Bind the socket to a local interface address
	def bind(self, local_address):
		err = libzt.zts_py_bind(self._fd, self._family, self._type, local_address)
		if (err < 0):
			zerotier.handle_error(err)

	# Connect the socket to a remote address
	def connect(self, remote_address):
		err = libzt.zts_py_connect(self._fd, self._family, self._type, remote_address)
		if (err < 0):
			zerotier.handle_error(err)

	# Put the socket in a listening state (with an optional backlog argument)
	def listen(self, backlog):
		err = libzt.zts_py_listen(self._fd, backlog)
		if (err < 0):
			zerotier.handle_error(err)

	# Accept connection on the socket
	def accept(self):
		new_conn_fd, addr, port = libzt.zts_py_accept(self._fd)
		if (new_conn_fd < 0):
			zerotier.handle_error(acc_fd)
			return None
		return ztsocket(self._family, self._type, self._proto, new_conn_fd), addr

	# Read data from the socket
	def recv(self, n_bytes, flags=0):
		err, data = libzt.zts_py_recv(self._fd, n_bytes, flags)
		if (err < 0):
			zerotier.handle_error(err)
			return None
		return data

	# Write data to the socket
	def send(self, data, flags=0):
		err = libzt.zts_py_send(self._fd, data, len(data), flags)
		if (err < 0):
			zerotier.handle_error(err)
		return err

	# Close the socket
	def close(self):
		err = libzt.zts_py_close(self._fd)
		if (err < 0):
			zerotier.handle_error(err) 
