<#
.SYNOPSIS
    Generates small image archives that each fail to load in a different way.
.DESCRIPTION
    Writes one .tar per failure case into TestData\ErrorCases, for checking the load-failure dialog by hand. They're
    assembled with tar.exe rather than docker save, so Docker isn't needed. ErrorCases is a subdirectory so that
    Flense.Benchmarks, which only looks in TestData itself, doesn't try to benchmark them.

    valid.tar loads successfully, as a baseline that the hand-built image layout is right.
#>

$ErrorActionPreference = 'Stop'

. (Join-Path $PSScriptRoot 'ImageTools.ps1')

$OutputDirectory = Join-Path $PSScriptRoot 'ErrorCases'
$RepoTag = 'flense-error-case:latest'
$MissingBlobPath = 'blobs/sha256/' + ('0' * 64)

New-Item -ItemType Directory -Force $OutputDirectory | Out-Null

function New-RandomLayerBytes([int]$Size) {
    $content = [byte[]]::new($Size)
    [System.Random]::new(1).NextBytes($content)
    return , (New-LayerBytes @{ 'file.bin' = $content })
}

function Add-ValidImage([string]$Staging, [int]$LayerSize = 1024) {
    $layerPath = Add-Blob $Staging (New-RandomLayerBytes $LayerSize)
    $configPath = Add-Blob $Staging (New-ConfigBytes -Commands @('COPY file.bin /'))
    Set-Manifest $Staging $configPath @($layerPath) $RepoTag
}

function Write-ErrorCase([string]$Name, [scriptblock]$Populate) {
    return Write-Image (Join-Path $OutputDirectory "$Name.tar") $Populate
}

Write-ErrorCase 'valid' { param($Staging) Add-ValidImage $Staging } | Out-Null

# ArchiveError (Unreadable): not an archive at all.
$notAnArchivePath = Join-Path $OutputDirectory 'not-an-archive.tar'
[System.IO.File]::WriteAllText($notAnArchivePath, "This is not a tar archive.`n", $Utf8)
Write-Host "Wrote $notAnArchivePath"

# ArchiveError (Corrupt): a valid image cut off part-way through its layer blob.
$truncatedPath = Write-ErrorCase 'truncated' { param($Staging) Add-ValidImage $Staging -LayerSize (256 * 1024) }
$stream = [System.IO.File]::Open($truncatedPath, 'Open')
try {
    $stream.SetLength([long]($stream.Length / 2))
}
finally {
    $stream.Dispose()
}

# ArchiveError (Unreadable), inside a layer: a layer blob that isn't a tar.
Write-ErrorCase 'corrupt-layer' {
    param($Staging)
    $layerPath = Add-Blob $Staging $Utf8.GetBytes("This is not a tar archive.`n")
    $configPath = Add-Blob $Staging (New-ConfigBytes -Commands @('COPY file.bin /'))
    Set-Manifest $Staging $configPath @($layerPath) $RepoTag
} | Out-Null

# ImageFormatError (NoManifest): a valid tar with nothing image-shaped in it.
Write-ErrorCase 'no-manifest' {
    param($Staging)
    [System.IO.File]::WriteAllText((Join-Path $Staging 'readme.txt'), "Just a tar.`n", $Utf8)
} | Out-Null

# ImageFormatError (MalformedMetadata), in manifest.json.
Write-ErrorCase 'malformed-manifest' {
    param($Staging)
    Add-ValidImage $Staging
    [System.IO.File]::WriteAllText((Join-Path $Staging 'manifest.json'), '[{"Config": ', $Utf8)
} | Out-Null

# ImageFormatError (MissingConfiguration): the manifest's Config names a blob that isn't there.
Write-ErrorCase 'missing-config' {
    param($Staging)
    $layerPath = Add-Blob $Staging (New-RandomLayerBytes 1024)
    Set-Manifest $Staging $MissingBlobPath @($layerPath) $RepoTag
} | Out-Null

# ImageFormatError (MalformedMetadata), in the config: a history entry whose created_by isn't a string.
Write-ErrorCase 'malformed-config' {
    param($Staging)
    $layerPath = Add-Blob $Staging (New-RandomLayerBytes 1024)
    $configPath = Add-Blob $Staging (ConvertTo-JsonBytes @{ architecture = 'amd64'; os = 'linux'; history = @(@{ created_by = 42 }) })
    Set-Manifest $Staging $configPath @($layerPath) $RepoTag
} | Out-Null

# ImageFormatError (MissingLayers): the manifest lists a layer blob that isn't there.
Write-ErrorCase 'missing-layer-blob' {
    param($Staging)
    $configPath = Add-Blob $Staging (New-ConfigBytes -Commands @('COPY file.bin /'))
    Set-Manifest $Staging $configPath @($MissingBlobPath) $RepoTag
} | Out-Null
