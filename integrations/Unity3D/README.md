Unity3D OSX + ZeroTier SDK
====

Welcome!

We want your Unity apps to talk *directly* over a flat, secure, no-config virtual network without sending everything into the "cloud". Thus, we introduce the ZeroTier-Unity3D integration!  

Our implementation currently intends to be the bare minimum required to get your Unity application to talk over ZeroTier virtual networks. As a result, we've created an API that is very similar to the classic BSD-style sockets API. With this basic API it is possible to construct more abstracted network layers much like Unity's LLAPI and HLAPI.

Our example project can be found [here]()

***
## API  

- `Join(nwid)`: Joins a ZeroTier virtual network
- `Leave(nwid)`: Leaves a ZeroTier virtual network
- `Socket(family, type, protocol)`: Creates a ZeroTier-administered socket (returns an `fd`)
- `Bind(fd, addr, port)`: Binds to that socket on the address and port given
- `Listen(fd, backlog)`: Puts a socket into a listening state
- `Accept(fd)`: Accepts an incoming connection
- `Connect(fd, addr, port)`: Connects to an endpoint associated with the given `fd` 
- `Write(fd, buf, len)`: Sends data to the endpoint associated with the given `fd`
- `Read(fd, buf, len)`: Receives data from an endpoint associated with the given `fd`
- `SendTo(fd, buf, len, flags, addr, port)`: Sends data to a given address
- `RecvFrom(fd, ref buf, len, flags, addr, port)`: Receives data
- `Close(fd)`: Closes a connection to an endpoint

***
## Adding ZeroTier to your Unity app

**Step 1: Create virtual ZeroTier [virtual network](https://my.zerotier.com/)**

**Step 2: Add plugin to Unity project**
 - Create folder `Assets/Plugins`
 - Place `ZeroTierSDK_Unity3D_OSX.bundle` in folder

**Step 3: Include wrapper class source**
 - Drag `ZeroTierNetworkInterface.cs` into your `Assets` folder.

**Step 4: Create and use a `ZeroTierNetworkInterface` object**
 - See examples below for how to use it!

***

## Listening for a connection
```
public class Example
{
	public ZeroTierNetworkInterface zt;

	public void example_server()
	{
		zt = new ZeroTierNetworkInterface (); // Start interface
		zt.Join("565799d8f6e1c11a"); // Join your network

		Thread connectThread = new Thread(() => { 
			// Create ZeroTier-administered socket
			int sock = zt.Socket ((int)AddressFamily.InterNetwork, (int)SocketType.Stream, (int)ProtocolType.Unspecified);
			zt.Bind(sock, "0.0.0.0", 8000);
			zt.Listen(sock, 1);

			// Accept client connection
			int accept_sock = -1;
			while(accept_res < 0) {
				accept_sock = zt.Accept(sock);
			}

			// Read data from client
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
	}
}
```

## Establishing a connection
```
public class Example
{
	public ZeroTierNetworkInterface zt;

	public void example_client()
	{
		zt = new ZeroTierNetworkInterface ();
		zt.Join("565799d8f6e1c11a");

		Thread connectThread = new Thread(() => {	
			// Create ZeroTier-administered socket		
			int sock = zt.Socket ((int)AddressFamily.InterNetwork, (int)SocketType.Stream, (int)ProtocolType.Unspecified);
			zt.Connect (sock, "0.0.0.0",8000);
			zt.Write(sock, "Welcome to the machine!", 24);
		});
		connectThread.IsBackground = true;
		connectThread.Start();
	}
}
```