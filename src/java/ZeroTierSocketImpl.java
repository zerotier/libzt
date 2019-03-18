package com.zerotier.libzt;

import com.zerotier.libzt.ZeroTier;
import com.zerotier.libzt.ZeroTierSocketAddress;
import com.zerotier.libzt.ZeroTierInputStream;
import com.zerotier.libzt.ZeroTierOutputStream;
import com.zerotier.libzt.ZeroTierSocketOptionValue;

import java.io.*;
import java.net.*;
import java.util.Objects;
import java.lang.Object;
import java.net.SocketImpl;
import java.net.InetSocketAddress;
import java.util.Set;
import java.lang.Boolean;

import com.zerotier.libzt.ZeroTier;

public class ZeroTierSocketImpl extends SocketImpl
{
	private int defaultProtocol = 0;

	/*
	 * File descriptor from lower native layer
	 */
	private int zfd = -1;
	private int zfd4 = -1;
	private int zfd6 = -1;

	/*
	 * Input and Output streams
	 */
	private ZeroTierInputStream in = new ZeroTierInputStream();
	private ZeroTierOutputStream out = new ZeroTierOutputStream();

	Socket socket = null;
	ServerSocket serverSocket = null;

	/*
	 * The remote address the socket is connected to
	 */
	protected InetAddress address;

	/*
	 * Sets the underlying file descriptor valud for the SocketImpl as well as the Input/OutputStream
	 */
	private void setNativeFileDescriptor(int fd)
	{
		zfd = fd;
		in.zfd = fd;
		out.zfd = fd;
	}

	/*
	 * Various socket options that are cached from calls to setOption() before
	 * the socket exists.
	 */
	private int _so_rcvtimeo;
	private boolean _so_keepalive;
	private int _so_sndbuf;
	private int _so_rcvbuf;
	private boolean _so_reuseaddr;
	private int _so_linger;
	private int _so_tos;
	private boolean _so_nodelay;

