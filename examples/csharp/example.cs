
using System;
using System.Threading;
using System.Net;
using System.Net.Sockets;
using System.Text;

using ZeroTier;

public class ExampleApp {
    // ZeroTier Node instance

    ZeroTier.Core.Node node;

    // Initialize and start ZeroTier

    public void StartZeroTier(string configFilePath, ulong networkId)
    {
        node = new ZeroTier.Core.Node();

        // (OPTIONAL) Initialize node

        node.InitFromStorage(configFilePath);
        node.InitAllowNetworkCaching(false);
        node.InitAllowPeerCaching(true);
        // node.InitAllowIdentityCaching(true);
        // node.InitAllowWorldCaching(false);
        node.InitSetEventHandler(OnZeroTierEvent);
        // node.InitSetPort(0);   // Will randomly attempt ports if not specified or is set to 0
        node.InitSetRandomPortRange(40000, 50000);
        // node.InitAllowSecondaryPort(false);

        // (OPTIONAL) Set custom signed roots

        // In this case we only allow ZeroTier to contact our Amsterdam root server
        // To see examples of how to generate and sign roots definitions see docs.zerotier.com

        /*
        var rootsData = new byte[] {
            0x01, 0x00, 0x00, 0x00, 0x00, 0x08, 0xea, 0xc9, 0x0a, 0x00, 0x00, 0x01, 0x6c, 0xe3, 0xe2, 0x39, 0x55, 0x74,
            0xeb, 0x27, 0x9d, 0xc9, 0xe7, 0x5a, 0x52, 0xbb, 0x91, 0x8f, 0xf7, 0x43, 0x3c, 0xbf, 0x77, 0x5a, 0x4b, 0x57,
            0xb4, 0xe1, 0xe9, 0xa1, 0x01, 0x61, 0x3d, 0x25, 0x35, 0x60, 0xcb, 0xe3, 0x30, 0x18, 0x1e, 0x6e, 0x44, 0xef,
            0x93, 0x89, 0xa0, 0x19, 0xb8, 0x7b, 0x36, 0x0b, 0x92, 0xff, 0x0f, 0x1b, 0xbe, 0x56, 0x5a, 0x46, 0x91, 0x36,
            0xf1, 0xd4, 0x5c, 0x09, 0x05, 0xe5, 0xf5, 0xfb, 0xba, 0xe8, 0x13, 0x2d, 0x47, 0xa8, 0xe4, 0x1b, 0xa5, 0x1c,
            0xcf, 0xb0, 0x2f, 0x27, 0x7e, 0x95, 0xa0, 0xdd, 0x49, 0xe1, 0x7d, 0xc0, 0x7e, 0x6d, 0xe3, 0x25, 0x91, 0x96,
            0xc2, 0x55, 0xf9, 0x20, 0x6d, 0x2a, 0x5e, 0x1b, 0x41, 0xcb, 0x1f, 0x8d, 0x57, 0x27, 0x69, 0x3e, 0xcc, 0x7f,
            0x0b, 0x36, 0x54, 0x6b, 0xd3, 0x80, 0x78, 0xf6, 0xd0, 0xec, 0xb4, 0x31, 0x6b, 0x87, 0x1b, 0x50, 0x08, 0xe4,
            0x0b, 0xa9, 0xd4, 0xfd, 0x37, 0x79, 0x14, 0x6a, 0xf5, 0x12, 0xf2, 0x45, 0x39, 0xca, 0x23, 0x00, 0x39, 0xbc,
            0xa3, 0x1e, 0xa8, 0x4e, 0x23, 0x2d, 0xc8, 0xdb, 0x9b, 0x0e, 0x52, 0x1b, 0x8d, 0x02, 0x72, 0x01, 0x99, 0x2f,
            0xcf, 0x1d, 0xb7, 0x00, 0x20, 0x6e, 0xd5, 0x93, 0x50, 0xb3, 0x19, 0x16, 0xf7, 0x49, 0xa1, 0xf8, 0x5d, 0xff,
            0xb3, 0xa8, 0x78, 0x7d, 0xcb, 0xf8, 0x3b, 0x8c, 0x6e, 0x94, 0x48, 0xd4, 0xe3, 0xea, 0x0e, 0x33, 0x69, 0x30,
            0x1b, 0xe7, 0x16, 0xc3, 0x60, 0x93, 0x44, 0xa9, 0xd1, 0x53, 0x38, 0x50, 0xfb, 0x44, 0x60, 0xc5, 0x0a, 0xf4,
            0x33, 0x22, 0xbc, 0xfc, 0x8e, 0x13, 0xd3, 0x30, 0x1a, 0x1f, 0x10, 0x03, 0xce, 0xb6, 0x00, 0x02, 0x04, 0xc3,
            0xb5, 0xad, 0x9f, 0x27, 0x09, 0x06, 0x2a, 0x02, 0x6e, 0xa0, 0xc0, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x27, 0x09
        };
        node.InitSetRoots(rootsData, rootsData.Length);
        */

        node.Start();   // Network activity only begins after calling Start()
        while (! node.Online) {
            Thread.Sleep(50);
        }

        Console.WriteLine("Id            : " + node.IdString);
        Console.WriteLine("Version       : " + node.Version);
        Console.WriteLine("PrimaryPort   : " + node.PrimaryPort);
        Console.WriteLine("SecondaryPort : " + node.SecondaryPort);
        Console.WriteLine("TertiaryPort  : " + node.TertiaryPort);

        node.Join(networkId);
        Console.WriteLine("Waiting for join to complete...");
        while (node.Networks.Count == 0) {
            Thread.Sleep(50);
        }

        // Wait until we've joined the network and we have routes + addresses
        Console.WriteLine("Waiting for network to become transport ready...");
        while (! node.IsNetworkTransportReady(networkId)) {
            Thread.Sleep(50);
        }

        Console.WriteLine("Num of assigned addresses : " + node.GetNetworkAddresses(networkId).Count);
        foreach (IPAddress addr in node.GetNetworkAddresses(networkId)) {
            Console.WriteLine(" - Address: " + addr);
        }

        Console.WriteLine("Num of routes             : " + node.GetNetworkRoutes(networkId).Count);
        foreach (ZeroTier.Core.RouteInfo route in node.GetNetworkRoutes(networkId)) {
            Console.WriteLine(
                " -   Route: target={0} via={1} flags={2} metric={3}",
                route.Target.ToString(),
                route.Via.ToString(),
                route.Flags,
                route.Metric);
        }
    }

