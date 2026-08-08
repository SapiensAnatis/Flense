$Images = @(
    'postgres:latest',
    'mcr.microsoft.com/devcontainers/cpp:latest'
)

docker info *> $null
if ($LASTEXITCODE -ne 0) {
    Write-Error "Docker Desktop does not appear to be running. Start Docker Desktop and try again."
    exit 1
}

foreach ($ImageName in $Images) {
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