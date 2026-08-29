<#
.SYNOPSIS
    Enters a Claude Code session in an isolated container.
.DESCRIPTION
    Starts a container using Container\docker-compose.yaml and begins a Claude Code session inside.
#>

$ErrorActionPreference = 'Stop'

$composeFile = "$PSScriptRoot\..\Container\docker-compose.yaml"

docker compose -f "$composeFile" up -d
docker compose -f "$composeFile" exec dev claude