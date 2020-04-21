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

mkdir %WinReleaseOutputDir%\win-x86
mkdir %WinReleaseOutputDir%\win-x86_64
mkdir %WinDebugOutputDir%\win-x86
mkdir %WinDebugOutputDir%\win-x86_64

mkdir %WinReleaseOutputDir%
mkdir %WinDebugOutputDir%

pushd %Win32ReleaseBuildDir%
cmake -G "Visual Studio 16 2019" ../../../
cmake --build . --config Release
popd
copy %Win32ReleaseBuildDir%\Release\zt.lib %WinReleaseOutputDir%\win-x86\libzt32.lib
copy %Win32ReleaseBuildDir%\Release\zt-shared.dll %WinReleaseOutputDir%\win-x86\libzt32.dll

pushd %Win32DebugBuildDir%
cmake -G "Visual Studio 16 2019" ../../../
cmake --build . --config Debug
popd
copy %Win32DebugBuildDir%\Debug\zt.lib %WinDebugOutputDir%\win-x86\libzt32d.lib
copy %Win32DebugBuildDir%\Debug\zt-shared.dll %WinDebugOutputDir%\win-x86\libzt32d.dll

pushd %Win64ReleaseBuildDir%
cmake -G "Visual Studio 16 2019" -A x64 ../../../
cmake --build . --config Release
popd
copy %Win64ReleaseBuildDir%\Release\zt.lib %WinReleaseOutputDir%\win-x86_64\libzt64.lib
copy %Win64ReleaseBuildDir%\Release\zt-shared.dll %WinReleaseOutputDir%\win-x86_64\libzt64.dll

pushd %Win64DebugBuildDir%
cmake -G "Visual Studio 16 2019" -A x64 ../../../
cmake --build . --config Debug
popd
copy %Win64DebugBuildDir%\Debug\zt.lib %WinDebugOutputDir%\win-x86_64\libzt64d.lib
copy %Win64DebugBuildDir%\Debug\zt-shared.dll %WinDebugOutputDir%\win-x86_64\libzt64d.dll

REM Copy example binaries

mkdir bin\debug\win-x86\
copy %Win32DebugBuildDir%\Debug\*.exe bin\debug\win-x86\
mkdir bin\debug\win-x86_64\
copy %Win64DebugBuildDir%\Debug\*.exe bin\debug\win-x86_64\

mkdir bin\release\win-x86\
copy %Win32ReleaseBuildDir%\Release\*.exe bin\release\win-x86\
mkdir bin\release\win-x86_64\
copy %Win64ReleaseBuildDir%\Release\*.exe bin\release\win-x86_64\

exit 0

rd /S /Q bin

# Build with JNI

mkdir WinBuild32 & pushd WinBuild32
cmake -D JNI:BOOL=ON -G "Visual Studio 16 2019" ../
popd
mkdir WinBuild64 & pushd WinBuild64
cmake -D JNI:BOOL=ON -G "Visual Studio 16 2019" -A x64 ../
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