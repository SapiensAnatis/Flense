<#
.SYNOPSIS
    Enters a Claude Code session in an isolated container.
.DESCRIPTION
    Starts a container using Container\docker-compose.yaml and begins a Claude Code session inside.
#>

$ErrorActionPreference = 'Stop'

$composeFile = "$PSScriptRoot\..\Container\docker-compose.yaml"

docker compose -f "$composeFile" up -d

if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to start Docker Compose environment"
    exit 1
}

function Invoke-InContainer {
    param([string] $User, [Parameter(ValueFromRemainingArguments)] [string[]] $Args)
    docker compose -f "$composeFile" exec -u $User dev @Args
    return $LASTEXITCODE
}

$volumeMounts = @("C:\Users\ContainerUser\.claude", "C:\Users\ContainerUser\.nuget\packages")

# The mounted volumes have the wrong permissions by default
foreach ($volumeMount in $volumeMounts) {
    Invoke-InContainer -User ContainerAdministrator icacls $volumeMount /grant '*S-1-5-32-545:(OI)(CI)F' /T /Q

    if ($LASTEXITCODE -ne 0) {
        Write-Host "icacls grant failed on $volumeMount ($LASTEXITCODE); reassigning owner"

        Invoke-InContainer -User ContainerAdministrator takeown /F $volumeMount /A /R /D Y
        Invoke-InContainer -User ContainerAdministrator icacls $volumeMount /reset /T /C /Q
        Invoke-InContainer -User ContainerAdministrator icacls $volumeMount /inheritance:e /Q
        Invoke-InContainer -User ContainerAdministrator icacls $volumeMount /grant '*S-1-5-32-545:(OI)(CI)F' /T /C /Q
    }
}

Invoke-InContainer -User ContainerUser claude