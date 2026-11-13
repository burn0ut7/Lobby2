$ErrorActionPreference = 'Stop'
$fixture = Join-Path ([IO.Path]::GetTempPath()) ('GC-dependency-test-' + [guid]::NewGuid().ToString('N'))
$keys = @('LOCALAPPDATA','GC_INSTALL_TOOLS','GC_INSTALL_GIT','GC_WORKBENCH_MOD_WorkbenchExecutable','GC_WORKBENCH_MOD_WorkbenchProfile','GC_TEST_TRACE')
$previous = @{}
foreach ($key in $keys) { $previous[$key] = [Environment]::GetEnvironmentVariable($key) }
try {
    New-Item -ItemType Directory -Force -Path (Join-Path $fixture 'payload/lobby2-tools') | Out-Null
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot '../../installer/support/Install.ps1') -Destination $fixture
    @'
function Install-GcFiles { Add-Content -LiteralPath $env:GC_TEST_TRACE -Value 'tools' }
function Enable-GcStartup { Add-Content -LiteralPath $env:GC_TEST_TRACE -Value 'startup' }
function Install-LatestGit { param([switch]$Quiet) Add-Content -LiteralPath $env:GC_TEST_TRACE -Value 'git'; return 'Git installed' }
'@ | Set-Content -LiteralPath (Join-Path $fixture 'Common.ps1') -Encoding UTF8
    'function Set-GcOutcome { param($Status,$Detail) Add-Content -LiteralPath $env:GC_TEST_TRACE -Value ($Status + ":" + $Detail) }' |
        Set-Content -LiteralPath (Join-Path $fixture 'payload/lobby2-tools/GC-Review.ps1') -Encoding UTF8
    $env:LOCALAPPDATA = $fixture
    $env:GC_WORKBENCH_MOD_WorkbenchExecutable = 'fixture'
    $env:GC_WORKBENCH_MOD_WorkbenchProfile = 'fixture'
    foreach ($selected in @('git','tools')) {
        $env:GC_WORKBENCH_MOD_WorkbenchExecutable = if ($selected -eq 'tools') { 'fixture' } else { $null }
        $env:GC_WORKBENCH_MOD_WorkbenchProfile = if ($selected -eq 'tools') { 'fixture' } else { $null }
        $env:GC_INSTALL_TOOLS = if ($selected -eq 'tools') { '1' } else { '0' }
        $env:GC_INSTALL_GIT = if ($selected -eq 'git') { '1' } else { '0' }
        $env:GC_TEST_TRACE = Join-Path $fixture ($selected + '.txt')
        $output = @(& (Join-Path $fixture 'Install.ps1'))
        $trace = Get-Content -LiteralPath $env:GC_TEST_TRACE
        if ($selected -notin $trace -or ($selected -eq 'git' -and 'tools' -in $trace) -or ($selected -eq 'tools' -and 'git' -in $trace)) { throw 'Installer ran an unselected dependency.' }
        $events = @($output | Where-Object { $_ -like 'GC_PROGRESS:*' } | ForEach-Object { $_.Substring(12) | ConvertFrom-Json })
        if ($events.Count -ne 2 -or $events[0].Dependency -ne $selected -or $events[0].Status -ne 'Updating...' -or $events[1].State -ne 'ready') { throw 'Installer progress was missing or wrong.' }
    }
    Write-Output 'PASS: production installer skips current dependencies and emits updating/completion for the selected row.'
} finally {
    foreach ($key in $keys) { [Environment]::SetEnvironmentVariable($key, $previous[$key]) }
    $resolved = [IO.Path]::GetFullPath($fixture)
    $temp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\') + '\'
    if (!$resolved.StartsWith($temp, [StringComparison]::OrdinalIgnoreCase) -or [IO.Path]::GetFileName($resolved) -notlike 'GC-dependency-test-*') { throw 'Unsafe fixture cleanup path.' }
    if (Test-Path -LiteralPath $resolved) { Remove-Item -LiteralPath $resolved -Recurse -Force }
}
