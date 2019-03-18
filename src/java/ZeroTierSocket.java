package com.zerotier.libzt;

import com.zerotier.libzt.ZeroTier;
import com.zerotier.libzt.ZeroTierSocketAddress;
import com.zerotier.libzt.ZeroTierSocketImpl;
import com.zerotier.libzt.ZeroTierSocketImplFactory;
import com.zerotier.libzt.ZeroTierInputStream;
import com.zerotier.libzt.ZeroTierOutputStream;

import java.io.*;
import java.net.*;
import java.util.Objects;
import java.nio.channels.SocketChannel;
import java.net.InetAddress;

public class ZeroTierSocket extends Socket
{
	/*
	 * Factory designated to create the underlying ZeroTierSocket implementation
	 */
	static ZeroTierSocketImplFactory factory = new ZeroTierSocketImplFactory();

	/*
	 * Underlying implementation of this ZeroTierSocket
	 */
	ZeroTierSocketImpl impl;

	/*
	 * Misc. state flags
	 */
	private boolean created = false;
	private boolean closed = false;
	private boolean connected = false;
	private boolean bound = false;
	private boolean inputShutdown = false;
	private boolean outputShutdown = false;

	/*
	 * Creates and sets the implementation
	 */
	void setImpl()
	{
		if (factory != null) {
			impl = factory.createSocketImpl();
		}
		if (impl != null) {
			impl.setSocket(this);
		}
	}

	/* 
	 * Returns the underlying socket implementation
	 */
	private ZeroTierSocketImpl getImpl()
		throws SocketException
	{
	  if (!created) {
		try {
			impl.create(true);
		}
		catch (IOException ex) {
			throw (SocketException) new SocketException().initCause(ex);
		}
		created = true;
	  }
	  return impl;
	}

	/*
	 * Create the underlying socket implementation
	 */
	void createImpl(boolean stream)
		throws SocketException
	{
		if (impl == null) {
			setImpl();
		}
		try {
			impl.create(stream);
			created = true;
		}
		catch (IOException ex) {
			throw new SocketException(ex.getMessage());
		}
	}

	/* 
	 * Constructor for ZeroTierSocket
	 */
	public ZeroTierSocket()
		throws IOException
	{
		this((InetAddress)null, 0, null, 0);
	}

	/*
	 * Creates an unconnected socket
	 */
	protected ZeroTierSocket(ZeroTierSocketImpl impl)
		throws SocketException
	{
        this.impl = impl;
        if (impl != null) {
            this.impl.setSocket(this);
        }
    }

	/* 
	 * Constructor for ZeroTierSocket
	 */
	public ZeroTierSocket(InetAddress raddr, int rport, InetAddress laddr, int lport)
		throws IOException
	{
		setImpl();

		try {
			if (laddr != null) {
				bind(new InetSocketAddress(laddr, lport));
			}
			if (raddr != null) {
				connect(new InetSocketAddress(raddr, rport));
			}
		}
		catch (Exception ex)
		{
			try {
				close();
			}
			catch (IOException _ex) {
				ex.addSuppressed(_ex);
			}
			throw ex;
		}
	}

	/* 
	 * Constructor for ZeroTierSocket 
	 */
	public ZeroTierSocket(InetAddress address, int port) 
		throws IOException 
	{
		this(address, port, null, 0);
	}

	/* 
	 * Constructor for ZeroTierSocket 
	 */
	public ZeroTierSocket(String address, int port) 
		throws IOException 
	{
		this(InetAddress.getByName(address), port, null, 0);
	}

	/* 
	 * Constructor for ZeroTierSocket 
	 */
	public ZeroTierSocket(String address, int port, InetAddress localHost, int localPort) 
		throws IOException 
	{
		this(InetAddress.getByName(address), port, localHost, localPort);
	}

	/* 
	 * Binds the socket to a local address
	 */
	public void bind(SocketAddress localAddr)
		throws IOException
	{
		if (isSocketBound()) {
			throw new SocketException("bind: ZeroTierSocket is already bound");
		}
		if (isSocketClosed()) {
			throw new SocketException("bind: ZeroTierSocket is closed");
		}
		if (localAddr != null && (!(localAddr instanceof InetSocketAddress))) {
			throw new IllegalArgumentException("bind: Unsupported address type");
		}
		InetSocketAddress addr = (InetSocketAddress)localAddr;
		if (addr != null && addr.isUnresolved()) {
			throw new SocketException("bind: Unresolved address");
		}
		if (addr == null) {
			addr = new InetSocketAddress(0);
		}
		getImpl().bind(addr.getAddress(), addr.getPort());
		bound = true;
	}

	/*
	 * Closes the socket
	 */
	public synchronized void close()
		throws IOException
	{
		if (isSocketClosed()) {
			return;
		}
		getOutputStream().flush();
		impl.close();
		closed = true;
	}

