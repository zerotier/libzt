package com.zerotier.libzt;

import com.zerotier.libzt.ZeroTier;

import java.net.InetAddress;

// Designed to transport address information across the JNI boundary
public class ZeroTierSocketAddress
{
	public byte[] _ip6 = new byte[16];
	public byte[] _ip4 = new byte[4];

	public int _family;
	public int _port; // Also reused for netmask or prefix

	public ZeroTierSocketAddress() {}

	public ZeroTierSocketAddress(String ipStr, int port)
	{
		if(ipStr.contains(":")) {
			_family = ZeroTier.AF_INET6;
			try {
				InetAddress ip = InetAddress.getByName(ipStr);
				_ip6 = ip.getAddress();
			}
			catch (Exception e) { }
		}
		else if(ipStr.contains(".")) {
			_family = ZeroTier.AF_INET;
			try {
				InetAddress ip = InetAddress.getByName(ipStr);
				_ip4 = ip.getAddress();
			}
			catch (Exception e) { }		
		}
		_port = port;
	}

	public int getPort() { return _port; }
	public int getNetmask() { return _port; }
	public int getPrefix() { return _port; }

	public String ipString()
	{
		if (_family == ZeroTier.AF_INET) {
			try {
				InetAddress inet = InetAddress.getByAddress(_ip4);
				return "" + inet.getHostAddress();
			} catch (Exception e) {
				System.out.println(e);
			}
		}
		if (_family == ZeroTier.AF_INET6) {
			try {
				InetAddress inet = InetAddress.getByAddress(_ip6);
				return "" + inet.getHostAddress();
			} catch (Exception e) {
				System.out.println(e);
			}
		}
		return "";
	}

	public String toString() { return ipString() + ":" + _port; }
	public String toCIDR() { return ipString() + "/" + _port; }
}
