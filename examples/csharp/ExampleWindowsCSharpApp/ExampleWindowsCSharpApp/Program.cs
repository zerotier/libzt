using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

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
            Console.Write("started. now performing a socket call\n");
            int fd = libzt.zts_socket(2, 1, 0);
            Console.Write("fd=" + fd);
            // zts_connect(), zts_bind(), etc...
            libzt.zts_stop();
        }
    }
}
