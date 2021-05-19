/*
 * Copyright (c)2013-2021 ZeroTier, Inc.
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file in the project's root directory.
 *
 * Change Date: 2026-01-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2.0 of the Apache License.
 */
/****/

package com.zerotier.sockets;

import com.zerotier.sockets.*;
import java.io.*;
import java.net.*;

/**
 * Implements Socket-like behavior over ZeroTier
 *
 * @author  ZeroTier, Inc.
 */
public class ZeroTierSocket {
    // File descriptor from lower native layer
    private int _zfd = -1;
    private int _family = -1;
    private int _type = -1;
    private int _protocol = -1;

    // State flags
    private boolean _isClosed = false;
    private boolean _isConnected = false;
    private boolean _isBound = false;
    private boolean _inputHasBeenShutdown = false;
    private boolean _outputHasBeenShutdown = false;

    // Input and Output streams
    private ZeroTierInputStream _inputStream = new ZeroTierInputStream();
    private ZeroTierOutputStream _outputStream = new ZeroTierOutputStream();

    // The remote address to which the ZeroTierSocket is connected
    private InetAddress _remoteAddr;
    private int _remotePort;
    private InetAddress _localAddr;
    private int _localPort;

    private void setNativeFileDescriptor(int fd)
    {
        _zfd = fd;
        _inputStream.zfd = fd;
        _outputStream.zfd = fd;
    }

    public int getNativeFileDescriptor()
    {
        return _zfd;
    }

    private ZeroTierSocket(int family, int type, int protocol, int zfd)
    {
        _family = family;
        _type = type;
        _protocol = protocol;
        setNativeFileDescriptor(zfd);
        // Since we only call this from accept() we will mark it as connected
        _isConnected = true;
    }

    public ZeroTierSocket(String remoteAddr, int remotePort) throws IOException
    {
        _protocol = 0;
        _type = ZeroTierNative.ZTS_SOCK_STREAM;

        InetAddress address = InetAddress.getByName(remoteAddr);
        if (address instanceof Inet6Address) {
            _family = ZeroTierNative.ZTS_AF_INET6;
        }
        else if (address instanceof Inet4Address) {
            _family = ZeroTierNative.ZTS_AF_INET;
        }

        _zfd = ZeroTierNative.zts_bsd_socket(_family, _type, _protocol);
        setNativeFileDescriptor(_zfd);
        int err;
        if ((err = ZeroTierNative.zts_connect(_zfd, remoteAddr, remotePort, 0)) < 0) {
            throw new IOException("Error while connecting to remote host (" + err + ")");
        }
        _isConnected = true;
    }

    /**
     * Create a new ZeroTierSocket with the given attributes
     * @param family The socket family
     * @param type The socket type
     * @param protocol Supported protocol
     *
     * @exception IOException when an I/O error occurs
     */
    public ZeroTierSocket(int family, int type, int protocol) throws IOException
    {
        if (_zfd > -1) {
            throw new IOException("This socket has already been created (fd=" + _zfd + ")");
        }
        _zfd = ZeroTierNative.zts_bsd_socket(family, type, protocol);
        if (_zfd < 0) {
            throw new IOException("Error while creating socket (" + _zfd + ")");
        }
        _family = family;
        _type = type;
        _protocol = protocol;
        setNativeFileDescriptor(_zfd);
    }

    /**
     * Connect to a remote host
     * @param remoteAddr Remote address to which this socket should connect
     * @param remotePort Remote port to which this socket should connect
     *
     * @exception IOException when an I/O error occurs
     */
    public void connect(InetAddress remoteAddr, int remotePort) throws IOException
    {
        if (_zfd < 0) {
            throw new IOException("Invalid socket (fd < 0)");
        }
        if ((remoteAddr instanceof Inet4Address) && _family != ZeroTierNative.ZTS_AF_INET) {
            throw new IOException("Invalid address type. Socket is of type AF_INET");
        }
        if ((remoteAddr instanceof Inet6Address) && _family != ZeroTierNative.ZTS_AF_INET6) {
            throw new IOException("Invalid address type. Socket is of type AF_INET6");
        }
        int err;
        if ((err = ZeroTierNative.zts_connect(_zfd, remoteAddr.getHostAddress(), remotePort, 0)) < 0) {
            throw new IOException("Error while connecting to remote host (" + err + ")");
        }
        _isConnected = true;
    }

