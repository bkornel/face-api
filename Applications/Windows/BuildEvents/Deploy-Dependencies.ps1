<#
.SYNOPSIS
Copies the OpenCV and Poco runtime binaries plus the test configurations next to the
built executable. Runs as the pre-build event of FaceApp.

.DESCRIPTION
Every path is derived from this script's own location, so the working directory the build
happens to start in does not matter. Missing runtime libraries fail the build instead of
producing an executable that cannot start.
#>
[CmdletBinding()]
param(
  [Parameter(Mandatory = $true)]
  [ValidateSet('Debug', 'Release')]
  [string] $Configuration,

  [ValidateSet('x64')]
  [string] $Platform = 'x64'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$windowsDir = Split-Path -Parent $PSScriptRoot                       # Applications\Windows
$repoRoot = Split-Path -Parent (Split-Path -Parent $windowsDir)      # face-api
$thirdParty = Join-Path (Split-Path -Parent $repoRoot) '3rdparty'

$outDir = Join-Path $windowsDir "Bin\$Configuration"
$configSrc = Join-Path $repoRoot 'Testing\configurations'
$configDst = Join-Path $outDir 'configurations'

$opencvVersion = 'opencv-4.14.0'
$opencvAbi = '4140'
$opencvBin = Join-Path $thirdParty "$opencvVersion\windows\$Platform\vc16\bin"
$pocoBin = Join-Path $thirdParty "poco-1.10.1\windows\$Platform\vc16\bin"

# Debug builds of OpenCV and Poco carry a 'd' suffix
$suffix = ''
if ($Configuration -eq 'Debug') { $suffix = 'd' }

# One 'world' library carries every module in this distribution
$libraries = @(Join-Path $opencvBin "opencv_world$opencvAbi$suffix.dll")

# The ffmpeg backend is not built per configuration, there is only one flavour of it
$libraries += (Join-Path $opencvBin "opencv_videoio_ffmpeg${opencvAbi}_64.dll")

foreach ($p in @('PocoFoundation', 'PocoUtil', 'PocoXML', 'PocoJSON')) {
  $libraries += (Join-Path $pocoBin "$p$suffix.dll")
}

foreach ($dir in @($opencvBin, $pocoBin)) {
  if (-not (Test-Path -LiteralPath $dir)) {
    Write-Error "Dependency directory not found: $dir`nSee the Prerequisites section of README.md for the expected layout."
  }
}

if (-not (Test-Path -LiteralPath $outDir)) {
  New-Item -ItemType Directory -Path $outDir -Force | Out-Null
}

# Copy a file only when it is newer than the one already deployed, like xcopy /D
function Copy-IfNewer {
  param([string] $Source, [string] $DestinationDir)

  if (-not (Test-Path -LiteralPath $Source)) { return $false }

  $target = Join-Path $DestinationDir (Split-Path -Leaf $Source)

  if (Test-Path -LiteralPath $target) {
    $src = Get-Item -LiteralPath $Source
    $dst = Get-Item -LiteralPath $target
    if ($dst.LastWriteTimeUtc -ge $src.LastWriteTimeUtc) { return $false }
  }

  Copy-Item -LiteralPath $Source -Destination $target -Force
  return $true
}

$copied = 0
$missing = @()

foreach ($lib in $libraries) {
  if (-not (Test-Path -LiteralPath $lib)) {
    $missing += $lib
    continue
  }

  if (Copy-IfNewer -Source $lib -DestinationDir $outDir) { $copied++ }

  # Symbols are optional, not every prebuilt library ships them
  $pdb = [System.IO.Path]::ChangeExtension($lib, '.pdb')
  if (Copy-IfNewer -Source $pdb -DestinationDir $outDir) { $copied++ }
}

if ($missing.Count -gt 0) {
  Write-Error ("Missing runtime libraries:`n  " + ($missing -join "`n  "))
}

if (-not (Test-Path -LiteralPath $configSrc)) {
  Write-Error "Configuration directory not found: $configSrc"
}

New-Item -ItemType Directory -Path $configDst -Force | Out-Null
Copy-Item -Path (Join-Path $configSrc '*') -Destination $configDst -Recurse -Force

Write-Host "Deploy-Dependencies: $Configuration|$Platform - $copied binaries updated, configurations synced to $configDst"
