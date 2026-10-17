<#
.SYNOPSIS
Send an indexed ntfy notification for the megahack-research agent.

.DESCRIPTION
Every notification carries a monotonic index, "#run.seq", so the receiving app
shows order without the user having to read the clock. A run begins when the
watchdog is armed; the sequence increments on every message after that, whether
it came from the heartbeat, the agent, or a question.

    #3.7 | 14:32:05 | heartbeat   | Still working on call graph.
    #3.8 | 14:33:05 | done        | Pushed f4b6b0b, 42,701 edges.
    #3.9 | 14:34:00 | question    | Which of these two?

The run index is the number of armed sessions; the sequence number is the message
number within the current run. Both reset only when the user clears the topic.

The payload is published to the public topic ntfy.sh/ai_agent, which anyone can
subscribe to. Send a status line only - never command output, file contents,
credentials, paths, hashes, or anything from samples/.

.PARAMETER Kind
Heartbeat, Armed, Done, Question, or Note.

.PARAMETER Text
The one-line message body.

.PARAMETER Task
Short task label, echoed in the title so heartbeats from different work are
distinguishable at a glance.

.PARAMETER SeqNumber
Use this sequence number instead of allocating one, so the watchdog can share
one counter with the agent's own messages. Leave at 0 to allocate automatically.

.EXAMPLE
powershell -File scripts\notify.ps1 -Kind Heartbeat -Task "call graph" -Text "Still working."

.EXAMPLE
powershell -File scripts\notify.ps1 -Kind Done -Task "call graph" -Text "Pushed f4b6b0b."
#>
[CmdletBinding()]
param(
    [ValidateSet('Heartbeat', 'Armed', 'Done', 'Question', 'Note')]
    [string]$Kind = 'Note',

    [string]$Text = '',

    [string]$Task = '',

    [int]$SeqNumber = 0,

    [string]$Topic = 'ai_agent'
)

$ErrorActionPreference = 'Continue'
$StateDir = Join-Path $PSScriptRoot '.watchdog'
$StateFile = Join-Path $StateDir 'notify-state.json'
$Curl = 'C:\Windows\System32\curl.exe'

New-Item -ItemType Directory -Force -Path $StateDir | Out-Null

# Read the counter, tolerating a missing or corrupt state file. Losing the index
# is not worth failing a notification over.
#
# NB: the counter is $counter, deliberately not $seq. PowerShell variable names
# are case-insensitive, so a parameter called -Seq would BE this variable and the
# "caller supplied a number" test would silently read the counter instead.
$run = 0
$counter = 0
if (Test-Path -LiteralPath $StateFile) {
    try {
        $state = (Get-Content -LiteralPath $StateFile -Raw) | ConvertFrom-Json
        if ($null -ne $state.run) { $run = [int]$state.run }
        if ($null -ne $state.seq) { $counter = [int]$state.seq }
    } catch {
        Write-Verbose "notify state unreadable, resetting: $_"
    }
}

if ($Kind -eq 'Armed') {
    $run++
    $counter = 1
} elseif ($SeqNumber -gt 0) {
    $counter = $SeqNumber
} else {
    $counter++
    if ($counter -lt 1) { $counter = 1 }
}

$index = "$run.$counter"
$stamp = Get-Date -Format 'HH:mm:ss'
$label = if ($Task) { $Task } else { 'work' }
$title = "megahack-research #$index | $label"
$body = if ($Text) { "#$index $stamp | $Kind | $Text" } else { "#$index $stamp | $Kind" }

try {
    [pscustomobject]@{ run = $run; seq = $counter } |
        ConvertTo-Json -Compress |
        Set-Content -LiteralPath $StateFile -Encoding ASCII
} catch {
    Write-Verbose "could not persist notify state: $_"
}

$tags = switch ($Kind) {
    'Heartbeat' { @('arrows_counterclockwise') }
    'Armed'     { @('rotating_light') }
    'Done'      { @('white_check_mark') }
    'Question'  { @('question') }
    default     { @() }
}

# curl.exe by full path: bare `curl` is PowerShell's Invoke-WebRequest alias and
# silently mishandles -H and -d.
$args = @('-sS', '-m', '20', '-H', "Title: $title")
foreach ($tag in $tags) { $args += @('-H', "Tags: $tag") }
$args += @('-d', $body, "https://ntfy.sh/$Topic")

& $Curl @args | Out-Null
Write-Output "#$index $stamp $Kind"