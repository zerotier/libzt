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
using UnityEngine.Networking;

using System;
using System.Collections;
using System.Runtime.InteropServices;
using System.Threading;
using System.Net.Sockets;
using System.Net;
using System.IO;
using System.Collections.Generic;

// Provides a bare-bones interface to ZeroTier-administered sockets
public class ZeroTierNetworkInterface {

	// Apple
	#if UNITY_STANDALONE_OSX || UNITY_EDITOR_OSX
	const string DLL_PATH = "ZeroTierSDK_Unity3D_OSX";
	#endif

	#if UNITY_IOS || UNITY_IPHONE
	const string DLL_PATH = "ZeroTierSDK_Unity3D_iOS";
	#endif

	// Windows
	#if UNITY_STANDALONE_WIN || UNITY_EDITOR_WIN
	const string DLL_PATH = "ZeroTierSDK_Unity3D_WIN";
	#endif

	// Linux
	#if UNITY_STANDALONE_LINUX
	const string DLL_PATH = "ZeroTierSDK_Unity3D_LINUX";
	#endif

	// Android
	#if UNITY_ANDROID
	const string DLL_PATH = "ZeroTierSDK_Unity3D_ANDROID";
	#endif 

	// Interop structures
	[System.Runtime.InteropServices.StructLayoutAttribute(System.Runtime.InteropServices.LayoutKind.Sequential, CharSet=System.Runtime.InteropServices.CharSet.Ansi)]
	public struct sockaddr {
		/// u_short->unsigned short
		public ushort sa_family;
		/// char[14]
		[System.Runtime.InteropServices.MarshalAsAttribute(System.Runtime.InteropServices.UnmanagedType.ByValTStr, SizeConst=14)]
		public string sa_data;
	}

	// ZeroTier background thread
	protected Thread ztThread;
	protected List<int> connections = new List<int> ();
	protected int MaxPacketSize;
	protected string rpc_path = "";

	[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
	public delegate void MyDelegate(string str);

	static void CallBackFunction(string str) {
		Debug.Log("Native ZT Plugin: " + str);
	}

#region DLL Imports
	// ZeroTier service / debug initialization
	[DllImport (DLL_PATH)]
	public static extern void SetDebugFunction( IntPtr fp );
	[DllImport (DLL_PATH)]
	private static extern int unity_start_service(string path);

	// Connection calls
	[DllImport (DLL_PATH)]
	protected static extern int zt_socket(int family, int type, int protocol);

	[DllImport (DLL_PATH)]
	unsafe protected static extern int zt_bind(int sockfd, System.IntPtr addr, int addrlen);
	[DllImport (DLL_PATH)]
	unsafe protected static extern int zt_connect(int sockfd, System.IntPtr addr, int addrlen);

	[DllImport (DLL_PATH)]
	protected static extern int zt_accept(int sockfd);
	[DllImport (DLL_PATH)]
	protected static extern int zt_listen(int sockfd, int backlog);
	[DllImport (DLL_PATH)]
	protected static extern int zt_close(int sockfd);

	// RX / TX
	[DllImport (DLL_PATH)]
	unsafe protected static extern int zt_recv(int sockfd, IntPtr buf, int len);
	[DllImport (DLL_PATH)]
	unsafe protected static extern int zt_send(int sockfd, IntPtr buf, int len);
	[DllImport (DLL_PATH)]
	unsafe protected static extern int zt_set_nonblock(int sockfd);

	// ZT Thread controls
	[DllImport (DLL_PATH)]
	protected static extern bool zt_is_running();
	[DllImport (DLL_PATH)]
	protected static extern void zt_terminate();

	// ZT Network controls
	[DllImport (DLL_PATH)]
	protected static extern bool zt_join_network(string nwid);
	[DllImport (DLL_PATH)]
	protected static extern void zt_leave_network(string nwid);
#endregion

	// Thread which starts the ZeroTier service
	// The ZeroTier service may spin off a SOCKS5 proxy server 
	// if -DUSE_SOCKS_PROXY is set when building the bundle
	protected void zt_service_thread()
	{
		MyDelegate callback_delegate = new MyDelegate( CallBackFunction );
		// Convert callback_delegate into a function pointer that can be
		// used in unmanaged code.
		IntPtr intptr_delegate = 
			Marshal.GetFunctionPointerForDelegate(callback_delegate);
		// Call the API passing along the function pointer.
		SetDebugFunction( intptr_delegate );
		Debug.Log ("rpc_path = " + rpc_path);
		unity_start_service (rpc_path);
	}

	// Start the ZeroTier service
	protected void Init()
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

#region Network Handling
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
#endregion

	// Low-level representations of ZeroTier sockets
	// The ZeroTier LLAPI is built on top of these

	// Creates a new ZeroTier-administered socket
	public int Socket(int family, int type, int protocol)
	{
		return zt_socket (family, type, protocol);
	}

	// Binds to a specific address
	public int Bind(int fd, string addr, int port)
	{
		GCHandle sockaddr_ptr = ZeroTierUtils.Generate_unmananged_sockaddr(addr + ":" + port);
		IntPtr pSockAddr = sockaddr_ptr.AddrOfPinnedObject ();
		int addrlen = Marshal.SizeOf (pSockAddr);
		return zt_bind (fd, pSockAddr, addrlen);
	}

	// Listens for an incoming connection request
	public int Listen(int fd, int backlog)
	{
		return zt_listen(fd, backlog);
	}

	// Accepts an incoming connection
	public int Accept(int fd)
	{
		return zt_accept (fd);
	}

	// Closes a connection
	public int Close(int fd)
	{
		return Close (fd);
	}

	// Connects to a remote host
	public int Connect(int fd, string addr, int port)
	{
		GCHandle sockaddr_ptr = ZeroTierUtils.Generate_unmananged_sockaddr(addr + ":" + port);
		IntPtr pSockAddr = sockaddr_ptr.AddrOfPinnedObject ();
		int addrlen = Marshal.SizeOf (pSockAddr);
		return zt_connect (fd, pSockAddr, addrlen);
	}


	/*
	unsafe { 
		byte *ptr = (byte *)buffer.ToPointer();

		int offset = 0;
		for (int i=0; i<height; i++)
		{
			for (int j=0; j<width; j++)
			{

				float b = (float)ptr[offset+0] / 255.0f;
				float g = (float)ptr[offset+1] / 255.0f;
				float r = (float)ptr[offset+2] / 255.0f;
				float a = (float)ptr[offset+3] / 255.0f;
				offset += 4;

				UnityEngine.Color color = new UnityEngine.Color(r, g, b, a);
				texture.SetPixel(j, height-i, color);
			}
		}
	}
	*/

	public int Read(int fd, char[] buf, int len)
	{
		GCHandle handle = GCHandle.Alloc(buf, GCHandleType.Pinned);
		IntPtr ptr = handle.AddrOfPinnedObject();
		int bytes_read = zt_recv (fd, ptr, len*2);
		Marshal.Copy (ptr, buf, 0, bytes_read); // FIXME: Copies back into managed memory, should maybe avoid copying
		return bytes_read;
	}

	public int Write(int fd, char[] buf, int len)
	{
		GCHandle handle = GCHandle.Alloc(buf, GCHandleType.Pinned);
		IntPtr ptr = handle.AddrOfPinnedObject();
		//error = 0;
		int bytes_written;
		// FIXME: Sending a length of 2X the buffer size seems to fix the object pinning issue
		if((bytes_written = zt_send(fd, ptr, len*2)) < 0) {
			//error = (byte)bytes_written;
		}
		return bytes_written;
	}

#region Service-Related calls
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
#endregion
}
