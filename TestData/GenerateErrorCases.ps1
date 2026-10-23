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

$OutputDirectory = Join-Path $PSScriptRoot 'ErrorCases'
$Tar = Join-Path $env:SystemRoot 'System32\tar.exe'
$Utf8 = [System.Text.UTF8Encoding]::new($false)
$MissingBlobPath = 'blobs/sha256/' + ('0' * 64)

New-Item -ItemType Directory -Force $OutputDirectory | Out-Null

function New-StagingDirectory {
    $path = Join-Path ([System.IO.Path]::GetTempPath()) ([System.IO.Path]::GetRandomFileName())
    New-Item -ItemType Directory $path | Out-Null
    return $path
}

function ConvertTo-JsonBytes($Value) {
    # The leading comma stops PowerShell unrolling the byte array into an object[] on return.
    return , $Utf8.GetBytes((ConvertTo-Json -InputObject $Value -Depth 10 -Compress))
}

function Add-Blob([string]$Staging, [byte[]]$Content) {
    $digest = [System.Convert]::ToHexString([System.Security.Cryptography.SHA256]::HashData($Content)).ToLowerInvariant()
    $blobDirectory = Join-Path $Staging 'blobs/sha256'
    New-Item -ItemType Directory -Force $blobDirectory | Out-Null
    [System.IO.File]::WriteAllBytes((Join-Path $blobDirectory $digest), $Content)
    return "blobs/sha256/$digest"
}

function New-LayerBytes([int]$Size) {
    $layerStaging = New-StagingDirectory
    try {
        $content = [byte[]]::new($Size)
        [System.Random]::new(1).NextBytes($content)
        [System.IO.File]::WriteAllBytes((Join-Path $layerStaging 'file.bin'), $content)

        $layerTar = Join-Path $layerStaging 'layer.tar'
        & $Tar -cf $layerTar -C $layerStaging 'file.bin'
        return , [System.IO.File]::ReadAllBytes($layerTar)
    }
    finally {
        Remove-Item -Recurse -Force $layerStaging
    }
}

function New-ConfigBytes([string[]]$Commands) {
    $history = @($Commands | ForEach-Object { @{ created_by = $_ } })
    return , (ConvertTo-JsonBytes @{ architecture = 'amd64'; os = 'linux'; history = $history })
}

function Set-Manifest([string]$Staging, [string]$ConfigPath, [string[]]$LayerPaths) {
    $manifest = @(@{ Config = $ConfigPath; RepoTags = @('flense-error-case:latest'); Layers = $LayerPaths })
    [System.IO.File]::WriteAllBytes((Join-Path $Staging 'manifest.json'), (ConvertTo-JsonBytes $manifest))
}

function Add-ValidImage([string]$Staging, [int]$LayerSize = 1024) {
    $layerPath = Add-Blob $Staging (New-LayerBytes $LayerSize)
    $configPath = Add-Blob $Staging (New-ConfigBytes @('COPY file.bin /'))
    Set-Manifest $Staging $configPath @($layerPath)
}

function Write-Image([string]$Name, [scriptblock]$Populate) {
    $staging = New-StagingDirectory
    try {
        & $Populate $staging

        # Entries are named explicitly, since archiving "." would prefix every path with "./".
        $entries = @(Get-ChildItem $staging | ForEach-Object Name)
        $outputPath = Join-Path $OutputDirectory "$Name.tar"
        & $Tar -cf $outputPath -C $staging @entries
        if ($LASTEXITCODE -ne 0) {
            throw "tar failed for $Name."
        }

        Write-Host "Wrote $outputPath"
        return $outputPath
    }
    finally {
        Remove-Item -Recurse -Force $staging
    }
}

Write-Image 'valid' { param($Staging) Add-ValidImage $Staging } | Out-Null

# ArchiveError (Unreadable): not an archive at all.
$notAnArchivePath = Join-Path $OutputDirectory 'not-an-archive.tar'
[System.IO.File]::WriteAllText($notAnArchivePath, "This is not a tar archive.`n", $Utf8)
Write-Host "Wrote $notAnArchivePath"

# ArchiveError (Corrupt): a valid image cut off part-way through its layer blob.
$truncatedPath = Write-Image 'truncated' { param($Staging) Add-ValidImage $Staging -LayerSize (256 * 1024) }
$stream = [System.IO.File]::Open($truncatedPath, 'Open')
try {
    $stream.SetLength([long]($stream.Length / 2))
}
finally {
    $stream.Dispose()
}

# ArchiveError (Unreadable), inside a layer: a layer blob that isn't a tar.
Write-Image 'corrupt-layer' {
    param($Staging)
    $layerPath = Add-Blob $Staging $Utf8.GetBytes("This is not a tar archive.`n")
    $configPath = Add-Blob $Staging (New-ConfigBytes @('COPY file.bin /'))
    Set-Manifest $Staging $configPath @($layerPath)
} | Out-Null

# ImageFormatError (NoManifest): a valid tar with nothing image-shaped in it.
Write-Image 'no-manifest' {
    param($Staging)
    [System.IO.File]::WriteAllText((Join-Path $Staging 'readme.txt'), "Just a tar.`n", $Utf8)
} | Out-Null

# ImageFormatError (MalformedMetadata), in manifest.json.
Write-Image 'malformed-manifest' {
    param($Staging)
    Add-ValidImage $Staging
    [System.IO.File]::WriteAllText((Join-Path $Staging 'manifest.json'), '[{"Config": ', $Utf8)
} | Out-Null

# ImageFormatError (MissingConfiguration): the manifest's Config names a blob that isn't there.
Write-Image 'missing-config' {
    param($Staging)
    $layerPath = Add-Blob $Staging (New-LayerBytes 1024)
    Set-Manifest $Staging $MissingBlobPath @($layerPath)
} | Out-Null

# ImageFormatError (MalformedMetadata), in the config: no history array.
Write-Image 'malformed-config' {
    param($Staging)
    $layerPath = Add-Blob $Staging (New-LayerBytes 1024)
    $configPath = Add-Blob $Staging (ConvertTo-JsonBytes @{ architecture = 'amd64'; os = 'linux' })
    Set-Manifest $Staging $configPath @($layerPath)
} | Out-Null

# ImageFormatError (MissingLayers): the config's history has more layers than the manifest lists.
Write-Image 'fewer-layers' {
    param($Staging)
    $layerPath = Add-Blob $Staging (New-LayerBytes 1024)
    $configPath = Add-Blob $Staging (New-ConfigBytes @('COPY file.bin /', 'RUN echo second layer'))
    Set-Manifest $Staging $configPath @($layerPath)
} | Out-Null

# ImageFormatError (MissingLayers): the manifest lists a layer blob that isn't there.
Write-Image 'missing-layer-blob' {
    param($Staging)
    $configPath = Add-Blob $Staging (New-ConfigBytes @('COPY file.bin /'))
    Set-Manifest $Staging $configPath @($MissingBlobPath)
} | Out-Null
