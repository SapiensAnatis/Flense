<#
.SYNOPSIS
    Benchmarks `dive` against the same test images used by Flense.Benchmarks.
.DESCRIPTION
    Times `dive --ci` against each of the .tar files Flense.Benchmarks measures, so the README's
    comparison table is produced the same honest way: every run reads from disk rather than the OS
    page cache, matching the FILE_FLAG_NO_BUFFERING approach Flense.Benchmarks uses internally.

    dive is a black box - there is no equivalent of FILE_FLAG_NO_BUFFERING we can hand it - so the
    only way to force a genuinely cold read for it is to drop the OS's file cache system-wide before
    every single run. This script does that via SetSystemFileCacheSize, which its own documentation
    calls out as a flush when both size parameters are (SIZE_T)-1. That's a real, machine-wide
    effect - it evicts every process's cached file data, not just dive's - and it requires the
    SeIncreaseQuotaPrivilege privilege, so this script must run elevated (as Administrator).

    Because every run re-reads the full image from disk, this takes noticeably longer than a normal
    `dive` invocation - expect it to take minutes for the larger test images.

    Vibe coded by Claude Opus 5, but looks roughly correct.
.EXAMPLE
    .\benchmark-dive.ps1
    .\benchmark-dive.ps1 -Runs 3 -DivePath C:\tools\dive.exe
#>
[CmdletBinding()]
param(
    [string]$DivePath = 'dive',

    [ValidateRange(1, [int]::MaxValue)]
    [int]$Runs = 5,

    [string]$TestDataDirectory
)

$ErrorActionPreference = 'Stop'

$currentPrincipal = [Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $currentPrincipal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw 'This script must run elevated (as Administrator) - flushing the system file cache to force uncached reads requires it.'
}

if (-not (Get-Command $DivePath -ErrorAction SilentlyContinue)) {
    throw "Could not find '$DivePath'. Install dive (https://github.com/wagoodman/dive) or pass -DivePath."
}

if (-not $TestDataDirectory) {
    $TestDataDirectory = Join-Path (Split-Path -Parent $PSScriptRoot) 'TestData'
}

if (-not (Test-Path $TestDataDirectory)) {
    throw "TestData directory not found at '$TestDataDirectory'. Run TestData\GenerateTestData.ps1 first."
}

