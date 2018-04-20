<#
.SYNOPSIS
    Build all installers of psqlodbc project.
.DESCRIPTION
    Build psqlodbc_x86.msi(msm), psqlodbc_x64.msi(msm).
.PARAMETER cpu
    Specify build cpu type, "both"(default), "x86" or "x64" is
    available.
.PARAMETER AlongWithDrivers
    Specify when you'd like to build drivers before building installers.
.PARAMETER ExcludeRuntime
    Specify when you'd like to exclude a msvc runtime dll from the installer.
.PARAMETER RedistUCRT
    Specify when you'd like to redistribute Visual C++ 2015(or later) Redistributable.
.PARAMETER BuildConfigPath
    Specify the configuration xml file name if you want to use
    the configuration file other than standard one.
    The relative path is relative to the current directory.
.EXAMPLE
    > .\buildInstallers
	Build 32bit and 64bit installers.
.EXAMPLE
    > .\buildInstallers x86
	Build 32bit installers.
.NOTES
    Author: Hiroshi Inoue
    Date:   July 4, 2014
#>
#	build 32bit and/or 64bit installers
#
Param(
[ValidateSet("x86", "x64", "both")]
[string]$cpu="both",
[switch]$AlongWithDrivers,
[switch]$ExcludeRuntime,
[switch]$RedistUCRT,
[string]$BuildConfigPath
)

[int]$ucrt_version=14
[String]$str_msvcr="msvcr"
[String]$str_vcrun="vcruntime"
[String]$str_msvcp="msvcp"
[String]$msrun_ptn="msvcr|vcruntime"

function msvcrun([int]$runtime_version)
{
	[String]$str = if ($runtime_version -lt $ucrt_version) {$str_msvcr} else {$str_vcrun}
	return $str
}

function findRuntime([int]$runtime_version, [String]$pgmvc)
{
	# where's the dll? 
	[String]$rt_dllname = (msvcrun $runtime_version) + "${runtime_version}0.dll"
	if ("$pgmvc" -ne "") {
		$dllspecified = "${pgmvc}\${rt_dllname}"
		if (Test-Path -Path $dllspecified) {
			return $dllspecified, ""
		}
	}
	$dllinredist = "${LIBPQBINDIR}\${rt_dllname}"
	if (Test-Path -Path $dllinredist) {
		return $dllinredist, ""
	}
	if ($env:PROCESSOR_ARCHITECTURE -eq "x86") {
		$pgmvc = "$env:ProgramFiles"
	} else {
		$pgmvc = "${env:ProgramFiles(x86)}"
	}
	$dllinredist = "$pgmvc\Microsoft Visual Studio ${runtime_version}.0\VC\redist\${CPUTYPE}\Microsoft.VC${runtime_version}0.CRT\${rt_dllname}"
	if (Test-Path -Path $dllinredist) {
		return $dllinredist, ""
	} else {
		$messageSpec = "Please specify Configuration.$CPUTYPE.runtime_folder element of the configuration file where msvc runtime dll $rt_dllname can be found"
		if ($CPUTYPE -eq "x86") {
			if ($env:PROCESSOR_ARCHITECTURE -eq "x86") {
				$pgmvc = "${env:SystemRoot}\system32"
			} else {
				$pgmvc = "${env:SystemRoot}\syswow64"
			}
		} else {
			if ($env:PROCESSOR_ARCHITECTURE -eq "AMD64") {
				$pgmvc = "${env:SystemRoot}\system32"
			} elseif ($env:PROCESSOR_ARCHITEW6432 -eq "AMD64") {
				$pgmvc = "${env:SystemRoot}\sysnative"
			} else {
				throw "${messageSpec}`n$dllinredist doesn't exist unfortunately"
			}
		}
		$dllinsystem = "${pgmvc}\${rt_dllname}"
		if (-not(Test-Path -Path $dllinsystem)) {
			throw "${messageSpec}`nneither $dllinredist nor $dllinsystem exists unfortunately"
		}
	}
	return "", $rt_dllname
}