    /**
     * Connect to a remote host
     * @param remoteAddr Remote address to which this socket should connect
     * @param remotePort Remote port to which this socket should connect
     *
     * @exception IOException when an I/O error occurs
     */
    public void connect(String remoteAddr, int remotePort) throws IOException
    {
        InetAddress remoteInetAddr = InetAddress.getByName(remoteAddr);
        connect(remoteInetAddr, remotePort);
    }

    /**
     * Connect to a remote host
     * @param remoteAddr Remote address to which this socket should connect
     *
     * @exception IOException when an I/O error occurs
     */
    public void connect(SocketAddress remoteAddr) throws IOException
    {
        int remotePort = ((InetSocketAddress)remoteAddr).getPort();
        connect(((InetSocketAddress)remoteAddr).getHostString(), remotePort);
    }

    /**
     * Bind to a local address
     * @param localAddr Local address to which this socket should bind
     * @param localPort Local port to which this socket should bind
     *
     * @exception IOException when an I/O error occurs
     */
    public void bind(InetAddress localAddr, int localPort) throws IOException
    {
        if (_zfd < 0) {
            throw new IOException("Invalid socket (fd < 0)");
        }
        if ((localAddr instanceof Inet4Address) && _family != ZeroTierNative.ZTS_AF_INET) {
            throw new IOException("Invalid address type. Socket is of type AF_INET");
        }
        if ((localAddr instanceof Inet6Address) && _family != ZeroTierNative.ZTS_AF_INET6) {
            throw new IOException("Invalid address type. Socket is of type AF_INET6");
        }
        int err;
        if ((err = ZeroTierNative.zts_bind(_zfd, localAddr.getHostAddress(), localPort)) < 0) {
            throw new IOException("Error while connecting to remote host (" + err + ")");
        }
        _localPort = localPort;
        _isBound = true;
    }

    /**
     * Bind to a local address
     * @param localAddr Local address to which this socket should bind
     * @param localPort Local port to which this socket should bind
     *
     * @exception IOException when an I/O error occurs
     */
    public void bind(String localAddr, int localPort) throws IOException
    {
        InetAddress localInetAddr = InetAddress.getByName(localAddr);
        bind(localInetAddr, localPort);
    }

    /**
     * Put the ZeroTierSocket into a listening state
     * @param backlog Size of connection backlog
     *
     * @exception IOException when an I/O error occurs
     */
    public void listen(int backlog) throws IOException
    {
        if (_zfd < 0) {
            throw new IOException("Invalid socket (fd < 0)");
        }
        if (backlog < 0) {
            throw new IOException("Invalid backlog value");
        }
        int err;
        if ((err = ZeroTierNative.zts_bsd_listen(_zfd, backlog)) < 0) {
            throw new IOException("Error while putting socket into listening state (" + err + ")");
        }
    }

    /**
     * Accept incoming connections on this ZeroTierSocket
     * @return New ZeroTierSocket representing the accepted connection
     * @exception IOException when an I/O error occurs
     */
    public ZeroTierSocket accept() throws IOException
    {
        if (_zfd < 0) {
            throw new IOException("Invalid socket (fd < 0)");
        }
        int accetpedFd = -1;
        ZeroTierSocketAddress addr = new ZeroTierSocketAddress();
        if ((accetpedFd = ZeroTierNative.zts_bsd_accept(_zfd, addr)) < 0) {
            throw new IOException("Error while accepting connection (" + accetpedFd + ")");
        }
        return new ZeroTierSocket(_family, _type, _protocol, accetpedFd);
    }

    /**
     * Close the ZeroTierSocket.
     *
     * @exception IOException when an I/O error occurs
     */
    public void close() throws IOException
    {
        if (_zfd < 0) {
            throw new IOException("Invalid socket (fd < 0)");
        }
        ZeroTierNative.zts_bsd_close(_zfd);
        _isClosed = true;
    }

    /**
     * Return whether keepalive is enabled.
     * @return true or false
     * @exception SocketException when an error occurs in the native socket layer
     */
    public boolean getKeepAlive() throws SocketException
    {
        if (_isClosed) {
            throw new SocketException("Error: ZeroTierSocket is closed");
        }
        return ZeroTierNative.zts_get_keepalive(_zfd) == 1;
    }