if (-not ('Flense.Benchmarks.CacheControl' -as [type])) {
    Add-Type -Language CSharp -TypeDefinition @'
using System;
using System.Runtime.InteropServices;

namespace Flense.Benchmarks
{
    // SetSystemFileCacheSize's own documentation calls this out directly: "To flush the cache,
    // specify (SIZE_T) -1" for both size parameters. That's the officially supported flush, as
    // opposed to the undocumented NtSetSystemInformation/MemoryPurgeStandbyList trick RAMMap's
    // "Empty Standby List" button uses. It needs SeIncreaseQuotaPrivilege enabled first.
    public static class CacheControl
    {
        private const uint TOKEN_ADJUST_PRIVILEGES = 0x0020;
        private const uint TOKEN_QUERY = 0x0008;
        private const uint SE_PRIVILEGE_ENABLED = 0x0002;

        [StructLayout(LayoutKind.Sequential)]
        private struct LUID
        {
            public uint LowPart;
            public int HighPart;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct LUID_AND_ATTRIBUTES
        {
            public LUID Luid;
            public uint Attributes;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct TOKEN_PRIVILEGES
        {
            public uint PrivilegeCount;
            public LUID_AND_ATTRIBUTES Privileges;
        }

        [DllImport("advapi32.dll", SetLastError = true)]
        private static extern bool OpenProcessToken(IntPtr processHandle, uint desiredAccess, out IntPtr tokenHandle);

        [DllImport("advapi32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        private static extern bool LookupPrivilegeValue(string systemName, string name, out LUID luid);

        [DllImport("advapi32.dll", SetLastError = true)]
        private static extern bool AdjustTokenPrivileges(IntPtr tokenHandle, bool disableAllPrivileges,
            ref TOKEN_PRIVILEGES newState, uint bufferLength, IntPtr previousState, IntPtr returnLength);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool SetSystemFileCacheSize(UIntPtr minimumFileCacheSize,
            UIntPtr maximumFileCacheSize, uint flags);

        [DllImport("kernel32.dll")]
        private static extern IntPtr GetCurrentProcess();

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool CloseHandle(IntPtr handle);

        public static void EnableFlushPrivilege()
        {
            IntPtr tokenHandle;
            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, out tokenHandle))
            {
                throw new InvalidOperationException("OpenProcessToken failed: " + Marshal.GetLastWin32Error());
            }

            try
            {
                LUID luid;
                if (!LookupPrivilegeValue(null, "SeIncreaseQuotaPrivilege", out luid))
                {
                    throw new InvalidOperationException("LookupPrivilegeValue failed: " + Marshal.GetLastWin32Error());
                }

                TOKEN_PRIVILEGES privileges = new TOKEN_PRIVILEGES
                {
                    PrivilegeCount = 1,
                    Privileges = new LUID_AND_ATTRIBUTES { Luid = luid, Attributes = SE_PRIVILEGE_ENABLED },
                };

                if (!AdjustTokenPrivileges(tokenHandle, false, ref privileges, 0, IntPtr.Zero, IntPtr.Zero))
                {
                    throw new InvalidOperationException("AdjustTokenPrivileges failed: " + Marshal.GetLastWin32Error());
                }
            }
            finally
            {
                CloseHandle(tokenHandle);
            }
        }

        public static void FlushSystemFileCache()
        {
            // UIntPtr.MaxValue isn't available on the .NET Framework Add-Type compiles against, so
            // (SIZE_T)-1 is built by hand, sized to the process's actual pointer width.
            UIntPtr sizeTMinusOne = IntPtr.Size == 8
                ? new UIntPtr(unchecked((ulong)-1))
                : new UIntPtr(unchecked((uint)-1));

            if (!SetSystemFileCacheSize(sizeTMinusOne, sizeTMinusOne, 0))
            {
                throw new InvalidOperationException("SetSystemFileCacheSize failed: " + Marshal.GetLastWin32Error());
            }
        }
    }
}
'@
}

[Flense.Benchmarks.CacheControl]::EnableFlushPrivilege()

$images = @(
    [pscustomobject]@{ Name = 'postgres:latest'; TarFile = 'postgres-latest.tar' }
    [pscustomobject]@{ Name = 'mcr.microsoft.com/devcontainers/cpp:latest'; TarFile = 'mcr-microsoft-com-devcontainers-cpp-latest.tar' }
    [pscustomobject]@{ Name = 'nvidia/cuda:latest'; TarFile = 'cuda.tar' }
)

function Invoke-DiveRun {
    param(
        [string]$TarPath
    )

    $stdout = [System.IO.Path]::GetTempFileName()
    $stderr = [System.IO.Path]::GetTempFileName()

    try {
        # Flushed immediately before every run, not just once - dive uses ordinary buffered I/O, so
        # without this, only the first of these runs would ever be genuinely cold.
        [Flense.Benchmarks.CacheControl]::FlushSystemFileCache()

        # dive was installed via winget, so $DivePath can resolve to a symlink or App Execution
        # Alias rather than the real binary - the process Start-Process hands back is not reliably
        # the one doing the work, so PeakWorkingSet64 on it can't be trusted. Poll by image name
        # instead, which finds whichever process is actually running regardless of how it got there.
        $diveProcessName = [System.IO.Path]::GetFileNameWithoutExtension($DivePath)
        $peakWorkingSet = 0L

        $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
        $process = Start-Process -FilePath $DivePath -ArgumentList @("docker-archive://$TarPath", '--ci') `
            -NoNewWindow -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr

        do {
            $running = Get-Process -Name $diveProcessName -ErrorAction SilentlyContinue
            foreach ($candidate in $running) {
                if ($candidate.WorkingSet64 -gt $peakWorkingSet) {
                    $peakWorkingSet = $candidate.WorkingSet64
                }
            }

            Start-Sleep -Milliseconds 50
        } while ($running -or -not $process.HasExited)

        $process.WaitForExit()
        $stopwatch.Stop()
        $exitCode = $process.ExitCode

        if ($exitCode -ne 0) {
            # --ci fails (exit 1) whenever the image doesn't meet dive's efficiency thresholds -
            # that's its designed behavior, not a crash, and the analysis still ran to completion
            # before the gate was evaluated, so the timing/memory sample is still valid. dive's own
            # output stays suppressed either way - only the exit code and the log locations are
            # surfaced, so a genuine early failure is still noticeable without dive's report text
            # spamming the console on every expected --ci threshold failure.
            Write-Warning ("dive exited with code $exitCode for '$TarPath' - likely a --ci threshold " +
                "failure rather than a crash, so the sample is kept. Output logged to '$stdout' / '$stderr'.")
        }

        [pscustomobject]@{
            ElapsedMs      = $stopwatch.Elapsed.TotalMilliseconds
            PeakWorkingSet = $peakWorkingSet
            ExitCode       = $exitCode
        }
    }
    finally {
        # Keep the logs around when something looked wrong, so the warning above points somewhere
        # useful; a clean run's output is genuinely of no further use.
        if ($exitCode -eq 0) {
            Remove-Item $stdout, $stderr -ErrorAction SilentlyContinue
        }
    }
}

function Get-Median {
    param([double[]]$Values)

    $sorted = $Values | Sort-Object
    return $sorted[[math]::Floor($sorted.Count / 2)]
}

$results = foreach ($image in $images) {
    $tarPath = Join-Path $TestDataDirectory $image.TarFile
    if (-not (Test-Path $tarPath)) {
        Write-Warning "Skipping $($image.Name): '$tarPath' not found."
        continue
    }

    Write-Host "Benchmarking $($image.Name) with dive ($Runs cold runs)..."

    $samples = for ($run = 1; $run -le $Runs; $run += 1) {
        $sample = Invoke-DiveRun -TarPath $tarPath
        Write-Host ("  run {0}/{1} - {2:N2} s, {3:N1} MiB" -f $run, $Runs, ($sample.ElapsedMs / 1000),
            ($sample.PeakWorkingSet / 1MB))
        $sample
    }

    [pscustomobject]@{
        Name              = $image.Name
        TarSizeMiB        = (Get-Item $tarPath).Length / 1MB
        MedianSeconds     = (Get-Median -Values $samples.ElapsedMs) / 1000
        PeakWorkingSetMiB = ($samples.PeakWorkingSet | Measure-Object -Maximum).Maximum / 1MB
    }
}

$results | Format-Table -AutoSize Name,
    @{ Label = 'Tar Size'; Expression = { '{0:N0} MiB' -f $_.TarSizeMiB } },
    @{ Label = 'Median Parse Time'; Expression = { '{0:N2} s' -f $_.MedianSeconds } },
    @{ Label = 'Peak Memory'; Expression = { '{0:N1} MiB' -f $_.PeakWorkingSetMiB } }
