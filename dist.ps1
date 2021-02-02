function CreateNugetPackage
{
	md builds\pkg\nuget -Force

	# runtimes
	md pkg\nuget\ZeroTier.Sockets\runtimes\win10-x86\native -Force
	md pkg\nuget\ZeroTier.Sockets\runtimes\win10-x64\native -Force
	md pkg\nuget\ZeroTier.Sockets\runtimes\win10-x64\lib\uap10.0 -Force
	md pkg\nuget\ZeroTier.Sockets\runtimes\win10-arm\native -Force

	# frameworks
	md pkg\nuget\ZeroTier.Sockets\lib\net48 -Force
	md pkg\nuget\ZeroTier.Sockets\lib\net5.0 -Force

	# Build native libzt with exported P/INVOKE symbols
	Build-Library -BuildType "Release" -Arch "x64" -LanguageBinding "csharp"
	Build-Library -BuildType "Release" -Arch "x64" -LanguageBinding ""

	Build-Library -BuildType "Release" -Arch "Win32" -LanguageBinding "csharp"
	#Build-Library -BuildType "Release" -Arch "ARM" -LanguageBinding "csharp"

	# Copy assemblies into NuGet package tree
	Copy-Item .\lib\release\win-x64-pinvoke\*.dll pkg\nuget\ZeroTier.Sockets\runtimes\win10-x64\lib\uap10.0\libzt.dll
	Copy-Item .\lib\release\win-x64\*.dll pkg\nuget\ZeroTier.Sockets\runtimes\win10-x64\native\libzt.dll

	Copy-Item .\lib\release\win-x86-pinvoke\*.dll pkg\nuget\ZeroTier.Sockets\runtimes\win10-x86\native\libzt.dll
	#Copy-Item .\lib\release\win-arm-pinvoke\*.dll pkg\nuget\ZeroTier.Sockets\runtimes\win10-arm\native\libzt.dll

	# Build wrapper library for C# ZeroTier.Sockets abstraction
	csc -target:library -out:pkg\nuget\ZeroTier.Sockets\lib\net5.0\ZeroTier.Sockets.dll .\src\bindings\csharp\*.cs

	# Package everything
	Push-Location -Path pkg\nuget\ZeroTier.Sockets

	del ZeroTier.Sockets.*.nupkg
	nuget pack ZeroTier.Sockets.nuspec -OutputDirectory ..\..\..\builds\pkg\nuget

	Pop-Location
}

function Clean
{
	Remove-Item builds -Recurse -Force -Confirm:$false -ErrorAction:'silentlycontinue'
	Remove-Item tmp -Recurse -Force -Confirm:$false -ErrorAction:'silentlycontinue'
	Remove-Item lib -Recurse -Force -Confirm:$false -ErrorAction:'silentlycontinue'
	Remove-Item bin -Recurse -Force -Confirm:$false -ErrorAction:'silentlycontinue'
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