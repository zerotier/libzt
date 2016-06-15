Unity3D + ZeroTier SDK
====

Welcome!

We want your Unity apps to talk *directly* over a flat, secure, no-config virtual network without sending everything into the "cloud". Thus, we introduce the ZeroTier-Unity3D integration!  

Our implementation currently intends to be the bare minimum required to get your Unity application to talk over ZeroTier virtual networks. As a result, we've created an API that is very similar to the built-in Unity LLAPI. It's possible that higher-level functionality could be added in the future. 

***
## Adding ZeroTier to your Unity app

**Step 1: Create virtual ZeroTier [virtual network](https://my.zerotier.com/)**

**Step 2: Add plugin**
 - Create a folder called `Plugins` in `Assets`
 - Place `ZeroTierUnity.bundle` in that folder

**Step 3: Add script to some `GameObject`**
 - Drag our `ZeroTier.cs` native plugin wrapper onto any `GameObject`


***
## Examples

Calling `ZeroTier.Init()` will start the network service in a separate thread. You can check if the service is running by checking `ZeroTier.IsRunning()`. Then, connecting and sending data to another endpoint would look something like the following:

```
public void zt_sample_network_test_thread()
{
	// Prepare sample data buffer
	byte[] buffer = new byte[1024];
	Stream stream = new MemoryStream(buffer);
	BinaryFormatter f = new BinaryFormatter();
	f.Serialize ( stream , "Welcome to the machine! (from Unity3D)" );

	// Connect and send
	int error;
	Connect (0, "192.168.0.6", 8887, out error);
	Send(connfd,buffer,0, out error);
}
```

Finally, when you're done running the service you can call `ZeroTier.Terminate()`

***
## API  

The API is designed to resemble the Unity LLAPI, so you'll see a few familiar functions but with a slight twist. 

- `Join(nwid)`: Joins a ZeroTier virtual network
- `Leave(nwid)`: Leaves a ZeroTier virtual network
- `AddHost(port)`: Creates a socket, and binds to that socket on the address and port given
- `Connect(fd, ip_address, port, out error)`: Connects to an endpoint associated with the given `fd` 
- `Send(fd, buf, pos, out error)`: Sends data to the endpoint associated with the given `fd`
- `Recv(fd, buf, out error)`: Receives data from an endpoint associated with the given `fd`
- `Disconnect(fd)`: Closes a connection with an endpoint

***
## Design and structure of the ZeroTier Unity OSX Bundle

XCode:
New XCode project
Select Cocoa bundle as target
Add C linkages to external functions
Build as 64bit (not universal)

Unity:
Select x86_64 build target in `Build Settings`
In new C# script asset:

```
[DllImport ("ZeroTierUnity")]
private static extern int unity_start_service ();
```

Add asset to GameObject
Start ZT service

***
## Future Roadmap  
With the ZeroTier sockets API in place, higher-level functionality such as lobbies, chat, and object synchronization could easily be built on top.


