ZeroTier Unity LLAPI
====

We've tried to replicate the behavior of the Unity3D LLAPI to make using ZeroTier as easy as possible. All you need to do is add the `ZeroTierSDK_Unity3D_YOUR-PLATFORM` library to the `assets/plugins` folder of your project and start using the `ZeroTierNetworkInterface`:


To start things off, go check out [ZeroTierSockets_Demo.cs](). Here are some examples of how to use the `ZeroTierNetworkInterface`:

## Server example
```
Thread connectThread = new Thread(() => { 
			// Socket()
			connection_socket = zt.Socket ((int)AddressFamily.InterNetwork, (int)SocketType.Stream, (int)ProtocolType.Unspecified);
			Debug.Log ("sockfd = " + connection_socket);

			// Bind()
			int port_num;
			int.TryParse(port.text,out port_num);
			int bind_res = zt.Bind(connection_socket, "0.0.0.0", port_num);
			Debug.Log ("bind_res = " + bind_res);

			// Listen()
			int listen_res = zt.Listen(connection_socket, 1);
			Debug.Log ("listen_res = " + listen_res);

			// Accept() loop
			Debug.Log("entering accept() loop");
			int accept_res = -1;
			while(accept_res < 0)
			{
				//yield return new WaitForSeconds(1);
				accept_res = zt.Accept(connection_socket);
				Debug.Log ("accept_res = " + accept_res);

			}

			char[] msg = new char[1024];
			int bytes_read = 0;
			while(bytes_read >= 0)
			{
				//Debug.Log("reading from socket");
				bytes_read = zt.Read(accept_res, ref msg, 80);

				string msgstr = new string(msg);
				Debug.Log("MSG (" + bytes_read + "):" + msgstr);
			}
		});
		connectThread.IsBackground = true;
		connectThread.Start();
	}
```

## Client example

```
Thread connectThread = new Thread(() => {			
			int sockfd = zt.Socket ((int)AddressFamily.InterNetwork, (int)SocketType.Stream, (int)ProtocolType.Unspecified);
			Debug.Log ("sockfd = " + sockfd);
			int port_num;
			int.TryParse(port.text,out port_num);
			zt.Connect (sockfd, addr.text,port_num);
			Debug.Log ("connection_socket = " + connection_socket);
		});
		connectThread.IsBackground = true;
		connectThread.Start();
```





***

## Creating a host and receiving data

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

## Connecting to a server and sending a message

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