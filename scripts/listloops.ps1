#!/usr/bin/env pwsh
<#
.SYNOPSIS
Show the state of OpenCode loop schedulers without changing anything.

.DESCRIPTION
Read-only companion to LoopEmergencyStop.ps1. It reports two things:

1. Loops persisted by @prevalentware/opencode-loop-plugin in
   %APPDATA%\opencode-loop-plugin\loops.json, with NextRun converted to local
   time and an explicit overdue/scheduled marker. The raw epoch millisecond
   value in the plugin's table is easy to misread; "overdue 512s" is not.
2. The API-driven driver (scripts/loop-driver.ps1), if it is running: its PID,
   run counter and the last few log lines. That driver exists because the
   plugin's in-memory timers do not survive between OpenCode requests in this
   environment (see .oc/CHECKPOINT.md).

No network request, no AI, no state mutation.

.PARAMETER StatePath
Plugin state file. Defaults to %APPDATA%\opencode-loop-plugin\loops.json.

.PARAMETER DriverPidPath
PID file written by scripts/loop-driver.ps1. Defaults to .oc/loop-driver.pid.

.PARAMETER DriverLogPath
Driver log. Defaults to .oc/loop-driver.log.

.EXAMPLE
pwsh -File scripts/listloops.ps1

.EXAMPLE
pwsh -File scripts/listloops.ps1 -StatePath C:\tmp\loops.json
#>
[CmdletBinding()]
param(
    [string]$StatePath = (Join-Path $env:APPDATA 'opencode-loop-plugin\loops.json'),
    [string]$DriverPidPath = '.oc/loop-driver.pid',
    [string]$DriverLogPath = '.oc/loop-driver.log'
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

function Get-When([object]$value, [DateTimeOffset]$now) {
    if ($null -eq $value) { return [pscustomobject]@{ Text = '-'; State = '' } }
    $ms = [double]$value
    $when = [DateTimeOffset]::FromUnixTimeMilliseconds([long]$ms).ToLocalTime()
    $delta = [int][Math]::Round(($ms - $now.ToUnixTimeMilliseconds()) / 1000)
    if ($delta -lt 0) {
        return [pscustomobject]@{ Text = "$($when.ToString('HH:mm:ss')) (overdue $([Math]::Abs($delta))s)"; State = 'overdue' }
    }
    return [pscustomobject]@{ Text = "$($when.ToString('HH:mm:ss')) (in $($delta)s)"; State = 'scheduled' }
}

$now = [DateTimeOffset]::UtcNow
$exit = 0

if (Test-Path -LiteralPath $StatePath -PathType Leaf) {
    $state = ConvertFrom-Json -InputObject (Get-Content -LiteralPath $StatePath -Raw -Encoding UTF8)
    $rows = @()
    if ($null -ne $state.loops) {
        foreach ($entry in $state.loops.PSObject.Properties) {
            $loop = $entry.Value
            $next = Get-When $loop.nextRunAt $now
            $last = Get-When $loop.lastRunAt $now
            $rows += [pscustomobject]@{
                Id         = [string]$entry.Name
                Status     = [string]$loop.status
                Mode       = if ($loop.mode -eq 'interval') { "$([int]$loop.intervalMs / 1000)s" } else { 'dynamic' }
                Runs       = [int]$loop.runCount
                NextRun    = $next.Text
                LastRun    = $last.Text
                LastResult = if ($null -eq $loop.lastResult) { '-' } else { [string]$loop.lastResult }
                Error      = if ($null -eq $loop.lastError) { '-' } else { [string]$loop.lastError }
                Owner      = if ($null -eq $loop.ownerID) { '-' } else { [string]$loop.ownerID }
            }
            if ($next.State -eq 'overdue' -and [string]$loop.status -eq 'active') { $exit = 2 }
        }
    }
    if ($rows.Count -eq 0) {
        Write-Host "Plugin loops: none ($StatePath)"
    }
    else {
        Write-Host "Plugin loops ($StatePath):"
        $rows | Format-Table Id, Status, Mode, Runs, NextRun, LastRun, LastResult, Error -AutoSize
    }
}
else {
    Write-Host "Plugin loops: no state file at $StatePath"
}

Write-Host ''
if ((Test-Path -LiteralPath $DriverPidPath -PathType Leaf)) {
    $driverPid = (Get-Content -LiteralPath $DriverPidPath -Raw).Trim()
    $proc = Get-Process -Id ([int]$driverPid) -ErrorAction SilentlyContinue
    if ($null -ne $proc) {
        Write-Host "Driver: running, PID $driverPid"
    }
    else {
        Write-Host "Driver: PID file present but process $driverPid is gone (stale pid file)"
    }
}
else {
    Write-Host 'Driver: not running (no pid file)'
}

if (Test-Path -LiteralPath $DriverLogPath -PathType Leaf) {
    Write-Host "Driver log tail ($DriverLogPath):"
    Get-Content -LiteralPath $DriverLogPath -Tail 5 | ForEach-Object { "  $_" }
}

exit $exit