    /**
     * Stop ZeroTier
     */
    public void StopZeroTier()
    {
        node.Free();
    }

    /**
     * (OPTIONAL)
     *
     * Your application should process event messages and return control as soon as possible. Blocking
     * or otherwise time-consuming operations are not recommended here.
     */
    public void OnZeroTierEvent(ZeroTier.Core.Event e)
    {
        Console.WriteLine("Event.Code = {0} ({1})", e.Code, e.Name);
        /*
        if (e.Code == ZeroTier.Constants.EVENT_NODE_ONLINE) {
            Console.WriteLine("Node is online");
            Console.WriteLine(" - Address (NodeId): " + node.Id.ToString("x16"));
        }

        if (e.Code == ZeroTier.Constants.EVENT_NETWORK_OK) {
            Console.WriteLine(" - Network ID: " + e.NetworkInfo.Id.ToString("x16"));
        }
        */
    }

    /**
     * Example server
     */
    public void SocketServer(IPEndPoint localEndPoint)
    {
        string data = null;

        // Data buffer for incoming data.
        byte[] bytes = new Byte[1024];

        Console.WriteLine(localEndPoint.ToString());
        ZeroTier.Sockets.Socket listener =
            new ZeroTier.Sockets.Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);

        // Bind the socket to the local endpoint and
        // listen for incoming connections.

