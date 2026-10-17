<#
.SYNOPSIS
Liveness watchdog for long agent runs.

.DESCRIPTION
An agent turn can end abruptly: the model runs out of tokens, a tool call dies,
the session is interrupted. Any notification sent at the "end" of such a turn is
simply never sent. This solves that by moving the signal out of the agent's
control flow entirely: a detached Windows PowerShell process pings ntfy on a
timer for as long as the work lasts.

  - If the turn completes normally, the agent sends a completion ping and kills
    the watchdog. No further heartbeats arrive.
  - If the turn dies unexpectedly, the heartbeats keep arriving and then stop
    when the watchdog's own deadline passes. Silence after a heartbeat means the
    agent is stuck.

ntfy's delayed-message cancel was tried for this and is NOT reliable: a cancel
returned success and the message still fired. So there is no cancel protocol to
get wrong. The watchdog just stops being stopped.

Payloads go to the public topic ntfy.sh/ai_agent, so they carry a status word and
nothing else - never command output, paths, hashes or anything from samples/.

.PARAMETER Action
Start (default), Stop, or Status.

.PARAMETER IntervalMinutes
Seconds between heartbeats. Default 3.

.PARAMETER MaxMinutes
Hard ceiling on watchdog lifetime. It exits and sends a final ping when reached.
Default 90.

.EXAMPLE
powershell -File scripts\watchdog.ps1 -Action Start -Label "decrypt analysis"

.EXAMPLE
powershell -File scripts\watchdog.ps1 -Action Stop
#>
[CmdletBinding()]
param(
    [ValidateSet('Start', 'Stop', 'Status')]
    [string]$Action = 'Start',

    [ValidateRange(0.5, 60)]
    [double]$IntervalMinutes = 1,

    [ValidateRange(1, 600)]
    [int]$MaxMinutes = 90,

    [string]$Label = 'work',

    [string]$Topic = 'ai_agent'
)

$ErrorActionPreference = 'Continue'
$StateDir = Join-Path $PSScriptRoot '.watchdog'
$PidFile = Join-Path $StateDir 'watchdog.pid'
$LogFile = Join-Path $StateDir 'watchdog.log'
$NotifyScript = Join-Path $PSScriptRoot 'notify.ps1'

function Get-WatchdogPid {
    if (-not (Test-Path -LiteralPath $PidFile)) { return $null }
    $raw = (Get-Content -LiteralPath $PidFile -ErrorAction SilentlyContinue | Select-Object -First 1)
    if (-not $raw) { return $null }
    $pid_ = 0
    if (-not [int]::TryParse($raw.Trim(), [ref]$pid_)) { return $null }
    if (Get-Process -Id $pid_ -ErrorAction SilentlyContinue) { return $pid_ }
    return $null
}

function Get-Python {
    # Bare `python` here is a Microsoft Store stub that exits without doing
    # anything, so prefer the real interpreter by absolute path.
    $candidates = @(
        $env:MEGAHACK_PYTHON,
        'C:\Users\Robert_\AppData\Local\Python\bin\python3.exe',
        'C:\Users\Robert_AppData\Local\Python\bin\python3.exe'
    ) | Where-Object { $_ -and (Test-Path -LiteralPath $_) }
    return $candidates | Select-Object -First 1
}

function Update-ProgressGraphic {
    # Arming the watchdog means work is starting, so refresh the progress
    # graphic at the same moment. Never let this block or fail the run.
    $python = Get-Python
    if (-not $python) { Write-Verbose 'no python found; progress graphic not refreshed'; return }
    $script = Join-Path $PSScriptRoot 'make_progress_svg.py'
    if (-not (Test-Path -LiteralPath $script)) { return }
    Push-Location (Split-Path $PSScriptRoot -Parent)
    try {
        & $python $script 2>&1 | Out-Null
    } catch {
        Write-Verbose "progress graphic refresh failed: $_"
    } finally {
        Pop-Location
    }
}

switch ($Action) {
    'Status' {
        $pid_ = Get-WatchdogPid
        if ($pid_) {
            $proc = Get-Process -Id $pid_
            "watchdog running: pid=$pid_ uptime=$([int]((Get-Date) - $proc.StartTime).TotalSeconds)s"
        } else {
            'watchdog not running'
        }
        break
    }

    'Stop' {
        $pid_ = Get-WatchdogPid
        if ($pid_) {
            Stop-Process -Id $pid_ -Force -ErrorAction SilentlyContinue
            Remove-Item -LiteralPath $PidFile -ErrorAction SilentlyContinue
            "watchdog $pid_ stopped"
        } else {
            'no watchdog running'
        }
        break
    }

    'Start' {
        $existing = Get-WatchdogPid
        if ($existing) {
            "watchdog already running: pid=$existing"
            break
        }
        New-Item -ItemType Directory -Force -Path $StateDir | Out-Null

        # The heartbeat loop lives in a generated worker script so it can be
        # launched detached. A scriptblock cannot be passed through Start-Process
        # cleanly, and inlining it here would leave dead code behind.
        $workerPath = Join-Path $StateDir 'worker.ps1'
        $workerText = @'
param([double]$IntervalMinutes, [int]$MaxMinutes, [string]$Label, [string]$Topic, [string]$LogFile, [string]$NotifyScript)
# Hard floor. An interval that rounds to zero would turn this into a tight loop
# that floods the topic; refuse rather than spam.
$IntervalMinutes = [Math]::Max($IntervalMinutes, 0.5)
$IntervalSeconds = [int][Math]::Round($IntervalMinutes * 60)

function Beat {
    param($Kind, $Text)
    $out = & $NotifyScript -Kind $Kind -Task $Label -Text $Text -Topic $Topic 2>&1 | Select-Object -Last 1
    if ($out) { Add-Content -LiteralPath $LogFile -Value "$(Get-Date -Format o) $out" }
}

$deadline = (Get-Date).AddMinutes($MaxMinutes)
Beat 'Armed' "watchdog armed for $Label, heartbeat every ${IntervalSeconds}s. If heartbeats stop, the agent stalled."
while ((Get-Date) -lt $deadline) {
    Start-Sleep -Seconds $IntervalSeconds
    Beat 'Heartbeat' "still working on $Label."
}
Beat 'Heartbeat' "watchdog deadline reached for $Label. The agent turn is over."
'@
        Set-Content -LiteralPath $workerPath -Value $workerText -Encoding UTF8

        $proc = Start-Process -FilePath 'powershell.exe' -WindowStyle Hidden -PassThru -ArgumentList @(
            '-NoProfile', '-NonInteractive', '-ExecutionPolicy', 'Bypass',
            '-File', $workerPath,
            '-IntervalMinutes', "$IntervalMinutes",
            '-MaxMinutes', "$MaxMinutes",
            '-Label', $Label,
            '-Topic', $Topic,
            '-LogFile', $LogFile,
            '-NotifyScript', $NotifyScript
        )

        Set-Content -LiteralPath $PidFile -Value $proc.Id -Encoding ASCII
        "watchdog started: pid=$($proc.Id) interval=${IntervalMinutes}m max=${MaxMinutes}m label=$Label"
        Update-ProgressGraphic
        'progress graphic refreshed'
        break
    }
}