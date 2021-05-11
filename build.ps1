function Build-Host([string]$BuildType, [string]$Arch)
{
	$Artifact="host"
	$Variant="-DBUILD_HOST=1"

	# Directory for CMake to build and store intermediate files
	$env:BuildDir="cache\win-$Arch-$Artifact-"+$BuildType.ToLower()
	md $env:BuildDir -ErrorAction:'silentlycontinue'
	# Directory where we plan to store the resultant libraries
	$env:OutputDir="dist\win-$Arch-$Artifact-"+$BuildType.ToLower()
	md $env:OutputDir -ErrorAction:'silentlycontinue'
	pushd -Path $env:BuildDir
	cmake $Variant -G "Visual Studio 16 2019" -A $Arch ../../
	cmake --build . --config $BuildType
	ctest -C debug
	popd
	#
	md $env:OutputDir\lib\ -ErrorAction:'silentlycontinue'
	md $env:OutputDir\bin\ -ErrorAction:'silentlycontinue'
	cp $env:BuildDir\lib\$BuildType\zt.lib $env:OutputDir\lib\libzt.lib
	cp $env:BuildDir\bin\$BuildType\*.exe $env:OutputDir\bin
	cp $env:BuildDir\lib\$BuildType\zt-shared.dll $env:OutputDir\lib\libzt.dll
	cp $env:BuildDir\lib\$BuildType\zt-shared.pdb $env:OutputDir\lib\libzt.pdb -ErrorAction:'silentlycontinue'
	tree /F $env:OutputDir
}

