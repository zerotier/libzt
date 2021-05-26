/*
 * Copyright (c)2013-2021 ZeroTier, Inc.
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file in the project's root directory.
 *
 * Change Date: 2026-01-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2.0 of the Apache License.
 */
/****/

using System;
using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices;

using ZeroTier;

namespace ZeroTier.Sockets
{
    public class Socket {
        /// <summary>No error.</summary>
        public static readonly int ZTS_ERR_OK = 0;
        /// <summary>Socket error, see Socket.ErrNo() for additional context.</summary>
        public static readonly int ZTS_ERR_SOCKET = -1;
        /// <summary>You probably did something at the wrong time.</summary>
        public static readonly int ZTS_ERR_SERVICE = -2;
        /// <summary>Invalid argument.</summary>
        public static readonly int ZTS_ERR_ARG = -3;
        /// <summary>No result. (not necessarily an error.)</summary>
        public static readonly int ZTS_ERR_NO_RESULT = -4;
        /// <summary>Consider filing a bug report.</summary>
        public static readonly int ZTS_ERR_GENERAL = -5;

        int _fd;
        bool _isClosed;
        bool _isListening;
        bool _isBound;
        bool _isConnected;

        int _connectTimeout = 30000;

        AddressFamily _socketFamily;
        SocketType _socketType;
        ProtocolType _socketProtocol;

        internal EndPoint _localEndPoint;
        internal EndPoint _remoteEndPoint;

        private void InitializeInternalFlags()
        {
            _isClosed = false;
            _isListening = false;
        }

        public Socket(AddressFamily addressFamily, SocketType socketType, ProtocolType protocolType)
        {
            int family = -1;
            int type = -1;
            int protocol = -1;
            // Map .NET socket parameters to ZeroTier equivalents
            switch (addressFamily) {
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
            switch (socketType) {
                case SocketType.Stream:
                    type = Constants.SOCK_STREAM;
                    break;
                case SocketType.Dgram:
                    type = Constants.SOCK_DGRAM;
                    break;
            }
            switch (protocolType) {
                case ProtocolType.Udp:
                    protocol = Constants.IPPROTO_UDP;
                    break;
                case ProtocolType.Tcp:
                    protocol = Constants.IPPROTO_TCP;
                    break;
                case ProtocolType.Unspecified:
                    protocol = 0;   // ?
                    break;
            }
            if ((_fd = zts_bsd_socket(family, type, protocol)) < 0) {
                throw new ZeroTier.Sockets.SocketException((int)_fd);
            }
            _socketFamily = addressFamily;
            _socketType = socketType;
            _socketProtocol = protocolType;
            InitializeInternalFlags();
        }

        private Socket(
            int fileDescriptor,
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
                throw new ZeroTier.Sockets.SocketException((int)Constants.ERR_SOCKET);
            }
            if (remoteEndPoint == null) {
                throw new ArgumentNullException("remoteEndPoint");
            }
            int err = zts_connect(_fd, remoteEndPoint.Address.ToString(), (ushort)remoteEndPoint.Port, _connectTimeout);
            if (err < 0) {
                throw new ZeroTier.Sockets.SocketException(err, ZeroTier.Core.Node.ErrNo);
            }
            _remoteEndPoint = remoteEndPoint;
            _isConnected = true;
        }

        public void Bind(IPEndPoint localEndPoint)
        {
            if (_isClosed) {
                throw new ObjectDisposedException("Socket has been closed");
            }
            if (_fd < 0) {
                // Invalid file descriptor
                throw new ZeroTier.Sockets.SocketException((int)Constants.ERR_SOCKET);
            }
            if (localEndPoint == null) {
                throw new ArgumentNullException("localEndPoint");
            }
            int err = Constants.ERR_OK;
            if (localEndPoint.AddressFamily == AddressFamily.InterNetwork) {
                err = zts_bind(_fd, "0.0.0.0", (ushort)localEndPoint.Port);
            }
            if (localEndPoint.AddressFamily == AddressFamily.InterNetworkV6) {
                // Todo: detect IPAddress.IPv6Any
                err = zts_bind(_fd, "::", (ushort)localEndPoint.Port);
            }
            if (err < 0) {
                throw new ZeroTier.Sockets.SocketException((int)err);
            }
            _localEndPoint = localEndPoint;
            _isBound = true;
        }

