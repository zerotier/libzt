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
using UnityEngine.UI;
using System;
using System.Collections;
using System.Runtime.InteropServices;
using System.Threading;
using UnityEngine.Networking;

using System.Net.Sockets;
using System.Net;

using System.IO;


public class ZeroTierNetworkInterface {

	// ZeroTier background thread
	private Thread ztThread;

	// Interop structures
	[System.Runtime.InteropServices.StructLayoutAttribute(System.Runtime.InteropServices.LayoutKind.Sequential, CharSet=System.Runtime.InteropServices.CharSet.Ansi)]
	public struct sockaddr {
		/// u_short->unsigned short
		public ushort sa_family;
		/// char[14]
		[System.Runtime.InteropServices.MarshalAsAttribute(System.Runtime.InteropServices.UnmanagedType.ByValTStr, SizeConst=14)]
		public string sa_data;
	}

	// Virtual network interace config
	private int MaxPacketSize;
	private string rpc_path = "/does/this/work";

	[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
	public delegate void MyDelegate(string str);

	static void CallBackFunction(string str) {
		Debug.Log("Native ZT Plugin: " + str);
	}

	// ZeroTier service / debug initialization
	[DllImport ("ZeroTierUnity")]
	public static extern void SetDebugFunction( IntPtr fp );
	[DllImport ("ZeroTierUnity")]
	private static extern int unity_start_service(string path);

	// Connection calls
	[DllImport ("ZeroTierUnity")]
	private static extern int zt_socket(int family, int type, int protocol);


	[DllImport ("ZeroTierUnity")]
	unsafe private static extern int zt_bind(int sockfd, System.IntPtr addr, int addrlen);
	[DllImport ("ZeroTierUnity")]
	unsafe private static extern int zt_connect(int sockfd, System.IntPtr addr, int addrlen);

	[DllImport ("ZeroTierUnity")]
	private static extern int zt_accept(int sockfd);
	[DllImport ("ZeroTierUnity")]
	private static extern int zt_listen(int sockfd, int backlog);
	[DllImport ("ZeroTierUnity")]
	private static extern int zt_close(int sockfd);

	// RX / TX
	[DllImport ("ZeroTierUnity")]
	unsafe private static extern int zt_recv(int sockfd, System.IntPtr buf, int len);
	[DllImport ("ZeroTierUnity")]
	unsafe private static extern int zt_send(int sockfd, System.IntPtr buf, int len);

	// ZT Thread controls
	[DllImport ("ZeroTierUnity")]
	private static extern bool zt_is_running();
	[DllImport ("ZeroTierUnity")]
	private static extern void zt_terminate();

	// ZT Network controls
	[DllImport ("ZeroTierUnity")]
	private static extern bool zt_join_network(string nwid);
	[DllImport ("ZeroTierUnity")]
	private static extern void zt_leave_network(string nwid);

	// Thread which starts the ZeroTier service
	// The ZeroTier service may spin off a SOCKS5 proxy server 
	// if -DUSE_SOCKS_PROXY is set when building the bundle
	private void zt_service_thread()
	{
		MyDelegate callback_delegate = new MyDelegate( CallBackFunction );
		// Convert callback_delegate into a function pointer that can be
		// used in unmanaged code.
		IntPtr intptr_delegate = 
			Marshal.GetFunctionPointerForDelegate(callback_delegate);
		// Call the API passing along the function pointer.
		SetDebugFunction( intptr_delegate );
		unity_start_service (rpc_path);
	}

	// Start the ZeroTier service
	private void Init()
	{
		// TODO: Handle exceptions from unmanaged code
		ztThread = new Thread(() => {
			try {
				zt_service_thread();
			} catch(Exception e) {
				Debug.Log(e.Message.ToString());
			}
		});
		ztThread.IsBackground = true; // Allow the thread to be aborted safely
		ztThread.Start();
	}

	// Initialize the ZeroTier service with a given path
	public ZeroTierNetworkInterface(string path)
	{
		rpc_path = path;
		Init();
	}

	// Initialize the ZeroTier service
	public ZeroTierNetworkInterface()
	{
		Init();
	}

	// Initialize the ZeroTier service
	// Use the GlobalConfig to set things like the max packet size
	public ZeroTierNetworkInterface(GlobalConfig gConfig)
	{
		MaxPacketSize = gConfig.MaxPacketSize; // TODO: Do something with this!
		Init();
	}

	// Joins a ZeroTier virtual network
	public void JoinNetwork(string nwid)
	{
		zt_join_network(nwid);
	}

	// Leaves a ZeroTier virtual network
	public void LeaveNetwork(string nwid)
	{
		zt_leave_network(nwid);
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
		return sockfd;
	}

	// Returns whether the ZeroTier service is currently running
	public bool IsRunning()
	{
		return zt_is_running ();
	}

	// Terminates the ZeroTier service 
	public void Terminate()
	{
		zt_terminate ();
	}

	// Shutdown a given connection
	public int Disconnect(int fd)
	{
		return zt_close (fd);
	}

	// Sends data out over the network

	public int Send(int fd, char[] buf, int len, out byte error)
	{
		int bytes_written = 0;
		error = 0;

		GCHandle buf_handle = GCHandle.Alloc (buf, GCHandleType.Pinned);
		IntPtr pBufPtr = buf_handle.AddrOfPinnedObject ();

		//int len = Marshal.SizeOf (pBufPtr);
		if((bytes_written = zt_send (fd, pBufPtr, len)) < 0) {
			error = (byte)bytes_written;
		}
		return bytes_written;
	}


	// Structure used to house arrays meant to be sent to unmanaged memory and passed to the 
	// ZeroTier service
	public struct UnityArrayInput
	{
		public IntPtr array;
	}

	/*
	// Sends data out over the network
	public int Send(int fd, char[] buf, int len, out byte error)
	{
		//char[] buffer = new char[1024];
		UnityArrayInput data = new UnityArrayInput ();
		data.array = Marshal.AllocHGlobal (Marshal.SizeOf (typeof(char))*buf.Length);
		//data.len = buf.Length;
		int bytes_written = 0;
		error = 0;

		try
		{
			Marshal.Copy(buf, 0, data.array, buf.Length);
		
			Debug.Log(buf.Length);
			// ZT API call
			if((bytes_written = zt_send (fd, data.array, buf.Length)) < 0) {
				error = (byte)bytes_written;
			}
			return bytes_written;
		}
		finally
		{
			Marshal.FreeHGlobal (data.array);
		}
		return 0;
	}
	*/

	// Checks for data to RX
	public int OnReceive(int fd, byte[] buf, int len)
	{
		return 0;
		//return zt_read(fd, buf, len);
	}
}
