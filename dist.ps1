function Clean
{
	Remove-Item builds -Recurse -Force -Confirm:$false -ErrorAction:'silentlycontinue'
	Remove-Item tmp -Recurse -Force -Confirm:$false -ErrorAction:'silentlycontinue'
	Remove-Item lib -Recurse -Force -Confirm:$false -ErrorAction:'silentlycontinue'
	Remove-Item bin -Recurse -Force -Confirm:$false -ErrorAction:'silentlycontinue'
	# pkg
	Clean-PackageDirectory
	Get-ChildItem pkg -recurse -include *.dll | remove-item
	Get-ChildItem pkg -recurse -include *.lib | remove-item
	Get-ChildItem pkg -recurse -include *.pdb | remove-item
	Get-ChildItem pkg -recurse -include *.nupkg | remove-item
	# src
	Get-ChildItem src -recurse -include *.dll | remove-item
	Get-ChildItem src -recurse -include *.lib | remove-item
	Get-ChildItem src -recurse -include *.pdb | remove-item
	Get-ChildItem src -recurse -include *.dylib | remove-item
	Get-ChildItem src -recurse -include *.so | remove-item
	Get-ChildItem src -recurse -include *.exe | remove-item
	Get-ChildItem src -recurse -include *.out | remove-item
	Get-ChildItem src -recurse -include *.a | remove-item
}

function Build-Library([string]$BuildType, [string]$Arch, [string]$LanguageBinding)
{
	$OptionalLanguageBinding=""

	if ($LanguageBinding -eq "csharp") {
		$OptionalLanguageBinding="-DZTS_PINVOKE:BOOL=ON"
		$LanguageBindingPostfix="-pinvoke"
	}
	if ($LanguageBinding -eq "java") {
		#$OptionalLanguageBinding="-DSDK_JNI=ON -DSDK_JNI=1"
		#$LanguageBindingPostfix="-jni"
	}

	$archAlias = ""
	$bitCount = ""

	if ($Arch -eq "Win32") {
		$bitCount="32"
		$archAlias="win-x86"
	}
	if ($Arch -eq "x64") {
		$bitCount="64"
		$archAlias="win-x64"
	}
	#if ($Arch -eq "ARM32") {
	#	$bitCount="32"
	#	$archAlias="win-arm"
	#}
	if ($Arch -eq "ARM") {
		$bitCount="64"
		$archAlias="win-arm64"
	}

	if ($archAlias -eq "" -or $bitCount -eq "") {
		echo "No valid architecture specified. Breaking."
		break
	}

	# Directory for CMake to build and store intermediate files
	$env:BuildDir="tmp\$BuildType\"+$Arch+$LanguageBindingPostfix
	md $env:BuildDir -ErrorAction:'silentlycontinue'
	# Directory where we plan to store the resultant libraries
	$env:OutputDir="lib\"+$BuildType.ToLower()
	md $env:OutputDir\$archAlias$LanguageBindingPostfix -ErrorAction:'silentlycontinue'
	Push-Location -Path $env:BuildDir
	cmake ${OptionalLanguageBinding} -G "Visual Studio 16 2019" -A $Arch ../../../
	cmake --build . --config $BuildType
	Pop-Location
	Copy-Item $env:BuildDir\$BuildType\zt.lib $env:OutputDir\$archAlias$LanguageBindingPostfix\libzt$bitCount.lib
	Copy-Item $env:BuildDir\$BuildType\zt-shared.dll $env:OutputDir\$archAlias$LanguageBindingPostfix\libzt$bitCount.dll
	Copy-Item $env:BuildDir\$BuildType\zt-shared.pdb $env:OutputDir\$archAlias$LanguageBindingPostfix\libzt$bitCount.pdb -ErrorAction:'silentlycontinue'
}

function Build-All
{
	# Win32
	Build-Library -BuildType "Release" -Arch "Win32" -LanguageBinding ""
	Build-Library -BuildType "Release" -Arch "Win32" -LanguageBinding "pinvoke"
	Build-Library -BuildType "Debug" -Arch "Win32" -LanguageBinding ""
	Build-Library -BuildType "Debug" -Arch "Win32" -LanguageBinding "pinvoke"
	# x64
	Build-Library -BuildType "Release" -Arch "x64" -LanguageBinding ""
	Build-Library -BuildType "Release" -Arch "x64" -LanguageBinding "pinvoke"
	Build-Library -BuildType "Debug" -Arch "x64" -LanguageBinding ""
	Build-Library -BuildType "Debug" -Arch "x64" -LanguageBinding "pinvoke"
}

function BuildNuGetPackages([string]$Version)
{
	BuildNuGetPackage-Sockets -BuildType "Release" -Arch "x64" -Version $Version
	BuildNuGetPackage-Sockets -BuildType "Debug" -Arch "x64" -Version $Version
	BuildNuGetPackage-Sockets -BuildType "Release" -Arch "Win32" -Version $Version
	BuildNuGetPackage-Sockets -BuildType "Debug" -Arch "Win32" -Version $Version
}

