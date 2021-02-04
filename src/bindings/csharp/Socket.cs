/*
 * Copyright (c)2013-2021 ZeroTier, Inc.
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file in the project's root directory.
 *
 * Change Date: 2025-01-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2.0 of the Apache License.
 */
/****/

using System; // For ObjectDisposedException
using System.Net; // For IPEndPoint
using System.Net.Sockets; // For ZeroTier.SocketException
using System.Runtime.InteropServices;

using ZeroTier;

/// <summary>
/// ZeroTier SDK
/// </summary>
namespace ZeroTier
{
	/// <summary>
	/// ZeroTier Socket - An lwIP socket mediated over a ZeroTier virtual link
	/// </summary>
	public class Socket
	{
		/// <summary>No error.</summary>
		public static readonly int ZTS_ERR_OK        =  0;
		/// <summary>Socket error, see Socket.ErrNo() for additional context.</summary>
		public static readonly int ZTS_ERR_SOCKET    = -1;
		/// <summary>You probably did something at the wrong time.</summary>
		public static readonly int ZTS_ERR_SERVICE   = -2;
		/// <summary>Invalid argument.</summary>
		public static readonly int ZTS_ERR_ARG       = -3;
		/// <summary>No result. (not necessarily an error.)</summary>
		public static readonly int ZTS_ERR_NO_RESULT = -4;
		/// <summary>Consider filing a bug report.</summary>
		public static readonly int ZTS_ERR_GENERAL   = -5;

		int _fd;
		bool _isClosed;
		bool _isListening;
		bool _isBlocking;

		AddressFamily _socketFamily;
		SocketType _socketType;
		ProtocolType _socketProtocol;

		internal EndPoint _localEndPoint;
		internal EndPoint _remoteEndPoint;

		private void InitializeInternalFlags()
		{
			_isClosed = false;
			_isListening = false;
			_isBlocking = false;
		}

		public Socket(AddressFamily addressFamily, SocketType socketType, ProtocolType protocolType)
		{
			int family = -1;
			int type = -1;
			int protocol = -1;
			// Map .NET socket parameters to ZeroTier equivalents
			switch (addressFamily)
			{
				case AddressFamily.InterNetwork:
					family = Constants.AF_INET;
					break;
				case AddressFamily.InterNetworkV6:
					family = Constants.AF_INET6;
					break;
				case AddressFamily.Unknown:
					family = Constants.AF_UNSPEC;
					break;
			}
			switch (socketType)
			{
				case SocketType.Stream:
					type = Constants.SOCK_STREAM;
					break;
				case SocketType.Dgram:
					type = Constants.SOCK_DGRAM;
					break;
			}
			switch (protocolType)
			{
				case ProtocolType.Udp:
					protocol = Constants.IPPROTO_UDP;
					break;
				case ProtocolType.Tcp:
					protocol = Constants.IPPROTO_TCP;
					break;
				case ProtocolType.Unspecified:
					protocol = 0; // ?
					break;
			}
			if ((_fd = zts_socket(family, type, protocol)) < 0)
			{
				throw new ZeroTier.SocketException((int)_fd);
			}
			_socketFamily = addressFamily;
			_socketType = socketType;
			_socketProtocol = protocolType;
			InitializeInternalFlags();
		}

		private Socket(int fileDescriptor,
			AddressFamily addressFamily,
			SocketType socketType,
			ProtocolType protocolType,
			EndPoint localEndPoint,
			EndPoint remoteEndPoint)
		{
			_socketFamily = addressFamily;
			_socketType = socketType;
			_socketProtocol = protocolType;
			_localEndPoint = localEndPoint;
			_remoteEndPoint = remoteEndPoint;
			_fd = fileDescriptor;
			InitializeInternalFlags();
		}

