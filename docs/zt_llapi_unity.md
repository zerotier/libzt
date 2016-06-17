ZeroTier Unity LLAPI
====

We've tried to replicate the behavior of the Unity3D LLAPI to make using ZeroTier as easy as possible. All you need to do is add the `ZeroTierSDK_Unity3D_YOUR-PLATFORM` library in the `assets/plugins` folder of your project and start using the `ZeroTierNetworkInterface`:

## Creating a host and receiving data

```
public class MyObject
{
	private ZeroTierNetworkInterface zt;

	void Start()
	{
		zt = new ZeroTierNetworkInterface("/Users/Bob/UnityGame/nc_8c493f5bef1747a6");
		zt.AddHost();
	}

	void Update()
	{
		NetworkEventType ne = zt.Receive();

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