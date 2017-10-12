using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using ZeroTier;

namespace libztHelloWorld_csharp
{
    class Program
    {
        static void Main(string[] args)
        {
            Console.Write("waiting for libzt to come online...\n");
            libzt.zts_startjoin("config_path", "17d709436c2c5367");
            Console.Write("started. now performing a socket call\n");
            int fd = libzt.zts_socket(2, 1, 0);
            Console.Write("fd=%d\n", fd);
            // zts_connect(), zts_bind(), etc...
            libzt.zts_stop();
        }
    }
}
