<#
.SYNOPSIS
    Helpers for assembling docker save-style image archives by hand, without Docker.
.DESCRIPTION
    Dot-source this from a generation script. An image is built up in a staging directory, with blobs under
    blobs/sha256 and a manifest.json at the root, and then archived by Write-Image.
#>

$Tar = if ($IsWindows) { Join-Path $env:SystemRoot 'System32\tar.exe' } else { 'tar' }
$Utf8 = [System.Text.UTF8Encoding]::new($false)

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

# Files maps each path inside the layer to its contents, given as either a string or a byte array.
function New-LayerBytes([hashtable]$Files) {
    $layerStaging = New-StagingDirectory
    try {
        foreach ($path in $Files.Keys) {
            $filePath = Join-Path $layerStaging $path
            New-Item -ItemType Directory -Force (Split-Path $filePath) | Out-Null

            $content = $Files[$path]
            if ($content -is [byte[]]) {
                [System.IO.File]::WriteAllBytes($filePath, $content)
            }
            else {
                [System.IO.File]::WriteAllText($filePath, $content, $Utf8)
            }
        }

        $entries = @(Get-ChildItem $layerStaging | ForEach-Object Name)
        $layerTar = Join-Path $layerStaging 'layer.tar'
        & $Tar -cf $layerTar -C $layerStaging @entries
        return , [System.IO.File]::ReadAllBytes($layerTar)
    }
    finally {
        Remove-Item -Recurse -Force $layerStaging
    }
}

# History is written only if Commands is passed, and rootfs only if LayerPaths is. The layers are uncompressed, so
# each blob's digest is also its diff ID.
function New-ConfigBytes([string[]]$Commands, [string[]]$LayerPaths) {
    $config = [ordered]@{ architecture = 'amd64'; os = 'linux' }

    if ($PSBoundParameters.ContainsKey('LayerPaths')) {
        $diffIds = @($LayerPaths | ForEach-Object { 'sha256:' + (Split-Path -Leaf $_) })
        $config.rootfs = @{ type = 'layers'; diff_ids = $diffIds }
    }

    if ($PSBoundParameters.ContainsKey('Commands')) {
        $config.history = @($Commands | ForEach-Object { @{ created_by = $_ } })
    }

    return , (ConvertTo-JsonBytes $config)
}

function Set-Manifest([string]$Staging, [string]$ConfigPath, [string[]]$LayerPaths, [string]$RepoTag) {
    $manifest = @(@{ Config = $ConfigPath; RepoTags = @($RepoTag); Layers = $LayerPaths })
    [System.IO.File]::WriteAllBytes((Join-Path $Staging 'manifest.json'), (ConvertTo-JsonBytes $manifest))
}

# Populate is called with the staging directory to fill in, and its contents are then archived to OutputPath.
function Write-Image([string]$OutputPath, [scriptblock]$Populate) {
    $staging = New-StagingDirectory
    try {
        & $Populate $staging

        # Entries are named explicitly, since archiving "." would prefix every path with "./".
        $entries = @(Get-ChildItem $staging | ForEach-Object Name)
        & $Tar -cf $OutputPath -C $staging @entries
        if ($LASTEXITCODE -ne 0) {
            throw "tar failed for $OutputPath."
        }

        Write-Host "Wrote $OutputPath"
        return $OutputPath
    }
    finally {
        Remove-Item -Recurse -Force $staging
    }
}
