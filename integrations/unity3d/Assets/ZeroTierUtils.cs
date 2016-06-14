using UnityEngine;
using System.Collections;
using System.Net;
using System;
using System.Globalization;
using System.Runtime.InteropServices;

public class ZeroTierUtils {

	// Handles IPv4 and IPv6 notation.
	public static IPEndPoint CreateIPEndPoint(string endPoint)
	{
		string[] ep = endPoint.Split(':');
		if (ep.Length < 2) throw new FormatException("Invalid endpoint format");
		IPAddress ip;
		if (ep.Length > 2) {
			if (!IPAddress.TryParse(string.Join(":", ep, 0, ep.Length - 1), out ip)) {
				throw new FormatException("Invalid ip-adress");
			}
		}
		else {
			if (!IPAddress.TryParse(ep[0], out ip)) {
				throw new FormatException("Invalid ip-adress");
			}
		}
		int port;
		if (!int.TryParse(ep[ep.Length - 1], NumberStyles.None, NumberFormatInfo.CurrentInfo, out port)) {
			throw new FormatException("Invalid port");
		}
		return new IPEndPoint(ip, port);
	}

	// Generates an unmanaged sockaddr structure from a string-formatted endpoint
	public static GCHandle Generate_unmananged_sockaddr(string endpoint_str)
	{
		IPEndPoint ipEndPoint;
		ipEndPoint = ZeroTierUtils.CreateIPEndPoint (endpoint_str);
		SocketAddress socketAddress = ipEndPoint.Serialize ();

		// use an array of bytes instead of the sockaddr structure 
		byte[] sockAddrStructureBytes = new byte[socketAddress.Size];
		GCHandle sockAddrHandle = GCHandle.Alloc (sockAddrStructureBytes, GCHandleType.Pinned);
		for (int i = 0; i < socketAddress.Size; ++i) {
			sockAddrStructureBytes [i] = socketAddress [i];
		}
		return sockAddrHandle;
	}

	public static GCHandle Generate_unmanaged_buffer(byte[] buf)
	{
		// use an array of bytes instead of the sockaddr structure 
		GCHandle sockAddrHandle = GCHandle.Alloc (buf, GCHandleType.Pinned);
		return sockAddrHandle;
	}
}
