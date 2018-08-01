using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

using System.Net;

using ZeroTier;

namespace ExampleWindowsCSharpApp
{
    class Program
    {
        static void Main(string[] args)
        {
            ulong nwid = 0xe4da7455b2b9ee6a;

            Console.Write("waiting for libzt to come online...\n");
            libzt.zts_startjoin("config_path", nwid);
            Console.Write("I am " + libzt.zts_get_node_id().ToString("X") +"\n");
            Console.Write("ZT ready\n");

            int fd;
            int err = 0;
            bool clientMode = false;

            if ((fd = libzt.zts_socket(2, 1, 0)) < 0)
            {
                Console.Write("error creating socket\n");
            }

            // CLIENT
            if (clientMode == true)
            {
                string remoteAddrStr = "172.28.221.116";
                Int16 remotePort = 2323;
                // Convert managed address object to pointer for unmanaged code
                libzt.SockAddrIn addr = new libzt.SockAddrIn();
                addr.Family = (byte)libzt.SockAddrFamily.Inet;
                addr.Port = IPAddress.HostToNetworkOrder((Int16)remotePort);
                addr.Addr = (uint)IPAddress.Parse(remoteAddrStr).Address;
                IntPtr addrPtr = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(libzt.SockAddrIn)));
                Marshal.StructureToPtr(addr, addrPtr, false);
                int addrlen = Marshal.SizeOf(addr);

                if ((err = libzt.zts_connect(fd, addrPtr, addrlen)) < 0)
                {
                    Console.Write("error connecting to remote server\n");
                }

                // Send message
                string msgStr = "Hello from C#!";
                byte[] msgBuf = Encoding.ASCII.GetBytes(msgStr);
                int buflen = System.Text.ASCIIEncoding.ASCII.GetByteCount(msgStr);

                if ((err = libzt.zts_write(fd, msgBuf, buflen)) < 0)
                {
                    Console.Write("error writing to remote server\n");
                }

                libzt.zts_close(fd);
                Marshal.FreeHGlobal(addrPtr);
            }
            else // SERVER
            {
                string localAddrStr = "0.0.0.0";
                Int16 bindPort = 5050;
                libzt.SockAddrIn addr = new libzt.SockAddrIn();
                addr.Family = (byte)libzt.SockAddrFamily.Inet;
                addr.Port = IPAddress.HostToNetworkOrder((Int16)bindPort);
                addr.Addr = (uint)IPAddress.Parse(localAddrStr).Address;
                IntPtr addrPtr = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(libzt.SockAddrIn)));
                Marshal.StructureToPtr(addr, addrPtr, false);
                int addrlen = Marshal.SizeOf(addr);

                if ((err = libzt.zts_bind(fd, addrPtr, addrlen)) < 0)
                {
                    Console.Write("error binding to local port\n");
                }
                if ((err = libzt.zts_listen(fd, 1)) < 0)
                {
                    Console.Write("error putting socket in listening state\n");
                }
                int accfd;
                Console.Write("waiting to accept connection...\n");
                if ((accfd = libzt.zts_accept(fd, IntPtr.Zero, IntPtr.Zero)) < 0)
                {
                    Console.Write("error accepting incoming connection\n");
                }
                Console.Write("accepted connection!\n");

                
                // Read message
                byte[] msgBuf = new byte[32];
                int buflen = 32;
                Console.Write("reading from client...\n");
                if ((err = libzt.zts_read(accfd, msgBuf, buflen)) < 0)
                {
                    Console.Write("error reading from remote client\n");
                }
                Console.Write("read " + err + " bytes from client\n");
                string msgStr = System.Text.Encoding.UTF8.GetString(msgBuf, 0, msgBuf.Length);
                Console.Write("msg from client = " + msgStr + "\n");

                libzt.zts_close(fd);
                libzt.zts_close(accfd);
                Marshal.FreeHGlobal(addrPtr);
            }

            Console.Write("fd=" + fd);
            Console.Write("wrote=" + err);
           
            libzt.zts_stop();
        }
    }
}