	private void setCachedSocketOptions(int fd)
	{
		if (fd < 0) {
			return;
		}

		// If we previously received a setSoTimeout() call but were unable to process it due
		// to the fact that the underlying socket didn't even exist yet, do so now.
		int err = 0;
		ZeroTierSocketOptionValue optval = new ZeroTierSocketOptionValue();

		try {
			// SO_TIMEOUT
			if (_so_rcvtimeo > 0) {
				optval.isInteger = true;
				optval.isBoolean = false;
				optval.integerValue = ((Integer)_so_rcvtimeo).intValue();
				if ((err = ZeroTier.setsockopt(fd, ZeroTier.SOL_SOCKET, ZeroTier.SO_RCVTIMEO, optval)) < 0) {
					throw new IOException("socket("+fd+"), errno="+err+", unable to set previously cached SO_RCVTIMEO");
				}
			}
			// SO_KEEPALIVE
			if (_so_keepalive == true) {
				optval.isInteger = false;
				optval.isBoolean = true;
				optval.booleanValue = ((Boolean)_so_keepalive).booleanValue() ? true : false;
				if ((err = ZeroTier.setsockopt(fd, ZeroTier.SOL_SOCKET, ZeroTier.SO_KEEPALIVE, optval)) < 0) {
					throw new IOException("socket("+fd+"), errno="+err+", unable to set previously cached SO_KEEPALIVE");
				}
			}
			// SO_SNDBUF
			if (_so_sndbuf > 0) {
				optval.isInteger = true;
				optval.isBoolean = false;
				optval.integerValue = ((Integer)_so_sndbuf).intValue();
				if ((err = ZeroTier.setsockopt(fd, ZeroTier.SOL_SOCKET, ZeroTier.SO_SNDBUF, optval)) < 0) {
					throw new IOException("socket("+fd+"), errno="+err+", unable to set previously cached SO_SNDBUF");
				}
			}
			// SO_RCVBUF
			if (_so_rcvbuf > 0) {
				optval.isInteger = true;
				optval.isBoolean = false;
				optval.integerValue = ((Integer)_so_rcvbuf).intValue();
				if ((err = ZeroTier.setsockopt(fd, ZeroTier.SOL_SOCKET, ZeroTier.SO_RCVBUF, optval)) < 0) {
					throw new IOException("socket("+fd+"), errno="+err+", unable to set previously cached SO_RCVBUF");
				}
			}
			// SO_REUSEADDR
			if (_so_reuseaddr == true) {
				optval.isInteger = false;
				optval.isBoolean = true;
				optval.booleanValue = ((Boolean)_so_reuseaddr).booleanValue() ? true : false;
				if ((err = ZeroTier.setsockopt(fd, ZeroTier.SOL_SOCKET, ZeroTier.SO_REUSEADDR, optval)) < 0) {
					throw new IOException("socket("+fd+"), errno="+err+", unable to set previously cached SO_REUSEADDR");
				}
			}
			// SO_LINGER
			if (_so_linger > 0) {
				optval.isInteger = true;
				optval.isBoolean = false;
				optval.integerValue = ((Integer)_so_linger).intValue();
				if ((err = ZeroTier.setsockopt(fd, ZeroTier.SOL_SOCKET, ZeroTier.SO_LINGER, optval)) < 0) {
					throw new IOException("socket("+fd+"), errno="+err+", unable to set previously cached SO_LINGER");
				}
			}
			// IP_TOS
			if (_so_tos > 0) {
				optval.isInteger = true;
				optval.isBoolean = false;
				optval.integerValue = ((Integer)_so_tos).intValue();
				if ((err = ZeroTier.setsockopt(fd, ZeroTier.SOL_SOCKET, ZeroTier.IP_TOS, optval)) < 0) {
					throw new IOException("socket("+fd+"), errno="+err+", unable to set previously cached IP_TOS");
				}
			}
			// TCP_NODELAY
			if (_so_nodelay == true) {
				optval.isInteger = false;
				optval.isBoolean = true;
				optval.booleanValue = ((Boolean)_so_nodelay).booleanValue() ? true : false;
				if ((err = ZeroTier.setsockopt(fd, ZeroTier.SOL_SOCKET, ZeroTier.TCP_NODELAY, optval)) < 0) {
					throw new IOException("socket("+fd+"), errno="+err+", unable to set previously cached TCP_NODELAY");
				}
			}
		}
		catch (Exception e) {
			System.err.println(e);
		}
	}

	/*
	 * Constructor which creates a new ZeroTierSocketImpl
	 */
	public ZeroTierSocketImpl()
	{
		if ((zfd > -1) | (zfd4 > -1) | (zfd6 > -1)) { return; }
		try {
			create(true);
		} catch (Exception x) {
			System.err.println("error creating ZeroTierSocketImpl instance: " + x);
		}
		in.zfd = zfd;
		out.zfd = zfd;
	}

	/*
	 * Constructor to be called when an underlying ZeroTier socket already exists (does not create a new ZeroTierSocketImpl)
	 */
	public ZeroTierSocketImpl(int fd) {
		setNativeFileDescriptor(fd);
	}

	/*
	 * Creates a new ZeroTier socket in the native layer
	 */
	protected void create(boolean stream)
		throws IOException
	{
		/*
		 * The native-layer socket is only created once a connect/bind call is made, this is due to the fact
		 * that beforehand we have no way to determine whether we should create an AF_INET or AF_INET6 socket,
		 * as a result, this method intentionally does nothing.
		 */
	}

