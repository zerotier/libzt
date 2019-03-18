package com.zerotier.libzt;

import com.zerotier.libzt.ZeroTier;
import com.zerotier.libzt.ZeroTierSocketImpl;

import java.io.IOException;
import java.net.InetAddress;
import java.net.Socket;
import java.net.UnknownHostException;
import javax.net.SocketFactory;
import java.net.SocketImplFactory;
import java.util.Objects;


public class ZeroTierSocketImplFactory implements SocketImplFactory
{
	/*
	 * Does nothing
	 */
	public ZeroTierSocketImplFactory() { }

	/*
	 * Produces a ZeroTierSocketImpl
	 */
	public ZeroTierSocketImpl createSocketImpl()
	{
		return new ZeroTierSocketImpl();
	}
}
