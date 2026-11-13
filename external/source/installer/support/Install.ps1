. (Join-Path $PSScriptRoot 'Common.ps1')
. (Join-Path $PSScriptRoot 'payload/lobby2-tools/GC-Review.ps1')
$logs = Join-Path $env:LOCALAPPDATA 'GCWorkbench/logs'
New-Item -ItemType Directory -Path $logs -Force | Out-Null
$log = Join-Path $logs 'dependencies.log'
Add-Content -LiteralPath $log -Value ("`n" + [DateTime]::UtcNow.ToString('o') + ' Starting dependency installation')
try {
    $workbench = $env:GC_WORKBENCH_MOD_WorkbenchExecutable
    $profile = $env:GC_WORKBENCH_MOD_WorkbenchProfile
    if (!$workbench -or !$profile) { throw 'Open GC Tools from Workbench to install dependencies.' }
    $destination = Join-Path $env:LOCALAPPDATA ('GCWorkbench/versions/' + [Guid]::NewGuid().ToString('N'))
    Install-GcFiles $PSScriptRoot $destination $workbench $profile | Out-Null
    Enable-GcStartup $destination
    Add-Content -LiteralPath $log -Value 'GC Tools installed. Installing Git.'
    $gitResult = Install-LatestGit -Quiet
    Add-Content -LiteralPath $log -Value $gitResult
    Set-GcOutcome 'Installed' 'Save and restart Workbench to apply the dropdown extension.'
} catch {
    Add-Content -LiteralPath $log -Value ('Installation stopped: ' + $_.Exception.Message)
    Set-GcOutcome 'Installation stopped' $_.Exception.Message
    if (!$env:GC_TOOLS_OUTCOME) { throw }
    exit 1
}