	/* 
	 * Connects the socket to a remote address
	 */
	public void connect(SocketAddress remoteAddr)
		throws IOException 
	{
		connect(remoteAddr, 0);
	}
	
	/* 
	 * Connects the socket to a remote address
	 */
	public void connect(SocketAddress remoteAddr, int timeout)
	  throws IOException
	{
		if (isSocketClosed()) {
			throw new SocketException("connect: ZeroTierSocket is closed");
		}
		if (isSocketConnected()) {
			throw new SocketException("connect: already connected");
		}
		if (remoteAddr == null) {
			throw new IllegalArgumentException("connect: The address can't be null");
		}
		if (!(remoteAddr instanceof InetSocketAddress)) {
			throw new IllegalArgumentException("connect: Unsupported address type");
		}
		if (timeout < 0) {
			throw new IllegalArgumentException("connect: timeout cannot be negative");
		}
		if (!created) {
			createImpl(true);
		}
		getImpl().connect(remoteAddr, 0);
		bound = true;
		connected = true;
	}

	/*
	 * Returns the associated Channel
	 */
	public SocketChannel getChannel​()
	{
		System.err.println("getChannel​: ZeroTierSocket does not currently support this feature");
		return null;
	}

	/*
	 * Returns the address to which the socket is connected
	 */
	public InetAddress getInetAddress​()
	{
		if (!isSocketConnected()) {
			return null;
		}
		try {
			return getImpl().getInetAddress();
		}
		catch (SocketException ex) {
			// Not Reachable
		}
		return null;
	}

	/* 
	 * Returns the input stream
	 */
	public ZeroTierInputStream getInputStream() 
		throws IOException 
	{
		if (!isSocketConnected()) {
			throw new SocketException("ZeroTierSocket is not connected");
		}
		if (isSocketClosed()) {
			throw new SocketException("ZeroTierSocket is closed");
		}
		if (isInputStreamShutdown()) {
			throw new SocketException("ZeroTierSocket input is shutdown");
		}
		return getImpl().getInputStream();
	}

	/*
	 * Returns whether SO_KEEPALIVE is enabled
	 */
	public boolean getKeepAlive()
		throws SocketException
	{
		if (isSocketClosed()) {
			throw new SocketException("ZeroTierSocket is closed");
		}
		return ((Boolean) getImpl().getOption(ZeroTier.SO_KEEPALIVE)).booleanValue();
	}

	/*
	 * Returns the local address to which the socket is bound
	 */
	public InetAddress getLocalAddress()
	{
		System.err.println("getLocalAddress: ZeroTierSocket does not currently support this feature");
		/*
		// This is for backward compatibility
		if (!isSocketBound()) {
			return InetAddress.anyLocalAddress();
		}
		InetAddress inAddr = null;
		try {
			inAddr = (InetAddress) getImpl().getOption(ZeroTier.SO_BINDADDR);
			if (inAddr.isAnyLocalAddress()) {
				inAddr = InetAddress.anyLocalAddress();
			}
		}
		catch (Exception ex) {
			// "0.0.0.0"
			inAddr = InetAddress.anyLocalAddress();
		}
		return inAddr;
		*/
		return null;
	}

	/* 
	 * Return the local port to which the socket is bound
	 */
	public int getLocalPort() 
	{
		if (!isSocketBound()) {
			return -1;
		}
		try {
			return getImpl().getLocalPort();
		}
		catch(SocketException ex) {
			// Unreachable
		}
		return -1;
	}

	/*
	 * Returns the address of the endpoint that the socket is bound to.
	 */
	public SocketAddress getLocalSocketAddress()
	{
		if (!isSocketBound()) {
			return null;
		}
		return new InetSocketAddress(getLocalAddress(), getLocalPort());
	}

	/*
	 * Returns whether SO_OOBINLINE is enabled.
	 */
	public boolean getOOBInline​()
		throws SocketException
	{
		System.err.println("getOOBInline​: ZeroTierSocket does not currently support this feature");
		return false;
	}

	/* 
	 * Returns the output stream.
	 */
	public ZeroTierOutputStream getOutputStream() 
		throws IOException 
	{
		if (!isSocketConnected()) {
			throw new SocketException("ZeroTierSocket is not connected");
		}
		if (isSocketClosed()) {
			throw new SocketException("ZeroTierSocket is closed");
		}
		if (isOutputStreamShutdown()) {
			throw new SocketException("ZeroTierSocket output is shutdown");
		}
		return getImpl().getOutputStream();
	}

	/* 
	 * Return the remote port to which the socket is connected
	 */
	public int getPort()
	{
		if (!isSocketConnected()) {
			return 0;
		}
		try {
			return getImpl().getPort();
		}
		catch (SocketException ex) {
			// Not reachable
		}
		return -1;
	}

