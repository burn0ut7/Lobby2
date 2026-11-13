. (Join-Path $PSScriptRoot 'Common.ps1')
. (Join-Path $PSScriptRoot 'payload/lobby2-tools/GC-Review.ps1')
$logs = Join-Path $env:LOCALAPPDATA 'GCWorkbench/logs'
New-Item -ItemType Directory -Path $logs -Force | Out-Null
$log = Join-Path $logs 'dependencies.log'
Add-Content -LiteralPath $log -Value ("`n" + [DateTime]::UtcNow.ToString('o') + ' Starting dependency installation')
function Set-GcDependencyProgress([string]$Dependency, [string]$Status, [string]$State) {
    Write-Output ('GC_PROGRESS:' + (@{Dependency=$Dependency;Status=$Status;State=$State} | ConvertTo-Json -Compress))
}
$currentDependency = 'tools'
try {
    $workbench = $env:GC_WORKBENCH_MOD_WorkbenchExecutable
    $profile = $env:GC_WORKBENCH_MOD_WorkbenchProfile
    if (!$workbench -or !$profile) { throw 'Open GC Tools from Workbench to install dependencies.' }
    $updatedTools = $env:GC_INSTALL_TOOLS -ne '0'
    if ($updatedTools) {
        Set-GcDependencyProgress 'tools' 'Updating...' 'pending'
        $destination = Join-Path $env:LOCALAPPDATA ('GCWorkbench/versions/' + [Guid]::NewGuid().ToString('N'))
        Install-GcFiles $PSScriptRoot $destination $workbench $profile | Out-Null
        Enable-GcStartup $destination
        Set-GcDependencyProgress 'tools' 'Restart Workbench' 'ready'
        Add-Content -LiteralPath $log -Value 'GC Tools installed.'
    }
    $currentDependency = 'git'
    if ($env:GC_INSTALL_GIT -ne '0') {
        Set-GcDependencyProgress 'git' 'Updating...' 'pending'
        $gitResult = Install-LatestGit -Quiet
        Add-Content -LiteralPath $log -Value $gitResult
        Set-GcDependencyProgress 'git' 'Up to date' 'ready'
    }
    $detail = if ($updatedTools) { 'Save and restart Workbench to apply the update.' } else { 'Dependencies are up to date.' }
    Set-GcOutcome 'Installed' $detail
} catch {
    Set-GcDependencyProgress $currentDependency 'Update failed' 'error'
    Add-Content -LiteralPath $log -Value ('Installation stopped: ' + $_.Exception.Message)
    Set-GcOutcome 'Installation stopped' $_.Exception.Message
    if (!$env:GC_TOOLS_OUTCOME) { throw }
    exit 1
}
