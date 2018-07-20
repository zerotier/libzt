## libzt in C++ via DLL import
***

 - Add [libzt.dll]() and [libzt.lib]() to solution as existing items. 
 - Place [libzt.dll](libzt.dll) in the same directory as the executable. 
 - Include `libzt.h` in application source.
 - Access functions grlobally like so: `zts_socket(...)`

*** 

The Windows `.dll` and `.lib` files are provided pre-built at the above links, but if you'd like build instructions you can check out [BUILDING.md](../../BUILDING.md)