        try {
            listener.Bind(localEndPoint);
            listener.Listen(10);
            ZeroTier.Sockets.Socket handler;
            Console.WriteLine("Server: Accepting...");
            handler = listener.Accept();

            data = null;
            Console.WriteLine("Server: Accepted connection from: " + handler.RemoteEndPoint.ToString());

            for (int i = 0; i < 4; i++) {
                int bytesRec = 0;
                try {
                    Console.WriteLine("Server: Receiving...");
                    bytesRec = handler.Receive(bytes);
                }
                catch (ZeroTier.Sockets.SocketException e) {
                    Console.WriteLine(
                        "ServiceErrorCode={0} SocketErrorCode={1}",
                        e.ServiceErrorCode,
                        e.SocketErrorCode);
                }
                if (bytesRec > 0) {
                    Console.WriteLine("Server: Bytes received: {0}", bytesRec);
                    data = Encoding.ASCII.GetString(bytes, 0, bytesRec);
                    Console.WriteLine("Server: Text received : {0}", data);
                    Thread.Sleep(1000);
                    // Echo the data back to the client.
                    byte[] msg = Encoding.ASCII.GetBytes(data);
                    handler.Send(msg);
                    Thread.Sleep(1000);
                }
            }
            // Release the socket.
            handler.Shutdown(SocketShutdown.Both);
            handler.Close();
        }
        catch (ZeroTier.Sockets.SocketException e) {
            Console.WriteLine(e);
            Console.WriteLine("ServiceErrorCode={0} SocketErrorCode={1}", e.ServiceErrorCode, e.SocketErrorCode);
        }
    }

    /**
     * Example client
     */
    public void SocketClient(IPEndPoint remoteServerEndPoint)
    {
        // Data buffer for incoming data.
        byte[] bytes = new byte[1024];

        // Connect to a remote device.
        try {
            // Create a TCP/IP  socket.
            ZeroTier.Sockets.Socket sender =
                new ZeroTier.Sockets.Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);

            try {
                Console.WriteLine("Client: Connecting to {0}...", remoteServerEndPoint.ToString());
                sender.Connect(remoteServerEndPoint);
                Console.WriteLine("Client: Connected to {0}", sender.RemoteEndPoint.ToString());

                // Encode the data string into a byte array.
                for (int i = 0; i < 4; i++) {
                    byte[] msg = Encoding.ASCII.GetBytes("This is a test");
                    int bytesSent = sender.Send(msg);
                    Console.WriteLine("Client: Sent ({0}) bytes", bytesSent);
                    Thread.Sleep(1000);
                    int bytesRec = sender.Receive(bytes);
                    Console.WriteLine("Client: Echoing {0}", Encoding.ASCII.GetString(bytes, 0, bytesRec));
                    Thread.Sleep(1000);
                }
                // Release the socket.
                sender.Shutdown(SocketShutdown.Both);
                sender.Close();
            }
            catch (ArgumentNullException ane) {
                Console.WriteLine("ArgumentNullException : {0}", ane.ToString());
            }
            catch (ZeroTier.Sockets.SocketException e) {
                Console.WriteLine(e);
                Console.WriteLine("ServiceErrorCode={0} SocketErrorCode={1}", e.ServiceErrorCode, e.SocketErrorCode);
            }
        }
        catch (Exception e) {
            Console.WriteLine(e.ToString());
        }
    }
}

public class example {
    static int Main(string[] args)
    {
        if (args.Length < 4 || args.Length > 5) {
            Console.WriteLine("\nPlease specify either client or server mode and required arguments:");
            Console.WriteLine("  Usage: example server <config_path> <nwid> <localPort>");
            Console.WriteLine("  Usage: example client <config_path> <nwid> <remoteAddress> <remotePort>\n");
            return 1;
        }
        string configFilePath = args[1];
        ulong networkId = (ulong)Int64.Parse(args[2], System.Globalization.NumberStyles.HexNumber);

        ExampleApp exampleApp = new ExampleApp();

        if (args[0].Equals("server")) {
            Console.WriteLine("Server mode...");
            ushort serverPort = (ushort)Int16.Parse(args[3]);
            exampleApp.StartZeroTier(configFilePath, networkId);
            IPAddress ipAddress = IPAddress.Parse("0.0.0.0");
            IPEndPoint localEndPoint = new IPEndPoint(ipAddress, serverPort);
            exampleApp.SocketServer(localEndPoint);
        }

        if (args[0].Equals("client")) {
            Console.WriteLine("Client mode...");
            string serverIP = args[3];
            int port = Int16.Parse(args[4]);
            IPAddress ipAddress = IPAddress.Parse(serverIP);
            IPEndPoint remoteEndPoint = new IPEndPoint(ipAddress, port);
            exampleApp.StartZeroTier(configFilePath, networkId);
            exampleApp.SocketClient(remoteEndPoint);
        }
        exampleApp.StopZeroTier();
        return 0;
    }
}