function Build-Library([string]$BuildType, [string]$Arch, [string]$LangBinding)
{
	$OptLangBinding=""

	if ($LangBinding -eq "csharp") {
		$OptLangBinding="-DZTS_ENABLE_PINVOKE=1"
		$LangBindingPostfix="pinvoke"
	}
	if ($LangBinding -eq "java") {
		$OptLangBinding="-DZTS_ENABLE_JAVA=1"
		$LangBindingPostfix="jni"
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
	$env:BuildDir="cache\win-$Arch-$LangBindingPostfix-"+$BuildType.ToLower()
	md $env:BuildDir -ErrorAction:'silentlycontinue'
	# Directory where we plan to store the resultant libraries
	$env:OutputDir="dist\win-$Arch-$LangBindingPostfix-"+$BuildType.ToLower()
	md $env:OutputDir -ErrorAction:'silentlycontinue'
	pushd -Path $env:BuildDir
	cmake ${OptLangBinding} -G "Visual Studio 16 2019" -A $Arch ../../
	cmake --build . --config $BuildType
	popd
	md $env:OutputDir\lib\ -ErrorAction:'silentlycontinue'
	#cp $env:BuildDir\lib\$BuildType\zt.lib $env:OutputDir\lib\libzt.lib
	cp $env:BuildDir\lib\$BuildType\zt-shared.dll $env:OutputDir\lib\libzt.dll
	cp $env:BuildDir\lib\$BuildType\zt-shared.pdb $env:OutputDir\lib\libzt.pdb -ErrorAction:'silentlycontinue'
}

function Build-All
{
	# Win32
	Build-Library -BuildType "Release" -Arch "Win32" -LangBinding ""
	Build-Library -BuildType "Release" -Arch "Win32" -LangBinding "csharp"
	Build-Library -BuildType "Debug" -Arch "Win32" -LangBinding ""
	Build-Library -BuildType "Debug" -Arch "Win32" -LangBinding "csharp"
	# x64
	Build-Library -BuildType "Release" -Arch "x64" -LangBinding ""
	Build-Library -BuildType "Release" -Arch "x64" -LangBinding "csharp"
	Build-Library -BuildType "Debug" -Arch "x64" -LangBinding ""
	Build-Library -BuildType "Debug" -Arch "x64" -LangBinding "csharp"
}

function BuildNuGetPackages([string]$Version)
{
	BuildNuGetPackage -BuildType "Release" -Arch "x64" -Version $Version
	BuildNuGetPackage -BuildType "Debug" -Arch "x64" -Version $Version
	BuildNuGetPackage -BuildType "Release" -Arch "Win32" -Version $Version
	BuildNuGetPackage -BuildType "Debug" -Arch "Win32" -Version $Version
}

function BuildNuGetPackage([string]$BuildType, [string]$Arch, [string]$Version)
{
	$archAlias = $Arch
	if ($Arch -eq "Win32") {
		$archAlias="x86"
	}

	$TargetTuple = "win-"+$archAlias+"-nuget-"+$($BuildType.ToLower())

	# Where we plan to output *.nupkg(s)
	md pkg\nuget\ZeroTier.Sockets\bin\ -Force
	md dist\$TargetTuple -Force
	del dist\$TargetTuple\*.nupkg -ErrorAction:'silentlycontinue'

	# licenses
	md pkg\nuget\ZeroTier.Sockets\licenses -Force
	cp LICENSE.txt pkg\nuget\ZeroTier.Sockets\licenses

	# contentFiles (sources)
	md pkg\nuget\ZeroTier.Sockets\contentFiles -Force
	cp src\bindings\csharp\*.cs pkg\nuget\ZeroTier.Sockets\contentFiles
	cp examples\csharp\*.cs pkg\nuget\ZeroTier.Sockets\contentFiles

	# runtimes
	md pkg\nuget\ZeroTier.Sockets\runtimes\win10-$archAlias\native -Force
	md pkg\nuget\ZeroTier.Sockets\runtimes\win10-$archAlias\lib\uap10.0 -Force
	#md pkg\nuget\ZeroTier.Sockets\runtimes\win10-arm\native -Force

	# Build wrapper library for C# ZeroTier.Sockets abstraction
	#csc -target:library -debug:pdbonly `
	#	-pdb:pkg\nuget\ZeroTier.Sockets\bin\ZeroTier.Sockets.pdb `
	#	-out:pkg\nuget\ZeroTier.Sockets\bin\ZeroTier.Sockets.dll `
	#	.\src\bindings\csharp\*.cs

	# Copy sources into bindings library project
	cp .\src\bindings\csharp\*.cs .\pkg\nuget\bindings\ZeroTier.Sockets\

	# Build bindings library
	pushd ./pkg/nuget/bindings/
	dotnet build --configuration Release
	popd

	# Build unmanaged native libzt.dll with exported P/INVOKE symbols
	Build-Library -BuildType $BuildType -Arch $Arch -LangBinding "csharp"

	# Copy native libzt.dll into package tree
	cp .\dist\win-$archAlias-pinvoke-$($BuildType.ToLower())\lib\*.dll `
		pkg\nuget\ZeroTier.Sockets\bin\libzt.dll

	# .NET Framework
	md pkg\nuget\ZeroTier.Sockets\lib\net40 -Force
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
	md pkg\nuget\ZeroTier.Sockets\lib\netstandard1.3 -Force
	md pkg\nuget\ZeroTier.Sockets\lib\netstandard1.4 -Force
	md pkg\nuget\ZeroTier.Sockets\lib\netstandard1.5 -Force
	md pkg\nuget\ZeroTier.Sockets\lib\netstandard1.6 -Force
	md pkg\nuget\ZeroTier.Sockets\lib\netstandard2.0 -Force
	md pkg\nuget\ZeroTier.Sockets\lib\netstandard2.1 -Force
	md pkg\nuget\ZeroTier.Sockets\lib\netcoreapp1.0 -Force
	md pkg\nuget\ZeroTier.Sockets\lib\netcoreapp1.1 -Force
	md pkg\nuget\ZeroTier.Sockets\lib\netcoreapp2.0 -Force
	md pkg\nuget\ZeroTier.Sockets\lib\netcoreapp2.1 -Force
	md pkg\nuget\ZeroTier.Sockets\lib\netcoreapp2.2 -Force
	md pkg\nuget\ZeroTier.Sockets\lib\netcoreapp3.0 -Force
	md pkg\nuget\ZeroTier.Sockets\lib\netcoreapp3.1 -Force

	# .NET "Core" 5.0 (moniker missing from microsoft documentation?)
	md pkg\nuget\ZeroTier.Sockets\lib\net5.0 -Force

	# Copy assemblies into framework-specific directories.
	$folders = Get-ChildItem pkg\nuget\ZeroTier.Sockets\lib\
	foreach ($folder in $folders.name){
		cp -Path "pkg\nuget\ZeroTier.Sockets\bin\*.*" `
			-Destination "pkg\nuget\ZeroTier.Sockets\lib\$folder" -Recurse
	}

	# Copy bindings library "ZeroTier.Sockets.dll"
	$folders = Get-ChildItem .\pkg\nuget\bindings\ZeroTier.Sockets\bin\Release
	foreach ($folder in $folders.name){
		cp -Path ".\pkg\nuget\bindings\ZeroTier.Sockets\bin\Release\$folder\*.dll" `
			-Destination "pkg\nuget\ZeroTier.Sockets\lib\$folder" -Recurse
	}

	# Native DLL placement

	cp .\dist\win-$archAlias-pinvoke-$($BuildType.ToLower())\lib\*.dll `
		pkg\nuget\ZeroTier.Sockets\runtimes\win10-$archAlias\lib\uap10.0\libzt.dll
	cp .\dist\win-$archAlias-pinvoke-$($BuildType.ToLower())\lib\*.dll `
		pkg\nuget\ZeroTier.Sockets\lib\net40\libzt.dll
	cp .\dist\win-$archAlias-pinvoke-$($BuildType.ToLower())\lib\*.dll `
		pkg\nuget\ZeroTier.Sockets\runtimes\win10-$archAlias\native\libzt.dll
	cp .\dist\win-$archAlias-pinvoke-$($BuildType.ToLower())\lib\*.pdb `
		pkg\nuget\ZeroTier.Sockets\runtimes\win10-$archAlias\lib\uap10.0\libzt.pdb

	# Package
	pushd -Path pkg\nuget\ZeroTier.Sockets
	nuget pack ZeroTier.Sockets.$archAlias.nuspec `
		-Version $Version -OutputDirectory ..\..\..\dist\$TargetTuple\
	popd
}

function Clean-PackageDirectory
{
	rm pkg\nuget\ZeroTier.Sockets\lib `
		-Recurse -Force -Confirm:$false -ErrorAction:'silentlycontinue'
	rm pkg\nuget\ZeroTier.Sockets\contentFiles `
		-Recurse -Force -Confirm:$false -ErrorAction:'silentlycontinue'
	rm pkg\nuget\ZeroTier.Sockets\licenses `
		-Recurse -Force -Confirm:$false -ErrorAction:'silentlycontinue'
	rm pkg\nuget\ZeroTier.Sockets\runtimes `
		-Recurse -Force -Confirm:$false -ErrorAction:'silentlycontinue'
	rm pkg\nuget\ZeroTier.Sockets\bin `
		-Recurse -Force -Confirm:$false -ErrorAction:'silentlycontinue'
}

function Clean
{
	rm cache -Recurse -Force -Confirm:$false -ErrorAction:'silentlycontinue'
	rm dist -Recurse -Force -Confirm:$false -ErrorAction:'silentlycontinue'
	#rm pkg\nuget\bindings\ZeroTier.Sockets\*.cs -Force -Confirm:$false -ErrorAction:'silentlycontinue'
	#rm pkg\nuget\bindings\ZeroTier.Sockets\bin -Recurse -Force -Confirm:$false -ErrorAction:'silentlycontinue'
	#rm pkg\nuget\bindings\ZeroTier.Sockets\obj -Recurse -Force -Confirm:$false -ErrorAction:'silentlycontinue'
	#rm pkg\nuget\ZeroTier.Sockets\contentFiles -Recurse -Force -Confirm:$false -ErrorAction:'silentlycontinue'
	rm pkg\nuget\ZeroTier.Sockets\lib -Recurse -Force -Confirm:$false -ErrorAction:'silentlycontinue'
	rm pkg\nuget\ZeroTier.Sockets\bin -Recurse -Force -Confirm:$false -ErrorAction:'silentlycontinue'
	rm 'pkg\nuget\*' -Recurse -Include *.pdb
	rm 'pkg\nuget\*' -Recurse -Include *.cs
	rm 'pkg\nuget\*' -Recurse -Include *.dll
	rm 'pkg\nuget\*' -Recurse -Include LICENSE.*
}
