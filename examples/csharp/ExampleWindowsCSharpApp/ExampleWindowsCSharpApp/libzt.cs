using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using System.Runtime.InteropServices;

namespace ZeroTier
{
    static class libzt
    {
        public enum SockAddrFamily
        {
            Inet = 2,
            Inet6 = 10
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct SockAddr
        {
            public ushort Family;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 14)]
            public byte[] Data;
        };

        [StructLayout(LayoutKind.Sequential)]
        public struct SockAddrIn
        {
            byte len; // unique to lwIP
            public byte Family;
            public Int16 Port;
            public uint Addr;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
            public byte[] Zero;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct SockAddrIn6
        {
            public ushort Family;
            public ushort Port;
            public uint FlowInfo;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
            public byte[] Addr;
            public uint ScopeId;
        };

        /****************************************************************************/
        /* ZeroTier Service Controls                                                */
        /****************************************************************************/

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int zts_start(string path);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int zts_startjoin(string path, ulong nwid);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void zts_stop();

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void zts_join(ulong nwid);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void zts_leave(ulong nwid);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void zts_get_homepath(string homePath, int len);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern Int64 zts_get_node_id();

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int zts_running();

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int zts_has_address(ulong nwid);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void zts_get_address(ulong nwid, IntPtr addr, int addrlen);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void zts_get_6plane_addr(IntPtr addr, string nwid, string nodeId);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void zts_get_rfc4193_addr(IntPtr addr, string nwid, string nodeId);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern long zts_get_peer_count();

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int zts_get_peer_address(string peer, ulong nodeId);

        /****************************************************************************/
        /* Socket-like API                                                          */
        /****************************************************************************/
        
        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int zts_socket(int socket_family, int socket_type, int protocol);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int zts_connect(int fd, IntPtr addr, int addrlen);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int zts_bind(int fd, IntPtr addr, int addrlen);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int zts_listen(int fd, int backlog);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int zts_accept(int fd, IntPtr addr, IntPtr addrlen);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int zts_setsockopt(int fd, int level, int optname, IntPtr optval, int optlen);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int zts_getsockopt(int fd, int level, int optname, IntPtr optval, IntPtr optlen);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int zts_getsockname(int fd, IntPtr addr, IntPtr addrlen);

        /*
                [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
                public static extern int zts_getpeername(int fd, System.IntPtr addr, IntPtr addrlen);

                [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
                public static extern int zts_gethostname(string name, int len);

                [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
                public static extern int zts_sethostname(string name, int len);

                [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
                unsafe public static extern IntPtr *zts_gethostbyname(string name);
        */

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int zts_close(int fd);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int zts_select(int nfds, IntPtr readfds, IntPtr writefds, IntPtr exceptfds, IntPtr timeout);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int zts_fcntl(int fd, int cmd, int flags);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int zts_ioctl(int fd, ulong request, IntPtr argp);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int zts_send(int fd, byte[] buf, int len, int flags);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int zts_sendto(int fd, byte[] buf, int len, int flags, IntPtr addr, int addrlen);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int zts_sendmsg(int fd, byte[] msg, int flags);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int zts_recv(int fd, byte[] buf, int len, int flags);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int zts_recvfrom(int fd, byte[] buf, int len, int flags, IntPtr addr, IntPtr addrlen);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int zts_recvmsg(int fd, byte[] msg, int flags);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int zts_read(int fd, byte[] buf, int len);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int zts_write(int fd, byte[] buf, int len);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int zts_shutdown(int fd, int how);

/*
        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int zts_add_dns_nameserver(System.IntPtr addr);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int zts_del_dns_nameserver(System.IntPtr addr);
*/
    }
}