	/*
	 * Creates the underlying libzt socket.
	 * 
	 * This does the real work that can't be done in the constructor. This is because the socket address type
	 * isn't known until a connect() or bind() request is given. Additionally we cache the value provided by any
	 * setSoTimeout() calls and implement it immediately after creation.
	 */
	private void createAppropriateSocketImpl(InetAddress addr)
		throws IOException
	{
		if ((zfd > -1) | (zfd4 > -1) | (zfd6 > -1))  {
			return; // Socket already created
		}
		if(addr instanceof Inet4Address) {
			if ((zfd4 = ZeroTier.socket(ZeroTier.AF_INET, ZeroTier.SOCK_STREAM, defaultProtocol)) < 0) {
				throw new IOException("socket(), errno="+zfd4+", see: libzt/ext/lwip/src/include/errno.h");
			}
			setCachedSocketOptions(zfd4);
		}
		/*
		* Since Java creates sockets capable of handling IPV4 and IPV6, we must simulate this. We do this by
		* creating two sockets (one of each type) 
		*/
		if(addr instanceof Inet6Address) {
			if ((zfd4 = ZeroTier.socket(ZeroTier.AF_INET, ZeroTier.SOCK_STREAM, defaultProtocol)) < 0) {
				throw new IOException("socket(), errno="+zfd4+", see: libzt/ext/lwip/src/include/errno.h");
			}
			if ((zfd6 = ZeroTier.socket(ZeroTier.AF_INET6, ZeroTier.SOCK_STREAM, defaultProtocol)) < 0) {
				throw new IOException("socket(), errno="+zfd6+", see: libzt/ext/lwip/src/include/errno.h");
			}
			setCachedSocketOptions(zfd4);
			setCachedSocketOptions(zfd6);
		}
	}

	/*
	 * Return the remote address the socket is connected to
	 */
	protected InetAddress getInetAddress()
	{
		return address;
	}

	/*
	 * Connects the socket to a remote address
	 */
	protected void connect(String host, int port)
		throws IOException
	{
		// TODO: Refactor and consolidate the connect() logic for all three methods
		createAppropriateSocketImpl(InetAddress.getByName(host));
		if ((zfd4 < 0) & (zfd6 < 0)) {
			throw new IOException("invalid fd");
		}
		int err;
		InetAddress address = InetAddress.getByName(host);
		ZeroTierSocketAddress zt_addr = new ZeroTierSocketAddress(host, port);
		if (address instanceof Inet4Address) {
			if ((err = ZeroTier.connect(zfd4, zt_addr)) < 0) {
				throw new IOException("connect(), errno="+err+", see: libzt/ext/lwip/src/include/errno.h");
			} 
			setNativeFileDescriptor(zfd4);
		}
		if (address instanceof Inet6Address) {
			if ((err = ZeroTier.connect(zfd6, zt_addr)) < 0) {
				throw new IOException("connect(), errno="+err+", see: libzt/ext/lwip/src/include/errno.h");
			} 
			setNativeFileDescriptor(zfd6);
		}
		super.port = port;
	}

	/*
	 * Connects the socket to a remote address
	 */
	protected void connect(InetAddress address, int port)
		throws IOException
	{
		// TODO: Refactor and consolidate the connect() logic for all three methods
		createAppropriateSocketImpl(address);
		if ((zfd4 < 0) & (zfd6 < 0)) {
			throw new IOException("invalid fd");
		}
		int err;
		ZeroTierSocketAddress zt_addr = new ZeroTierSocketAddress(address.getHostAddress(), port);
		if (address instanceof Inet4Address) {
			if ((err = ZeroTier.connect(zfd4, zt_addr)) < 0) {
				throw new IOException("connect(), errno="+err+", see: libzt/ext/lwip/src/include/errno.h");
			} 
			setNativeFileDescriptor(zfd4);
		}
		if (address instanceof Inet6Address) {
			if ((err = ZeroTier.connect(zfd6, zt_addr)) < 0) {
				throw new IOException("connect(), errno="+err+", see: libzt/ext/lwip/src/include/errno.h");
			} 
			setNativeFileDescriptor(zfd6);
		}
		super.port = port;
	}

