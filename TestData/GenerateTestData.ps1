$CotsImages = @(
    'postgres:latest',
    'mcr.microsoft.com/devcontainers/cpp:latest'
)

docker info *> $null
if ($LASTEXITCODE -ne 0) {
    Write-Error "Docker Desktop does not appear to be running. Start Docker Desktop and try again."
    exit 1
}

foreach ($ImageName in $CotsImages) {
    $OutputFile = Join-Path $PSScriptRoot (($ImageName -replace '[:/.]', '-') + '.tar')

    if (Test-Path $OutputFile) {
        Write-Host "Skipping $ImageName, $OutputFile already exists."
        continue
    }

    docker pull $ImageName
    docker save -o $OutputFile $ImageName

    if ($LASTEXITCODE -ne 0) {
        Write-Error "docker save failed for $ImageName."
        exit 1
    }

    Write-Host "Saved $ImageName to $OutputFile"
}

& (Join-Path $PSScriptRoot 'GenerateTestImages.ps1')
& (Join-Path $PSScriptRoot 'GenerateHandBuiltImages.ps1')
& (Join-Path $PSScriptRoot 'GenerateErrorCases.ps1')

if (Get-Command ko -ErrorAction SilentlyContinue) {
    & (Join-Path $PSScriptRoot 'GenerateKoImage.ps1')
}
else {
    Write-Warning "ko was not found on PATH, so Images\ko.tar was not generated."
}
