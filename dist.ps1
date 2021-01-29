function Clean
{
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
	if ($Arch -eq "ARM32") {
		$bitCount="32"
		$archAlias="win-arm"
	}
	if ($Arch -eq "ARM64") {
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