ZeroTier Sockets for C# .NET (Work In Progress)
=====

This library is a re-implementation of the .NET socket class ([System.Net.Sockets.Socket](https://docs.microsoft.com/en-us/dotnet/api/system.net.sockets.socket)) built atop ZeroTier's SDK using P/INVOKE and is designed to be a direct drop-in replacement. The library consists of three main objects: `ZeroTier.Node`, `ZeroTier.Event`, and `ZeroTier.Socket`. No code change is required in your application beyond a small snippet of startup code, renaming `Socket` to `ZeroTier.Socket` (where applicable) and handling a smattering of events.


tl;dr:

```
using System.Net.Sockets;
using ZeroTier;

void myCallback(ZeroTier.Event e)
{
	Console.WriteLine("{0} ({1})", e.EventCode, e.EventName);
}
...

ZeroTier.Node node = new ZeroTier.Node("path", myCallback, 9991);     	

node.Start();	
node.Join(0xc287ac0b42a6fb4c);

...

ZeroTier.Socket sock = new ZeroTier.Socket(ipAddr.AddressFamily, SocketType.Stream, ProtocolType.Tcp);

sock.Connect(remoteEndPoint);

...

node.Stop();
```
See [example.cs](./example.cs) for a complete client/server implementation.

## Building and running the example

```
make debug|release
./example.exe
```

## Development notes

The SWIG interface file `zt.i` is only present for historical reference purposes. SWIG generates a ton of unnecessary boilerplate code which is hard to completely prevent using hints. You can generate a new wrapper for yourself using `swig -c++ -csharp -dllimport "./libzt.so" zt.i` but I would not recommend doing so unless you know what you're in for.