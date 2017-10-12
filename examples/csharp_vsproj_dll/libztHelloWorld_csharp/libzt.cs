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
        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_startjoin(string path, string nwid);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_socket(int socket_family, int socket_type, int protocol);

        [DllImport("libzt.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int zts_stop();
    }
}
