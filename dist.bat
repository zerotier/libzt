REM build temp directories
set Win32ReleaseBuildDir=tmp\release\win32
set Win64ReleaseBuildDir=tmp\release\win64
set Win32DebugBuildDir=tmp\debug\win32
set Win64DebugBuildDir=tmp\debug\win64

mkdir %Win32ReleaseBuildDir%
mkdir %Win64ReleaseBuildDir%
mkdir %Win32DebugBuildDir%
mkdir %Win64DebugBuildDir%

REM final output directories
set WinReleaseOutputDir=lib\release
set WinDebugOutputDir=lib\debug

mkdir %WinReleaseOutputDir%
mkdir %WinDebugOutputDir%

pushd %Win32ReleaseBuildDir%
cmake -G "Visual Studio 15 2017" ../../../
cmake --build . --config Release
popd
copy %Win32ReleaseBuildDir%\Release\zt.lib %WinReleaseOutputDir%\libzt32.lib
copy %Win32ReleaseBuildDir%\Release\zt-shared.dll %WinReleaseOutputDir%\libzt32.dll

pushd %Win32DebugBuildDir%
cmake -G "Visual Studio 15 2017" ../../../
cmake --build . --config Debug
popd
copy %Win32DebugBuildDir%\Debug\zt.lib %WinDebugOutputDir%\libzt32d.lib
copy %Win32DebugBuildDir%\Debug\zt-shared.dll %WinDebugOutputDir%\libzt32d.dll

pushd %Win64ReleaseBuildDir%
cmake -G "Visual Studio 15 2017 Win64" ../../../
cmake --build . --config Release
popd
copy %Win64ReleaseBuildDir%\Release\zt.lib %WinReleaseOutputDir%\libzt64.lib
copy %Win64ReleaseBuildDir%\Release\zt-shared.dll %WinReleaseOutputDir%\libzt64.dll

pushd %Win64DebugBuildDir%
cmake -G "Visual Studio 15 2017 Win64" ../../../
cmake --build . --config Debug
popd
copy %Win64DebugBuildDir%\Debug\zt.lib %WinDebugOutputDir%\libzt64d.lib
copy %Win64DebugBuildDir%\Debug\zt-shared.dll %WinDebugOutputDir%\libzt64d.dll

exit 0

rd /S /Q bin

# Build with JNI

mkdir WinBuild32 & pushd WinBuild32
cmake -D JNI:BOOL=ON -G "Visual Studio 15 2017" ../
popd
mkdir WinBuild64 & pushd WinBuild64
cmake -D JNI:BOOL=ON -G "Visual Studio 15 2017 Win64" ../
popd

cmake --build WinBuild32 --config Release
cmake --build WinBuild32 --config Debug

REM Build JAR file
REM release variant
cd packages\java
del com/zerotier/libzt/*.class
move ..\..\%ReleaseWinBuildDir%\zt-shared.dll zt.dll
javac com/zerotier/libzt/*.java
jar cf zt.jar zt.dll  com/zerotier/libzt/*.class
move zt.jar ..\..\%PrebuiltReleaseWin32Dir%
REM debug variant
del com/zerotier/libzt/*.class
move ..\..\%DebugWinBuildDir%\zt-shared.dll zt.dll
javac com/zerotier/libzt/*.java
jar cf zt.jar zt.dll  com/zerotier/libzt/*.class
move zt.jar ..\..\%PrebuiltDebugWin32Dir%
popd
popd

cmake --build WinBuild64 --config Release
cmake --build WinBuild64 --config Debug

REM Build JAR file
REM release variant
cd packages\java
del com/zerotier/libzt/*.class
move ..\..\%ReleaseWinBuildDir%\zt-shared.dll zt.dll
javac com/zerotier/libzt/*.java
jar cf zt.jar zt.dll  com/zerotier/libzt/*.class
move zt.jar ..\..\%PrebuiltReleaseWin64Dir%
REM debug variant
del com/zerotier/libzt/*.class
move ..\..\%DebugWinBuildDir%\zt-shared.dll zt.dll
javac com/zerotier/libzt/*.java
jar cf zt.jar zt.dll  com/zerotier/libzt/*.class
move zt.jar ..\..\%PrebuiltDebugWin64Dir%
popd
popd