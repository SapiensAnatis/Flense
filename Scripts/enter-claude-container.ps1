<#
.SYNOPSIS
    Enters a Claude Code session in an isolated container.
.DESCRIPTION
    Starts a container using Container\docker-compose.yaml and begins a Claude Code session inside.
#>

$ErrorActionPreference = "Stop"

$composeFile = "$PSScriptRoot\..\Container\docker-compose.yaml"

docker compose -f "$composeFile" up -d


$volumeMounts = @("C:\Users\ContainerUser\.claude", "C:\Users\ContainerUser\.nuget\packages")

# The mounted volumes have the wrong permissions by default
foreach ($volumeMount in $volumeMounts) {
    docker compose -f "$composeFile" exec -u ContainerAdministrator dev icacls $volumeMount /grant 'ContainerUser:(OI)(CI)M' /T /Q 
}

docker compose -f "$composeFile" exec dev claude