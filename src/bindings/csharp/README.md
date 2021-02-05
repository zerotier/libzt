# C# Language Bindings

 - Install (via [NuGet package](https://www.nuget.org/packages/ZeroTier.Sockets/)): `Install-Package ZeroTier.Sockets`
 - Example usage: [examples/csharp](./../../../examples/csharp/)

# Development Notes

 - The SWIG interface file `zt.i` is only present for historical reference purposes. SWIG generates a ton of unnecessary boilerplate code which is hard to completely prevent by using hints. You can generate a new wrapper for yourself using `swig -c++ -csharp -dllimport "./libzt.so" zt.i` but I would not recommend doing so unless you know what you're in for.