<#
.SYNOPSIS
    Builds Flense.
.DESCRIPTION
    Self-contained and location-independent: works from any shell, on the host or
    inside the dev container. Finds Visual Studio itself if the MSVC environment
    is not already set, and resolves all paths relative to this script.

    By default, compiled binaries land in-tree, exactly as Visual Studio would place them, but you can 
    pass -OutputDirectory to also move compiled binaries out from under the repo, e.g. so builds from
    this script never touch (or get clobbered by) a Visual Studio build of the same checkout.
.EXAMPLE
    .\build-app.ps1
    .\build-app.ps1 -Configuration Release
    .\build-app.ps1 OutputDirectory C:\build\x64
#>
[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',

    [ValidateSet('x64', 'Win32', 'ARM64')]
    [string]$Platform = 'x64',

    [string]$OutputDirectory
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot

# Only set the MSVC environment if it is not already present.
# VSCMD_VER is the documented marker VsDevCmd/Launch-VsDevShell set once they've
# initialized the environment.
if (-not $env:VSCMD_VER) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path $vswhere)) {
        throw 'vswhere.exe not found -- install Visual Studio or the VS Build Tools.'
    }

    $vsPath = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
    if (-not $vsPath) { throw 'No Visual Studio installation with MSBuild was found.' }

    & (Join-Path $vsPath 'Common7\Tools\Launch-VsDevShell.ps1') `
        -Arch amd64 -HostArch amd64 -SkipAutomaticLocation -NoLogo
}

$msbuildArgs = @(
    (Join-Path $repoRoot 'Flense.slnx')
    '/restore'
    "/p:Configuration=$Configuration"
    "/p:Platform=$Platform"
    '/p:AppxPackageSigningEnabled=false'
    '/p:GenerateAppxPackageOnBuild=false'
    '/m'
    '/v:minimal'
)

if ($OutputDirectory) {
    $msbuildArgs += "/p:FlenseOutputDirectory=$OutputDirectory"
}

msbuild @msbuildArgs
if ($LASTEXITCODE -ne 0) { throw "Build failed ($LASTEXITCODE)" }

