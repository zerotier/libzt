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
        /****************************************************************************/
        /* ZeroTier Service Controls                                                */
        /****************************************************************************/

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_start(string path);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_startjoin(string path, string nwid);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern void zts_stop();

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern void zts_join(string nwid);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern void zts_join_soft(string filepath, string nwid);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern void zts_leave(string nwid);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern void zts_leave_soft(string filepath, string nwid);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern void zts_get_homepath(string homePath, int len);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_get_device_id(string devID);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_running();

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_has_ipv4_address(string nwid);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_has_ipv6_address(string nwid);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_has_address(string nwid);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern void zts_get_ipv4_address(string nwid, string addrstr, int addrlen);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern void zts_get_ipv6_address(string nwid, string addrstr, int addrlen);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern void zts_get_6plane_addr(string addr, string nwid, string devID);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern void zts_get_rfc4193_addr(string addr, string nwid, string devID);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern long zts_get_peer_count();

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_get_peer_address(string peer, string devID);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern void zts_enable_http_control_plane();

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern void zts_disable_http_control_plane();

        /****************************************************************************/
        /* POSIX-like socket API                                                    */
        /****************************************************************************/

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_socket(int socket_family, int socket_type, int protocol);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_connect(int fd, System.IntPtr addr, int addrlen);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_bind(int fd, System.IntPtr addr, int addrlen);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_listen(int fd, int backlog);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_accept(int fd, System.IntPtr addr, IntPtr addrlen);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_setsockopt(int fd, int level, int optname, IntPtr optval, int optlen);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_getsockopt(int fd, int level, int optname, IntPtr optval, IntPtr optlen);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_getsockname(int fd, System.IntPtr addr, IntPtr addrlen);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_getpeername(int fd, System.IntPtr addr, IntPtr addrlen);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_gethostname(string name, int len);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_sethostname(string name, int len);

        //[DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        //unsafe public static extern IntPtr *zts_gethostbyname(string name);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_close(int fd);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_select(int nfds, IntPtr readfds, IntPtr writefds, IntPtr exceptfds, IntPtr timeout);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_fcntl(int fd, int cmd, int flags);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_ioctl(int fd, ulong request, IntPtr argp);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_send(int fd, IntPtr buf, int len, int flags);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_sendto(int fd, IntPtr buf, int len, int flags, System.IntPtr addr, int addrlen);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_sendmsg(int fd, IntPtr msg, int flags);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_recv(int fd, IntPtr buf, int len, int flags);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_recvfrom(int fd, IntPtr buf, int len, int flags, System.IntPtr addr, IntPtr addrlen);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_recvmsg(int fd, IntPtr msg, int flags);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_read(int fd, IntPtr buf, int len);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_write(int fd, IntPtr buf, int len);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_shutdown(int fd, int how);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_add_dns_nameserver(System.IntPtr addr);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_del_dns_nameserver(System.IntPtr addr);
    }
}
