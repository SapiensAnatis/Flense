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

# NoHistoryTest.tar is a valid image whose config has no history array, which the OCI spec allows (buildah's
# --omit-history produces one). Docker always writes history, so it's assembled by hand instead.
. (Join-Path $PSScriptRoot 'ImageTools.ps1')

Write-Image (Join-Path $PSScriptRoot 'NoHistoryTest.tar') {
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
Write-Image (Join-Path $PSScriptRoot 'ExtraHistoryTest.tar') {
    param($Staging)
    $layerPaths = @(Add-Blob $Staging (New-LayerBytes @{ 'app/hello.txt' = "Hello from an image with extra history.`n" }))
    $configPath = Add-Blob $Staging (New-ConfigBytes -Commands @('COPY hello.txt /app/', 'RUN echo second layer') -LayerPaths $layerPaths)
    Set-Manifest $Staging $configPath $layerPaths 'flense.test/extra-history:latest'
} | Out-Null
