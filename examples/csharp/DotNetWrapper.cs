/*
 * ZeroTier SDK - Network Virtualization Everywhere
 * Copyright (C) 2011-2017  ZeroTier, Inc.  https://www.zerotier.com/
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
 * You can be released from the requirements of the license by purchasing
 * a commercial license. Buying such a license is mandatory as soon as you
 * develop commercial closed-source software that incorporates or links
 * directly against ZeroTier software without disclosing the source code
 * of your own application.
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
using System.Globalization;

// TODO:
/*
 * check for mem leaks surrounding managed/unmanaged barrier
 * find root of 2X buffer size requirement issue
 * check that cross-thread oprations are handled correctly
 * check that .IsRunning() doesn't bork the entire system anymore
 * Allow max packet size configuration
 * Handle exceptions from unmanaged code
 * */

// Provides a bare-bones interface to ZeroTier-administered sockets
public class ZTSDK {

	// ZeroTier background thread
	protected Thread ztThread;
	protected List<int> connections = new List<int> ();
	protected int MaxPacketSize;

	// Only allow one network at a time for BETA
	protected bool joined_to_network = false;
	protected string nwid = "";
	protected string path = "";

	// Platform-specific paths and bundle/libary names
	#if UNITY_STANDALONE_OSX || UNITY_EDITOR_OSX
	const string DLL_PATH = "ZeroTierSDK_Unity3D_OSX";
	protected string rpc_path = "/Library/Application\\ Support/ZeroTier/SDK/";
	#endif
	#if UNITY_IOS || UNITY_IPHONE
	const string DLL_PATH = "ZeroTierSDK_Unity3D_iOS";
	protected string rpc_path = "ZeroTier/One/";
	#endif
	#if UNITY_STANDALONE_WIN || UNITY_EDITOR_WIN
	const string DLL_PATH = "ZeroTierSDK_Unity3D_WIN";
	protected string rpc_path = "";
	#endif
	#if UNITY_STANDALONE_LINUX
	const string DLL_PATH = "ZeroTierSDK_Unity3D_LINUX";
	protected string rpc_path = "";
	#endif
	#if UNITY_ANDROID
	const string DLL_PATH = "ZeroTierSDK_Unity3D_ANDROID";
	protected string rpc_path = "ZeroTier/One/";
	#endif 

#region DLL Imports
	// ZeroTier service / debug initialization
	[DllImport (DLL_PATH)]
	public static extern void SetDebugFunction( IntPtr fp );
	[DllImport (DLL_PATH)]
	private static extern int unity_start_service(string path);
	[DllImport (DLL_PATH)]
	private static extern int unity_start_service_and_rpc(string path, string nwid);
	[DllImport (DLL_PATH)]
	protected static extern bool zts_is_running();
	[DllImport (DLL_PATH)]
	protected static extern void zts_stop_service();

	// Connection calls
	[DllImport (DLL_PATH)]
	protected static extern int zts_socket(int family, int type, int protocol);

	[DllImport (DLL_PATH)]
	unsafe protected static extern int zts_bind(int sockfd, System.IntPtr addr, int addrlen);
	[DllImport (DLL_PATH)]
	unsafe protected static extern int zts_connect(int sockfd, System.IntPtr addr, int addrlen);

	[DllImport (DLL_PATH)]
	protected static extern int zts_accept(int sockfd);
	[DllImport (DLL_PATH)]
	protected static extern int zts_listen(int sockfd, int backlog);
	[DllImport (DLL_PATH)]
	protected static extern int zts_close(int sockfd);

	// RX / TX
	[DllImport (DLL_PATH)]
	unsafe protected static extern int zts_recv(int sockfd, [In, Out] IntPtr buf, int len);
	[DllImport (DLL_PATH)]
	unsafe protected static extern int zts_send(int sockfd, IntPtr buf, int len);
	[DllImport (DLL_PATH)]
	unsafe protected static extern int zts_set_nonblock(int sockfd);

