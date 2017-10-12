## libzt in C# via DLL import
***

 - Add [libzt.dll]() and [libzt.lib]() to solution as existing items. 
 - Place [libzt.dll](libzt.dll) in the same directory as the executable. 
 - Add `using ZeroTier;` to beginning of application source.
 - Access functions via `static class libzt`. For example, `libzt.zt_socket(...)` 

*** 

The Windows `.dll` and `.lib` files are provided pre-built at the above links, but if you'd like build instructions you can check out [BUILDING.md](../../BUILDING.md)