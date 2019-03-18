package com.zerotier.libzt;

import com.zerotier.libzt.ZeroTier;
import com.zerotier.libzt.ZeroTierSocket;

import java.io.IOException;
import java.net.InetAddress;
import java.net.Socket;
import java.net.UnknownHostException;
import javax.net.SocketFactory;
import java.util.Objects;

public class ZeroTierSocketFactory extends SocketFactory
{
	public ZeroTierSocketFactory() { }

	public static SocketFactory getDefault()
	{
		return null;
	}

	public Socket createSocket()
		throws IOException, UnknownHostException
	{
		return new ZeroTierSocket();
	}

	public Socket createSocket(String host, int port)
		throws IOException, UnknownHostException
	{
		return new ZeroTierSocket(host, port);
	}

	public Socket createSocket(String host, int port, InetAddress localHost, int localPort)
		throws IOException, UnknownHostException
	{
		return new ZeroTierSocket(host, port, localHost, localPort);
	}

	public Socket createSocket(InetAddress host, int port)
		throws IOException
	{
		return new ZeroTierSocket(host, port);
	}

	public Socket createSocket(InetAddress address, int port, InetAddress localAddress, int localPort)
		throws IOException
	{
		return new ZeroTierSocket(address, port, localAddress, localPort);
	}
}