        public void Listen(int backlog)
        {
            if (_isClosed) {
                throw new ObjectDisposedException("Socket has been closed");
            }
            if (_fd < 0) {
                // Invalid file descriptor
                throw new ZeroTier.Sockets.SocketException((int)Constants.ERR_SOCKET);
            }
            int err = Constants.ERR_OK;
            if ((err = zts_bsd_listen(_fd, backlog)) < 0) {
                // Invalid backlog value perhaps?
                throw new ZeroTier.Sockets.SocketException((int)Constants.ERR_SOCKET);
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
                throw new ZeroTier.Sockets.SocketException((int)Constants.ERR_SOCKET);
            }
            if (_isListening == false) {
                throw new InvalidOperationException("Socket is not in a listening state. Call Listen() first");
            }
            IntPtr lpBuffer = Marshal.AllocHGlobal(ZeroTier.Constants.INET6_ADDRSTRLEN);
            int port = 0;
            int accepted_fd = zts_accept(_fd, lpBuffer, ZeroTier.Constants.INET6_ADDRSTRLEN, ref port);
            // Convert buffer to managed string
            string str = Marshal.PtrToStringAnsi(lpBuffer);
            Marshal.FreeHGlobal(lpBuffer);
            lpBuffer = IntPtr.Zero;
            IPEndPoint clientEndPoint = new IPEndPoint(IPAddress.Parse(str), port);
            // Create new socket by providing file descriptor returned from zts_bsd_accept call.
            Socket clientSocket =
                new Socket(accepted_fd, _socketFamily, _socketType, _socketProtocol, _localEndPoint, clientEndPoint);
            return clientSocket;
        }