function BuildNuGetPackage-Sockets([string]$BuildType, [string]$Arch, [string]$Version)
{
	$archAlias = $Arch
	if ($Arch -eq "Win32") {
		$archAlias="x86"
	}

	md pkg\nuget\ZeroTier.Sockets\bin\ -Force
	md builds\pkg\nuget\$($BuildType.ToLower())\$archAlias -Force
	del builds\pkg\nuget\$($BuildType.ToLower())\$archAlias\*.nupkg -ErrorAction:'silentlycontinue'

	# licenses
	md pkg\nuget\ZeroTier.Sockets\licenses -Force
	Copy-Item LICENSE.txt pkg\nuget\ZeroTier.Sockets\licenses

	# contentFiles (sources)
	md pkg\nuget\ZeroTier.Sockets\contentFiles -Force
	Copy-Item src\bindings\csharp\*.cs pkg\nuget\ZeroTier.Sockets\contentFiles

	# Where we plan to output *.nupkg(s)
	md builds\pkg\nuget\$($BuildType.ToLower()) -Force

	# runtimes
	md pkg\nuget\ZeroTier.Sockets\runtimes\win10-$archAlias\native -Force
	md pkg\nuget\ZeroTier.Sockets\runtimes\win10-$archAlias\lib\uap10.0 -Force
	#md pkg\nuget\ZeroTier.Sockets\runtimes\win10-arm\native -Force

	# Build wrapper library for C# ZeroTier.Sockets abstraction
	csc -target:library -debug:pdbonly -pdb:pkg\nuget\ZeroTier.Sockets\bin\ZeroTier.Sockets.pdb -out:pkg\nuget\ZeroTier.Sockets\bin\ZeroTier.Sockets.dll .\src\bindings\csharp\*.cs

	# Build unmanaged native libzt.dll with exported P/INVOKE symbols
	Build-Library -BuildType $BuildType -Arch $Arch -LanguageBinding "csharp"
	Copy-Item .\lib\$($BuildType.ToLower())\win-$archAlias-pinvoke\*.dll pkg\nuget\ZeroTier.Sockets\bin\libzt.dll

	# .NET Framework
	md pkg\nuget\ZeroTier.Sockets\lib\net40 -Force
	md pkg\nuget\ZeroTier.Sockets\lib\net403 -Force
	md pkg\nuget\ZeroTier.Sockets\lib\net45 -Force
	md pkg\nuget\ZeroTier.Sockets\lib\net451 -Force
	md pkg\nuget\ZeroTier.Sockets\lib\net452 -Force
	md pkg\nuget\ZeroTier.Sockets\lib\net46 -Force
	md pkg\nuget\ZeroTier.Sockets\lib\net461 -Force
	md pkg\nuget\ZeroTier.Sockets\lib\net462 -Force
	md pkg\nuget\ZeroTier.Sockets\lib\net47 -Force
	md pkg\nuget\ZeroTier.Sockets\lib\net471 -Force
	md pkg\nuget\ZeroTier.Sockets\lib\net472 -Force
	md pkg\nuget\ZeroTier.Sockets\lib\net48 -Force

	# .NET "Core" 5.0 (moniker missing from microsoft documentation?)
	md pkg\nuget\ZeroTier.Sockets\lib\net5.0 -Force

	# Copy assemblies into framework-specific directories.
	$folders = Get-ChildItem pkg\nuget\ZeroTier.Sockets\lib\
	foreach ($folder in $folders.name){
		Copy-Item -Path "pkg\nuget\ZeroTier.Sockets\bin\*.*" -Destination "pkg\nuget\ZeroTier.Sockets\lib\$folder" -Recurse
	}

	# Native DLL placement
	Copy-Item .\lib\$($BuildType.ToLower())\win-$archAlias-pinvoke\*.dll pkg\nuget\ZeroTier.Sockets\runtimes\win10-$archAlias\lib\uap10.0\libzt.dll
	Copy-Item .\lib\$($BuildType.ToLower())\win-$archAlias-pinvoke\*.dll pkg\nuget\ZeroTier.Sockets\lib\net40\libzt.dll
	Copy-Item .\lib\$($BuildType.ToLower())\win-$archAlias-pinvoke\*.dll pkg\nuget\ZeroTier.Sockets\runtimes\win10-$archAlias\native\libzt.dll
	Copy-Item .\lib\$($BuildType.ToLower())\win-$archAlias-pinvoke\*.pdb pkg\nuget\ZeroTier.Sockets\runtimes\win10-$archAlias\lib\uap10.0\libzt.pdb

	# Package
	Push-Location -Path pkg\nuget\ZeroTier.Sockets
	nuget pack ZeroTier.Sockets.$archAlias.nuspec -Version $Version -OutputDirectory ..\..\..\builds\pkg\nuget\$($BuildType.ToLower())\$archAlias
	Pop-Location
}

function Clean-PackageDirectory
{
	Remove-Item pkg\nuget\ZeroTier.Sockets\lib -Recurse -Force -Confirm:$false -ErrorAction:'silentlycontinue'
	Remove-Item pkg\nuget\ZeroTier.Sockets\contentFiles -Recurse -Force -Confirm:$false -ErrorAction:'silentlycontinue'
	Remove-Item pkg\nuget\ZeroTier.Sockets\licenses -Recurse -Force -Confirm:$false -ErrorAction:'silentlycontinue'
	Remove-Item pkg\nuget\ZeroTier.Sockets\runtimes -Recurse -Force -Confirm:$false -ErrorAction:'silentlycontinue'
	Remove-Item pkg\nuget\ZeroTier.Sockets\bin -Recurse -Force -Confirm:$false -ErrorAction:'silentlycontinue'
}
