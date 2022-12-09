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
 * Implements SocketServer-like behavior over ZeroTier
 *
 * @author  ZeroTier, Inc.
 */
public class ZeroTierServerSocket {
    private ZeroTierSocket _socket;

    /**
     * Create an unbound ZeroTierServerSocket
     */
    public ZeroTierServerSocket() throws IOException
    {
        _socket = new ZeroTierSocket(ZeroTierNative.ZTS_AF_INET6, ZeroTierNative.ZTS_SOCK_STREAM, 0);
    }

    /**
     * Create a ZeroTierServerSocket bound to the given port
     */
    public ZeroTierServerSocket(int localPort) throws IOException
    {
        _socket = new ZeroTierSocket(ZeroTierNative.ZTS_AF_INET6, ZeroTierNative.ZTS_SOCK_STREAM, 0);
        _socket.bind("::", localPort);
        _socket.listen(0);
    }

    /**
     * Create a ZeroTierServerSocket bound to the given port with a backlog
     */
    public ZeroTierServerSocket(int localPort, int backlog) throws IOException
    {
        _socket = new ZeroTierSocket(ZeroTierNative.ZTS_AF_INET6, ZeroTierNative.ZTS_SOCK_STREAM, 0);
        _socket.bind("::", localPort);
        _socket.listen(backlog);
    }

    /**
     * Create a ZeroTierServerSocket bound to the given port and local address
     */
    public ZeroTierServerSocket(int localPort, int backlog, InetAddress localAddr) throws IOException
    {
        _socket = new ZeroTierSocket(ZeroTierNative.ZTS_AF_INET6, ZeroTierNative.ZTS_SOCK_STREAM, 0);
        _socket.bind(localAddr.getHostAddress(), localPort);
        _socket.listen(backlog);
    }

    /**
     * Accept incoming connections on this ZeroTierSocket
     * @return New ZeroTierSocket representing the accepted connection
     * @exception IOException when an I/O error occurs
     */
    public ZeroTierSocket accept() throws IOException
    {
        return _socket.accept();
    }

    /**
     * Bind to a local address
     * @param localAddr Local address to which this socket should bind
     * @param localPort Local port to which this socket should bind
     *
     * @exception IOException when an I/O error occurs
     */
    public void bind(SocketAddress localAddr) throws IOException
    {
        InetSocketAddress inetAddr = (InetSocketAddress)localAddr;
        _socket.bind(inetAddr.getHostName(), inetAddr.getPort());
    }

    /**
     * Bind to a local address
     * @param localAddr Local address to which this socket should bind
     * @param localPort Local port to which this socket should bind
     *
     * @exception IOException when an I/O error occurs
     */
    public void bind(SocketAddress localAddr, int backlog) throws IOException
    {
        InetSocketAddress inetAddr = (InetSocketAddress)localAddr;
        _socket.bind(inetAddr.getHostName(), inetAddr.getPort());
    }

    /**
     * Close the ZeroTierSocket.
     *
     * @exception IOException when an I/O error occurs
     */
    public void close() throws IOException
    {
        _socket.close();
    }

    /**
     * Get the remote address to which this ZeroTierSocket is bound
     * @return Remote address
     */
    public InetAddress getInetAddress()
    {
        return _socket.getLocalAddress();
    }

    /**
     * Get the local port to which this ZeroTierSocket is bound
     * @return Local port
     */
    public int getLocalPort()
    {
        return _socket.getLocalPort();
    }

    /**
     * Get the local address to which this ZeroTierSocket is bound
     * @return Local address
     */
    public SocketAddress getLocalSocketAddress()
    {
        return _socket.getLocalSocketAddress();
    }

    /**
     * Return the size of the receive buffer for the ZeroTierSocket's ZeroTierInputStream (SO_RCVBUF)
     * @return Size of the receive buffer
     * @exception SocketException when an error occurs in the native socket layer
     */
    public int getReceiveBufferSize() throws IOException
    {
        return _socket.getReceiveBufferSize();
    }

    /**
     * Return whether address reuse is enabled on this ZeroTierSocket (SO_REUSEADDR)
     * @return true or false
     * @exception SocketException when an error occurs in the native socket layer
     */
    public boolean getReuseAddress() throws IOException
    {
        return _socket.getReuseAddress();
    }

    /**
     * Get the ZeroTierSocket's timeout value (SO_RCVTIMEO)
     * @return Nothing.
     * @exception SocketException when an error occurs in the native socket layer
     */
    public int getSoTimeout() throws IOException
    {
        return _socket.getSoTimeout();
    }

    /**
     * Return whether this ZeroTierSocket is bound to a local address
     * @return true or false
     */
    public boolean isBound()
    {
        return _socket.isBound();
    }

    /**
     * Return whether this ZeroTierSocket has been closed
     * @return true or false
     */
    public boolean isClosed()
    {
        return _socket.isClosed();
    }

    /**
     * Set the size of the receive buffer for the ZeroTierSocket's ZeroTierInputStream.
     * @param bufferSize Size of receive buffer
     *
     * @exception SocketException when an error occurs in the native socket layer
     */
    public void setReceiveBufferSize(int bufferSize) throws IOException
    {
        _socket.setReceiveBufferSize(bufferSize);
    }

    /**
     * Enable or disable the re-use of addresses (SO_REUSEADDR)
     * @param enabled Whether SO_REUSEADDR is enabled
     *
     * @exception SocketException when an error occurs in the native socket layer
     */
    public void setReuseAddress(boolean enabled) throws IOException
    {
        _socket.setReuseAddress(enabled);
    }

    /**
     * Set the timeout value for SO_RCVTIMEO
     * @param timeout Socket receive timeout value.
     *
     * @exception SocketException when an error occurs in the native socket layer
     */
    public void setSoTimeout(int timeout) throws IOException
    {
        _socket.setSoTimeout(timeout);
    }
}
