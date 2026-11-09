<#
.SYNOPSIS
    Assembles valid images that Docker can't produce, for Flense.Core.Tests to parse.
.DESCRIPTION
    Writes into TestData\Images. They're assembled with tar.exe rather than docker save, so Docker isn't needed.
#>

$ErrorActionPreference = 'Stop'

. (Join-Path $PSScriptRoot 'ImageTools.ps1')

$OutputDirectory = Join-Path $PSScriptRoot 'Images'
New-Item -ItemType Directory -Force $OutputDirectory | Out-Null

# NoHistoryTest.tar is a valid image whose config has no history array, which the OCI spec allows (buildah's
# --omit-history produces one). Docker always writes history, so it's assembled by hand instead.
Write-Image (Join-Path $OutputDirectory 'NoHistoryTest.tar') {
    param($Staging)
    $layerPaths = @(
        Add-Blob $Staging (New-LayerBytes @{ 'etc/app.conf' = "greeting = hello`n" })
        Add-Blob $Staging (New-LayerBytes @{ 'app/hello.txt' = "Hello from an image with no history.`n" })
    )
    $configPath = Add-Blob $Staging (New-ConfigBytes -LayerPaths $layerPaths)
    Set-Manifest $Staging $configPath $layerPaths 'flense.test/no-history:latest'
} | Out-Null

# ExtraHistoryTest.tar has more history entries than layers. It should load with every layer labelled as an unknown
# command, rather than pairing history entries with the wrong layers.
Write-Image (Join-Path $OutputDirectory 'ExtraHistoryTest.tar') {
    param($Staging)
    $layerPaths = @(Add-Blob $Staging (New-LayerBytes @{ 'app/hello.txt' = "Hello from an image with extra history.`n" }))
    $configPath = Add-Blob $Staging (New-ConfigBytes -Commands @('COPY hello.txt /app/', 'RUN echo second layer') -LayerPaths $layerPaths)
    Set-Manifest $Staging $configPath $layerPaths 'flense.test/extra-history:latest'
} | Out-Null
