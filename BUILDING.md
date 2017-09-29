## Build Instructions for various platforms

### Linux
 
 - `make static_lib LIBZT_IPV4=1`
 - `make tests`

### macOS
 
 - `make static_lib LIBZT_IPV4=1`
 - `make tests`

### iOS

 - `make ios_framework`

### FreeBSD
 
 - `make static_lib LIBZT_IPV4=1`
 - `make tests`

### Windows
 
 - [Prebuilt Binaries\Libraries Here]()

 Otherwise, 

 - Set up [mingw-w64](https://mingw-w64.org/doku.php)
 - Install a copy of [MSYS](http://www.mingw.org/wiki/MSYS)
 - Run `msys.bat` (should be in the base directory of your MSYS installation)
 	- Tell it where your `mingw-64` installation is
 - `make static_lib`
 - `make tests`

 This will output binaries to `build/mingw-w64`

