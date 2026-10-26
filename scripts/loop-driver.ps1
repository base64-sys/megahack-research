#!/usr/bin/env pwsh
<#
.SYNOPSIS
Drive recurring agent iterations through the local OpenCode HTTP API.

.DESCRIPTION
Replacement for the loop scheduler in @prevalentware/opencode-loop-plugin when
its in-memory timers do not survive between requests. That plugin arms a
`setTimeout` per loop inside a plugin instance, keeps it in memory, and persists
the loop itself to %APPDATA%\opencode-loop-plugin\loops.json. In this
environment the instance dies before the timer fires, so `nextRunAt` sits in the
past with `runCount: 0` and `updatedAt` frozen. This script bypasses the plugin
entirely and uses the documented server API:

    POST /api/session/{id}/prompt          enqueue the iteration instruction
    POST /api/experimental/session/{id}/wait
                                           block until the agent loop is idle

Order matters: post, sleep the interval, then wait for idle. That keeps at most
one iteration queued, so a long turn cannot accumulate a backlog.

Safety: a stop file ends the loop between iterations, `-MaxRuns` caps the total,
and the PID is written so `scripts/listloops.ps1` can report it. Nothing is
deleted and no plugin state is modified.

.PARAMETER SessionID
Target session, e.g. ses_e85efe9a6ffeFHoFCCGQV2TBJf.

.PARAMETER IntervalSeconds
Delay between the end of a turn and the next prompt. Minimum 10.

.PARAMETER MaxRuns
Hard cap on iterations. Default 20.

.EXAMPLE
pwsh -File scripts/loop-driver.ps1 -SessionID ses_abc -IntervalSeconds 30
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$SessionID,
    [string]$InstructionPath = '.oc/loop-instruction.txt',
    [ValidateRange(10, 86400)][int]$IntervalSeconds = 30,
    [ValidateRange(1, 1000)][int]$MaxRuns = 20,
    [string]$StopFile = '.oc/loop.stop',
    [string]$PidPath = '.oc/loop-driver.pid',
    [string]$LogPath = '.oc/loop-driver.log',
    [string]$OpenCodeExe = ''
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

function Write-Log([string]$message) {
    $line = '{0} {1}' -f (Get-Date -Format 'yyyy-MM-dd HH:mm:ss'), $message
    Add-Content -LiteralPath $LogPath -Value $line -Encoding UTF8
    Write-Host $line
}

function Exit-Driver([string]$message, [int]$code) {
    Write-Log $message
    if (Test-Path -LiteralPath $PidPath) { Remove-Item -LiteralPath $PidPath -Force -ErrorAction SilentlyContinue }
    exit $code
}

function Invoke-OpenCodeApi {
    param([string]$CommandLine, [string]$Body)

    $argLine = $CommandLine
    if ($Body) {
        # PowerShell 5.1 drops embedded double quotes when it builds a native
        # command line, which turns JSON into garbage. Escape them and add the
        # surrounding quotes ourselves; CommandLineToArgvW restores them.
        $argLine = '{0} -d "{1}"' -f $CommandLine, ($Body -replace '"', '\"')
    }

    $outFile = [System.IO.Path]::GetTempFileName()
    $errFile = [System.IO.Path]::GetTempFileName()
    try {
        $process = Start-Process -FilePath $script:OpenCodeExe -ArgumentList $argLine -NoNewWindow -Wait -PassThru `
            -RedirectStandardOutput $outFile -RedirectStandardError $errFile
        $text = (Get-Content -LiteralPath $outFile -Raw -ErrorAction SilentlyContinue) +
                (Get-Content -LiteralPath $errFile -Raw -ErrorAction SilentlyContinue)
        return [pscustomobject]@{ ExitCode = $process.ExitCode; Text = $text }
    }
    finally {
        Remove-Item -LiteralPath $outFile, $errFile -Force -ErrorAction SilentlyContinue
    }
}

$script:OpenCodeExe = if ($OpenCodeExe) { $OpenCodeExe } else { Join-Path $env:APPDATA 'npm\node_modules\@opencode\cli\bin\opencode.exe' }
if (-not (Test-Path -LiteralPath $script:OpenCodeExe -PathType Leaf)) {
    throw "opencode.exe not found at $script:OpenCodeExe; pass -OpenCodeExe"
}

if (-not (Test-Path -LiteralPath $InstructionPath -PathType Leaf)) {
    throw "instruction file not found: $InstructionPath"
}
$instruction = [System.IO.File]::ReadAllText((Resolve-Path -LiteralPath $InstructionPath).Path, [System.Text.Encoding]::UTF8)
if ([string]::IsNullOrWhiteSpace($instruction)) {
    throw "instruction file is empty: $InstructionPath"
}

if (Test-Path -LiteralPath $PidPath -PathType Leaf) {
    $existing = (Get-Content -LiteralPath $PidPath -Raw).Trim()
    if ($existing -and ($null -ne (Get-Process -Id ([int]$existing) -ErrorAction SilentlyContinue))) {
        throw "another driver is already running with PID $existing"
    }
}

$runDir = Split-Path -Parent $PidPath
if ($runDir -and -not (Test-Path -LiteralPath $runDir)) {
    New-Item -ItemType Directory -Path $runDir -Force | Out-Null
}
Set-Content -LiteralPath $PidPath -Value $PID -Encoding ASCII
Write-Log "driver start pid=$PID session=$SessionID interval=${IntervalSeconds}s maxRuns=$MaxRuns"

$body = ConvertTo-Json -Compress -InputObject ([ordered]@{ text = [string]$instruction })
$runs = 0

try {
    while ($true) {
        if (Test-Path -LiteralPath $StopFile) {
            Exit-Driver "stop file $StopFile present; exiting after $runs run(s)" 0
        }
        if ($runs -ge $MaxRuns) {
            Exit-Driver "max run count $MaxRuns reached; exiting" 0
        }

        # Wait for the session to be idle *before* enqueuing, then hold the
        # interval. This gives "one iteration per idle interval" instead of
        # back-to-back turns, and stops a long turn from accumulating a queue.
        $wait = Invoke-OpenCodeApi -CommandLine "api post /api/experimental/session/$SessionID/wait"
        if ($wait.ExitCode -ne 0) {
            Write-Log "wait returned exit $($wait.ExitCode); retrying after the interval"
            Start-Sleep -Seconds $IntervalSeconds
            continue
        }
        if (Test-Path -LiteralPath $StopFile) {
            Exit-Driver "stop file $StopFile present before run $($runs + 1); exiting" 0
        }
        Start-Sleep -Seconds $IntervalSeconds

        $response = Invoke-OpenCodeApi -CommandLine "api post /api/session/$SessionID/prompt" -Body $body
        if ($response.ExitCode -ne 0) {
            $detail = $response.Text
            if ($detail.Length -gt 400) { $detail = $detail.Substring(0, 400) + '...' }
            Write-Log "prompt failed (exit $($response.ExitCode)): $detail"
            Exit-Driver 'cannot enqueue iterations; stopping rather than retrying in a tight loop' 1
        }

        $runs++
        Write-Log "run $runs/$MaxRuns enqueued"
    }
}
catch {
    Write-Log "driver error: $_"
    if (Test-Path -LiteralPath $PidPath) { Remove-Item -LiteralPath $PidPath -Force -ErrorAction SilentlyContinue }
    exit 1
}
