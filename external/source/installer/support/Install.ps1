. (Join-Path $PSScriptRoot 'Common.ps1')
. (Join-Path $PSScriptRoot 'payload/lobby2-tools/GC-Review.ps1')
try {
    $workbench = $env:GC_WORKBENCH_MOD_WorkbenchExecutable
    $profile = $env:GC_WORKBENCH_MOD_WorkbenchProfile
    if (!$workbench -or !$profile) { throw 'Open GC Tools from Workbench to install dependencies.' }
    $destination = Join-Path $env:LOCALAPPDATA ('GCWorkbench/versions/' + [Guid]::NewGuid().ToString('N'))
    Install-GcFiles $PSScriptRoot $destination $workbench $profile | Out-Null
    Enable-GcStartup $destination
    Install-LatestGit -Quiet | Out-Null
    Set-GcOutcome 'Installed' 'Save and restart Workbench to apply the dropdown extension.'
} catch {
    Set-GcOutcome 'Installation stopped' $_.Exception.Message
    if (!$env:GC_TOOLS_OUTCOME) { throw }
    exit 1
}
