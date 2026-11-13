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
    $hash = 'a' * 64
    Set-Content -LiteralPath (Join-Path $package 'GCWorkbench.zip.sha256') -Value ($hash + '  GCWorkbench.zip')
    $cache = Join-Path $env:LOCALAPPDATA ('GCWorkbench/ui/' + $hash)
    $helper = Join-Path $cache 'support/payload/helper'
    New-Item -ItemType Directory -Path $helper -Force | Out-Null
    # Standalone executable records inherited context and a distinctive exit.
    Add-Type -TypeDefinition @'
using System;
using System.IO;
public static class LaunchFixture {
    public static int Main(string[] args) {
        if (args.Length != 1 || args[0] != "--tools") return 20;
        var package = Environment.GetEnvironmentVariable("GC_WORKBENCH_PACKAGE");
        if (Environment.GetEnvironmentVariable("GC_WORKBENCH_INSTALL_SCRIPT") != Path.Combine(package,"Install.ps1")) return 21;
        if (!Environment.GetEnvironmentVariable("GC_WORKBENCH_MOD_Lobby2").EndsWith("addon.gproj")) return 22;
        File.WriteAllText(Path.Combine(package,"cached"),"ready");
        return 7;
    }
}
'@ -OutputAssembly (Join-Path $helper 'GC Tools.exe') -OutputType WindowsApplication
    function Invoke-Launcher {
        & "$env:SystemRoot/System32/cscript.exe" //nologo //E:JScript (Join-Path $package 'Install.js') --tools -Lobby2 (Join-Path $fixture 'Lobby only/addon.gproj')
        return $LASTEXITCODE
    }
    [IO.File]::WriteAllText((Join-Path $cache 'complete'),$hash)
    Remove-Item -LiteralPath (Join-Path $package 'launched')
    $timer=[Diagnostics.Stopwatch]::StartNew()
    if ((Invoke-Launcher) -ne 7 -or !(Test-Path (Join-Path $package 'cached')) -or (Test-Path (Join-Path $package 'launched'))) { throw 'Ready cache did not bypass PowerShell bootstrap.' }
    Write-Output ('Cached launch overhead: ' + $timer.ElapsedMilliseconds + ' ms')
    foreach ($state in @('incomplete','wrong receipt','new package','missing executable')) {
        [IO.File]::WriteAllText((Join-Path $cache 'complete'),$hash)
        Set-Content -LiteralPath (Join-Path $package 'GCWorkbench.zip.sha256') -Value ($hash + '  GCWorkbench.zip')
        switch ($state) {
            'incomplete' { Remove-Item -LiteralPath (Join-Path $cache 'complete') }
            'wrong receipt' { Set-Content -LiteralPath (Join-Path $cache 'complete') -Value ('b' * 64) }
            'new package' { Set-Content -LiteralPath (Join-Path $package 'GCWorkbench.zip.sha256') -Value (('b' * 64) + '  GCWorkbench.zip') }
            'missing executable' { Remove-Item -LiteralPath (Join-Path $helper 'GC Tools.exe') }
        }
        if ((Invoke-Launcher) -ne 0 -or !(Test-Path (Join-Path $package 'launched'))) { throw "Bootstrap skipped for $state." }
        Remove-Item -LiteralPath (Join-Path $package 'launched')
    }
    Write-Output 'PASS: prepared package launches directly with context and exit status; changed/incomplete packages use bootstrap.'
} finally {
    $env:LOCALAPPDATA = $previousLocal
    $resolved = [IO.Path]::GetFullPath($fixture)
    $temp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\') + '\'
    if (!$resolved.StartsWith($temp, [StringComparison]::OrdinalIgnoreCase) -or [IO.Path]::GetFileName($resolved) -notlike 'GC-launch-test-*') { throw 'Unsafe fixture cleanup.' }
    if (Test-Path -LiteralPath $resolved) { Remove-Item -LiteralPath $resolved -Recurse -Force }
}