    /**
     * Get the local port to which this ZeroTierSocket is bound
     * @return Local port
     */
    public int getLocalPort()
    {
        if (! _isBound) {
            return -1;
        }
        return _localPort;
    }

    /**
     * Get the local address to which this ZeroTierSocket is bound
     * @return Local address
     */
    public InetAddress getLocalAddress()
    {
        if (! _isBound) {
            return null;
        }
        return _localAddr;
    }

    /**
     * Get the remote port to which this ZeroTierSocket is connected
     * @return Remote port
     */
    public int getRemotePort()
    {
        if (! _isConnected) {
            return -1;
        }
        return _remotePort;
    }

    /**
     * Get the remote address to which this ZeroTierSocket is connected
     * @return Remote address
     */
    public InetAddress getRemoteAddress()
    {
        if (! _isConnected) {
            return null;
        }
        return _remoteAddr;
    }

    /**
     * Get the remote address to which this ZeroTierSocket is connected. Same as getRemoteAddress()
     * @return Remote address
     */
    public InetAddress getInetAddress()
    {
        if (! _isConnected) {
            return null;
        }
        return _remoteAddr;
    }

    /**
     * Get the local endpoint address to which this socket is bound to
     * @return Local endpoint address
     */
    public SocketAddress getLocalSocketAddress()
    {
        if (! _isConnected) {
            return null;
        }
        return new InetSocketAddress(_remoteAddr, _remotePort);
    }

    /**
     * Return the size of the receive buffer for the ZeroTierSocket's ZeroTierInputStream (SO_RCVBUF)
     * @return Size of the receive buffer
     * @exception SocketException when an error occurs in the native socket layer
     */
    public int getReceiveBufferSize() throws SocketException
    {
        if (_isClosed) {
            throw new SocketException("Error: ZeroTierSocket is closed");
        }
        return ZeroTierNative.zts_get_recv_buf_size(_zfd);
    }

    /**
     * Return the size of the send buffer for the ZeroTierSocket's ZeroTierOutputStream (SO_SNDBUF)
     * @return Size of the send buffer
     * @exception SocketException when an error occurs in the native socket layer
     */
    public int getSendBufferSize() throws SocketException
    {
        if (_isClosed) {
            throw new SocketException("Error: ZeroTierSocket is closed");
        }
        return ZeroTierNative.zts_get_send_buf_size(_zfd);
    }

    /**
     * Return whether address reuse is enabled on this ZeroTierSocket (SO_REUSEADDR)
     * @return true or false
     * @exception SocketException when an error occurs in the native socket layer
     */
    public boolean getReuseAddress() throws SocketException
    {
        if (_isClosed) {
            throw new SocketException("Error: ZeroTierSocket is closed");
        }
        return ZeroTierNative.zts_get_reuse_addr(_zfd) == 1;
    }

    /**
     * Return the amount of time that a ZeroTierSocket will linger after closure (SO_LINGER)
     * @return Nothing.
     * @exception SocketException when an error occurs in the native socket layer
     */
    public int getSoLingerTime() throws SocketException
    {
        if (_isClosed) {
            throw new SocketException("Error: ZeroTierSocket is closed");
        }
        return ZeroTierNative.zts_get_linger_value(_zfd);
    }

    /**
     * Get the ZeroTierSocket's timeout value (SO_RCVTIMEO)
     * @return Nothing.
     * @exception SocketException when an error occurs in the native socket layer
     */
    public int getSoTimeout() throws SocketException
    {
        if (_isClosed) {
            throw new SocketException("Error: ZeroTierSocket is closed");
        }
        return ZeroTierNative.zts_get_recv_timeout(_zfd);
    }

    /**
     * Return whether TCP no-delay is enabled (TCP_NODELAY)
     * @return true or false
     * @exception SocketException when an error occurs in the native socket layer
     */
    public boolean tcpNoDelayEnabled() throws SocketException
    {
        if (_isClosed) {
            throw new SocketException("Error: ZeroTierSocket is closed");
        }
        return ZeroTierNative.zts_get_no_delay(_zfd) == 1;
    }

    /**
     * Return whether this ZeroTierSocket is bound to a local address
     * @return true or false
     */
    public boolean isBound​()
    {
        return _isBound;
    }

