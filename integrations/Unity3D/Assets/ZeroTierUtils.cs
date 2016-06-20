/*
 * ZeroTier One - Network Virtualization Everywhere
 * Copyright (C) 2011-2015  ZeroTier, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * --
 *
 * ZeroTier may be used and distributed under the terms of the GPLv3, which
 * are available at: http://www.gnu.org/licenses/gpl-3.0.html
 *
 * If you would like to embed ZeroTier into a commercial application or
 * redistribute it in a modified binary form, please contact ZeroTier Networks
 * LLC. Start here: http://www.zerotier.com/
 */

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
