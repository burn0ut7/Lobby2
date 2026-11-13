$ErrorActionPreference = 'Stop'
$fixture = Join-Path ([IO.Path]::GetTempPath()) ('GC-launch-test-' + [guid]::NewGuid().ToString('N'))
$previousLocal = $env:LOCALAPPDATA
try {
    $package = Join-Path $fixture 'Lobby only/external/workbench'
    New-Item -ItemType Directory -Path $package -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot '../../installer/external/Install.js') -Destination $package
    @'
param([switch]$Tools)
if (!$Tools) { exit 10 }
if ($env:GC_WORKBENCH_PACKAGE -ne $PSScriptRoot) { exit 11 }
if ($env:GC_WORKBENCH_INSTALL_SCRIPT -ne (Join-Path $PSScriptRoot 'Install.ps1')) { exit 12 }
[IO.File]::WriteAllText((Join-Path $PSScriptRoot 'launched'), 'Lobby package')
exit 0
'@ | Set-Content -LiteralPath (Join-Path $package 'Install.ps1')
    $env:LOCALAPPDATA = Join-Path $fixture 'local'
    New-Item -ItemType Directory -Path (Join-Path $env:LOCALAPPDATA 'GCWorkbench') -Force | Out-Null
    # A stale registration must not override the cloned Lobby package.
    [IO.File]::WriteAllText((Join-Path $env:LOCALAPPDATA 'GCWorkbench/ui-path.txt'), 'Z:\deleted-install\GC Tools.exe', [Text.Encoding]::Unicode)
    & "$env:SystemRoot\System32\cscript.exe" //nologo //E:JScript (Join-Path $package 'Install.js') --tools -Lobby2 (Join-Path $fixture 'Lobby only/addon.gproj')
    if ($LASTEXITCODE -ne 0 -or !(Test-Path -LiteralPath (Join-Path $package 'launched'))) { throw 'Lobby-only bootstrap failed.' }
    Write-Output 'PASS: Lobby-only launcher uses its own package despite stale registration, without optional addon paths.'
} finally {
    $env:LOCALAPPDATA = $previousLocal
    $resolved = [IO.Path]::GetFullPath($fixture)
    $temp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\') + '\'
    if (!$resolved.StartsWith($temp, [StringComparison]::OrdinalIgnoreCase) -or [IO.Path]::GetFileName($resolved) -notlike 'GC-launch-test-*') { throw 'Unsafe fixture cleanup.' }
    if (Test-Path -LiteralPath $resolved) { Remove-Item -LiteralPath $resolved -Recurse -Force }
}
