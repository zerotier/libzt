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
using System.Threading;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System;
using UnityEngine.Networking;

// Provides a familiar interface which is modeled after the Unity LLAPI
public class ZeroTierLLAPI : ZeroTierNetworkInterface {

	// Initialize the ZeroTier service with a given path
	public ZeroTierLLAPI(string path)
	{
		rpc_path = path;
		Init();
	}

	// Creates a ZeroTier Socket and binds on the port provided
	// A client instance can now connect to this "host"
	public int AddHost(int port)
	{
		int sockfd = zt_socket ((int)AddressFamily.InterNetwork, (int)SocketType.Stream, (int)ProtocolType.Unspecified);
		if (sockfd < 0) {
			return -1;
		}
		GCHandle sockaddr_ptr = ZeroTierUtils.Generate_unmananged_sockaddr("0.0.0.0" + ":" + port);
		IntPtr pSockAddr = sockaddr_ptr.AddrOfPinnedObject ();
		int addrlen = Marshal.SizeOf (pSockAddr);
		// Set socket to non-blocking for RX polling in Receive()
		zt_set_nonblock(sockfd);
		return zt_bind (sockfd, pSockAddr, addrlen);
	}

	// hostId - host socket ID for this connection
	public int Connect(int hostId, string address, int port, out byte error)
	{
		int sockfd = zt_socket ((int)AddressFamily.InterNetwork, (int)SocketType.Stream, (int)ProtocolType.Unspecified);
		Debug.Log ("sockfd = " + sockfd);

		if (sockfd < 0) {
			error = (byte)sockfd;
			return -1;
		}
		GCHandle sockaddr_ptr = ZeroTierUtils.Generate_unmananged_sockaddr(address + ":" + port);
		IntPtr pSockAddr = sockaddr_ptr.AddrOfPinnedObject ();
		int addrlen = Marshal.SizeOf (pSockAddr);
		error = (byte)zt_connect (sockfd, pSockAddr, addrlen);

		// Set socket to non-blocking for RX polling in Receive()
		zt_set_nonblock(sockfd);
		return sockfd;
	}

	// Write data to a ZeroTier socket
	public int Send(int fd, char[] buf, int len, out byte error)
	{
		error = 0;
		return Write(fd, buf, len);
	}

	// Checks for data to RX
	/*
	public enum NetworkEventType
	{
		DataEvent,
		ConnectEvent,
		DisconnectEvent,
		Nothing,
		BroadcastEvent
	}
	*/
	public NetworkEventType Receive(out int hostId, out int connectionId, out int channelId, byte[] buffer, int bufferSize, out int receivedSize, out byte error)
	{
		hostId = 0;
		connectionId = 0;
		channelId = 0;
		receivedSize = 0;
		error = 0;


		// Read() ... 

		/*
		 *  If recBuffer is big enough to contain data, data will be copied in the buffer. 
		 *  If not, error will contain MessageToLong error and you will need reallocate 
		 *  buffer and call this function again. */

		//for (int i = 0; i < connections.Count; i++) {

		//}

		/*
		int res;
		res = zt_recv (connectionId, buffer, bufferSize);

		// FIXME: Not quite semantically the same, but close enough for alpha release?
		// FIXME: Get notifications of disconnect events?
		if (res == -1) {
			error = -1;
			return NetworkEventType.DisconnectEvent; 
		}
		if(res == 0) {
			error = 0;
			return NetworkEventType.Nothing; // No data read
		}
		if (res > 0) {
			receivedSize = res;
			Marshal.Copy(buffer, buffer, 0, res);
			return NetworkEventType.DataEvent; // Data read into buffer
		}
		*/

		return NetworkEventType.Nothing;
	}

	// Shutdown a given connection
	public int Disconnect(int fd)
	{
		return zt_close (fd);
	}

	// Test serialization methods, should be removed for production

	public static byte[] RawSerializeEx( object anything )
	{
		int rawsize = Marshal.SizeOf( anything );
		byte[] rawdatas = new byte[ rawsize ];
		GCHandle handle = GCHandle.Alloc( rawdatas, GCHandleType.Pinned );
		IntPtr buffer = handle.AddrOfPinnedObject();
		Marshal.StructureToPtr( anything, buffer, false );
		handle.Free();
		return rawdatas;
	}


	public static object RawDeserializeEx( byte[] rawdatas, Type anytype )
	{
		int rawsize = Marshal.SizeOf( anytype );
		if( rawsize > rawdatas.Length )
			return null;
		GCHandle handle = GCHandle.Alloc( rawdatas, GCHandleType.Pinned );
		IntPtr buffer = handle.AddrOfPinnedObject();
		object retobj = Marshal.PtrToStructure( buffer, anytype );
		handle.Free();
		return retobj;
	}
}
