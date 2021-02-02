ZeroTier Sockets for C# .NET
=====

A replacement for the [System.Net.Sockets.Socket](https://docs.microsoft.com/en-us/dotnet/api/system.net.sockets.socket) class built atop ZeroTier's SDK using P/INVOKE. It is designed to be a direct drop-in replacement. The library consists of three main objects: `ZeroTier.Node`, `ZeroTier.Event`, and `ZeroTier.Socket`. No code change is required in your application beyond a small snippet of startup code, renaming `Socket` to `ZeroTier.Socket` (where applicable) and handling a smattering of events.

# Overview

Add `ZeroTier.Sockets` to your project: 
```powershell
Install-Package ZeroTier.Sockets
```

See [example.cs](./example.cs) for complete client/server app implementation.

```csharp
using System.Net.Sockets;
using ZeroTier;

void OnZeroTierEvent(ZeroTier.Event e)
{
	Console.WriteLine("{0} ({1})", e.EventCode, e.EventName);
}
...

ZeroTier.Node node = new ZeroTier.Node("path", OnZeroTierEvent, 9991);     	

node.Start();	
node.Join(0xc287ac0b42a6fb4c);

...

ZeroTier.Socket sock = new ZeroTier.Socket(ipAddr.AddressFamily, SocketType.Stream, ProtocolType.Tcp);

sock.Connect(remoteEndPoint);

...

node.Stop();
```

# Building example without NuGet package (Advanced)

From top-level repo directory, build `libzt.dll/so/dylib`:

```bash
make host_pinvoke_release
```

Copy `libzt.dll/so/dylib` into this project directory:

```
cp ../../lib/release/${YOUR_HOST_TUPLE}-pinvoke/libzt.* .
```
Where `${YOUR_HOST_TUPLE}` is something like: `linux-x86_64`, `macOS-x86_64`, etc.

Build language binding layer, `ZeroTier.Sockets.dll`:

```bash
cd examples/csharp
${CSHARP_COMPILER} -target:library -out:ZeroTier.Sockets.dll ../../src/bindings/csharp/*.cs
${CSHARP_COMPILER} -reference:ZeroTier.Sockets.dll example.cs
./example.exe
```
Where `${CSHARP_COMPILER}` may be `csc` or `mono-csc` depending on your platform.
