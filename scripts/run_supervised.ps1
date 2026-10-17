param(
  [Parameter(Mandatory=$true)][string]$FilePath,
  [string[]]$ArgumentList = @(),
  [int]$MaxSilentSeconds = 180,
  [int]$MaxTotalSeconds  = 7200
)
$ErrorActionPreference = "Continue"
New-Item -ItemType Directory -Force -Path .\.oc | Out-Null
$out = ".\.oc\out.log"; $err = ".\.oc\err.log"
Remove-Item $out, $err -ErrorAction SilentlyContinue
$env:PYTHONUNBUFFERED = "1"   # python buffers stdout when redirected to a file — critical

$p = Start-Process -FilePath $FilePath -ArgumentList $ArgumentList -NoNewWindow `
       -PassThru -RedirectStandardOutput $out -RedirectStandardError $err
$start = Get-Date; $lastLen = 0; $lastChange = Get-Date; $reason = "completed"

while (-not $p.HasExited) {
  Start-Sleep -Seconds 5
  $len = 0
  foreach ($f in @($out, $err)) { $i = Get-Item $f -ErrorAction SilentlyContinue; if ($i) { $len += $i.Length } }
  if ($len -gt $lastLen) { $lastLen = $len; $lastChange = Get-Date }
  $silent = ((Get-Date) - $lastChange).TotalSeconds
  $total  = ((Get-Date) - $start).TotalSeconds
  if ($silent -gt $MaxSilentSeconds) { $reason = "KILLED: no output for $([int]$silent)s"; break }
  if ($total  -gt $MaxTotalSeconds)  { $reason = "KILLED: exceeded $MaxTotalSeconds s total";  break }
}
if (-not $p.HasExited) { taskkill /F /T /PID $p.Id 2>$null; Start-Sleep -Seconds 2 }

"EXITCODE=$(if ($p.HasExited) { $p.ExitCode } else { 'killed-by-monitor' })"
"MONITOR=$reason"
"---- out.log (last 40 lines) ----"
Get-Content $out -Tail 40 -ErrorAction SilentlyContinue
"---- err.log (last 20 lines) ----"
Get-Content $err -Tail 20 -ErrorAction SilentlyContinue