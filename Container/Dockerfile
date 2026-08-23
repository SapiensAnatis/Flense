# escape=`

# Build:  docker compose -f Container/docker-compose.yaml build
#
# Needs process isolation (compose sets it) or "docker build -m 4GB": the VS
# installer wants >2 GB and Hyper-V isolation caps build containers at 1023 MB.
# Do not use BuildKit - support is extremely limited for Windows containers:
# https://docs.docker.com/build/buildkit/#buildkit-on-windows

# Overridable: process isolation needs compatible host/container Windows builds.
ARG FROM_IMAGE=mcr.microsoft.com/dotnet/framework/runtime:4.8.1-windowsservercore-ltsc2025

# .NET Framework base for MSBuild
FROM ${FROM_IMAGE} AS toolchain

SHELL ["powershell", "-NoLogo", "-ExecutionPolicy", "Bypass", "-Command", "$ErrorActionPreference='Stop'; $ProgressPreference='SilentlyContinue';"]

COPY vs.vsconfig C:/TEMP/

# Pinned to a specific release at the following URL to maximise caching of the giga layer
# https://learn.microsoft.com/visualstudio/releases/2026/release-history
ARG VS_BUILDTOOLS_URL=https://download.visualstudio.microsoft.com/download/pr/7834fc97-5a8c-4392-a2a9-ed4b98f77180/e1dcfb35b5b7a285cdf3f877d0bc835c5373999ce0fc78b637c7d5fea2966c05/vs_BuildTools.exe

# Start-Process -Wait because the bootstrapper is a GUI-subsystem binary that
# returns immediately when invoked directly.
RUN Invoke-WebRequest -Uri $env:VS_BUILDTOOLS_URL -OutFile C:\TEMP\vs_buildtools.exe; `
    $p = Start-Process C:\TEMP\vs_buildtools.exe -Wait -PassThru -ArgumentList `
        '--quiet','--wait','--norestart','--nocache','install', `
        '--installPath','\"C:\Program Files (x86)\Microsoft Visual Studio\BuildTools\"', `
        '--config','C:\TEMP\vs.vsconfig'; `
    if ($p.ExitCode -notin 0,3010) { throw \"VS Build Tools install failed ($($p.ExitCode))\" }; `
    Remove-Item C:\TEMP\vs_buildtools.exe; `
    Remove-Item -Recurse -Force \"$env:ProgramData\Package Cache\" -ErrorAction SilentlyContinue

RUN if (-not (Test-Path 'C:\Program Files (x86)\Microsoft Visual Studio\BuildTools\MSBuild\Microsoft\VisualStudio\v18.0\AppxPackage')) { `
        throw 'AppxPackage MSBuild targets missing -- check component ids in vs.vsconfig' `
    }

FROM toolchain AS dev

USER ContainerUser

ENV PATH="C:\tools\git\cmd;C:\tools;C:\vcpkg;C:\Users\ContainerUser\.local\bin;C:\Windows\system32;C:\Windows;C:\Windows\System32\Wbem;C:\Windows\System32\WindowsPowerShell\v1.0;C:\Windows\Microsoft.NET\Framework64\v4.0.30319"
# Claude Code keeps .claude.json beside ~\.claude rather than inside it, so this
# is what pulls it into the claude-state volume and persists login across runs.
ENV CLAUDE_CONFIG_DIR="C:\Users\ContainerUser\.claude"
ENV VCPKG_ROOT="C:\vcpkg"
# Holds the pinned version.
ENV DISABLE_AUTOUPDATER=1

# git (MinGit: portable, no installer)
RUN Invoke-WebRequest -Uri 'https://github.com/git-for-windows/git/releases/download/v2.55.0.windows.5/MinGit-2.55.0.5-64-bit.zip' -OutFile C:\TEMP\git.zip; `
    Expand-Archive C:\TEMP\git.zip -DestinationPath C:\tools\git; `
    Remove-Item C:\TEMP\git.zip; `
    git config --global --add safe.directory C:/src

# NuGet CLI. VS ships NuGet targets but no nuget.exe, and "msbuild -t:restore"
# does not handle packages.config: it reports "Nothing to do" and exits 0, so
# dropping this would surface much later as missing ..\packages\... imports.
RUN Invoke-WebRequest -Uri 'https://dist.nuget.org/win-x86-commandline/v6.14.0/nuget.exe' -OutFile C:\tools\nuget.exe

RUN git clone https://github.com/microsoft/vcpkg C:\vcpkg; `
    & C:\vcpkg\bootstrap-vcpkg.bat -disableMetrics; `
    vcpkg integrate install

ARG CLAUDE_CODE_VERSION=2.1.241

RUN Invoke-WebRequest https://claude.ai/install.ps1 -OutFile C:\TEMP\claude-install.ps1; `
    & C:\TEMP\claude-install.ps1 $env:CLAUDE_CODE_VERSION; `
    Remove-Item C:\TEMP\claude-install.ps1

WORKDIR C:/src
