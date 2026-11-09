<#
.SYNOPSIS
    Builds the small Dockerfile-based images that Flense.Core.Tests parses.
.DESCRIPTION
    Writes one .tar per Dockerfile into TestData\Images, plus a copy of DeleteTest whose layers are all compressed
    with zstd. Needs Docker with a BuildKit builder that can build Linux images, so CI runs it on a Linux runner and
    hands the results to the Windows test job as an artifact.
#>

$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $true

$OutputDirectory = Join-Path $PSScriptRoot 'Images'
New-Item -ItemType Directory -Force $OutputDirectory | Out-Null

function Build-Image([string]$Dockerfile, [string]$Name, [string]$ExporterOptions = '') {
    $dest = Join-Path $OutputDirectory "$Name.tar"
    docker buildx build --provenance=false -f $Dockerfile -o "type=docker,dest=$dest$ExporterOptions" $PSScriptRoot
    Write-Host "Saved $Name to $dest"
}

Get-ChildItem -Path $PSScriptRoot -Filter '*.dockerfile' | ForEach-Object {
    Build-Image $_.FullName $_.BaseName
}

# force-compression recompresses the base image's gzip layers too, so that every layer in the archive is zstd.
Build-Image (Join-Path $PSScriptRoot 'DeleteTest.dockerfile') 'DeleteTest.zstd' ',compression=zstd,force-compression=true,oci-mediatypes=true'