	/*
	 * Connects the socket to a remote address
	 */
	protected void connect(SocketAddress address, int timeout)
		throws IOException
	{
		// TODO: Refactor and consolidate the connect() logic for all three methods
		//System.out.println("host="+((InetSocketAddress)address).getHostString()+", port="+((InetSocketAddress)address).getPort() + ", timeout="+timeout);
		createAppropriateSocketImpl(((InetSocketAddress)address).getAddress());
		if ((zfd4 < 0) & (zfd6 < 0)) {
			throw new IOException("invalid fd");
		}
		ZeroTierSocketAddress zt_addr = null;
		int err;
		int port = ((InetSocketAddress)address).getPort();
		if (((InetSocketAddress)address).getAddress() instanceof Inet4Address) {
			zt_addr = new ZeroTierSocketAddress(((InetSocketAddress)address).getHostString(), ((InetSocketAddress)address).getPort());
			if ((err = ZeroTier.connect(zfd4, zt_addr)) < 0) {
				throw new IOException("connect("+zfd4+"), errno="+err+", see: libzt/ext/lwip/src/include/errno.h");
			}
			setNativeFileDescriptor(zfd4);
		}
		if (((InetSocketAddress)address).getAddress() instanceof Inet6Address) {
			zt_addr = new ZeroTierSocketAddress(((InetSocketAddress)address).getHostString(), ((InetSocketAddress)address).getPort());
			if ((err = ZeroTier.connect(zfd6, zt_addr)) < 0) {
				throw new IOException("connect("+zfd6+"), errno="+err+", see: libzt/ext/lwip/src/include/errno.h");
			}
			setNativeFileDescriptor(zfd6);
		}
		super.port = port;
	}

	/*
	 * Binds the socket to a local address.
	 * 
	 * If this gets a bind() request on [::] it will create a both an IPv4 and an IPv6
	 * socket. This is because we might receive a subsequent listen() and accept() request
	 * and want to accept an IPv6 connection. (or) we may get a connect() request with
	 * an IPv4 address. In the latter case we must abandon the IPv6 socket and use the IPv4
	 * socket exclusively.
	 */
	protected void bind(InetAddress host, int port)
		throws IOException
	{
		createAppropriateSocketImpl(host);
		/*
		After this point we may have either a) created a single IPv4 socket, or b) created
		an IPv4 and IPv6 socket in anticipation of either verion being used
		*/
		//System.out.println("host="+host.toString()+", port="+port);
		int err;
		if ((zfd < 0) & (zfd4 < 0) & (zfd6 < 0)) {
			throw new IOException("invalid fd");
		}
		ZeroTierSocketAddress zt_addr = new ZeroTierSocketAddress(host.getHostAddress(), port);
		
		if (zfd6 > -1) {
			// Since an IPv6 socket and accept IPv4 connections we will only bind to this address
			if ((err = ZeroTier.bind(zfd6, zt_addr)) < 0) {
				throw new IOException("bind("+zfd6+"), errno="+err+", see: libzt/ext/lwip/src/include/errno.h");
			}
			super.localport = port;
			return;
		}
		if (zfd4 > -1) {
			// Otherwise, just bind to the regular IPv4 address
			if ((err = ZeroTier.bind(zfd4, zt_addr)) < 0) {
				throw new IOException("bind("+zfd4+"), errno="+err+", see: libzt/ext/lwip/src/include/errno.h");
			}
			super.localport = port;
			return;
		}
		
	}

	/*
	 * Puts the socket into a listening state.
	 * 
	 * We listen on the IPv6 socket since it can listen for IPv4 connections
	 */
	protected void listen(int backlog)
		throws IOException
	{
		int err;
		if ((zfd6 < 0) | (backlog < 0)) {
			throw new IOException("invalid fd and/or backlog");
		}
		if ((err = ZeroTier.listen(zfd6, backlog)) < 0) {
			throw new IOException("listen("+zfd6+"), errno="+err+", see: libzt/ext/lwip/src/include/errno.h");
		}
	}

	/*
	 * Accepts an incoming connection.
	 * 
	 * We accept on the IPv6 socket since it can accept IPv4 connections
	 */
	protected void accept(SocketImpl si)
		throws IOException
	{
		if (zfd6 < 0) {
			throw new IOException("invalid fd");
		}
		int accetpedFd = -1;
		ZeroTierSocketAddress addr = new ZeroTierSocketAddress();
		if ((accetpedFd = ZeroTier.accept(zfd6, addr)) < 0) {
			throw new IOException("accept("+zfd6+"), errno="+accetpedFd+", see: libzt/ext/lwip/src/include/errno.h");
		}
		// Give the new socket fd from the native layer to the new unconnected ZeroTierSocketImpl
		((ZeroTierSocketImpl)si).setFileDescriptor(accetpedFd);
	}

