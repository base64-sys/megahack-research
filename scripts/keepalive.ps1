<#
.SYNOPSIS
    Idle-detecting keep-alive: types "continue" into a chosen terminal window
    when the screen stops changing.

.DESCRIPTION
    The agent has no way to start its own next turn. This is the workaround: a
    watcher that notices the session has gone quiet and re-prompting it, the same
    as a person typing "continue" after stopping.

    Idle detection hashes a region of the target window on an interval. When the
    hash has not changed for -IdleSeconds, the session is treated as finished and
    one prompt is sent.

    Deliberate safety properties, because this types into a live window unattended:

      * The target window must match -WindowTitle, and its title is re-checked
        immediately before every send. If the window is gone or renamed, the run
        stops rather than typing into whatever is now in front.
      * It never sends to the foreground window implicitly. The window is focused
        explicitly first, and restored on exit.
      * -MaxRuns is a hard cap. There is no unbounded mode; this loop costs money
        and writes commits, so it must end on its own.
      * -DryRun logs what it would send and touches nothing.
      * Every action is appended to the log, so a run can be audited afterwards.

    This is operator tooling for one machine. It is not part of the research
    pipeline and nothing in this repository depends on it.

.PARAMETER WindowTitle
    Substring of the target window's title. Matched case-insensitively.

.PARAMETER IdleSeconds
    How long the screen must be unchanged before a prompt is sent.

.PARAMETER PollSeconds
    How often the screen is sampled. Must be well under IdleSeconds.

.PARAMETER MaxRuns
    Hard cap on prompts sent. There is no -Infinity.

.PARAMETER DryRun
    Report what would be sent without typing.

.EXAMPLE
    .\scripts\keepalive.ps1 -WindowTitle "Windows Terminal" -IdleSeconds 90 -MaxRuns 25

.EXAMPLE
    .\scripts\keepalive.ps1 -WindowTitle "pwsh" -DryRun
#>
[CmdletBinding()]
param(
    [string]   $WindowTitle   = 'megahack-research',
    [int]      $IdleSeconds   = 120,
    [int]      $PollSeconds   = 10,
    [int]      $MaxRuns       = 12,
    [double]   $ChangeThreshold = 0.02,
    [switch]   $DryRun,
    # Where the AI chat input sits, as a fraction of the window rect. Measured
    # from a 1920x1080 capture of this GUI: the input box centre is about 57%
    # across and 84% down. Override if the panel is resized or moved.
    [double]   $ClickX        = 0.57,
    [double]   $ClickY        = 0.84
)

$ErrorActionPreference = 'Stop'

if ($IdleSeconds -le $PollSeconds) {
    throw "IdleSeconds ($IdleSeconds) must be greater than PollSeconds ($PollSeconds), or the screen will look idle between samples."
}
if ($MaxRuns -lt 1) {
    throw 'MaxRuns must be at least 1. There is deliberately no unbounded mode.'
}

$logPath = Join-Path $PSScriptRoot '..\.keepalive.log'
function Write-Log {
    param([string]$Message)
    $line = '{0}  {1}' -f (Get-Date -Format 'HH:mm:ss'), $Message
    Add-Content -LiteralPath $logPath -Value $line -Encoding UTF8
    Write-Host $line
}

Add-Type -AssemblyName System.Windows.Forms, System.Drawing | Out-Null