	[DllImport (DLL_PATH)]
	unsafe protected static extern int zts_sendto(int fd, IntPtr buf, int len, int flags, System.IntPtr addr, int addrlen);
	[DllImport (DLL_PATH)]
	unsafe protected static extern int zts_recvfrom(int fd, [In, Out] IntPtr buf, int len, int flags, System.IntPtr addr, int addrlen);

	// ZT Network controls
	[DllImport (DLL_PATH)]
	protected static extern void zts_join_network(string nwid);
	[DllImport (DLL_PATH)]
	protected static extern void zts_leave_network(string nwid);
#endregion

	// Interop structures
	[System.Runtime.InteropServices.StructLayoutAttribute(System.Runtime.InteropServices.LayoutKind.Sequential, CharSet=System.Runtime.InteropServices.CharSet.Ansi)]
	public struct sockaddr {
		/// u_short->unsigned short
		public ushort sa_family;
		/// char[14]
		[System.Runtime.InteropServices.MarshalAsAttribute(System.Runtime.InteropServices.UnmanagedType.ByValTStr, SizeConst=14)]
		public string sa_data;
	}

	[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
	public delegate void MyDelegate(string str);

	// Debug output callback
	static void CallBackFunction(string str) {
		Debug.Log("ZeroTier: " + str);
	}
		

	// Returns a path for RPC communications to the service
	private string rpcCommPath()
	{
		if(path != "" && nwid != "") {
			return path + "nc_" + nwid;
		}
		return "";
	}

	// Thread which starts the ZeroTier service
	protected void zt_service_thread()
	{
		// Set up debug callback
		MyDelegate callback_delegate = new MyDelegate( CallBackFunction );
		IntPtr intptr_delegate = Marshal.GetFunctionPointerForDelegate(callback_delegate);
		SetDebugFunction( intptr_delegate );

		// Start service
		/* This new instance will communicate via a named pipe, so any 
		 * API calls (ZeroTier.Connect(), ZeroTier.Send(), etc) will be sent to the service
		 * via this pipe.
		 */
		if(nwid.Length > 0) {
			unity_start_service_and_rpc (path, nwid);
		}
		else {
			unity_start_service(rpcCommPath());		
		}
	}

	// Returns the nwid of the network you're currently connected to
	public string GetNetworkID() {
		return nwid;
	}

	// Returns whether you're currently connected to a network
	public bool IsConnected() {
		return nwid != "";
	}

	// Start the ZeroTier service
	protected void Init()
	{
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
	public ZTSDK(string path, string nwid) {
		Debug.Log("ZTSDK(): " + nwid);

		this.path = path;
		this.nwid = nwid;
		Init();
	}

	public ZTSDK (string path) {
		this.path = path;
		Init();
	}

	// Initialize the ZeroTier service
	public ZTSDK() {
		Init();
	}

#region Network Handling
	// Joins a ZeroTier virtual network
	public bool JoinNetwork(string nwid)
	{
		if(!joined_to_network) {
			zts_join_network(nwid);
			return true;
		}
		return false;
	}

	// Leaves a ZeroTier virtual network
	public bool LeaveNetwork(string nwid)
	{
		if(!joined_to_network) {
			return false;
		}
		else {
			zts_leave_network(nwid);
			return true;
		}
	}
#endregion

	// Creates a new ZeroTier-administered socket
	public int Socket(int family, int type, int protocol)
	{
		return zts_socket (family, type, protocol);
	}

	// Binds to a specific address
	public int Bind(int fd, string addr, int port)
	{
		GCHandle sockaddr_ptr = Generate_unmananged_sockaddr(addr + ":" + port);
		IntPtr pSockAddr = sockaddr_ptr.AddrOfPinnedObject ();
		int addrlen = Marshal.SizeOf (pSockAddr);
		return zts_bind (fd, pSockAddr, addrlen);
	}

	// Listens for an incoming connection request
	public int Listen(int fd, int backlog)
	{
		return zts_listen(fd, backlog);
	}

	// Accepts an incoming connection
	public int Accept(int fd)
	{
		return zts_accept (fd);
	}

	// Closes a connection
	public int Close(int fd)
	{
		return Close (fd);
	}

	// Connects to a remote host
	public int Connect(int fd, string addr, int port)
	{
		GCHandle sockaddr_ptr = Generate_unmananged_sockaddr(addr + ":" + port);
		IntPtr pSockAddr = sockaddr_ptr.AddrOfPinnedObject ();
		int addrlen = Marshal.SizeOf (pSockAddr);
		return zts_connect (fd, pSockAddr, addrlen);
	}

	public int Read(int fd, ref char[] buf, int len)
	{
		GCHandle handle = GCHandle.Alloc(buf, GCHandleType.Pinned);
		IntPtr ptr = handle.AddrOfPinnedObject();
		int bytes_read = zts_recv (fd, ptr, len*2);
		string str = Marshal.PtrToStringAuto(ptr);
		//Marshal.Copy (ptr, buf, 0, bytes_read);
		buf = Marshal.PtrToStringAnsi(ptr).ToCharArray();
		return bytes_read;
	}

	public int Write(int fd, char[] buf, int len)
	{
		GCHandle handle = GCHandle.Alloc(buf, GCHandleType.Pinned);
		IntPtr ptr = handle.AddrOfPinnedObject();
		//error = 0;
		int bytes_written;
		// FIXME: Sending a length of 2X the buffer size seems to fix the object pinning issue
		if((bytes_written = zts_send(fd, ptr, len*2)) < 0) {
			//error = (byte)bytes_written;
		}
		return bytes_written;
	}

	// Sends data to an address
	public int SendTo(int fd, char[] buf, int len, int flags, string addr, int port)
	{
		GCHandle handle = GCHandle.Alloc(buf, GCHandleType.Pinned);
		IntPtr ptr = handle.AddrOfPinnedObject();
		int bytes_written;

		// Form address structure
		GCHandle sockaddr_ptr = Generate_unmananged_sockaddr(addr + ":" + port);
		IntPtr pSockAddr = sockaddr_ptr.AddrOfPinnedObject ();
		int addrlen = Marshal.SizeOf (pSockAddr);

		if((bytes_written = zts_sendto(fd, ptr, len*2, flags, pSockAddr, addrlen)) < 0) {
			//error = (byte)bytes_written;
		}
		return bytes_written;
	}
		
	// Receives data from an address
	public int RecvFrom(int fd, ref char[] buf, int len, int flags, string addr, int port)
	{
		GCHandle handle = GCHandle.Alloc(buf, GCHandleType.Pinned);
		IntPtr ptr = handle.AddrOfPinnedObject();

		// Form address structure
		GCHandle sockaddr_ptr = Generate_unmananged_sockaddr(addr + ":" + port);
		IntPtr pSockAddr = sockaddr_ptr.AddrOfPinnedObject ();
		int addrlen = Marshal.SizeOf (pSockAddr);

		int bytes_read = zts_recvfrom(fd, ptr, len*2, flags, pSockAddr, addrlen);
		string str = Marshal.PtrToStringAuto(ptr);
		//Marshal.Copy (ptr, buf, 0, bytes_read);
		buf = Marshal.PtrToStringAnsi(ptr).ToCharArray();
		return bytes_read;
	}
		
#region Service-Related calls
	// Returns whether the ZeroTier service is currently running
	public bool IsRunning()
	{
		return zts_is_running ();
	}

	// Terminates the ZeroTier service 
	public void Terminate()
	{
		zts_stop_service ();
	}
#endregion


// --- Utilities ---


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
		ipEndPoint = CreateIPEndPoint (endpoint_str);
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