	/*
	 * Returns SO_RCVBUF
	 */
	public synchronized int getReceiveBufferSize()
		throws SocketException
	{
		if (isSocketClosed()) {
			throw new SocketException("ZeroTierSocket is closed");
		}
		Object opt = getImpl().getOption(ZeroTier.SO_RCVBUF);
		int sz = 0;
		if (opt instanceof Integer) {
			sz = ((Integer)opt).intValue();
		}
		return sz;
	}

	/*
	 * Returns the remote address to which this socket is connected
	 */
	public SocketAddress getRemoteSocketAddress()
	{
		if (!isSocketConnected()) {
			return null;
		}
		return new InetSocketAddress(getInetAddress(), getPort());
	}

	/*
	 * Checks whether SO_REUSEADDR is enabled.
	 */
	public boolean getReuseAddress()
		throws SocketException
	{
		if (isSocketClosed()) {
			throw new SocketException("ZeroTierSocket is closed");
		}
		return ((Boolean)(getImpl().getOption(ZeroTier.SO_REUSEADDR))).booleanValue();
	}

	/*
	 * Returns SO_SNDBUF.
	 */
	public synchronized int getSendBufferSize()
		throws SocketException
	{
		if (isSocketClosed()) {
			throw new SocketException("ZeroTierSocket is closed");
		}
		int sz = 0;
		Object opt = getImpl().getOption(ZeroTier.SO_SNDBUF);
		if (opt instanceof Integer) {
			sz = ((Integer)opt).intValue();
		}
		return sz;
	}

	/*
	 * Returns SO_LINGER.
	 */
	public int getSoLinger()
		throws SocketException
	{
		if (isSocketClosed()) {
			throw new SocketException("ZeroTierSocket is closed");
		}
		Object opt = getImpl().getOption(ZeroTier.SO_LINGER);
		if (opt instanceof Integer) {
			return ((Integer)opt).intValue();
		}
		return -1;
	}

	/*
	 * Returns SO_TIMEOUT.
	 */
	public synchronized int getSoTimeout()
		throws SocketException
	{
		if (isSocketClosed()) {
			throw new SocketException("ZeroTierSocket is closed");
		}
		Object opt = getImpl().getOption(ZeroTier.SO_RCVTIMEO);
		if (opt instanceof Integer) {
			return ((Integer)opt).intValue();
		}
		else {
			return 0;
		}
	}

	/*
	 * Checks whether TCP_NODELAY is enabled.
	 */
	public boolean getTcpNoDelay()
		throws SocketException
	{
		if (isSocketClosed()) {
			throw new SocketException("ZeroTierSocket is closed");
		}
		return ((Boolean)getImpl().getOption(ZeroTier.TCP_NODELAY)).booleanValue();
	}

	/*
	 * Gets traffic class or type-of-service in the IP header for packets sent from this Socket
	 */
	public int getTrafficClass​()
		throws SocketException
	{
		System.err.println("getTrafficClass​: ZeroTierSocket does not currently support this feature");
		return 0;
	}

	/*
	 * Returns whether or not the socket is bound to a local interface.
	 */
	public boolean isSocketBound​()
	{
		return bound;
	}

	/*
	 * Returns whether or not the socket has been closed.
	 */
	public boolean isSocketClosed​()
	{
		return closed;
	}

	/*
	 * Returns whether or not the socket is connected to a remote host.
	 */
	public boolean isSocketConnected​()
	{
		return connected;
	}

	/*
	 * Returns whether the input aspect of the socket has been disabled.
	 */
	public boolean isInputStreamShutdown​()
	{
		return inputShutdown;
	}

	/*
	 * Returns whether the output aspect of the socket has been disabled.
	 */
	public boolean isOutputStreamShutdown​()
	{
		return outputShutdown;
	}

	/*
	 * Send a byte of urgent data on the socket.
	 */
	public void sendUrgentData​(int data)
		throws IOException
	{
		System.err.println("sendUrgentData​: ZeroTierSocket does not currently support this feature");
	}

	/*
	 * Enable or disable SO_KEEPALIVE.
	 */
	public void setKeepAlive(boolean on) 
		throws SocketException
	{
		if (isSocketClosed()) {
			throw new SocketException("ZeroTierSocket is closed");
		}
		getImpl().setOption(ZeroTier.SO_KEEPALIVE, Boolean.valueOf(on));
	}

	/*
	 * Enable or disable SO_OOBINLINE.
	 */
	public void setOOBInline​(boolean on)
		throws SocketException
	{
		System.err.println("setOOBInline​: ZeroTierSocket does not currently support this feature");
	}

