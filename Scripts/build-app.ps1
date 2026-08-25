<#
.SYNOPSIS
    Builds Flense.
.DESCRIPTION
    Self-contained and location-independent: works from any shell, on the host or
    inside the dev container. Finds Visual Studio itself if the MSVC environment
    is not already set, and resolves all paths relative to this script.

    Compiled binaries land in-tree, exactly as Visual Studio would place them, but NuGet restore output
    (project.assets.json etc.) goes to a dedicated folder under the repo -- see -IntermediateOutputRoot
    below -- rather than each project's own obj\, since this repo can be built by more than one Windows
    account against the same checkout (e.g. a host user and a sandboxed coding-agent account), and those
    accounts can't read each other's NuGet package cache. Restore output keyed to one account left in
    place for another to find looks like a normal, unchanged file, but its cached package paths are
    unreadable -- forcing a full re-restore and rebuild that has nothing to do with what actually changed.
.EXAMPLE
    .\build-app.ps1
    .\build-app.ps1 -Configuration Release
#>
[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',

    [ValidateSet('x64', 'Win32', 'ARM64')]
    [string]$Platform = 'x64',

    # See Directory.Build.props: redirected per-project (via $(MSBuildProjectName)) so different projects
    # in the solution don't collide here. Gitignored via the existing [Oo]bj/ rule.
    [string]$IntermediateOutputRoot = (Join-Path (Split-Path -Parent $PSScriptRoot) '.msbuild\obj')
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
    "/p:FlenseIntermediateOutputRoot=$IntermediateOutputRoot"
    '/p:AppxPackageSigningEnabled=false'
    '/p:GenerateAppxPackageOnBuild=false'
    '/m'
    '/v:minimal'
)

msbuild @msbuildArgs
if ($LASTEXITCODE -ne 0) { throw "Build failed ($LASTEXITCODE)" }

Write-Host ''
Write-Host "Output: $repoRoot\$Platform\$Configuration" -ForegroundColor Green
