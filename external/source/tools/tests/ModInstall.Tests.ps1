$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '../GC-ModStatus.ps1')
function Assert($value, $message) { if (!$value) { throw $message } }
$fixture = Join-Path ([IO.Path]::GetTempPath()) ('gc-mod-install-' + [guid]::NewGuid().ToString('N'))
$keys = @('GIT_CONFIG_GLOBAL','GIT_CONFIG_NOSYSTEM','GC_TOOLS_OUTCOME')
$previous = @{}
foreach ($key in $keys) { $previous[$key] = [Environment]::GetEnvironmentVariable($key) }
try {
    $parent = Join-Path $fixture 'addons'
    $lobby = Join-Path $parent 'Lobby renamed'
    $null = New-Item -ItemType Directory -Path $lobby -Force
    $Lobby2 = Join-Path $lobby 'addon.gproj'
    Set-Content -LiteralPath $Lobby2 -Value ' GUID "6A115D5DFF7ED870"'
    $env:GIT_CONFIG_GLOBAL = Join-Path $fixture 'gitconfig'
    $env:GIT_CONFIG_NOSYSTEM = '1'
    $addons = @(Get-GcAddons $Lobby2 '' '' '') | Where-Object Key -ne 'Lobby2'
    foreach ($addon in $addons) {
        $upstream = Join-Path $fixture ($addon.Key + '-upstream')
        $null = New-Item -ItemType Directory -Path $upstream
        $null = Git $upstream @('init','-q','-b','main')
        $null = Git $upstream @('config','user.name','Fixture')
        $null = Git $upstream @('config','user.email','fixture@example.invalid')
        Set-Content -LiteralPath (Join-Path $upstream 'addon.gproj') -Value ' GUID "RELEASE-NOT-DEV"'
        $null = Git $upstream @('add','.'); $null = Git $upstream @('commit','-qm','Release')
        $null = Git $upstream @('switch','-qc','dev')
        Set-Content -LiteralPath (Join-Path $upstream 'addon.gproj') -Value (' GUID "' + $addon.Guid + '"')
        $null = Git $upstream @('add','.'); $null = Git $upstream @('commit','-qm','Development')
        if ($addon.Key -eq 'GlobalConflictsMissions') {
            # Populate the tree without depending on PowerShell's own path limits.
            $longPath = 'worlds/' + ('MissionAssets/' * 15) + 'Character_Sharpshooter.et.meta'
            $blob = Git $upstream @('hash-object','-w','addon.gproj')
            $null = Git $upstream @('update-index','--add','--cacheinfo',("100644,$blob,$longPath"))
            $null = Git $upstream @('commit','-qm','Long mission path')
        }
        $null = Git $upstream @('switch','-q','main')
        $null = Git $upstream @('config','--global','--add',("url.$upstream.insteadOf"),("https://github.com/" + $addon.Repo + '.git'))
        $status = Get-GcModStatus $addon
        Assert ($status.UpdateNeeded -and $status.Status -eq 'Not installed') 'Missing Dev mod was not offered.'
    }
    # Exercise the real selection dispatcher; unchecked repositories stay absent.
    $SelectedMods = 'GlobalConflictsCore'
    $env:GC_TOOLS_OUTCOME = Join-Path $fixture 'outcome.json'
    $null = Invoke-ModWorkflow
    $outcome = Get-Content -LiteralPath $env:GC_TOOLS_OUTCOME -Raw | ConvertFrom-Json
    Assert ($outcome.Status -eq 'Updated' -and $outcome.Detail -match 'Installed\.') 'Install was not reported complete.'
    Assert ((Test-Path -LiteralPath $addons[0].Project) -and !(Test-Path -LiteralPath $addons[1].Project) -and !(Test-Path -LiteralPath $addons[2].Project)) 'Unselected mods were cloned.'
    foreach ($addon in $addons) {
        $changed = $false; $pending = $false
        $result = Update-Mod $addon ([ref]$changed) ([ref]$pending)
        Assert ($result -in @('Installed.','Up to date.')) 'Clone/update failed.'
        $root = Split-Path -Parent $addon.Project
        Assert ((Split-Path -Parent $root) -eq $parent) 'Clone was not beside Lobby.'
        Assert ((Git $root @('branch','--show-current')) -eq 'dev') 'Clone selected a release branch.'
        Assert (!(Get-GcModStatus $addon).UpdateNeeded) 'Installed mod remained actionable.'
        Assert ((Git $root @('config','--local','core.longpaths')) -eq 'true') 'Clone did not retain long-path support.'
        if ($addon.Key -eq 'GlobalConflictsMissions') {
            Assert ((Join-Path $root $longPath).Length -gt 260) 'Fixture did not exceed Windows path limit.'
            Assert ((Git $root @('hash-object','--',$longPath)) -eq $blob) 'Long file was not checked out.'
            # Existing checkouts also work even if their saved setting disables it.
            $null = Git $root @('config','--local','core.longpaths','false')
            Assert (!(Git $root @('status','--porcelain'))) 'Long files reported missing in an existing checkout.'
            $upstream = Join-Path $fixture ($addon.Key + '-upstream')
            $null = Git $upstream @('switch','-q','dev')
            $nextBlob = Git $upstream @('hash-object','-w','.git/HEAD')
            $null = Git $upstream @('update-index','--cacheinfo',("100644,$nextBlob,$longPath"))
            $null = Git $upstream @('commit','-qm','Update long mission path')
            Assert ((Update-Mod $addon ([ref]$changed) ([ref]$pending)) -eq 'Updated.') 'Long-path update failed.'
            Assert ((Git $root @('hash-object','--',$longPath)) -eq $nextBlob) 'Long file was not updated.'
        }
    }
    # Reuse metadata in a fresh sibling area for failure/preservation checks.
    $other = Join-Path $fixture 'other-addons'
    $null = New-Item -ItemType Directory -Path $other
    $addon = $addons[0].Clone()
    $addon.AddonsDirectory = $other
    $root = Join-Path $other 'gc-reforger-core'
    $addon.Project = Join-Path $root 'addon.gproj'
    $null = New-Item -ItemType Directory -Path $root
    Set-Content -LiteralPath (Join-Path $root 'my-file') -Value 'keep'
    Assert (!(Get-GcModStatus $addon).UpdateNeeded) 'Incomplete existing folder enabled cloning.'
    $changed = $false; $pending = $false; $rejected = $false
    try { Update-Mod $addon ([ref]$changed) ([ref]$pending) } catch { $rejected = $true }
    Assert ($rejected -and (Get-Content -LiteralPath (Join-Path $root 'my-file')) -eq 'keep') 'Existing files were overwritten.'
    # Missing dev and wrong addon identity must leave no published/partial checkout.
    $failParent = Join-Path $fixture 'failed-addons'
    $null = New-Item -ItemType Directory -Path $failParent
    $addon.AddonsDirectory = $failParent
    $addon.Project = Join-Path $failParent 'gc-reforger-core/addon.gproj'
    $upstream = Join-Path $fixture 'GlobalConflictsCore-upstream'
    $null = Git $upstream @('branch','-D','dev')
    $rejected = $false
    try { Update-Mod $addon ([ref]$changed) ([ref]$pending) } catch { $rejected = $true }
    Assert ($rejected -and !(Get-ChildItem -LiteralPath $failParent -Force)) 'Failed clone left a partial folder or used main.'
    $null = Git $upstream @('branch','dev')
    $rejected = $false
    try { Update-Mod $addon ([ref]$changed) ([ref]$pending) } catch { $rejected = $_.Exception.Message -eq 'Wrong mod folder.' }
    Assert ($rejected -and !(Get-ChildItem -LiteralPath $failParent -Force)) 'Wrong addon identity was published.'
    Write-Output 'PASS: selected Dev clones, long-path checkout and update, current status, no release fallback, existing-file preservation and failed-clone cleanup.'
} finally {
    foreach ($key in $keys) { [Environment]::SetEnvironmentVariable($key, $previous[$key]) }
    $resolved = [IO.Path]::GetFullPath($fixture)
    $temp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\') + '\'
    if (!$resolved.StartsWith($temp, [StringComparison]::OrdinalIgnoreCase) -or [IO.Path]::GetFileName($resolved) -notlike 'gc-mod-install-*') { throw 'Unsafe fixture cleanup.' }
    if (Test-Path -LiteralPath $resolved) { Remove-Item -LiteralPath $resolved -Recurse -Force }
}
