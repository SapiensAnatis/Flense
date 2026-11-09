$ErrorActionPreference = 'Stop'

if (-not (Get-Command ko -ErrorAction SilentlyContinue)) {
    Write-Error "ko was not found on PATH. Install it with 'go install github.com/google/ko@latest' and try again."
    exit 1
}

$OutputDirectory = Join-Path $PSScriptRoot 'Images'
New-Item -ItemType Directory -Force $OutputDirectory | Out-Null
$OutputFile = Join-Path $OutputDirectory 'ko.tar'
$PreviousRepo = $env:KO_DOCKER_REPO

# ko needs a repository name to tag the image with, even when it isn't pushing anywhere. It can't be ko.local,
# since that makes ko load the image into Docker and ignore --tarball.
$env:KO_DOCKER_REPO = 'flense.test/ko'
Push-Location (Join-Path $PSScriptRoot 'GoProject')
try {
    ko build --bare --push=false --tarball=$OutputFile .
    if ($LASTEXITCODE -ne 0) {
        Write-Error "ko build failed."
        exit 1
    }
}
finally {
    Pop-Location
    $env:KO_DOCKER_REPO = $PreviousRepo
}

Write-Host "Saved ko image to $OutputFile"
