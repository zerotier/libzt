REM Build all target configurations and copy results into "prebuilt"

set PrebuiltDebugWin32Dir=prebuilt\debug\win32
set PrebuiltDebugWin64Dir=prebuilt\debug\win64
set PrebuiltReleaseWin32Dir=prebuilt\release\win32
set PrebuiltReleaseWin64Dir=prebuilt\release\win64

mkdir %PrebuiltDebugWin32Dir%
mkdir %PrebuiltDebugWin64Dir%
mkdir %PrebuiltReleaseWin32Dir%
mkdir %PrebuiltReleaseWin64Dir%

set DebugWinBuildDir=bin\lib\Debug
set ReleaseWinBuildDir=bin\lib\Release

mkdir WinBuild32 & pushd WinBuild32
cmake -G "Visual Studio 15 2017" ../
popd
mkdir WinBuild64 & pushd WinBuild64
cmake -G "Visual Studio 15 2017 Win64" ../
popd

cmake --build WinBuild32 --config Release
cmake --build WinBuild32 --config Debug

copy %DebugWinBuildDir%\zt-static.lib %PrebuiltDebugWin32Dir%\zt.lib
copy %DebugWinBuildDir%\zt-shared.dll %PrebuiltDebugWin32Dir%\zt.dll
copy %ReleaseWinBuildDir%\zt-static.lib %PrebuiltReleaseWin32Dir%\zt.lib
copy %ReleaseWinBuildDir%\zt-shared.dll %PrebuiltReleaseWin32Dir%\zt.dll

cmake --build WinBuild64 --config Release
cmake --build WinBuild64 --config Debug

copy %DebugWinBuildDir%\zt-static.lib %PrebuiltDebugWin64Dir%\zt.lib
copy %DebugWinBuildDir%\zt-shared.dll %PrebuiltDebugWin64Dir%\zt.dll
copy %ReleaseWinBuildDir%\zt-static.lib %PrebuiltReleaseWin64Dir%\zt.lib
copy %ReleaseWinBuildDir%\zt-shared.dll %PrebuiltReleaseWin64Dir%\zt.dll

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

copy %DebugWinBuildDir%\zt-static.lib %PrebuiltDebugWin32Dir%\zt-jni.lib
copy %DebugWinBuildDir%\zt-shared.dll %PrebuiltDebugWin32Dir%\zt-jni.dll
copy %ReleaseWinBuildDir%\zt-static.lib %PrebuiltReleaseWin32Dir%\zt-jni.lib
copy %ReleaseWinBuildDir%\zt-shared.dll %PrebuiltReleaseWin32Dir%\zt-jni.dll

cmake --build WinBuild64 --config Release
cmake --build WinBuild64 --config Debug

copy %DebugWinBuildDir%\zt-static.lib %PrebuiltDebugWin64Dir%\zt-jni.lib
copy %DebugWinBuildDir%\zt-shared.dll %PrebuiltDebugWin64Dir%\zt-jni.dll
copy %ReleaseWinBuildDir%\zt-static.lib %PrebuiltReleaseWin64Dir%\zt-jni.lib
copy %ReleaseWinBuildDir%\zt-shared.dll %PrebuiltReleaseWin64Dir%\zt-jni.dll
