# configure.ps1 - send a saved calibration to the gauge board over USB serial, or save the board's current one.
#   Apply (default): powershell -ExecutionPolicy Bypass -File configure.ps1 -Port COM6
#   Save:            powershell -ExecutionPolicy Bypass -File configure.ps1 -Port COM6 -Save
#   Another file:    ... -File configure.ps1 -Port COM6 -File my-gauges.txt
# Close the Arduino serial monitor first: only one program can have the port open.
param(
  [Parameter(Mandatory = $true)][string]$Port,
  [string]$File = (Join-Path $PSScriptRoot "calibration.txt"),
  [switch]$Save
)
$ErrorActionPreference = "Stop"

$sp = New-Object System.IO.Ports.SerialPort $Port, 115200, "None", 8, "One"
$sp.NewLine = "`n"; $sp.ReadTimeout = 500; $sp.DtrEnable = $false; $sp.RtsEnable = $false   # no reset on open
try { $sp.Open() } catch { Write-Host "Cannot open $Port. Is the serial monitor still open, or is it another port?" -ForegroundColor Red; exit 1 }
Start-Sleep -Milliseconds 1500
$sp.DiscardInBuffer()

function Send([string]$line) {
  $sp.WriteLine($line)
  Start-Sleep -Milliseconds 200
  $out = ""
  while ($sp.BytesToRead -gt 0) { $out += $sp.ReadExisting(); Start-Sleep -Milliseconds 60 }
  return $out
}

if ($Save) {
  $show = Send "show"
  Start-Sleep -Milliseconds 300; while ($sp.BytesToRead -gt 0) { $show += $sp.ReadExisting(); Start-Sleep -Milliseconds 60 }
  $lines = @("# Gauge board settings saved from $Port on $(Get-Date -Format 'yyyy-MM-dd HH:mm'). Send with: configure.ps1 -Port $Port")
  if ($show -match "(slow|fast) decay") { $lines += "decay $($Matches[1])" }
  if ($show -match "pwm (\d+) Hz") { $lines += "pwm $($Matches[1])" }
  $gauge = $null
  foreach ($l in ($show -split "`r?`n")) {
    if ($l -match "^\s+(tach|speedo)\s+value .*min (\S+) max (\S+) zero (\S+) span (\S+) strength (\S+) slew (\S+)") {
      $gauge = $Matches[1]
      $lines += "$gauge strength $($Matches[6])", "$gauge slew $($Matches[7])", "$gauge min $($Matches[2])", "$gauge max $($Matches[3])",
                "$gauge zero $($Matches[4])", "$gauge span $($Matches[5])", "$gauge clearpoints"
    } elseif ($gauge -and $l -match "points \([^)]*\):\s*(.*)$") {
      foreach ($p in ($Matches[1].Trim() -split "\s+")) { if ($p -match "^(-?[\d.]+)=(-?[\d.]+)$") { $lines += "$gauge point $($Matches[1]) $($Matches[2])" } }
    }
  }
  if ($lines.Count -lt 4) { Write-Host "No answer to 'show' from $Port. Is it the gauge board?" -ForegroundColor Red; $sp.Close(); exit 1 }
  Set-Content -Path $File -Value $lines
  Write-Host "Saved $($lines.Count - 1) settings to $File" -ForegroundColor Green
  $sp.Close(); exit 0
}

if (-not (Test-Path $File)) { Write-Host "No file $File" -ForegroundColor Red; $sp.Close(); exit 1 }
$bad = 0
foreach ($line in Get-Content $File) {
  $l = $line.Trim()
  if ($l -eq "" -or $l.StartsWith("#")) { continue }
  $ans = (Send $l).Trim()
  if ($ans -match "\?|Out of range|No mark|marks already|missing") { Write-Host "$l  ->  $ans" -ForegroundColor Red; $bad++ }
  else { Write-Host "$l  ->  $ans" }
}
Write-Host ""
Write-Host (Send "show")
$sp.Close()
if ($bad) { Write-Host "$bad line(s) refused" -ForegroundColor Red; exit 1 } else { Write-Host "Done" -ForegroundColor Green }
