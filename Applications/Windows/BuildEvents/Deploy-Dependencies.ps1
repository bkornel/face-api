<#
.SYNOPSIS
Copies the OpenCV and Poco runtime binaries, the shared model files and each application's
own settings next to the built executables. Runs as a pre-build event.

.DESCRIPTION
Every path is derived from this script's own location, so the working directory the build
happens to start in does not matter. Missing runtime libraries fail the build instead of
producing an executable that cannot start.

The model files are shared - they are large and identical for every host - but the settings
are not: the console application and Face Studio configure the graph differently, and one
deployment overwriting the other's settings would be a puzzle to debug. Each therefore gets
its own working directory next to the executable, seeded from its own project, and only
when the file in the project is the newer of the two - so that a setting changed in a
deployed copy, by hand or by the settings editor, survives the next build.
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

# The models, shared by every application
$configSrc = Join-Path $repoRoot 'Testing\configurations'
$configDst = Join-Path $outDir 'configurations'

# One working directory per application, each holding only that application's settings.json
$appConfigs = @(
  @{ Source = Join-Path $windowsDir 'FaceApp\Configurations'; Destination = Join-Path $outDir 'faceapp' }
  @{ Source = Join-Path $windowsDir 'FaceStudio\Configurations'; Destination = Join-Path $outDir 'studio' }
)

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

$seeded = 0

foreach ($app in $appConfigs) {
  if (-not (Test-Path -LiteralPath $app.Source)) { continue }

  New-Item -ItemType Directory -Path $app.Destination -Force | Out-Null

  foreach ($file in Get-ChildItem -LiteralPath $app.Source -File) {
    if (Copy-IfNewer -Source $file.FullName -DestinationDir $app.Destination) { $seeded++ }
  }
}

Write-Host "Deploy-Dependencies: $Configuration|$Platform - $copied binaries updated, models synced to $configDst, $seeded application settings seeded"