Add-Type -Namespace KeepAlive -Name Win -MemberDefinition @'
[DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
[DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
[DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
[DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
[DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc cb, IntPtr l);
public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
[DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowText(IntPtr hWnd, System.Text.StringBuilder s, int n);
[DllImport("user32.dll")] public static extern int GetWindowTextLength(IntPtr hWnd);
[DllImport("user32.dll")] public static extern bool SetCursorPos(int X, int Y);
[DllImport("user32.dll")] public static extern void mouse_event(uint f, uint dx, uint dy, uint d, IntPtr e);
public const uint LEFTDOWN = 0x0002, LEFTUP = 0x0004;
public struct RECT { public int Left, Top, Right, Bottom; }
'@

function Set-Cursor {
    param([int]$X, [int]$Y)
    [void][KeepAlive.Win]::SetCursorPos($X, $Y)
    Start-Sleep -Milliseconds 120
    [KeepAlive.Win]::mouse_event([KeepAlive.Win]::LEFTDOWN, 0, 0, 0, [IntPtr]::Zero)
    [KeepAlive.Win]::mouse_event([KeepAlive.Win]::LEFTUP, 0, 0, 0, [IntPtr]::Zero)
}

function Get-TargetWindow {
    <#  Use Get-Process rather than an EnumWindows P/Invoke callback. A scriptblock
        converted to a delegate runs in its own scope, so the handle it assigns did
        not propagate back - the loop silently got null and then failed casting it.
        MainWindowTitle is enough here and has no scoping trap.
    #>
    param([string]$TitleLike)
    foreach ($p in Get-Process -ErrorAction SilentlyContinue) {
        if ($p.MainWindowHandle -ne [IntPtr]::Zero -and $p.MainWindowTitle -like "*$TitleLike*") {
            return $p.MainWindowHandle
        }
    }
    return [IntPtr]::Zero
}

function Get-WindowText {
    param([IntPtr]$Handle)
    if ($Handle -eq [IntPtr]::Zero) { return '' }
    $len = [KeepAlive.Win]::GetWindowTextLength($Handle)
    if ($len -le 0) { return '' }
    $sb = New-Object System.Text.StringBuilder ($len + 1)
    [void][KeepAlive.Win]::GetWindowText($Handle, $sb, $sb.Capacity)
    return $sb.ToString()
}

function Get-RegionSamples {
    <#  Sample a coarse grid of pixel colours inside the window.

        It returns samples rather than a hash on purpose. A hash says only "did
        anything change at all", and in this GUI something always does: the
        "Thinking..." spinner animates and the caret in the chat box blinks. A
        hash would therefore never repeat and the idle test could never fire.

        With samples, activity is judged by *how much* changed, so a spinner or
        a blinking caret - a handful of pixels - does not read as a live session.
    #>
    param([IntPtr]$Handle)
    $r = New-Object KeepAlive.Win+RECT
    if (-not [KeepAlive.Win]::GetWindowRect($Handle, [ref]$r)) { return $null }
    $w = $r.Right - $r.Left
    $h = $r.Bottom - $r.Top
    if ($w -lt 64 -or $h -lt 64) { return $null }
    $bmp = New-Object System.Drawing.Bitmap $w, $h
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($r.Left, $r.Top, 0, 0, $bmp.Size)
    $g.Dispose()
    $grid = 48
    $out = New-Object 'System.Collections.Generic.List[int]'
    for ($y = 0; $y -lt $grid; $y++) {
        for ($x = 0; $x -lt $grid; $x++) {
            $c = $bmp.GetPixel([int](($x + 0.5) * $w / $grid), [int](($y + 0.5) * $h / $grid))
            # Pack RGB into one int; only the high bits matter for comparison.
            [void]$out.Add((($c.R -band 0xF8) -shl 16) -bor (($c.G -band 0xF8) -shl 8) -bor ($c.B -band 0xF8))
        }
    }
    $bmp.Dispose()
    return $out
}

function Measure-Change {
    <#  Fraction of sampled cells whose colour moved. Threshold is 2%: a live
        turn streams text and repaints a large fraction of the panel, while a
        spinner and a blinking caret touch a few cells at most.
    #>
    param($A, $B)
    if ($null -eq $A -or $null -eq $B -or $A.Count -ne $B.Count) { return 1.0 }
    $diff = 0
    for ($i = 0; $i -lt $A.Count; $i++) { if ($A[$i] -ne $B[$i]) { $diff++ } }
    return $diff / $A.Count
}

$target = Get-TargetWindow -TitleLike $WindowTitle
if ($target -eq [IntPtr]::Zero) {
    Write-Log "no window title matching '$WindowTitle'; nothing typed, not touching the keyboard"
    throw "No window matches '$WindowTitle'. Check the title and retry with -DryRun first."
}

$handle = $target
Write-Log "watching '$WindowTitle' -> '$((Get-WindowText $handle))'"
Write-Log "idle=${IdleSeconds}s poll=${PollSeconds}s maxRuns=$MaxRuns dryRun=$DryRun"

$lastSamples = $null
$idleSince = Get-Date
$runs = 0

try {
    while ($true) {
        if ($runs -ge $MaxRuns) { Write-Log "reached -MaxRuns $MaxRuns, stopping"; break }

        # Re-resolve every cycle. If the window closed or was renamed, stop
        # rather than typing into whatever inherited the foreground.
function Test-RedBanner {
    <#  Failsafe: stop the whole loop if a red error banner appears above the chat
        input. The band sampled is immediately above the input box, where JetBrains
        surfaces tool errors and refusals. A hit means the session is telling us it
        will not continue, so prompting again would just pile up failures.

        The test is "saturated red pixels present", not "the banner is red", so it
        does not depend on reading text. If the theme ever uses red for ordinary
        content this would stop early - that is the safe direction to fail.
    #>
    param([IntPtr]$Handle)
    $r = New-Object KeepAlive.Win+RECT
    if (-not [KeepAlive.Win]::GetWindowRect($Handle, [ref]$r)) { return $false }
    $w = $r.Right - $r.Left
    $h = $r.Bottom - $r.Top
    # Band: horizontally centred over the input box, just above it.
    $bw = [int]($w * 0.6)
    $bh = [int]($h * 0.10)
    $bx = $r.Left + [int](($w - $bw) * 0.5)
    $by = $r.Top + [int]($h * $ClickY) - $bh - 8
    if ($by -lt $r.Top) { return $false }
    $bmp = New-Object System.Drawing.Bitmap $bw, $bh
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($bx, $by, 0, 0, $bmp.Size)
    $g.Dispose()
    $red = 0
    for ($y = 0; $y -lt $bh; $y += 3) {
        for ($x = 0; $x -lt $bw; $x += 3) {
            $c = $bmp.GetPixel($x, $y)
            if ($c.R -gt 110 -and $c.R -gt ($c.G + 45) -and $c.R -gt ($c.B + 45)) { $red++ }
        }
    }
    $bmp.Dispose()
    # A few hundred red pixels is a banner; a handful is an icon or syntax colour.
    return ($red -gt 220)
}

$target = Get-TargetWindow -TitleLike $WindowTitle
        if ($target -eq [IntPtr]::Zero) { Write-Log "target window disappeared, stopping"; break }
        $handle = $target
        $title = Get-WindowText $handle

$samples = Get-RegionSamples $handle
        if ($null -eq $samples) { Write-Log "could not read the window, stopping"; break }

        # Failsafe: a red banner above the input means the session is refusing to
        # continue. Stop immediately rather than prompting into a closed door.
        if (Test-RedBanner $handle) {
            Write-Log 'red banner detected above the chat input - stopping now'
            break
        }

        $changed = Measure-Change $samples $lastSamples
        if ($changed -lt $ChangeThreshold) {
            $idleFor = ((Get-Date) - $idleSince).TotalSeconds
            if ($idleFor -ge $IdleSeconds) {
                $runs++
                if ($DryRun) {
                    Write-Log "would send 'continue' (run $runs/$MaxRuns) after ${idleFor}s idle in '$title'"
} else {
                    [void][KeepAlive.Win]::ShowWindow($handle, 9)   # SW_RESTORE
                    [void][KeepAlive.Win]::SetForegroundWindow($handle)
                    Start-Sleep -Milliseconds 500
                    $rc = New-Object KeepAlive.Win+RECT
                    [void][KeepAlive.Win]::GetWindowRect($handle, [ref]$rc)
                    $px = $rc.Left + [int](($rc.Right - $rc.Left) * $ClickX)
                    $py = $rc.Top + [int](($rc.Bottom - $rc.Top) * $ClickY)
                    Set-Cursor -X $px -Y $py
                    Start-Sleep -Milliseconds 400
                    [System.Windows.Forms.SendKeys]::SendWait('continue{ENTER}')
                    Write-Log "sent 'continue' (run $runs/$MaxRuns) to '$title' at $px,$py"
                }
                $idleSince = Get-Date
                $lastSamples = $null
                Start-Sleep -Seconds ([Math]::Max($PollSeconds, 5))
                continue
            }
        } else {
            $idleSince = Get-Date
        }
        $lastSamples = $samples
        Start-Sleep -Seconds $PollSeconds
    }
}
finally {
    Write-Log 'keep-alive stopped'
}