	/*
	 * Returns the input stream for this socket
	 */
	protected ZeroTierInputStream getInputStream()
		throws IOException
	{
		if (in == null) {
			throw new IOException();
		}
		return in;
	}

	/*
	 * Returns the output stream for this socket
	 */
	protected ZeroTierOutputStream getOutputStream()
		throws IOException
	{
		if (out == null) {
			throw new IOException();
		}
		return out;
	}

	/*
	 * Returns the remote port to which this socket is connected
	 */
	protected int getPort()
	{
		return super.port;
	}

	/*
	 * Returns the local port to which this socket is bound
	 */
	protected int getLocalPort()
	{
		return super.localport;
	}

	/*
	 * Returns whether this socket implementation supports urgent data (hint: it doesn't)
	 */
	protected boolean supportsUrgentData()
	{
        return false;
    }

	/*
	 *
	 */
	void setSocket(ZeroTierSocket soc)
	{
		this.socket = soc;
	}

	/*
	 *
	 */
	Socket getSocket()
	{
		return socket;
	}

	/*
	 *
	 */
	/*
	void setServerSocket(ZeroTierServerSocket soc)
	{
		this.serverSocket = soc;
	}
	*/
	
	/*
	 *
	 */
	ServerSocket getServerSocket()
	{
		return serverSocket;
	}

	/*
	 * Return the number of bytes that can be read from the socket without blocking
	 */
	protected int available()
		throws IOException
	{
		// TODO
		return 0;
	}

	/*
	 * Closes the socket
	 */
	protected void close()
		throws IOException
	{
		if (zfd > -1) {
			ZeroTier.close(zfd);
		}
		if (zfd4 > -1) {
			ZeroTier.close(zfd4);
		}
		if (zfd6 > -1) {
			ZeroTier.close(zfd6);
		}
	}

	/*
	 * Send one byte of urgent data on the socket
	 */
	protected void sendUrgentData(int data)
		throws IOException
	{
		System.err.println("sendUrgentData: ZeroTierSocketImpl does not currently support this feature");
	}

	/*
	 * Gets some specified socket option
	 */
	public Object getOption(int optID)
		throws SocketException
	{
		// Call native layer
		ZeroTierSocketOptionValue optval = new ZeroTierSocketOptionValue();
		int option = -1;
		int level = -1;

		if (zfd < 0) { // If we haven't committed to a socket version yet, cache the value
			if (optID == SocketOptions.SO_TIMEOUT || optID == ZeroTier.SO_RCVTIMEO) {
				return Integer.valueOf(_so_rcvtimeo);
			}
			if (optID == SocketOptions.SO_KEEPALIVE || optID == ZeroTier.SO_KEEPALIVE) {
				return Boolean.valueOf(_so_keepalive);
			}
			if (optID == SocketOptions.SO_SNDBUF || optID == ZeroTier.SO_SNDBUF) {
				return Integer.valueOf(_so_sndbuf);
			}
			if (optID == SocketOptions.SO_RCVBUF || optID == ZeroTier.SO_RCVBUF) {
				return Integer.valueOf(_so_rcvbuf);
			}
			if (optID == SocketOptions.SO_REUSEADDR || optID == ZeroTier.SO_REUSEADDR) {
				return Boolean.valueOf(_so_reuseaddr);
			}
			if (optID == SocketOptions.SO_LINGER || optID == ZeroTier.SO_LINGER) {
				return Integer.valueOf(_so_linger);
			}
			if (optID == SocketOptions.IP_TOS || optID == ZeroTier.IP_TOS) {
				return Integer.valueOf(_so_tos);
			}
			if (optID == SocketOptions.TCP_NODELAY || optID == ZeroTier.TCP_NODELAY) {
				return Boolean.valueOf(_so_nodelay);
			}
		}
		else {
			if (optID == SocketOptions.SO_TIMEOUT || optID == ZeroTier.SO_RCVTIMEO) {
				option = ZeroTier.SO_RCVTIMEO;
				level = ZeroTier.SOL_SOCKET;
			}
			if (optID == SocketOptions.SO_KEEPALIVE || optID == ZeroTier.SO_KEEPALIVE) {
				option = ZeroTier.SO_KEEPALIVE;
				level = ZeroTier.SOL_SOCKET;
			}
			if (optID == SocketOptions.SO_SNDBUF || optID == ZeroTier.SO_SNDBUF) {
				option = ZeroTier.SO_SNDBUF;
				level = ZeroTier.SOL_SOCKET;
			}
			if (optID == SocketOptions.SO_RCVBUF || optID == ZeroTier.SO_RCVBUF) {
				option = ZeroTier.SO_RCVBUF;
				level = ZeroTier.SOL_SOCKET;
			}
			if (optID == SocketOptions.SO_REUSEADDR || optID == ZeroTier.SO_REUSEADDR) {
				option = ZeroTier.SO_REUSEADDR;
				level = ZeroTier.SOL_SOCKET;
			}
			if (optID == SocketOptions.SO_LINGER || optID == ZeroTier.SO_LINGER) {
				option = ZeroTier.SO_LINGER;
				level = ZeroTier.SOL_SOCKET;
			}
			// IP
			if (optID == SocketOptions.IP_TOS || optID == ZeroTier.IP_TOS) {
				option = ZeroTier.IP_TOS;
				level = ZeroTier.IPPROTO_IP;
			}
			// TCP
			if (optID == SocketOptions.TCP_NODELAY || optID == ZeroTier.TCP_NODELAY) {
				option = ZeroTier.TCP_NODELAY;
				level = ZeroTier.IPPROTO_TCP;
			}
			ZeroTier.getsockopt(zfd, level, option, optval);
			// Convert native layer's response into Java object of some sort
			if (optval.isBoolean) {
				return Boolean.valueOf(optval.booleanValue);
			}
			if (optval.isInteger) {
				return Integer.valueOf(optval.integerValue);
			}
		}	
		return null;
	}

