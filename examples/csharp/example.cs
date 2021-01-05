
using System;
using System.Threading;
using System.Net;
using System.Net.Sockets; // For SocketType, etc
using System.Text; // For Encoding

using ZeroTier; // For ZeroTier.Node, ZeroTier.Event, and ZeroTier.Socket

public class ExampleApp {

	ZeroTier.Node node;

	/**
	 * Initialize and start ZeroTier
	 */
	public void StartZeroTier(string configFilePath, ushort servicePort, ulong networkId)
	{
		node = new ZeroTier.Node(configFilePath, myZeroTierEventCallback, servicePort);
		node.Start(); // Network activity only begins after calling Start()

		/* How you do this next part is up to you, but essentially we're waiting for the node
		to signal to us (via a ZeroTierEvent) that it has access to the internet and is
		able to talk to one of our root servers. As a convenience you can just periodically check
		IsOnline() instead of looking for the event via the callback. */
		while (!node.IsOnline()) { Thread.Sleep(100); }

		/* After the node comes online you may now join/leave networks. You will receive
		notifications via the callback function regarding the status of your join request as well
		as any subsequent network-related events such as the assignment of an IP address, added
		or removed routes, etc. */
		node.Join(networkId);

		/* Note that ZeroTierSocket calls will fail if there are no routes available, for this
		reason we should wait to make those calls until the node has indicated to us that at
		least one network has been joined successfully. */
		while (!node.HasRoutes()) { Thread.Sleep(100); }
	}

	/**
	 * Stop ZeroTier
	 */
	public void StopZeroTier()
	{
		node.Stop();
	}

	/**
	 * Your application should process event messages and return control as soon as possible. Blocking
	 * or otherwise time-consuming operations are not reccomended here.
	 */
	public void myZeroTierEventCallback(ZeroTier.Event e)
	{
		Console.WriteLine("Event.eventCode = {0} ({1})", e.EventCode, e.EventName);
		
		if (e.EventCode == ZeroTier.Constants.EVENT_NODE_ONLINE) {
			Console.WriteLine("Node is online");
			Console.WriteLine(" - Address (NodeId): " + node.NodeId);
		}

		if (e.EventCode == ZeroTier.Constants.EVENT_NETWORK_OK) {
			Console.WriteLine(" - Network ID: " + e.networkDetails.networkId.ToString("x16"));
		}
	}

	/**
	 * Example server
	 */
	public void YourServer() {
		string data = null;

		// Data buffer for incoming data.  
		byte[] bytes = new Byte[1024];  
  
		string serverIP = "0.0.0.0";
		int port = 8000;
		IPAddress ipAddress = IPAddress.Parse(serverIP);
		IPEndPoint localEndPoint = new IPEndPoint(ipAddress, port);

		Console.WriteLine(localEndPoint.ToString());
		ZeroTier.Socket listener = new ZeroTier.Socket(ipAddress.AddressFamily, SocketType.Stream, ProtocolType.Tcp ); 
  
		// Bind the socket to the local endpoint and
		// listen for incoming connections.  

		try {  
			listener.Bind(localEndPoint);  
			listener.Listen(10);  
  
			// Start listening for connections.  
			while (true) {  
				Console.WriteLine("Waiting for a connection...");  
				// Program is suspended while waiting for an incoming connection.  
				Console.WriteLine("accepting...");
				ZeroTier.Socket handler = listener.Accept();  
				data = null;  

				Console.WriteLine("accepted connection from: " + handler.RemoteEndPoint.ToString());
 
				// An incoming connection needs to be processed.  
				while (true) {  
					int bytesRec = handler.Receive(bytes);  
					data += Encoding.ASCII.GetString(bytes,0,bytesRec);  
					if (data.IndexOf("<EOF>") > -1) {  
						break;  
					}  
				}  
  
				// Show the data on the console.  
				Console.WriteLine( "Text received : {0}", data);  
  
				// Echo the data back to the client.  
				byte[] msg = Encoding.ASCII.GetBytes(data);  
  
				handler.Send(msg);  
				handler.Shutdown(SocketShutdown.Both);  
				handler.Close();  
			}  
  
		} catch (Exception e) {  
			Console.WriteLine(e.ToString());  
		}  
  
		Console.WriteLine("\nPress ENTER to continue...");  
		Console.Read();
	}  

	/**
	 * Example client
	 */
	public void YourClient() {  
		// Data buffer for incoming data.  
		byte[] bytes = new byte[1024];  
  
		// Connect to a remote device.  
		try {
			string serverIP = "10.244.180.7";
			int port = 8000;
			IPAddress ipAddress = IPAddress.Parse(serverIP);
			IPEndPoint remoteEndPoint = new IPEndPoint(ipAddress, port); 
  
			// Create a TCP/IP  socket.  
			ZeroTier.Socket sender = new ZeroTier.Socket(ipAddress.AddressFamily,
				SocketType.Stream, ProtocolType.Tcp );  
  
			// Connect the socket to the remote endpoint. Catch any errors.  
			try {  

				Console.WriteLine("Socket connecting to {0}...",  
					remoteEndPoint.ToString());  

				sender.Connect(remoteEndPoint);  

				Console.WriteLine("Socket connected to {0}",  
					sender.RemoteEndPoint.ToString());  
  
				// Encode the data string into a byte array.  
				byte[] msg = Encoding.ASCII.GetBytes("This is a test");  
  
				// Send the data through the socket.  
				int bytesSent = sender.Send(msg);  
  
				// Receive the response from the remote device.  
				int bytesRec = sender.Receive(bytes);  
				Console.WriteLine("Echoed test = {0}",  
					Encoding.ASCII.GetString(bytes,0,bytesRec));
  
				// Release the socket.  
				sender.Shutdown(SocketShutdown.Both);  
				sender.Close();  
  
			} catch (ArgumentNullException ane) {  
				Console.WriteLine("ArgumentNullException : {0}",ane.ToString());  
			} catch (SocketException se) {  
				Console.WriteLine("SocketException : {0}",se.ToString());  
			} catch (Exception e) {  
				Console.WriteLine("Unexpected exception : {0}", e.ToString());  
			}  
  
		} catch (Exception e) {  
			Console.WriteLine( e.ToString());  
		}  
	}
}

public class example
{
	static void Main()
	{
		ExampleApp exampleApp = new ExampleApp();

		ulong networkId = 0x8216ab0a47c622a1;
		ushort servicePort = 9991;
		string configFilePath = "path";

		exampleApp.StartZeroTier(configFilePath, servicePort, networkId);
		exampleApp.YourClient();
		exampleApp.StopZeroTier();		
	}
}
 