function buildInstaller($CPUTYPE)
{
	$LIBPQBINDIR=getPGDir $configInfo $CPUTYPE "bin"
	# msvc runtime psqlodbc links
	$PODBCMSVCDLL = ""
	$PODBCMSVPDLL = ""
	$PODBCMSVCSYS = ""
	$PODBCMSVPSYS = ""
	# msvc runtime libpq links
	$LIBPQMSVCDLL = ""
	$LIBPQMSVCSYS = ""
	$pgmvc = $configInfo.Configuration.$CPUTYPE.runtime_folder
	if (-not $ExcludeRuntime) {
		$toolset = $configInfo.Configuration.BuildResult.PlatformToolset
		if ($toolset -match "^v(\d+)0") {
			$runtime_version0 = [int]$matches[1]
		} else {
			$runtime_version0 = 10
		}
		# where's the msvc runtime dll psqlodbc links?
		if ($runtime_version0 -ge $ucrt_version -and $RedistUCRT) {
			$script:wRedist=$true
		} else {
			$dlls=findRuntime $runtime_version0 $pgmvc
			$PODBCMSVCDLL=$dlls[0]
			$PODBCMSVCSYS=$dlls[1]
			$PODBCMSVPDLL=$PODBCMSVCDLL.Replace((msvcrun $runtime_version0), $str_msvcp)
			$PODBCMSVPSYS=$PODBCMSVCSYS.Replace((msvcrun $runtime_version0), $str_msvcp)
		}
		# where's the runtime dll libpq links? 
		$msvclist=& ${dumpbinexe} /imports $LIBPQBINDIR\libpq.dll | select-string -pattern "^\s*($msrun_ptn)(\d+)0\.dll" | % {$_.matches[0].Groups[2].Value}
		if ($msvclist -ne $Null -and $msvclist.length -gt 0) {
			if ($msvclist.GetType().Name -eq "String") {
				$runtime_version1=[int]$msvclist
			} else {
				$runtime_version1=[int]$msvclist[0]
			}
			if ($runtime_version1 -ge $ucrt_version -and $RedistUCRT) {
				$script:wRedist=$true
			} elseif ($runtime_version1 -ne $runtime_version0) {
				$dlls=findRuntime $runtime_version1 $pgmvc
				$LIBPQMSVCDLL=$dlls[0]
				$LIBPQMSVCSYS=$dlls[1]
				Write-Host "LIBPQ requires $LIBPQMSVCDLL$LIBPQMSYCSYS"
			}
		} else {
			$script:wRedist=$true
		}
	}

	Write-Host "CPUTYPE    : $CPUTYPE"
	Write-Host "VERSION    : $VERSION"
	Write-Host "LIBPQBINDIR: $LIBPQBINDIR"

	if ($env:WIX -ne "")
	{
		$wix = "$env:WIX"
		$env:Path += ";$WIX/bin"
	}
	# The subdirectory to install into
	$SUBLOC=$VERSION.substring(0, 2) + $VERSION.substring(3, 2)

	#
	$maxmem=10
	$libpqmem=Get-RelatedDlls "libpq.dll" $LIBPQBINDIR
	for ($i=0; $i -lt $libpqmem.length; ) {
		if ($libpqmem[$i] -match "^($msrun_ptn)\d+0.dll") {
			$libpqmem[$i]=$Null	
		} else {
			$i++
		}
	}
	if ($libpqmem.length -gt $maxmem) {
		throw("number of libpq related dlls exceeds $maxmem")
	}
	for ($i=$libpqmem.length; $i -lt $maxmem; $i++) {
		$libpqmem += ""
	}

	[string []]$libpqRelArgs=@()
	for ($i=0; $i -lt $maxmem; $i++) {
		$libpqRelArgs += ("-dLIBPQMEM$i=" + $libpqmem[$i])
	}

	if (-not(Test-Path -Path $CPUTYPE)) {
		New-Item -ItemType directory -Path $CPUTYPE | Out-Null
	}

	$PRODUCTCODE = [GUID]::NewGuid();
	Write-Host "PRODUCTCODE: $PRODUCTCODE"

	try {
		pushd "$scriptPath"

		Write-Host ".`nBuilding psqlODBC/$SUBLOC merge module..."
		candle -nologo $libpqRelArgs "-dPlatform=$CPUTYPE" "-dVERSION=$VERSION" "-dSUBLOC=$SUBLOC" "-dLIBPQBINDIR=$LIBPQBINDIR" "-dLIBPQMSVCDLL=$LIBPQMSVCDLL" "-dLIBPQMSVCSYS=$LIBPQMSVCSYS" "-dPODBCMSVCDLL=$PODBCMSVCDLL" "-dPODBCMSVPDLL=$PODBCMSVPDLL" "-dPODBCMSVCSYS=$PODBCMSVCSYS" "-dPODBCMSVPSYS=$PODBCMSVPSYS" -o $CPUTYPE\psqlodbcm.wixobj psqlodbcm_cpu.wxs
		if ($LASTEXITCODE -ne 0) {
			throw "Failed to build merge module"
		}

		Write-Host ".`nLinking psqlODBC merge module..."
		light -nologo -o $CPUTYPE\psqlodbc_$CPUTYPE.msm $CPUTYPE\psqlodbcm.wixobj
		if ($LASTEXITCODE -ne 0) {
			throw "Failed to link merge module"
		}

		Write-Host ".`nBuilding psqlODBC installer database..."

		candle -nologo "-dPlatform=$CPUTYPE" "-dVERSION=$VERSION" "-dSUBLOC=$SUBLOC" "-dPRODUCTCODE=$PRODUCTCODE" -o $CPUTYPE\psqlodbc.wixobj psqlodbc_cpu.wxs
		if ($LASTEXITCODE -ne 0) {
			throw "Failed to build installer database"
		}

		Write-Host ".`nLinking psqlODBC installer database..."
		light -nologo -ext WixUIExtension -cultures:en-us -o $CPUTYPE\tdeforpg_psqlodbc_${VERSION}_$CPUTYPE.msi $CPUTYPE\psqlodbc.wixobj
		if ($LASTEXITCODE -ne 0) {
			throw "Failed to link installer database"
		}

		Write-Host ".`nModifying psqlODBC installer database..."
		cscript modify_msi.vbs $CPUTYPE\tdeforpg_psqlodbc_${VERSION}_$CPUTYPE.msi
		if ($LASTEXITCODE -ne 0) {
			throw "Failed to modify installer database"
		}

		Write-Host ".`nDone!`n"
	}
	catch [Exception] {
		Write-Host ".`Aborting build!"
		throw $error[0]
	}
	finally {
		popd
	}
}

