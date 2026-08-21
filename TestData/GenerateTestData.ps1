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

Get-ChildItem -Path $PSScriptRoot -Filter "*.dockerfile" | ForEach-Object {
    $dest = (Join-Path $PSScriptRoot "$($_.BaseName).tar")
    docker build -f $_.FullName -o "type=docker,dest=$dest" $PSScriptRoot

    if ($LASTEXITCODE -ne 0) {
        Write-Error "docker build failed for $($_.Name)."
        exit 1
    }

    Write-Host "Saved $($_.Name) to $dest"
}
