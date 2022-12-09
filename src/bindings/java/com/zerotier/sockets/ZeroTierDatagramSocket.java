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
public class ZeroTierDatagramSocket {
    ZeroTierSocket _socket;

    /**
     * Create a ZeroTierDatagramSocket and bind it to any available port
     */
    public ZeroTierDatagramSocket() throws IOException
    {
        _socket = new ZeroTierSocket(ZeroTierNative.ZTS_AF_INET, ZeroTierNative.ZTS_SOCK_DGRAM, 0);
        _socket.bind("0.0.0.0", 0);
    }

    /**
     * Create a ZeroTierDatagramSocket bound to the given port
     */
    public ZeroTierDatagramSocket(int localPort) throws IOException
    {
        _socket = new ZeroTierSocket(ZeroTierNative.ZTS_AF_INET, ZeroTierNative.ZTS_SOCK_DGRAM, 0);
        _socket.bind("0.0.0.0", localPort);
    }

    /**
     * Create a ZeroTierDatagramSocket bound to the given port and local address
     */
    public ZeroTierDatagramSocket(int localPort, InetAddress localAddr) throws IOException
    {
        _socket = new ZeroTierSocket(ZeroTierNative.ZTS_AF_INET, ZeroTierNative.ZTS_SOCK_DGRAM, 0);
        _socket.bind(localAddr.getHostAddress(), localPort);
    }

    /**
     * Create a ZeroTierDatagramSocket bound to the given port and local address
     */
    public ZeroTierDatagramSocket(String localAddr, int localPort) throws IOException
    {
        _socket = new ZeroTierSocket(ZeroTierNative.ZTS_AF_INET, ZeroTierNative.ZTS_SOCK_DGRAM, 0);
        _socket.bind(localAddr, localPort);
    }

    /**
     * Create a ZeroTierDatagramSocket bound to the given endpoint
     */
    public ZeroTierDatagramSocket(SocketAddress localAddr) throws IOException
    {
        _socket = new ZeroTierSocket(ZeroTierNative.ZTS_AF_INET, ZeroTierNative.ZTS_SOCK_DGRAM, 0);
        int localPort = ((InetSocketAddress)localAddr).getPort();
        _socket.bind(((InetSocketAddress)localAddr).getHostString(), localPort);
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
        _socket.connect(remoteAddr, remotePort);
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
        _socket.bind(localAddr, localPort);
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
     * Send a DatagramPacket to a remote host
     * @param packet The packet to send
     *
     * @exception IOException when an I/O error occurs
     */
    public void send(DatagramPacket packet) throws IOException
    {
        int bytesWritten = ZeroTierNative.zts_bsd_write_offset(
            _socket.getNativeFileDescriptor(),
            packet.getData(),
            0,
            packet.getLength());
        if (bytesWritten < 0) {
            throw new IOException("send(DatagramPacket), errno=" + bytesWritten);
        }
    }

    /**
     * Receive a DatagramPacket from a remote host
     * @param packet The packet received
     *
     * @exception IOException when an I/O error occurs
     */
    public void receive(DatagramPacket packet) throws IOException
    {
        int bytesRead = ZeroTierNative.zts_bsd_read_offset(
            _socket.getNativeFileDescriptor(),
            packet.getData(),
            0,
            packet.getLength());
        if ((bytesRead <= 0) | (bytesRead == -104) /* EINTR, from SO_RCVTIMEO */) {
            throw new IOException("read​(DatagramPacket), errno=" + bytesRead);
        }
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
    public InetAddress getLocalAddress()
    {
        return _socket.getLocalAddress();
    }

    /**
     * Get the remote port to which this ZeroTierSocket is connected
     * @return Remote port
     */
    public int getRemotePort()
    {
        return _socket.getRemotePort();
    }

    /**
     * Get the remote address to which this ZeroTierSocket is connected
     * @return Remote address
     */
    public InetAddress getRemoteAddress()
    {
        return _socket.getRemoteAddress();
    }

    /**
     * Get the remote address to which this ZeroTierSocket is connected. Same as getRemoteAddress()
     * @return Remote address
     */
    public InetAddress getInetAddress()
    {
        return _socket.getInetAddress();
    }

    /**
     * Get the local endpoint address to which this socket is bound to
     * @return Local endpoint address
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
    public int getReceiveBufferSize() throws SocketException
    {
        return _socket.getReceiveBufferSize();
    }

    /**
     * Return the size of the send buffer for the ZeroTierSocket's ZeroTierOutputStream (SO_SNDBUF)
     * @return Size of the send buffer
     * @exception SocketException when an error occurs in the native socket layer
     */
    public int getSendBufferSize() throws SocketException
    {
        return _socket.getSendBufferSize();
    }

    /**
     * Return whether address reuse is enabled on this ZeroTierSocket (SO_REUSEADDR)
     * @return true or false
     * @exception SocketException when an error occurs in the native socket layer
     */
    public boolean getReuseAddress() throws SocketException
    {
        return _socket.getReuseAddress();
    }

    /**
     * Get the ZeroTierSocket's timeout value (SO_RCVTIMEO)
     * @return Nothing.
     * @exception SocketException when an error occurs in the native socket layer
     */
    public int getSoTimeout() throws SocketException
    {
        return _socket.getSoTimeout();
    }

    /**
     * Return whether this ZeroTierSocket is bound to a local address
     * @return true or false
     */
    public boolean isBound​()
    {
        return _socket.isBound​();
    }

    /**
     * Return whether this ZeroTierSocket has been closed
     * @return true or false
     */
    public boolean isClosed​()
    {
        return _socket.isClosed();
    }

    /**
     * Return whether this ZeroTierSocket is connected to a remote address
     * @return true or false
     */
    public boolean isConnected​()
    {
        return _socket.isConnected();
    }

    /**
     * Set the size of the receive buffer for the ZeroTierSocket's ZeroTierInputStream.
     * @param bufferSize Size of receive buffer
     *
     * @exception SocketException when an error occurs in the native socket layer
     */
    public void setReceiveBufferSize(int bufferSize) throws SocketException
    {
        _socket.setReceiveBufferSize(bufferSize);
    }

    /**
     * Enable or disable the re-use of addresses (SO_REUSEADDR)
     * @param enabled Whether SO_REUSEADDR is enabled
     *
     * @exception SocketException when an error occurs in the native socket layer
     */
    public void setReuseAddress(boolean enabled) throws SocketException
    {
        _socket.setReuseAddress(enabled);
    }

    /**
     * Set the size of the send buffer for the ZeroTierSocket's ZeroTierOutputStream (SO_SNDBUF)
     * @param bufferSize Size of send buffer
     *
     * @exception SocketException when an error occurs in the native socket layer
     */
    public void setSendBufferSize(int bufferSize) throws SocketException
    {
        _socket.setSendBufferSize(bufferSize);
    }

    /**
     * Set the timeout value for SO_RCVTIMEO
     * @param timeout Socket receive timeout value.
     *
     * @exception SocketException when an error occurs in the native socket layer
     */
    public void setSoTimeout(int timeout) throws SocketException
    {
        _socket.setSoTimeout(timeout);
    }
}
