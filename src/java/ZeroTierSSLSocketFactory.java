package com.zerotier.libzt;

import com.zerotier.libzt.ZeroTierSocket;

import java.net.*;
import javax.net.SocketFactory;
import java.io.IOException;
import java.io.InputStream;
import java.security.*;
import java.util.Locale;

import javax.net.ssl.SSLSocketFactory;

public class ZeroTierSSLSocketFactory extends SSLSocketFactory
{
	private final SSLSocketFactory delegate;

	/*
	 *
	 */
	public ZeroTierSSLSocketFactory(SSLSocketFactory delegate)
	{
		this.delegate = delegate;
	}

	/*
	 *
	 */
	public Socket createSocket(Socket s, String host, int port, boolean autoClose)
		throws IOException
	{
		ZeroTierSocket zs = new ZeroTierSocket();
		zs.connect((SocketAddress)new InetSocketAddress(host, port), 10);
		return delegate.createSocket(zs, host, port, autoClose);
	}

	/*
	 *
	 */
	public Socket createSocket(Socket s, InputStream consumed, boolean autoClose)
		throws IOException
	{
		throw new UnsupportedOperationException();
	}

	/*
	 *
	 */
	public Socket createSocket(InetAddress a,int b,InetAddress c,int d)
		throws IOException
	{
		ZeroTierSocket s = new ZeroTierSocket();
		return delegate.createSocket(a, b, c, d);
	}

	/*
	 *
	 */
	public Socket createSocket(InetAddress a,int b)
		throws IOException
	{
		ZeroTierSocket s = new ZeroTierSocket();
		return delegate.createSocket(a, b);
	}

	/*
	 *
	 */
	public Socket createSocket(String a,int b,InetAddress c,int d)
		throws IOException
	{
		ZeroTierSocket s = new ZeroTierSocket();
		return delegate.createSocket(a, b, c, d);
	}

	/*
	 *
	 */
	public Socket createSocket(String a,int b)
		throws IOException
	{
		ZeroTierSocket s = new ZeroTierSocket();
		return delegate.createSocket(a, b);
	}

	/*
	 *
	 */
	public String [] getSupportedCipherSuites()
	{
		return new String[0];
	}

	/*
	 *
	 */
	public String [] getDefaultCipherSuites()
	{
		return new String[0];
	}
}