	/*
	 * Set performance preferences.
	 */
	public void setPerformancePreferences​(int connectionTime, int latency, int bandwidth)
	{
		System.err.println("setPerformancePreferences​: ZeroTierSocket does not currently support this feature");
	}

	/*
	 * Set SO_RCVBUF.
	 */
	public synchronized void setReceiveBufferSize(int size)
		throws SocketException
	{
		if (isSocketClosed()) {
			throw new SocketException("ZeroTierSocket is closed");
		}
		if (size <= 0) {
			throw new IllegalArgumentException("invalid receive buffer size argument");
		}
		getImpl().setOption(ZeroTier.SO_RCVBUF, new Integer(size));
	}

	/*
	 * Enable or disable SO_REUSEADDR.
	 */
	public void setReuseAddress(boolean on)
		throws SocketException
	{
		if (isSocketClosed()) {
			throw new SocketException("ZeroTierSocket is closed");
		}
		getImpl().setOption(ZeroTier.SO_REUSEADDR, Boolean.valueOf(on));
	}

	/*
	 * Set SO_SNDBUF.
	 */
	public synchronized void setSendBufferSize(int size)
		throws SocketException
	{
		if (isSocketClosed()) {
			throw new SocketException("ZeroTierSocket is closed");
		}
		if (size < 0) {
			throw new IllegalArgumentException("size argument cannot be negative");
		}
		getImpl().setOption(ZeroTier.SO_SNDBUF, new Integer(size));
	}

	/*
	 * Set Socket implementation factory for all clients.
	 */
	public static void setSocketImplFactory​(ZeroTierSocketImplFactory fact)
		throws IOException
	{
		if (factory != null) {
			throw new SocketException("ZeroTierSocket factory is already defined");
		}
		factory = fact;
	}

	/*
	 * Enable or disable SO_LINGER time (seconds).
	 */
	public void setSoLinger(boolean on, int linger)
		throws SocketException
	{
		if (isSocketClosed()) {
			throw new SocketException("ZeroTierSocket is closed");
		}
		if (!on) {
			getImpl().setOption(ZeroTier.SO_LINGER, new Boolean(on));
		}
		else {
			if (linger < 0) {
				throw new IllegalArgumentException("linger argument is invalid");
			}
			if (linger > 0xFFFF) {
				linger = 0xFFFF;
			}
			getImpl().setOption(ZeroTier.SO_LINGER, new Integer(linger));
		}
	}

	/*
	 * Enable or disable SO_TIMEOUT with the specified timeout, in milliseconds.
	 */
	public void setSoTimeout​(int timeout)
		throws SocketException
	{
		if (timeout < 0) {
		  throw new IllegalArgumentException("timeout argument cannot be negative");
		}
		if (isSocketClosed()) {
			throw new SocketException("ZeroTierSocket is closed");
		}
		getImpl().setOption(ZeroTier.SO_RCVTIMEO, new Integer(timeout));
	}

	/*
	 * Enable or disable TCP_NODELAY (Nagle's algorithm).
	 */
	public void setTcpNoDelay(boolean on)
		throws SocketException
	{
		if (isSocketClosed()) {
			throw new SocketException("ZeroTierSocket is closed");
		}
		getImpl().setOption(ZeroTier.TCP_NODELAY, Boolean.valueOf(on));
	}

	/*
	 * Sets traffic class or ToS.
	 */
	public void setTrafficClass​(int tc)
		throws SocketException
	{
		System.err.println("setTrafficClass​: ZeroTierSocket does not currently support this feature");
	}

	/*
	 * Disable the input stream for this socket.
	 */
	public void shutdownInput()
		throws IOException
	{
		if (isSocketClosed()) {
			throw new SocketException("ZeroTierSocket is closed");
		}
		if (isInputStreamShutdown()) {
			throw new SocketException("ZeroTierSocket input is already shutdown");
		}
		if (!isSocketConnected()) {
			throw new SocketException("ZeroTierSocket is not connected");
		}
		getImpl().shutdownInput();
		inputShutdown = true;
	}

	/*
	 * Disable the output stream for this socket.
	 */
	public void shutdownOutput()
		throws IOException
	{
		if (isSocketClosed()) {
			throw new SocketException("ZeroTierSocket is closed");
		}
		if (isOutputStreamShutdown()) {
			throw new SocketException("ZeroTierSocket output is already shutdown");
		}
		if (!isSocketConnected()) {
			throw new SocketException("ZeroTierSocket is not connected");
		}
		getImpl().shutdownOutput();
		outputShutdown = true;
	}

	/*
	 * Gets the underlying implementation's file descriptor.
	 */
	/*
	public FileDescriptor getFileDescriptor()
	{
		return impl.getFileDescriptor();
	}
	*/
}