        public void Shutdown(SocketShutdown how)
        {
            if (_isClosed) {
                throw new ObjectDisposedException("Socket has been closed");
            }
            int ztHow = 0;
            switch (how) {
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
            zts_bsd_shutdown(_fd, ztHow);
        }

        public void Close()
        {
            Close(0);
        }

        public void Close(int timeout)
        {
            // TODO: Timeout needs to be implemented
            if (_isClosed) {
                throw new ObjectDisposedException("Socket has already been closed");
            }
            zts_bsd_close(_fd);
            _isClosed = true;
        }

        public bool Blocking
        {
            get {
                return Convert.ToBoolean(zts_get_blocking(_fd));
            }
            set {
                zts_set_blocking(_fd, Convert.ToInt32(value));
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
            if ((result = zts_bsd_poll(poll_fd_ptr, numfds, timeout_ms)) < 0) {
                throw new ZeroTier.Sockets.SocketException(result, ZeroTier.Core.Node.ErrNo);
            }
            poll_set = (zts_pollfd)Marshal.PtrToStructure(poll_fd_ptr, typeof(zts_pollfd));
            if (result != 0) {
                if (mode == SelectMode.SelectRead) {
                    result = Convert.ToInt32(((byte)poll_set.revents & (byte)ZeroTier.Constants.POLLIN) != 0);
                }
                if (mode == SelectMode.SelectWrite) {
                    result = Convert.ToInt32(((byte)poll_set.revents & (byte)ZeroTier.Constants.POLLOUT) != 0);
                }
                if (mode == SelectMode.SelectError) {
                    result = Convert.ToInt32(
                        ((poll_set.revents & (byte)ZeroTier.Constants.POLLERR) != 0)
                        || ((poll_set.revents & (byte)ZeroTier.Constants.POLLNVAL) != 0));
                }
            }
            Marshal.FreeHGlobal(poll_fd_ptr);
            return result > 0;
        }

        public Int32 Send(Byte[] buffer)
        {
            return Send(buffer, 0, buffer != null ? buffer.Length : 0, SocketFlags.None);
        }

        public Int32 Send(Byte[] buffer, int offset, int size, SocketFlags socketFlags)
        {
            if (_isClosed) {
                throw new ObjectDisposedException("Socket has been closed");
            }
            if (_fd < 0) {
                throw new ZeroTier.Sockets.SocketException((int)ZeroTier.Constants.ERR_SOCKET);
            }
            if (buffer == null) {
                throw new ArgumentNullException("buffer");
            }
            if (size < 0 || size > buffer.Length - offset) {
                throw new ArgumentOutOfRangeException("size");
            }
            if (offset < 0 || offset > buffer.Length) {
                throw new ArgumentOutOfRangeException("offset");
            }
            int flags = 0;
            IntPtr bufferPtr = Marshal.UnsafeAddrOfPinnedArrayElement(buffer, 0);
            return zts_bsd_send(_fd, bufferPtr + offset, (uint)Buffer.ByteLength(buffer), (int)flags);
        }

        public int Available
        {
            get {
                return zts_get_data_available(_fd);
            }
        }

        public Int32 Receive(Byte[] buffer)
        {
            return Receive(buffer, 0, buffer != null ? buffer.Length : 0, SocketFlags.None);
        }

        public Int32 Receive(byte[] buffer, int offset, int size, SocketFlags socketFlags)
        {
            if (_isClosed) {
                throw new ObjectDisposedException("Socket has been closed");
            }
            if (_fd < 0) {
                throw new ZeroTier.Sockets.SocketException((int)ZeroTier.Constants.ERR_SOCKET);
            }
            if (buffer == null) {
                throw new ArgumentNullException("buffer");
            }
            if (size < 0 || size > buffer.Length - offset) {
                throw new ArgumentOutOfRangeException("size");
            }
            if (offset < 0 || offset > buffer.Length) {
                throw new ArgumentOutOfRangeException("offset");
            }
            int flags = 0;
            IntPtr bufferPtr = Marshal.UnsafeAddrOfPinnedArrayElement(buffer, 0);
            return zts_bsd_recv(_fd, bufferPtr + offset, (uint)Buffer.ByteLength(buffer), (int)flags);
        }

        public int ReceiveTimeout
        {
            get {
                return zts_get_recv_timeout(_fd);
            }
            // TODO: microseconds
            set {
                zts_set_recv_timeout(_fd, value, 0);
            }
        }

        public int SendTimeout
        {
            get {
                return zts_get_send_timeout(_fd);
            }
            // TODO: microseconds
            set {
                zts_set_send_timeout(_fd, value, 0);
            }
        }

        public int ConnectTimeout
        {
            get {
                return _connectTimeout;
            }
            set {
                _connectTimeout = value;
            }
        }

        public int ReceiveBufferSize
        {
            get {
                return zts_get_recv_buf_size(_fd);
            }
            set {
                zts_set_recv_buf_size(_fd, value);
            }
        }

        public int SendBufferSize
        {
            get {
                return zts_get_send_buf_size(_fd);
            }
            set {
                zts_set_send_buf_size(_fd, value);
            }
        }

        public short Ttl
        {
            get {
                return Convert.ToInt16(zts_get_ttl(_fd));
            }
            set {
                zts_set_ttl(_fd, value);
            }
        }

        public LingerOption LingerState
        {
            get {
                LingerOption lo =
                    new LingerOption(Convert.ToBoolean(zts_get_linger_enabled(_fd)), zts_get_linger_value(_fd));
                return lo;
            }
            set {
                zts_set_linger(_fd, Convert.ToInt32(value.Enabled), value.LingerTime);
            }
        }

        public bool NoDelay
        {
            get {
                return Convert.ToBoolean(zts_get_no_delay(_fd));
            }
            set {
                zts_set_no_delay(_fd, Convert.ToInt32(value));
            }
        }

        public bool KeepAlive
        {
            get {
                return Convert.ToBoolean(zts_get_keepalive(_fd));
            }
            set {
                zts_set_keepalive(_fd, Convert.ToInt32(value));
            }
        }

        public bool Connected
        {
            get {
                return _isConnected;
            }
        }

        public bool IsBound
        {
            get {
                return _isBound;
            }
        }

        public AddressFamily AddressFamily
        {
            get {
                return _socketFamily;
            }
        }

        public SocketType SocketType
        {
            get {
                return _socketType;
            }
        }

        public ProtocolType ProtocolType
        {
            get {
                return _socketProtocol;
            }
        }

        /* .NET has moved to OSSupportsIPv* but libzt isn't an OS so we keep this old convention */
        public static bool SupportsIPv4
        {
            get {
                return true;
            }
        }

        /* .NET has moved to OSSupportsIPv* but libzt isn't an OS so we keep this old convention */
        public static bool SupportsIPv6
        {
            get {
                return true;
            }
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

        /* Structures and functions used internally to communicate with
        lower-level C API defined in include/ZeroTierSockets.h */

        [DllImport(
            "libzt",
            CharSet = CharSet.Ansi,
            EntryPoint = "CSharp_zts_bsd_gethostbyname")] public static extern global::System.IntPtr
        zts_bsd_gethostbyname(string jarg1);

        [DllImport("libzt", EntryPoint = "CSharp_zts_bsd_select")]
        static extern int zts_bsd_select(int jarg1, IntPtr jarg2, IntPtr jarg3, IntPtr jarg4, IntPtr jarg5);

        [DllImport("libzt", EntryPoint = "CSharp_zts_get_all_stats")]
        static extern int zts_get_all_stats(IntPtr arg1);

        [DllImport("libzt", EntryPoint = "CSharp_zts_get_protocol_stats")]
        static extern int zts_get_protocol_stats(int arg1, IntPtr arg2);

        [DllImport("libzt", EntryPoint = "CSharp_zts_bsd_socket")]
        static extern int zts_bsd_socket(int arg1, int arg2, int arg3);

        [DllImport("libzt", EntryPoint = "CSharp_zts_bsd_connect")]
        static extern int zts_bsd_connect(int arg1, IntPtr arg2, ushort arg3);

        [DllImport("libzt", CharSet = CharSet.Ansi, EntryPoint = "CSharp_zts_bsd_connect_easy")]
        static extern int zts_bsd_connect_easy(int arg1, int arg2, string arg3, ushort arg4, int arg5);

        [DllImport("libzt", EntryPoint = "CSharp_zts_bsd_bind")]
        static extern int zts_bsd_bind(int arg1, IntPtr arg2, ushort arg3);

        [DllImport("libzt", CharSet = CharSet.Ansi, EntryPoint = "CSharp_zts_bsd_bind_easy")]
        static extern int zts_bsd_bind_easy(int arg1, int arg2, string arg3, ushort arg4);

        [DllImport("libzt", EntryPoint = "CSharp_zts_bsd_listen")]
        static extern int zts_bsd_listen(int arg1, int arg2);

        [DllImport("libzt", EntryPoint = "CSharp_zts_bsd_accept")]
        static extern int zts_bsd_accept(int arg1, IntPtr arg2, IntPtr arg3);

        [DllImport("libzt", CharSet = CharSet.Ansi, EntryPoint = "CSharp_zts_bsd_accept_easy")]
        static extern int zts_bsd_accept_easy(int arg1, IntPtr remoteAddrStr, int arg2, ref int arg3);

        [DllImport("libzt", EntryPoint = "CSharp_zts_bsd_setsockopt")]
        static extern int zts_bsd_setsockopt(int arg1, int arg2, int arg3, IntPtr arg4, ushort arg5);

        [DllImport("libzt", EntryPoint = "CSharp_zts_bsd_getsockopt")]
        static extern int zts_bsd_getsockopt(int arg1, int arg2, int arg3, IntPtr arg4, IntPtr arg5);

        [DllImport("libzt", EntryPoint = "CSharp_zts_bsd_getsockname")]
        static extern int zts_bsd_getsockname(int arg1, IntPtr arg2, IntPtr arg3);

        [DllImport("libzt", EntryPoint = "CSharp_zts_bsd_getpeername")]
        static extern int zts_bsd_getpeername(int arg1, IntPtr arg2, IntPtr arg3);

        [DllImport("libzt", EntryPoint = "CSharp_zts_bsd_close")]
        static extern int zts_bsd_close(int arg1);

        [DllImport("libzt", EntryPoint = "CSharp_zts_bsd_fcntl")]
        static extern int zts_bsd_fcntl(int arg1, int arg2, int arg3);

        [DllImport("libzt", EntryPoint = "CSharp_zts_bsd_poll")]
        static extern int zts_bsd_poll(IntPtr arg1, uint arg2, int arg3);

        [DllImport("libzt", EntryPoint = "CSharp_zts_bsd_ioctl")]
        static extern int zts_bsd_ioctl(int arg1, uint arg2, IntPtr arg3);

        [DllImport("libzt", EntryPoint = "CSharp_zts_bsd_send")]
        static extern int zts_bsd_send(int arg1, IntPtr arg2, uint arg3, int arg4);

        [DllImport("libzt", EntryPoint = "CSharp_zts_bsd_sendto")]
        static extern int zts_bsd_sendto(int arg1, IntPtr arg2, uint arg3, int arg4, IntPtr arg5, ushort arg6);

        [DllImport("libzt", EntryPoint = "CSharp_zts_bsd_sendmsg")]
        static extern int zts_bsd_sendmsg(int arg1, IntPtr arg2, int arg3);

        [DllImport("libzt", EntryPoint = "CSharp_zts_bsd_recv")]
        static extern int zts_bsd_recv(int arg1, IntPtr arg2, uint arg3, int arg4);

        [DllImport("libzt", EntryPoint = "CSharp_zts_bsd_recvfrom")]
        static extern int zts_bsd_recvfrom(int arg1, IntPtr arg2, uint arg3, int arg4, IntPtr arg5, IntPtr arg6);

        [DllImport("libzt", EntryPoint = "CSharp_zts_bsd_recvmsg")]
        static extern int zts_bsd_recvmsg(int arg1, IntPtr arg2, int arg3);

        [DllImport("libzt", EntryPoint = "CSharp_zts_bsd_read")]
        static extern int zts_bsd_read(int arg1, IntPtr arg2, uint arg3);

        [DllImport("libzt", EntryPoint = "CSharp_zts_bsd_readv")]
        static extern int zts_bsd_readv(int arg1, IntPtr arg2, int arg3);

        [DllImport("libzt", EntryPoint = "CSharp_zts_bsd_write")]
        static extern int zts_bsd_write(int arg1, IntPtr arg2, uint arg3);

        [DllImport("libzt", EntryPoint = "CSharp_zts_bsd_writev")]
        static extern int zts_bsd_writev(int arg1, IntPtr arg2, int arg3);

        [DllImport("libzt", EntryPoint = "CSharp_zts_bsd_shutdown")]
        static extern int zts_bsd_shutdown(int arg1, int arg2);

        [DllImport("libzt", EntryPoint = "CSharp_zts_get_data_available")]
        static extern int zts_get_data_available(int fd);

        [DllImport("libzt", EntryPoint = "CSharp_zts_set_no_delay")]
        static extern int zts_set_no_delay(int fd, int enabled);

        [DllImport("libzt", EntryPoint = "CSharp_zts_get_no_delay")]
        static extern int zts_get_no_delay(int fd);

        [DllImport("libzt", EntryPoint = "CSharp_zts_set_linger")]
        static extern int zts_set_linger(int fd, int enabled, int value);

        [DllImport("libzt", EntryPoint = "CSharp_zts_get_linger_enabled")]
        static extern int zts_get_linger_enabled(int fd);

        [DllImport("libzt", EntryPoint = "CSharp_zts_get_linger_value")]
        static extern int zts_get_linger_value(int fd);

        [DllImport("libzt", EntryPoint = "CSharp_zts_set_reuse_addr")]
        static extern int zts_set_reuse_addr(int fd, int enabled);

        [DllImport("libzt", EntryPoint = "CSharp_zts_get_reuse_addr")]
        static extern int zts_get_reuse_addr(int fd);

        [DllImport("libzt", EntryPoint = "CSharp_zts_set_recv_timeout")]
        static extern int zts_set_recv_timeout(int fd, int seconds, int microseconds);

        [DllImport("libzt", EntryPoint = "CSharp_zts_get_recv_timeout")]
        static extern int zts_get_recv_timeout(int fd);

        [DllImport("libzt", EntryPoint = "CSharp_zts_set_send_timeout")]
        static extern int zts_set_send_timeout(int fd, int seconds, int microseconds);

        [DllImport("libzt", EntryPoint = "CSharp_zts_get_send_timeout")]
        static extern int zts_get_send_timeout(int fd);

        [DllImport("libzt", EntryPoint = "CSharp_zts_set_send_buf_size")]
        static extern int zts_set_send_buf_size(int fd, int size);

        [DllImport("libzt", EntryPoint = "CSharp_zts_get_send_buf_size")]
        static extern int zts_get_send_buf_size(int fd);

        [DllImport("libzt", EntryPoint = "CSharp_zts_set_recv_buf_size")]
        static extern int zts_set_recv_buf_size(int fd, int size);

        [DllImport("libzt", EntryPoint = "CSharp_zts_get_recv_buf_size")]
        static extern int zts_get_recv_buf_size(int fd);

        [DllImport("libzt", EntryPoint = "CSharp_zts_set_ttl")]
        static extern int zts_set_ttl(int fd, int ttl);

        [DllImport("libzt", EntryPoint = "CSharp_zts_get_ttl")]
        static extern int zts_get_ttl(int fd);

        [DllImport("libzt", EntryPoint = "CSharp_zts_set_blocking")]
        static extern int zts_set_blocking(int fd, int enabled);

        [DllImport("libzt", EntryPoint = "CSharp_zts_get_blocking")]
        static extern int zts_get_blocking(int fd);

        [DllImport("libzt", EntryPoint = "CSharp_zts_set_keepalive")]
        static extern int zts_set_keepalive(int fd, int enabled);

        [DllImport("libzt", EntryPoint = "CSharp_zts_get_keepalive")]
        static extern int zts_get_keepalive(int fd);

        [DllImport("libzt", EntryPoint = "CSharp_zts_add_dns_nameserver")]
        static extern int zts_add_dns_nameserver(IntPtr arg1);

        [DllImport("libzt", EntryPoint = "CSharp_zts_del_dns_nameserver")]
        static extern int zts_del_dns_nameserver(IntPtr arg1);

        [DllImport("libzt", EntryPoint = "CSharp_zts_errno_get")]
        static extern int zts_errno_get();

        [DllImport("libzt", CharSet = CharSet.Ansi, EntryPoint = "CSharp_zts_accept")]
        static extern int zts_accept(int jarg1, IntPtr jarg2, int jarg3, ref int jarg4);

        [DllImport("libzt", CharSet = CharSet.Ansi, EntryPoint = "CSharp_zts_tcp_client")]
        static extern int zts_tcp_client(string jarg1, int jarg2);

        [DllImport("libzt", CharSet = CharSet.Ansi, EntryPoint = "CSharp_zts_tcp_server")]
        static extern int zts_tcp_server(string jarg1, int jarg2, string jarg3, int jarg4, IntPtr jarg5);

        [DllImport("libzt", CharSet = CharSet.Ansi, EntryPoint = "CSharp_zts_udp_server")]
        static extern int zts_udp_server(string jarg1, int jarg2);

        [DllImport("libzt", CharSet = CharSet.Ansi, EntryPoint = "CSharp_zts_udp_client")]
        static extern int zts_udp_client(string jarg1);

        [DllImport("libzt", CharSet = CharSet.Ansi, EntryPoint = "CSharp_zts_bind")]
        static extern int zts_bind(int jarg1, string jarg2, int jarg3);

        [DllImport("libzt", CharSet = CharSet.Ansi, EntryPoint = "CSharp_zts_connect")]
        static extern int zts_connect(int jarg1, string jarg2, int jarg3, int jarg4);

        [DllImport("libzt", EntryPoint = "CSharp_zts_stats_get_all")]
        static extern int zts_stats_get_all(IntPtr jarg1);

        /*
                [DllImport("libzt", EntryPoint = "CSharp_zts_set_no_delay")]
                static extern int zts_set_no_delay(int jarg1, int jarg2);

                [DllImport("libzt", EntryPoint = "CSharp_zts_get_no_delay")]
                static extern int zts_get_no_delay(int jarg1);

                [DllImport("libzt", EntryPoint = "CSharp_zts_set_linger")]
                static extern int zts_set_linger(int jarg1, int jarg2, int jarg3);

                [DllImport("libzt", EntryPoint = "CSharp_zts_get_linger_enabled")]
                static extern int zts_get_linger_enabled(int jarg1);

                [DllImport("libzt", EntryPoint = "CSharp_zts_get_linger_value")]
                static extern int zts_get_linger_value(int jarg1);

                [DllImport("libzt", EntryPoint = "CSharp_zts_set_reuse_addr")]
                static extern int zts_set_reuse_addr(int jarg1, int jarg2);

                [DllImport("libzt", EntryPoint = "CSharp_zts_get_reuse_addr")]
                static extern int zts_get_reuse_addr(int jarg1);

                [DllImport("libzt", EntryPoint = "CSharp_zts_set_recv_timeout")]
                static extern int zts_set_recv_timeout(int jarg1, int jarg2, int jarg3);

                [DllImport("libzt", EntryPoint = "CSharp_zts_get_recv_timeout")]
                static extern int zts_get_recv_timeout(int jarg1);

                [DllImport("libzt", EntryPoint = "CSharp_zts_set_send_timeout")]
                static extern int zts_set_send_timeout(int jarg1, int jarg2, int jarg3);

                [DllImport("libzt", EntryPoint = "CSharp_zts_get_send_timeout")]
                static extern int zts_get_send_timeout(int jarg1);

                [DllImport("libzt", EntryPoint = "CSharp_zts_set_send_buf_size")]
                static extern int zts_set_send_buf_size(int jarg1, int jarg2);

                [DllImport("libzt", EntryPoint = "CSharp_zts_get_send_buf_size")]
                static extern int zts_get_send_buf_size(int jarg1);

                [DllImport("libzt", EntryPoint = "CSharp_zts_set_recv_buf_size")]
                static extern int zts_set_recv_buf_size(int jarg1, int jarg2);

                [DllImport("libzt", EntryPoint = "CSharp_zts_get_recv_buf_size")]
                static extern int zts_get_recv_buf_size(int jarg1);

                [DllImport("libzt", EntryPoint = "CSharp_zts_set_ttl")]
                static extern int zts_set_ttl(int jarg1, int jarg2);

                [DllImport("libzt", EntryPoint = "CSharp_zts_get_ttl")]
                static extern int zts_get_ttl(int jarg1);

                [DllImport("libzt", EntryPoint = "CSharp_zts_set_blocking")]
                static extern int zts_set_blocking(int jarg1, int jarg2);

                [DllImport("libzt", EntryPoint = "CSharp_zts_get_blocking")]
                static extern int zts_get_blocking(int jarg1);

                [DllImport("libzt", EntryPoint = "CSharp_zts_set_keepalive")]
                static extern int zts_set_keepalive(int jarg1, int jarg2);

                [DllImport("libzt", EntryPoint = "CSharp_zts_get_keepalive")]
                static extern int zts_get_keepalive(int jarg1);



        */

        [DllImport("libzt", EntryPoint = "CSharp_zts_util_delay")]
        public static extern void zts_util_delay(int jarg1);

        [DllImport("libzt", CharSet = CharSet.Ansi, EntryPoint = "CSharp_zts_util_get_ip_family")]
        static extern int zts_util_get_ip_family(string jarg1);

        [DllImport("libzt", CharSet = CharSet.Ansi, EntryPoint = "CSharp_zts_util_ipstr_to_saddr")]
        static extern int zts_util_ipstr_to_saddr(string jarg1, int jarg2, IntPtr jarg3, IntPtr jarg4);

        /// <value>The value of errno for the low-level socket layer</value>
        public static int ErrNo
        {
            get {
                return zts_errno_get();
            }
        }

        [StructLayout(LayoutKind.Sequential)] struct zts_sockaddr {
            public byte sa_len;
            public byte sa_family;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 14)]
            public byte[] sa_data;
        }

        [StructLayout(LayoutKind.Sequential)]
        struct zts_in_addr {
            public uint s_addr;
        }

        [StructLayout(LayoutKind.Sequential)]
        struct zts_sockaddr_in {
            public byte sin_len;
            public byte sin_family;
            public short sin_port;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
            public byte[] sin_addr;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
            public char[] sin_zero;   // SIN_ZERO_LEN
        }

        [StructLayout(LayoutKind.Sequential)]
        struct zts_pollfd {
            public int fd;
            public short events;
            public short revents;
        }

        [StructLayout(LayoutKind.Sequential)]
        struct zts_timeval {
            public long tv_sec;
            public long tv_usec;
        }
    }
}
