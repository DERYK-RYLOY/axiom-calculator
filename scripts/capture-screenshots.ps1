# AXIOM X1 — capture real product screenshots with a locally installed Chromium browser.
param([string]$Browser)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot

if (-not $Browser) {
  $candidates = @(
    "$env:ProgramFiles\BraveSoftware\Brave-Browser\Application\brave.exe",
    "${env:ProgramFiles(x86)}\BraveSoftware\Brave-Browser\Application\brave.exe",
    "$env:ProgramFiles\Google\Chrome\Application\chrome.exe",
    "${env:ProgramFiles(x86)}\Google\Chrome\Application\chrome.exe",
    "$env:ProgramFiles\Microsoft\Edge\Application\msedge.exe",
    "${env:ProgramFiles(x86)}\Microsoft\Edge\Application\msedge.exe"
  )
  $Browser = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
}
if (-not $Browser) { throw 'No Chromium-based browser found. Pass -Browser <path>.' }

$page = "file:///$(((Resolve-Path (Join-Path $root 'app\index.html')).Path) -replace '\\','/')"
$outDir = Join-Path $root 'docs\screenshots'
New-Item -ItemType Directory -Force $outDir | Out-Null

$shots = @(
  @{ name = '01-classic.png';   query = '?theme=classic&expr=sqrt(2)%2Bpi' },
  @{ name = '02-solar.png';     query = '?theme=solar&expr=nCr(10,3)' },
  @{ name = '03-graphite.png';  query = '?theme=graphite&expr=sin(pi/2)' },
  @{ name = '04-graph.png';     query = '?theme=classic&view=graph' },
  @{ name = '05-convert.png';   query = '?theme=neon&view=convert' },
  @{ name = '06-base.png';      query = '?theme=graphite&view=base' },
  @{ name = '07-voice.png';     query = '?theme=solar&view=voice' },
  @{ name = '08-history.png';   query = '?theme=classic&view=history&demo=1' }
)

Write-Host 'Capturing AXIOM X1 screenshots...'
foreach ($shot in $shots) {
  $dst = Join-Path $outDir $shot.name
  $old = $ErrorActionPreference
  $ErrorActionPreference = 'Continue'
  & $Browser '--headless=new' '--disable-gpu' '--hide-scrollbars' `
    '--force-device-scale-factor=1' '--virtual-time-budget=3500' `
    '--window-size=1000,1120' "--screenshot=$dst" ($page + $shot.query) 2>$null | Out-Null
  $ErrorActionPreference = $old
  if (-not (Test-Path $dst)) { throw "Failed to capture $($shot.name)" }
  Write-Host "  captured $($shot.name)"
}
Write-Host 'Screenshots complete.'
