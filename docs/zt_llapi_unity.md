ZeroTier Unity LLAPI
====

We've tried to replicate the behavior of the Unity3D LLAPI to make using ZeroTier as easy as possible. All you need to do is add the `ZeroTierSDK_Unity3D_YOUR-PLATFORM` library to the `assets/plugins` folder of your project and start using the `ZeroTierNetworkInterface`:


To start things off, go check out [ZeroTierSockets_Demo.cs](). Here are some examples of how to use the `ZeroTierNetworkInterface`:

## Using ZeroTier Sockets API
### Server example
```
Thread connectThread = new Thread(() => { 
	// Create ZeroTier-administered socket
	int sock = zt.Socket ((int)AddressFamily.InterNetwork, (int)SocketType.Stream, (int)ProtocolType.Unspecified);
	// Bind()
	zt.Bind(sock, "0.0.0.0", 8000);

	// Listen()
	zt.Listen(sock, 1);

	// Accept() loop
	int accept_sock = -1;
	while(accept_res < 0) {
		accept_sock = zt.Accept(sock);
	}

	char[] msg = new char[1024];
	int bytes_read = 0;
	while(bytes_read >= 0) { 
		bytes_read = zt.Read(accept_sock, ref msg, 80);
		string msgstr = new string(msg);
		Debug.Log("MSG (" + bytes_read + "):" + msgstr);
	}
});
connectThread.IsBackground = true;
connectThread.Start();
```

### Client example

```
Thread connectThread = new Thread(() => {	
	// Create ZeroTier-administered socket		
	int sock = zt.Socket ((int)AddressFamily.InterNetwork, (int)SocketType.Stream, (int)ProtocolType.Unspecified);
	zt.Connect (sock, "0.0.0.0",8000);
	zt.Write(sock, "Welcome to the machine!", 24);
});
connectThread.IsBackground = true;
connectThread.Start();
```



***
## ~~Using the ZeroTier Low-level API (LLAPI)~~
### Creating a host and receiving data

```
public class MyObject
{
	private ZeroTierNetworkInterface zt;

	void Start()
	{
		zt = new ZeroTierNetworkInterface("/Users/Bob/UnityGame/nc_8c493f5bef1747a6");
		zt.AddHost(8888);
	}

	void Update()
	{
		int hostId;
		int connectionId;
		int channelId;
		byte[] buffer;
		int bufferSize;
		int receivedSize;
		byte error;
		
		NetworkEventType ne = zt.Receive(out hostId, out connectionId, out channelId, buffer, bufferSize, out receivedSize, out error);

		switch(ne)
		{
			case NetworkEventType.ConnectEvent:
				Debug.Log("Client connected!");
				break;
			case NetworkEventType.DataEvent:
				Debug.Log("Received data from client!");
		}
	}
	
}
```

### Connecting to a server and sending a message

```
public class MyObject
{
	private ZeroTierNetworkInterface zt;

	void Start()
	{
		zt = new ZeroTierNetworkInterface("/Users/Bob/UnityGame/nc_8c493f5bef1747a6");

		byte error;
		int conn_id = zt.Connect(0, "192.168.0.50", "8080", out error);

		if(conn_id) {
			zt.Send(conn_id, "Welcome to the machine!", 24, error);
		}
		else {
			Debug.Log("Unable to connect to host");
		}

	}
}
```