		public void Connect(IPEndPoint remoteEndPoint)
		{
			if (_isClosed) {
				throw new ObjectDisposedException("Socket has been closed");
			}
			if (_fd < 0) {
				// Invalid file descriptor
				throw new ZeroTier.SocketException((int)Constants.ERR_SOCKET);
			}
			if (remoteEndPoint == null) {
				throw new ArgumentNullException("remoteEndPoint");
			}
			int err = Constants.ERR_OK;
			int addrlen = 0;
			if (remoteEndPoint.AddressFamily == AddressFamily.InterNetwork)
			{	            
				zts_sockaddr_in sa = new zts_sockaddr_in();
				addrlen = Marshal.SizeOf(typeof(zts_sockaddr_in));
				switch (remoteEndPoint.AddressFamily)
				{
					case AddressFamily.InterNetwork:
						sa.sin_family = (byte)Constants.AF_INET;
						break;
					case AddressFamily.InterNetworkV6:
						sa.sin_family = (byte)Constants.AF_INET6;
						break;
					case AddressFamily.Unknown:
						sa.sin_family = (byte)Constants.AF_UNSPEC;
						break;
				}
				sa.sin_port = (short)zts_htons((ushort)remoteEndPoint.Port);
				sa.sin_addr = remoteEndPoint.Address.GetAddressBytes();
				sa.sin_len = (byte)addrlen; // lwIP-specific
			
				IntPtr ptr1 = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(zts_sockaddr)));
				Marshal.StructureToPtr(sa, ptr1, false);
				//zts_sockaddr sAddr = (zts_sockaddr)Marshal.PtrToStructure(ptr1, typeof(zts_sockaddr));
				err = zts_connect(_fd, ptr1, (byte)addrlen);

			}
			if (remoteEndPoint.AddressFamily == AddressFamily.InterNetworkV6)
			{
				/*
				socketAddress.iSockaddrLength = Marshal.SizeOf(typeof(sockaddr_in6));
				socketAddress.lpSockAddr = CriticalAllocHandle.FromSize(socketAddress.iSockaddrLength);
				sockaddr_in6 sa = new sockaddr_in6();
				sa.sin6_family = (short)AddressFamily.InterNetworkV6;
				sa.sin6_port = (ushort)endpoint.Port;
				sa.sin6_addr = endpoint.Address.GetAddressBytes();
				sa.sin6_scope_id = (uint)endpoint.Address.ScopeId;
				Marshal.StructureToPtr(sa, (IntPtr)socketAddress.lpSockAddr, false);
				*/
			}
			if (err < 0) {
				throw new ZeroTier.SocketException(err, ZeroTier.Node.ErrNo);
			}
			_remoteEndPoint = remoteEndPoint;
		}

		public void Bind(IPEndPoint localEndPoint)
		{
			if (_isClosed) {
				throw new ObjectDisposedException("Socket has been closed");
			}
			if (_fd < 0) {
				// Invalid file descriptor
				throw new ZeroTier.SocketException((int)Constants.ERR_SOCKET);
			}
			if (localEndPoint == null) {
				throw new ArgumentNullException("localEndPoint");
			}
			int err = Constants.ERR_OK;
			int addrlen = 0;
			if (localEndPoint.AddressFamily == AddressFamily.InterNetwork)
			{	            
				zts_sockaddr_in sa = new zts_sockaddr_in();
				addrlen = Marshal.SizeOf(typeof(zts_sockaddr_in));
				switch (localEndPoint.AddressFamily)
				{
					case AddressFamily.InterNetwork:
						sa.sin_family = (byte)Constants.AF_INET;
						break;
					case AddressFamily.InterNetworkV6:
						sa.sin_family = (byte)Constants.AF_INET6;
						break;
					case AddressFamily.Unknown:
						sa.sin_family = (byte)Constants.AF_UNSPEC;
						break;
				}
				sa.sin_port = (short)zts_htons((ushort)localEndPoint.Port);
				sa.sin_addr = localEndPoint.Address.GetAddressBytes();
				sa.sin_len = (byte)addrlen; // lwIP-specific
			
				IntPtr ptr1 = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(zts_sockaddr)));
				Marshal.StructureToPtr(sa, ptr1, false);
				err = zts_bind(_fd, ptr1, (byte)addrlen);
			}
			if (localEndPoint.AddressFamily == AddressFamily.InterNetworkV6)
			{
				/*
				socketAddress.iSockaddrLength = Marshal.SizeOf(typeof(sockaddr_in6));
				socketAddress.lpSockAddr = CriticalAllocHandle.FromSize(socketAddress.iSockaddrLength);
				sockaddr_in6 sa = new sockaddr_in6();
				sa.sin6_family = (short)AddressFamily.InterNetworkV6;
				sa.sin6_port = (ushort)endpoint.Port;
				sa.sin6_addr = endpoint.Address.GetAddressBytes();
				sa.sin6_scope_id = (uint)endpoint.Address.ScopeId;
				Marshal.StructureToPtr(sa, (IntPtr)socketAddress.lpSockAddr, false);
				*/
			}
			if (err < 0) {
				throw new ZeroTier.SocketException((int)err);
			}
			_localEndPoint = localEndPoint;
		}

		public void Listen(int backlog)
		{
			if (_isClosed) {
				throw new ObjectDisposedException("Socket has been closed");
			}
			if (_fd < 0) {
				// Invalid file descriptor
				throw new ZeroTier.SocketException((int)Constants.ERR_SOCKET);
			}
			int err = Constants.ERR_OK;
			if ((err = zts_listen(_fd, backlog)) < 0) {
				// Invalid backlog value perhaps?
				throw new ZeroTier.SocketException((int)Constants.ERR_SOCKET);
			}
			_isListening = true;
		}

		public Socket Accept()
		{
			if (_isClosed) {
				throw new ObjectDisposedException("Socket has been closed");
			}
			if (_fd < 0) {
				// Invalid file descriptor
				throw new ZeroTier.SocketException((int)Constants.ERR_SOCKET);
			}
			if (_isListening == false) {
				throw new InvalidOperationException("Socket is not in a listening state. Call Listen() first");
			}
			// TODO: Rewrite -- Check for memory leaks
			// Inform zts_accept of the size of the available address buffer
			int addrlen = Marshal.SizeOf(typeof(zts_sockaddr_in));
			IntPtr addrlenPtr = GCHandle.Alloc(addrlen, GCHandleType.Pinned).AddrOfPinnedObject();
			// Allocate space for address buffer and provide pointer to zts_accept
			zts_sockaddr_in in4 = new zts_sockaddr_in();
			IntPtr remoteAddrPtr = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(zts_sockaddr_in)));
			Marshal.StructureToPtr(in4, remoteAddrPtr, false);

			int err = zts_accept(_fd, remoteAddrPtr, addrlenPtr);
			if (err < 0) {
				throw new ZeroTier.SocketException(err, ZeroTier.Node.ErrNo);
			}
			in4 = (zts_sockaddr_in)Marshal.PtrToStructure(remoteAddrPtr, typeof(zts_sockaddr_in));
			// Convert sockaddr contents to IPEndPoint
			IPAddress ipAddress = new IPAddress(in4.sin_addr);
			IPEndPoint clientEndPoint = new IPEndPoint(ipAddress, zts_ntohs((ushort)in4.sin_port));
			// Create new socket by providing file descriptor returned from zts_accept call.
			Socket clientSocket = new Socket(
				err, _socketFamily, _socketType, _socketProtocol, _localEndPoint, clientEndPoint);
			return clientSocket;
		}

		public void Shutdown(SocketShutdown how)
		{
			if (_isClosed) {
				throw new ObjectDisposedException("Socket has been closed");
			}
			int ztHow = 0;
			switch (how)
			{
				case SocketShutdown.Receive:
					ztHow = Constants.O_RDONLY;
					break;
				case SocketShutdown.Send:
					ztHow = Constants.O_WRONLY;
					break;
				case SocketShutdown.Both:
					ztHow = Constants.O_RDWR;
					break;
			}
			zts_shutdown(_fd, ztHow);
		}

		public void Close()
		{
			if (_isClosed) {
				throw new ObjectDisposedException("Socket has already been closed");
			}
			zts_close(_fd);
			_isClosed = true;
		}

		public EndPoint RemoteEndPoint
		{
			get {
				return _remoteEndPoint;
			}
		}

		public EndPoint LocalEndPoint
		{
			get {
				return _localEndPoint;
			}
		}

		public bool Blocking
		{
			get {
				return _isBlocking;
			}
			set {
				if (_isClosed) {
					throw new ObjectDisposedException("Socket has been closed");
				}
				int opts = 0;
				if ((opts = zts_fcntl(_fd, (int)(ZeroTier.Constants.F_GETFL), 0)) < 0) {
					throw new ZeroTier.SocketException(opts, ZeroTier.Node.ErrNo);
				}
				Console.WriteLine("before.opts={0}", opts);
				if (value) { // Blocking
					opts = opts & (~(ZeroTier.Constants.O_NONBLOCK));
				}
				if (!value) { // Non-Blocking
					opts = opts | (int)(ZeroTier.Constants.O_NONBLOCK);
				}
				Console.WriteLine("after.opts={0}", opts);
				if ((opts = zts_fcntl(_fd, ZeroTier.Constants.F_SETFL, (int)opts)) < 0) {
					throw new ZeroTier.SocketException(opts, ZeroTier.Node.ErrNo);
				}
				_isBlocking = value;
			}
		}

		public bool Poll(int microSeconds, System.Net.Sockets.SelectMode mode)
		{
			if (_isClosed) {
				throw new ObjectDisposedException("Socket has been closed");
			}
			zts_pollfd poll_set = new zts_pollfd();
			poll_set.fd = _fd;
			if (mode == SelectMode.SelectRead) {
				poll_set.events = (short)((byte)ZeroTier.Constants.POLLIN);
			}
			if (mode == SelectMode.SelectWrite) {
				poll_set.events = (short)((byte)ZeroTier.Constants.POLLOUT);
			}
			if (mode == SelectMode.SelectError) {
				poll_set.events = (short)((byte)ZeroTier.Constants.POLLERR | (byte)ZeroTier.Constants.POLLNVAL);
			}
			IntPtr poll_fd_ptr = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(zts_pollfd)));
			Marshal.StructureToPtr(poll_set, poll_fd_ptr, false);
			int result = 0;
			int timeout_ms = (microSeconds / 1000);
			uint numfds = 1;
			if ((result = zts_poll(poll_fd_ptr, numfds, timeout_ms)) < 0) {
				throw new ZeroTier.SocketException(result, ZeroTier.Node.ErrNo);
			}
			poll_set = (zts_pollfd)Marshal.PtrToStructure(poll_fd_ptr, typeof(zts_pollfd));
			if (result == 0) { return false; } // No events
			if (mode == SelectMode.SelectRead) {
				return ((byte)poll_set.revents & (byte)ZeroTier.Constants.POLLIN) != 0;
			}
			if (mode == SelectMode.SelectWrite) {
				return ((byte)poll_set.revents & (byte)ZeroTier.Constants.POLLOUT) != 0;
			}
			if (mode == SelectMode.SelectError) {
				return ((poll_set.revents & (byte)ZeroTier.Constants.POLLERR) != 0) || ((poll_set.revents & (byte)ZeroTier.Constants.POLLNVAL) != 0);
			}
			return false;
		}

		public Int32 Send(Byte[] buffer)
		{
			if (_isClosed) {
				throw new ObjectDisposedException("Socket has been closed");
			}
			if (_fd < 0) {
				throw new ZeroTier.SocketException((int)ZeroTier.Constants.ERR_SOCKET);
			}
			if (buffer == null) {
				throw new ArgumentNullException("buffer");
			}
			int flags = 0;
			IntPtr bufferPtr = Marshal.UnsafeAddrOfPinnedArrayElement(buffer, 0);
			return zts_send(_fd, bufferPtr, (uint)Buffer.ByteLength(buffer), (int)flags);
		}

		public Int32 Receive(Byte[] buffer)
		{
			if (_isClosed) {
				throw new ObjectDisposedException("Socket has been closed");
			}
			if (_fd < 0) {
				throw new ZeroTier.SocketException((int)ZeroTier.Constants.ERR_SOCKET);
			}
			if (buffer == null) {
				throw new ArgumentNullException("buffer");
			}
			int flags = 0;
			IntPtr bufferPtr = Marshal.UnsafeAddrOfPinnedArrayElement(buffer, 0);
			return zts_recv(_fd, bufferPtr, (uint)Buffer.ByteLength(buffer), (int)flags);
		}

		/* Structures and functions used internally to communicate with
		lower-level C API defined in include/ZeroTierSockets.h */

		[DllImport("libzt", EntryPoint="CSharp_zts_get_all_stats")]
		static extern int zts_get_all_stats(IntPtr arg1);

		[DllImport("libzt", EntryPoint="CSharp_zts_get_protocol_stats")]
		static extern int zts_get_protocol_stats(int arg1, IntPtr arg2);

		[DllImport("libzt", EntryPoint="CSharp_zts_socket")]
		static extern int zts_socket(int arg1, int arg2, int arg3);

		[DllImport("libzt", EntryPoint="CSharp_zts_connect")]
		static extern int zts_connect(int arg1, IntPtr arg2, ushort arg3);

		[DllImport("libzt", EntryPoint="CSharp_zts_bind")]
		static extern int zts_bind(int arg1, IntPtr arg2, ushort arg3);

		[DllImport("libzt", EntryPoint="CSharp_zts_listen")]
		static extern int zts_listen(int arg1, int arg2);

		[DllImport("libzt", EntryPoint="CSharp_zts_accept")]
		static extern int zts_accept(int arg1, IntPtr arg2, IntPtr arg3);

		[DllImport("libzt", EntryPoint="CSharp_zts_setsockopt")]
		static extern int zts_setsockopt(int arg1, int arg2, int arg3, IntPtr arg4, ushort arg5);

		[DllImport("libzt", EntryPoint="CSharp_zts_getsockopt")]
		static extern int zts_getsockopt(int arg1, int arg2, int arg3, IntPtr arg4, IntPtr arg5);

		[DllImport("libzt", EntryPoint="CSharp_zts_getsockname")]
		static extern int zts_getsockname(int arg1, IntPtr arg2, IntPtr arg3);

		[DllImport("libzt", EntryPoint="CSharp_zts_getpeername")]
		static extern int zts_getpeername(int arg1, IntPtr arg2, IntPtr arg3);

		[DllImport("libzt", EntryPoint="CSharp_zts_close")]
		static extern int zts_close(int arg1);

		[DllImport("libzt", EntryPoint="CSharp_zts_fcntl")]
		static extern int zts_fcntl(int arg1, int arg2, int arg3);

		[DllImport("libzt", EntryPoint="CSharp_zts_poll")]
		static extern int zts_poll(IntPtr arg1, uint arg2, int arg3);

		[DllImport("libzt", EntryPoint="CSharp_zts_ioctl")]
		static extern int zts_ioctl(int arg1, uint arg2, IntPtr arg3);

		[DllImport("libzt", EntryPoint="CSharp_zts_send")]
		static extern int zts_send(int arg1, IntPtr arg2, uint arg3, int arg4);

		[DllImport("libzt", EntryPoint="CSharp_zts_sendto")]
		static extern int zts_sendto(int arg1, IntPtr arg2, uint arg3, int arg4, IntPtr arg5, ushort arg6);

		[DllImport("libzt", EntryPoint="CSharp_zts_sendmsg")]
		static extern int zts_sendmsg(int arg1, IntPtr arg2, int arg3);

		[DllImport("libzt", EntryPoint="CSharp_zts_recv")]
		static extern int zts_recv(int arg1, IntPtr arg2, uint arg3, int arg4);

		[DllImport("libzt", EntryPoint="CSharp_zts_recvfrom")]
		static extern int zts_recvfrom(int arg1, IntPtr arg2, uint arg3, int arg4, IntPtr arg5, IntPtr arg6);

		[DllImport("libzt", EntryPoint="CSharp_zts_recvmsg")]
		static extern int zts_recvmsg(int arg1, IntPtr arg2, int arg3);

		[DllImport("libzt", EntryPoint="CSharp_zts_read")]
		static extern int zts_read(int arg1, IntPtr arg2, uint arg3);

		[DllImport("libzt", EntryPoint="CSharp_zts_readv")]
		static extern int zts_readv(int arg1, IntPtr arg2, int arg3);

		[DllImport("libzt", EntryPoint="CSharp_zts_write")]
		static extern int zts_write(int arg1, IntPtr arg2, uint arg3);

		[DllImport("libzt", EntryPoint="CSharp_zts_writev")]
		static extern int zts_writev(int arg1, IntPtr arg2, int arg3);

		[DllImport("libzt", EntryPoint="CSharp_zts_shutdown")]
		static extern int zts_shutdown(int arg1, int arg2);

		[DllImport("libzt", EntryPoint="CSharp_zts_add_dns_nameserver")]
		static extern int zts_add_dns_nameserver(IntPtr arg1);

		[DllImport("libzt", EntryPoint="CSharp_zts_del_dns_nameserver")]
		static extern int zts_del_dns_nameserver(IntPtr arg1);

		[DllImport("libzt", EntryPoint="CSharp_zts_htons")]
		static extern ushort zts_htons(ushort arg1);

		[DllImport("libzt", EntryPoint="CSharp_zts_htonl")]
		static extern ushort zts_htonl(ushort arg1);

		[DllImport("libzt", EntryPoint="CSharp_zts_ntohs")]
		static extern ushort zts_ntohs(ushort arg1);

		[DllImport("libzt", EntryPoint="CSharp_zts_ntohl")]
		static extern ushort zts_ntohl(ushort arg1);

		[DllImport("libzt", EntryPoint="CSharp_zts_inet_ntop")]
		static extern string zts_inet_ntop(int arg1, IntPtr arg2, string arg3, ushort arg4);

		[DllImport("libzt", EntryPoint="CSharp_zts_inet_pton")]
		static extern int zts_inet_pton(int arg1, string arg2, IntPtr arg3);

		[DllImport("libzt", EntryPoint="CSharp_zts_inet_addr")]
		static extern ushort zts_inet_addr(string arg1);

		[DllImport("libzt", EntryPoint="CSharp_zts_errno_get")]
		static extern int zts_errno_get();

		/// <value>The value of errno for the low-level socket layer</value>
		public static int ErrNo {
			get {
				return zts_errno_get();
			}
		}

		[StructLayout(LayoutKind.Sequential)]
		struct zts_sockaddr
		{
			public byte sa_len;
			public byte sa_family;
			[MarshalAs(UnmanagedType.ByValArray, SizeConst = 14)]
			public byte[] sa_data;
		}

		[StructLayout(LayoutKind.Sequential)]
		struct zts_in_addr
		{
			public uint s_addr;
		}

		[StructLayout(LayoutKind.Sequential)]
		struct zts_sockaddr_in
		{
			public byte sin_len;
			public byte sin_family;
			public short sin_port;
			[MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
			public byte[] sin_addr;
			[MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
			public char[] sin_zero; // SIN_ZERO_LEN
		}

		[StructLayout(LayoutKind.Sequential)]
		struct zts_pollfd
		{
			public int fd;
			public short events;
			public short revents;
		}
	}
}