$scriptPath = (Split-Path $MyInvocation.MyCommand.Path)
$modulePath="${scriptPath}\..\winbuild"
Import-Module ${modulePath}\Psqlodbc-config.psm1
$defaultConfigDir=$modulePath
$configInfo = LoadConfiguration $BuildConfigPath $defaultConfigDir

if ($AlongWithDrivers) {
	try {
		pushd "$scriptpath"
		$platform = $cpu
		if ($cpu -eq "x86") {
			$platform = "win32"
		}
		..\winbuild\BuildAll.ps1 -Platform $platform -BuildConfigPath "$BuildConfigPath"
		if ($LASTEXITCODE -ne 0) {
			throw "Failed to build binaries"
		}
	} catch [Exception] {
		throw $error[0]
	} finally {
		popd
	} 
}

Import-Module ${scriptPath}\..\winbuild\MSProgram-Get.psm1
try {
	$dumpbinexe = Find-Dumpbin

	$wRedist=$false
	$VERSION = GetPackageVersion $configInfo "$scriptPath/.."
	if ($cpu -eq "both") {
		buildInstaller "x86"
		buildInstaller "x64"
		write-host "wRedist=$wRedist"
		try {
			pushd "$scriptPath"
			psqlodbc-setup\buildBootstrapper.ps1 -version $VERSION -withRedist:$wRedist
			if ($LASTEXITCODE -ne 0) {
				throw "Failed to build bootstrapper"
			}
		} catch [Exception] {
			throw $error[0]
		} finally {
			popd
		} 
	}
	else {
		buildInstaller $cpu
	}
} catch [Exception] {
	throw $error[0]
} finally {
	Remove-Module MSProgram-Get
	Remove-Module Psqlodbc-config
}