	/*
	 * Sets a socket option to a specified value. This method should be able to handle SocketOptions values
	 * as well as native ZeroTier.* options
	 */
	public void setOption(int optID, Object value)
		throws SocketException
	{
		if (value == null) {
			throw new UnsupportedOperationException();
		}

		int option = -1;
		int level = -1;

		ZeroTierSocketOptionValue optval = new ZeroTierSocketOptionValue();

		if (zfd < 0) { // If we haven't committed to a socket version yet, cache the value
			if (optID == SocketOptions.SO_TIMEOUT || optID == ZeroTier.SO_RCVTIMEO) {
				_so_rcvtimeo = ((Integer)value).intValue(); return;
			}
			if (optID == SocketOptions.SO_KEEPALIVE || optID == ZeroTier.SO_KEEPALIVE) {
				_so_keepalive = ((Boolean)value).booleanValue() ? true : false; return;
			}
			if (optID == SocketOptions.SO_SNDBUF || optID == ZeroTier.SO_SNDBUF) {
				_so_sndbuf = ((Integer)value).intValue(); return;
			}
			if (optID == SocketOptions.SO_RCVBUF || optID == ZeroTier.SO_RCVBUF) {
				_so_rcvbuf = ((Integer)value).intValue(); return;
			}
			if (optID == SocketOptions.SO_REUSEADDR || optID == ZeroTier.SO_REUSEADDR) {
				_so_reuseaddr = ((Boolean)value).booleanValue() ? true : false; return;
			}
			if (optID == SocketOptions.SO_LINGER || optID == ZeroTier.SO_LINGER) {
				_so_linger = ((Integer)value).intValue(); return;
			}
			if (optID == SocketOptions.IP_TOS || optID == ZeroTier.IP_TOS) {
				_so_tos = ((Integer)value).intValue(); return;
			}
			if (optID == SocketOptions.TCP_NODELAY || optID == ZeroTier.TCP_NODELAY) {
				_so_nodelay = ((Boolean)value).booleanValue() ? true : false; return;
			}
		}
		else {
			// SOL
			if (optID == SocketOptions.SO_TIMEOUT || optID == ZeroTier.SO_RCVTIMEO) {
				option = ZeroTier.SO_RCVTIMEO;
				level = ZeroTier.SOL_SOCKET;
				if (value instanceof Integer) {
					optval.isInteger = true;
					optval.integerValue = ((Integer)value).intValue();
				}
			}

			if (optID == SocketOptions.SO_KEEPALIVE || optID == ZeroTier.SO_KEEPALIVE) {
				option = ZeroTier.SO_KEEPALIVE;
				level = ZeroTier.SOL_SOCKET;
				if (value instanceof Integer) {
					optval.isBoolean = true;
					optval.booleanValue = ((Boolean)value).booleanValue() ? true : false;
				}
			}
			if (optID == SocketOptions.SO_SNDBUF || optID == ZeroTier.SO_SNDBUF) {
				option = ZeroTier.SO_SNDBUF;
				level = ZeroTier.SOL_SOCKET;
				if (value instanceof Integer) {
					optval.isInteger = true;
					optval.integerValue = ((Integer)value).intValue();
				}
			}
			if (optID == SocketOptions.SO_RCVBUF || optID == ZeroTier.SO_RCVBUF) {
				option = ZeroTier.SO_RCVBUF;
				level = ZeroTier.SOL_SOCKET;
				if (value instanceof Integer) {
					optval.isInteger = true;
					optval.integerValue = ((Integer)value).intValue();
				}
			}
			if (optID == SocketOptions.SO_REUSEADDR || optID == ZeroTier.SO_REUSEADDR) {
				option = ZeroTier.SO_REUSEADDR;
				level = ZeroTier.SOL_SOCKET;
				if (value instanceof Integer) {
					optval.isBoolean = true;
					optval.booleanValue = ((Boolean)value).booleanValue() ? true : false;
				}
			}
			if (optID == SocketOptions.SO_LINGER || optID == ZeroTier.SO_LINGER) {
				option = ZeroTier.SO_LINGER;
				level = ZeroTier.SOL_SOCKET;
				if (value instanceof Integer) {
					optval.isInteger = true;
					optval.integerValue = ((Integer)value).intValue();
				}
			}
			// IP
			if (optID == SocketOptions.IP_TOS || optID == ZeroTier.IP_TOS) {
				option = ZeroTier.IP_TOS;
				level = ZeroTier.IPPROTO_IP;
				if (value instanceof Integer) {
					optval.isInteger = true;
					optval.integerValue = ((Integer)value).intValue();
				}
			}
			// TCP
			if (optID == SocketOptions.TCP_NODELAY || optID == ZeroTier.TCP_NODELAY) {
				option = ZeroTier.TCP_NODELAY;
				level = ZeroTier.IPPROTO_TCP;
				if (value instanceof Integer) {
					optval.isBoolean = true;
					optval.booleanValue = ((Boolean)value).booleanValue() ? true : false;
				}
			}

			if (option < 0) { // No option was properly set
				//throw new UnsupportedOperationException();
			}		
			ZeroTier.setsockopt(zfd, level, option, optval);
		}
	}

	/*
	 * Disables the input aspect of the socket
	 */
	public void shutdownInput()
	{
		ZeroTier.shutdown(zfd, ZeroTier.SHUT_RD);
		// Alternatively: getInputStream().close();
	}

	/*
	 * Disables the output aspect of the socket
	 */
	public void shutdownOutput()
	{
		ZeroTier.shutdown(zfd, ZeroTier.SHUT_WR);
		// Alternatively: getOutputStream().close();
	}

	/*
	 * Sets the file descriptor
	 */
	public void setFileDescriptor(int fd)
	{
		zfd = fd;
	}

	/*
	 * Resets the socket
	 */
	void reset()
		throws IOException
	{
		localport = 0;
		address = null;
		port = 0;
	}
/*
	public FileDescriptor getFileDescriptor()
	{
		// TODO: Should probably remove this for production
		System.out.println("getFileDescriptor(), zfd="+zfd);
		ParcelFileDescriptor pfd = ParcelFileDescriptor.adoptFd(zfd);
		return pfd.getFileDescriptor();
	}
*/
}