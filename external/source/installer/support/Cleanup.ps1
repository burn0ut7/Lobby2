param([switch]$Wait)
$ErrorActionPreference = 'Stop'
# Runs outside the installation. Never removes shared registration or cache.
try {
    $job = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'job.json') -Raw | ConvertFrom-Json
    $root = [IO.Path]::GetFullPath($job.root).TrimEnd('\')
    $expected = [IO.Path]::GetFullPath((Join-Path $env:LOCALAPPDATA 'GCWorkbench/versions')).TrimEnd('\')
    if ($root -ne $expected) { throw 'Unexpected cleanup root.' }
    foreach ($target in $job.targets) {
        if ([IO.Path]::GetDirectoryName([IO.Path]::GetFullPath($target)) -ne $root) { throw 'Unexpected cleanup target.' }
    }
    if ($Wait) {
        do {
            $busy = @(Get-Process -Name 'ArmaReforgerWorkbenchSteamDiag','GCWorkbench.UI','GCWorkbench.Loader' -ErrorAction SilentlyContinue)
            if ($busy.Count) { Start-Sleep -Seconds 3 }
        } while ($busy.Count)
    }
    foreach ($target in $job.targets) {
        $path = [IO.Path]::GetFullPath($target)
        $ancestor = $path
        while ($ancestor) {
            if ((Test-Path -LiteralPath $ancestor) -and ((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'Linked cleanup path preserved.' }
            $ancestor = [IO.Path]::GetDirectoryName($ancestor)
        }
        if (!(Test-Path -LiteralPath $path)) { continue }
        if (@(Get-ChildItem -LiteralPath $path -Recurse -Force | Where-Object { $_.Attributes -band [IO.FileAttributes]::ReparsePoint }).Count) { throw 'Linked installation preserved.' }
        Remove-Item -LiteralPath $path -Recurse -Force
    }
    Remove-Item -LiteralPath (Join-Path $PSScriptRoot 'job.json')
} catch {
    $_.Exception.Message | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'cleanup-error.txt')
    exit 1
}
