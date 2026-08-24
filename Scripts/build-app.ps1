<#
.SYNOPSIS
    Builds Flense.
.DESCRIPTION
    Self-contained and location-independent: works from any shell, on the host or
    inside the dev container. Finds Visual Studio itself if the MSVC environment
    is not already set, and resolves all paths relative to this script.

    Output goes in-tree, exactly as Visual Studio would place it.
.EXAMPLE
    .\build-app.ps1
    .\build-app.ps1 -Configuration Release
#>
[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',

    [ValidateSet('x64', 'Win32', 'ARM64')]
    [string]$Platform = 'x64'
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

msbuild @msbuildArgs
if ($LASTEXITCODE -ne 0) { throw "Build failed ($LASTEXITCODE)" }

Write-Host ''
Write-Host "Output: $repoRoot\$Platform\$Configuration" -ForegroundColor Green