    /**
     * Return whether this ZeroTierSocket has been closed
     * @return true or false
     */
    public boolean isClosed​()
    {
        return _isClosed;
    }

    /**
     * Return whether this ZeroTierSocket is connected to a remote address
     * @return true or false
     */
    public boolean isConnected​()
    {
        return _isConnected;
    }

    /**
     * Disable the input-aspect of the ZeroTierSocket.
     *
     * @exception SocketException when an error occurs in the native socket layer
     */
    public void shutdownInput() throws SocketException
    {
        if (! _isConnected) {
            throw new SocketException("Error: ZeroTierSocket is not connected");
        }
        if (_isClosed) {
            throw new SocketException("Error: ZeroTierSocket is closed");
        }
        if (_inputHasBeenShutdown) {
            throw new SocketException("Error: ZeroTierSocket input has been shut down");
        }
        ZeroTierNative.zts_bsd_shutdown(_zfd, ZeroTierNative.ZTS_SHUT_RD);
        _inputHasBeenShutdown = true;
    }

    /**
     * Disable the output-aspect of the ZeroTierSocket.
     *
     * @exception SocketException when an error occurs in the native socket layer
     */
    public void shutdownOutput() throws SocketException
    {
        if (! _isConnected) {
            throw new SocketException("Error: ZeroTierSocket is not connected");
        }
        if (_isClosed) {
            throw new SocketException("Error: ZeroTierSocket is closed");
        }
        if (_outputHasBeenShutdown) {
            throw new SocketException("Error: ZeroTierSocket output has been shut down");
        }
        ZeroTierNative.zts_bsd_shutdown(_zfd, ZeroTierNative.ZTS_SHUT_WR);
        _outputHasBeenShutdown = true;
    }

    /**
     * Return a reference to the ZeroTierInputStream used by this ZeroTierSocket
     * @return A reference to the ZeroTierInputStream
     * @exception SocketException when an error occurs in the native socket layer
     */
    public ZeroTierInputStream getInputStream() throws SocketException
    {
        if (! _isConnected) {
            throw new SocketException("Error: ZeroTierSocket is not connected");
        }
        if (_isClosed) {
            throw new SocketException("Error: ZeroTierSocket is closed");
        }
        if (_inputHasBeenShutdown) {
            throw new SocketException("Error: ZeroTierSocket input has been shut down");
        }
        return _inputStream;
    }

    /**
     * Return a reference to the ZeroTierOutputStream used by this ZeroTierSocket
     * @return A reference to the ZeroTierOutputStream
     * @exception SocketException when an error occurs in the native socket layer
     */
    public ZeroTierOutputStream getOutputStream() throws SocketException
    {
        if (! _isConnected) {
            throw new SocketException("Error: ZeroTierSocket is not connected");
        }
        if (_isClosed) {
            throw new SocketException("Error: ZeroTierSocket is closed");
        }
        if (_outputHasBeenShutdown) {
            throw new SocketException("Error: ZeroTierSocket output has been shut down");
        }
        return _outputStream;
    }

    /**
     * Return whether the input-aspect of the ZeroTierSocket has been disabled.
     * @return true or false
     */
    public boolean inputStreamHasBeenShutdown​()
    {
        return _inputHasBeenShutdown;
    }

    /**
     * Return whether the output-aspect of the ZeroTierSocket has been disabled.
     * @return true or false
     */
    public boolean outputStreamHasBeenShutdown​()
    {
        return _outputHasBeenShutdown;
    }

    /**
     * Enable or disable the keepalive setting (SO_KEEPALIVE)
     * @param enabled Whether SO_KEEPALIVE is enabled.
     *
     * @exception SocketException when an error occurs in the native socket layer
     */
    public void setKeepAliveEnabled(boolean enabled) throws SocketException
    {
        if (_isClosed) {
            throw new SocketException("Error: ZeroTierSocket is closed");
        }
        if (ZeroTierNative.zts_set_keepalive(_zfd, (enabled ? 1 : 0)) != ZeroTierNative.ZTS_ERR_OK) {
            throw new SocketException("Error: Could not set SO_KEEPALIVE");
        }
    }

