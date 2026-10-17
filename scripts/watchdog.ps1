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
    [double]$IntervalMinutes = 3,

    [ValidateRange(1, 600)]
    [int]$MaxMinutes = 90,

    [string]$Label = 'work',

    [string]$Topic = 'ai_agent'
)

$ErrorActionPreference = 'Continue'
$StateDir = Join-Path $PSScriptRoot '.watchdog'
$PidFile = Join-Path $StateDir 'watchdog.pid'
$LogFile = Join-Path $StateDir 'watchdog.log'
$Curl = 'C:\Windows\System32\curl.exe'

function Send-Ntfy {
    param(
        [string]$Title,
        [string]$Message,
        [string[]]$Tags = @()
    )
    # curl.exe by full path: bare `curl` is PowerShell's Invoke-WebRequest alias
    # and silently mishandles -H and -d.
    $args = @('-sS', '-m', '20', '-H', "Title: $Title")
    foreach ($tag in $Tags) { $args += @('-H', "Tags: $tag") }
    $args += @('-d', $Message, "https://ntfy.sh/$Topic")
    & $Curl @args | Out-Null
}

function Get-WatchdogPid {
    if (-not (Test-Path -LiteralPath $PidFile)) { return $null }
    $raw = (Get-Content -LiteralPath $PidFile -ErrorAction SilentlyContinue | Select-Object -First 1)
    if (-not $raw) { return $null }
    $pid_ = 0
    if (-not [int]::TryParse($raw.Trim(), [ref]$pid_)) { return $null }
    if (Get-Process -Id $pid_ -ErrorAction SilentlyContinue) { return $pid_ }
    return $null
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
param([double]$IntervalMinutes, [int]$MaxMinutes, [string]$Label, [string]$Topic, [string]$LogFile)
# Hard floor. An interval that rounds to zero would turn this into a tight loop
# that floods the topic; refuse rather than spam.
$IntervalMinutes = [Math]::Max($IntervalMinutes, 0.5)
$IntervalSeconds = [int][Math]::Round($IntervalMinutes * 60)
$Curl = 'C:\Windows\System32\curl.exe'
function Beat {
    param($Text, $Tags)
    $a = @('-sS', '-m', '20', '-H', 'Title: megahack-research: alive')
    foreach ($t in $Tags) { $a += @('-H', "Tags: $t") }
    $a += @('-d', $Text, "https://ntfy.sh/$Topic")
    & $Curl @a | Out-Null
    Add-Content -LiteralPath $LogFile -Value "$(Get-Date -Format o) $Text"
}
$deadline = (Get-Date).AddMinutes($MaxMinutes)
Beat "Watchdog armed for $Label, every $([int]$IntervalSeconds)s. If heartbeats stop, the agent stalled." @('rotating_light')
while ((Get-Date) -lt $deadline) {
    Start-Sleep -Seconds $IntervalSeconds
    Beat "Still working on $Label." @()
}
Beat "Watchdog deadline reached for $Label. Agent turn is over." @('warning')
'@
        Set-Content -LiteralPath $workerPath -Value $workerText -Encoding UTF8

        $proc = Start-Process -FilePath 'powershell.exe' -WindowStyle Hidden -PassThru -ArgumentList @(
            '-NoProfile', '-NonInteractive', '-ExecutionPolicy', 'Bypass',
            '-File', $workerPath,
            '-IntervalMinutes', "$IntervalMinutes",
            '-MaxMinutes', "$MaxMinutes",
            '-Label', $Label,
            '-Topic', $Topic,
            '-LogFile', $LogFile
        )

        Set-Content -LiteralPath $PidFile -Value $proc.Id -Encoding ASCII
        "watchdog started: pid=$($proc.Id) interval=${IntervalMinutes}m max=${MaxMinutes}m label=$Label"
        break
    }
}