$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '../../installer/support/Common.ps1')
# Exercise real install/bootstrap functions in a fresh local profile, without
# changing Windows startup registration, starting an editor or installing Git.
function Remove-GcLegacyStartup { }
$original = $env:LOCALAPPDATA
$fixture = Join-Path ([IO.Path]::GetTempPath()) ('GC-install-test-' + [guid]::NewGuid().ToString('N'))
try {
    $env:LOCALAPPDATA = Join-Path $fixture 'local'
    $profile = Join-Path $fixture 'profile'
    $source = Join-Path $fixture 'source'
    New-Item -ItemType Directory -Force -Path $profile,(Join-Path $source 'payload/generic'),(Join-Path $source 'payload/helper'),(Join-Path $source 'payload/lobby2-tools') | Out-Null
    $exe = Join-Path $fixture 'ArmaReforgerWorkbenchSteamDiag.exe'
    Set-Content -LiteralPath $exe -Value 'fixture'
    Set-Content -LiteralPath (Join-Path $profile 'wbSettingsDump.ini') -Value "NetAPI_Enabled=1`nNetAPI_Port=5776"
    foreach ($name in @('Common.ps1','Uninstall.ps1','LobbyTools.ps1','Cleanup.ps1')) { Copy-Item -LiteralPath (Join-Path $PSScriptRoot ('../../installer/support/' + $name)) -Destination $source }
    foreach ($name in @('GC-ModLauncher.js','GC-ModWorkflow.ps1','GC-SubmitMission.ps1','GC-Review.ps1','GC-GitHub.ps1','GC-ModStatus.ps1')) {
        Set-Content -LiteralPath (Join-Path $source ('payload/lobby2-tools/' + $name)) -Value 'fixture'
    }
    Set-Content -LiteralPath (Join-Path $source 'payload/generic/gcworkbench.dll') -Value 'fixture'
    Set-Content -LiteralPath (Join-Path $source 'payload/helper/GCWorkbench.Loader.exe') -Value 'fixture'
    $destination = Join-Path $env:LOCALAPPDATA 'GCWorkbench/versions/test'
    Install-GcFiles $source $destination $exe $profile | Out-Null
    Enable-GcStartup $destination
    # Workbench supplies a trailing slash, while older installers saved none.
    # Exercise a real new-version update, including preexisting helper ownership.
    $update = Join-Path $env:LOCALAPPDATA 'GCWorkbench/versions/update'
    Install-GcFiles $source $update $exe ($profile.Replace('\', '/') + '/') | Out-Null
    Enable-GcStartup $update
    Install-GcLobbyTools $update ($profile.ToUpperInvariant() + '\.\')
    $updated = Get-Content -LiteralPath (Join-Path $env:LOCALAPPDATA 'GCWorkbench/installation.json') -Raw | ConvertFrom-Json
    if ($updated.destination -ne $update -or $updated.profileDirectory -ne $profile) { throw 'Equivalent profile paths did not update in place.' }
    if (Test-GcSamePath $profile ($profile + '-other')) { throw 'Distinct profiles must not compare equal.' }
    # Legacy records may themselves contain mixed separators and trailing slashes.
    $helperRecord = Join-Path $env:LOCALAPPDATA 'GCWorkbench/lobby-tools.json'
    $legacy = Get-Content -LiteralPath $helperRecord -Raw | ConvertFrom-Json
    $legacy.profile = $profile.Replace('\', '/') + '/'
    $legacy | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $helperRecord -Encoding UTF8
    Install-GcLobbyTools $update $profile
    $destination = $update
    $record = Get-Content -LiteralPath (Join-Path $env:LOCALAPPDATA 'GCWorkbench/installation.json') -Raw | ConvertFrom-Json
    $script = Get-Content -LiteralPath $record.startupPath -Raw
    if ($script -notmatch 'GCWorkbench.Loader.exe' -or $script -match 'ReforgerMcp|--config|--start-authoring') { throw 'Bootstrap is not independent.' }
    if (Test-Path -LiteralPath (Join-Path $destination 'configuration.json')) { throw 'Server configuration leaked into install.' }
    $settings = Get-Content -LiteralPath (Join-Path $destination 'settings.json') -Raw | ConvertFrom-Json
    if ($settings.port -ne 5776 -or $settings.profileDirectory -ne $profile) { throw 'Wrong profile/port.' }
    Enable-GcStartup $destination
    Add-Content -LiteralPath $record.startupPath -Value '// user edit'
    $refused = $false
    try { Enable-GcStartup $destination } catch { $refused = $_.Exception.Message -match 'Modified startup script preserved' }
    if (!$refused) { throw 'Modified startup was not preserved.' }
    $refused = $false
    try { Uninstall-GcIntegration } catch { $refused = $_.Exception.Message -match 'modified startup script was preserved' }
    $disabled = Get-Content -LiteralPath (Join-Path $env:LOCALAPPDATA 'GCWorkbench/installation.json') -Raw | ConvertFrom-Json
    if (!$refused -or $disabled.enabled -ne $false -or !(Test-Path -LiteralPath $record.startupPath)) { throw 'Uninstall must revoke loading and preserve edited bootstrap.' }
    [IO.File]::WriteAllText($record.startupPath, $script, (New-Object Text.UTF8Encoding($true)))
    $disabled.startupSha256 = (Get-FileHash -LiteralPath $record.startupPath).Hash
    $disabled | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $env:LOCALAPPDATA 'GCWorkbench/installation.json') -Encoding UTF8
    # Capture the background worker rather than touching the real running editor.
    function Start-Process { param($FilePath,$WindowStyle,$ArgumentList) $script:cleanupScript = $ArgumentList[5].Trim('"') }
    $removed = Uninstall-GcIntegration
    if (!$removed.Contains('Restart Workbench') -or (Test-Path -LiteralPath $record.startupPath) -or (Test-Path -LiteralPath (Join-Path $env:LOCALAPPDATA 'GCWorkbench/installation.json'))) { throw 'Uninstall did not unregister immediately.' }
    if (!(Test-Path -LiteralPath $destination)) { throw 'Uninstall removed live binaries before worker cleanup.' }
    $next = Join-Path $env:LOCALAPPDATA 'GCWorkbench/versions/reinstalled'
    Install-GcFiles $source $next $exe $profile | Out-Null
    Enable-GcStartup $next
    & $script:cleanupScript
    if ((Test-Path -LiteralPath $destination) -or !(Test-Path -LiteralPath $next) -or !(Test-Path -LiteralPath $record.startupPath)) { throw 'Deferred cleanup missed old files or damaged reinstall.' }
    $jobPath = Join-Path (Split-Path $script:cleanupScript -Parent) 'job.json'
    @{ root = (Join-Path $env:LOCALAPPDATA 'GCWorkbench/versions'); targets = @($next, $profile) } | ConvertTo-Json | Set-Content -LiteralPath $jobPath -Encoding UTF8
    & (Join-Path $PSHOME 'powershell.exe') -NoProfile -File $script:cleanupScript
    if ($LASTEXITCODE -ne 1 -or !(Test-Path -LiteralPath $next) -or !(Test-Path -LiteralPath $profile)) { throw 'Cleanup must validate all targets before any deletion.' }
    Write-Output 'PASS: out-of-root cleanup rejected before deleting any files.'
    Write-Output 'PASS: uninstall unregisters immediately, defers binaries and preserves a subsequent reinstall.'
    Write-Output 'PASS: independent fresh install, fixed bootstrap, profile settings, repeat installation and modified-file preservation.'
} finally {
    $env:LOCALAPPDATA = $original
    $resolved = [IO.Path]::GetFullPath($fixture)
    $temp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\') + '\'
    if (!$resolved.StartsWith($temp, [StringComparison]::OrdinalIgnoreCase) -or [IO.Path]::GetFileName($resolved) -notlike 'GC-install-test-*') { throw 'Unsafe fixture cleanup path.' }
    if (Test-Path -LiteralPath $resolved) { Remove-Item -LiteralPath $resolved -Recurse -Force }
}