    /**
     * Set the size of the receive buffer for the ZeroTierSocket's ZeroTierInputStream.
     * @param bufferSize Size of receive buffer
     *
     * @exception SocketException when an error occurs in the native socket layer
     */
    public void setReceiveBufferSize(int bufferSize) throws SocketException
    {
        if (_isClosed) {
            throw new SocketException("Error: ZeroTierSocket is closed");
        }
        if (bufferSize <= 0) {
            throw new IllegalArgumentException("Error: bufferSize <= 0");
        }
        if (ZeroTierNative.zts_set_recv_buf_size(_zfd, bufferSize) != ZeroTierNative.ZTS_ERR_OK) {
            throw new SocketException("Error: Could not set receive buffer size");
        }
    }

    /**
     * Enable or disable the re-use of addresses (SO_REUSEADDR)
     * @param enabled Whether SO_REUSEADDR is enabled
     *
     * @exception SocketException when an error occurs in the native socket layer
     */
    public void setReuseAddress(boolean enabled) throws SocketException
    {
        if (_isClosed) {
            throw new SocketException("Error: ZeroTierSocket is closed");
        }
        if (ZeroTierNative.zts_set_reuse_addr(_zfd, (enabled ? 1 : 0)) != ZeroTierNative.ZTS_ERR_OK) {
            throw new SocketException("Error: Could not set SO_REUSEADDR");
        }
    }

    /**
     * Set the size of the send buffer for the ZeroTierSocket's ZeroTierOutputStream (SO_SNDBUF)
     * @param bufferSize Size of send buffer
     *
     * @exception SocketException when an error occurs in the native socket layer
     */
    public void setSendBufferSize(int bufferSize) throws SocketException
    {
        if (_isClosed) {
            throw new SocketException("Error: ZeroTierSocket is closed");
        }
        if (bufferSize <= 0) {
            throw new IllegalArgumentException("Error: bufferSize <= 0");
        }
        if (ZeroTierNative.zts_set_send_buf_size(_zfd, bufferSize) != ZeroTierNative.ZTS_ERR_OK) {
            throw new SocketException("Error: Could not set SO_SNDBUF");
        }
    }

    /**
     * Set the amount of time that a ZeroTierSocket will linger after closure (SO_LINGER)
     * @param enabled Whether SO_LINGER is enabled
     * @param lingerTime SO_LINGER time
     *
     * @exception SocketException when an error occurs in the native socket layer
     */
    public void setSoLinger(boolean enabled, int lingerTime) throws SocketException
    {
        if (_isClosed) {
            throw new SocketException("Error: ZeroTierSocket is closed");
        }
        if (lingerTime < 0) {
            throw new IllegalArgumentException("Error: lingerTime < 0");
        }
        if (ZeroTierNative.zts_set_linger(_zfd, (enabled ? 1 : 0), lingerTime) != ZeroTierNative.ZTS_ERR_OK) {
            throw new SocketException("Error: Could not set ZTS_SO_LINGER");
        }
    }

    /**
     * Set the timeout value for SO_RCVTIMEO
     * @param timeout Socket receive timeout value.
     *
     * @exception SocketException when an error occurs in the native socket layer
     */
    public void setSoTimeout(int timeout) throws SocketException
    {
        if (_isClosed) {
            throw new SocketException("Error: ZeroTierSocket is closed");
        }
        if (timeout < 0) {
            throw new IllegalArgumentException("Error: SO_TIMEOUT < 0");
        }
        // TODO: This is incorrect
        if (ZeroTierNative.zts_set_recv_timeout(_zfd, timeout, timeout) != ZeroTierNative.ZTS_ERR_OK) {
            throw new SocketException("Error: Could not set SO_RCVTIMEO");
        }
    }

    /**
     * Enable or disable TCP_NODELAY
     * @param enabled Whether TCP_NODELAY is enabled
     *
     * @exception SocketException when an error occurs in the native socket layer
     */
    public void setTcpNoDelayEnabled(boolean enabled) throws SocketException
    {
        if (_isClosed) {
            throw new SocketException("Error: ZeroTierSocket is closed");
        }
        if (ZeroTierNative.zts_set_no_delay(_zfd, (enabled ? 1 : 0)) != ZeroTierNative.ZTS_ERR_OK) {
            throw new SocketException("Error: Could not set TCP_NODELAY");
        }
    }
}
