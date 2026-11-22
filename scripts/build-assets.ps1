# AXIOM X1 — render vector artwork to PNG with a locally installed Chromium browser.
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

function Render-Svg([string]$svg, [string]$out, [int]$w, [int]$h) {
  $src = (Resolve-Path (Join-Path $root $svg)).Path
  $dst = Join-Path $root $out
  New-Item -ItemType Directory -Force (Split-Path -Parent $dst) | Out-Null
  $old = $ErrorActionPreference
  $ErrorActionPreference = 'Continue'
  & $Browser '--headless=new' '--disable-gpu' '--hide-scrollbars' `
    '--default-background-color=00000000' '--virtual-time-budget=3000' `
    "--window-size=$w,$h" "--screenshot=$dst" "file:///$($src -replace '\\','/')" 2>$null | Out-Null
  $ErrorActionPreference = $old
  if (-not (Test-Path $dst)) { throw "Failed to render $svg" }
  Write-Host "  rendered $out ($w x $h)"
}

Write-Host 'Rendering AXIOM X1 artwork...'
Render-Svg 'assets/icon.svg'    'app/icons/icon-512.png'         512  512
Render-Svg 'assets/wordmark.svg' 'assets/wordmark.png'           900  220

# Derive all the icon sizes from the master with .NET
Add-Type -AssemblyName System.Drawing
$master = [System.Drawing.Bitmap]::FromFile((Join-Path $root 'app\icons\icon-512.png'))
$sizes = @{ 'icon-192.png' = 192; 'apple-touch-icon.png' = 180; 'icon-64.png' = 64 }
foreach ($name in $sizes.Keys) {
  $size = $sizes[$name]
  $resized = New-Object System.Drawing.Bitmap($size, $size)
  $g = [System.Drawing.Graphics]::FromImage($resized)
  $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
  $g.DrawImage($master, 0, 0, $size, $size)
  $g.Dispose()
  $resized.Save((Join-Path $root "app\icons\$name"), [System.Drawing.Imaging.ImageFormat]::Png)
  $resized.Dispose()
  Write-Host "  derived $name"
}
Copy-Item (Join-Path $root 'app\icons\icon-512.png') (Join-Path $root 'app\icons\icon-maskable-512.png') -Force
try {
  $ico = [System.Drawing.Icon]::FromHandle((New-Object System.Drawing.Bitmap((Join-Path $root 'app\icons\icon-64.png'))).GetHicon())
  $ico.Save((Join-Path $root 'app\icons\favicon.ico'))
  $ico.Dispose()
  Write-Host '  derived favicon.ico'
} catch { Write-Host '  (favicon.ico skipped)' }
$master.Dispose()
Write-Host 'Artwork complete.'
