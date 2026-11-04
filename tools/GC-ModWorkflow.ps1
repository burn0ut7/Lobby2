[CmdletBinding()]
param(
    [string]$Lobby2, [string]$GlobalConflictsCore, [string]$GlobalConflictsUnits,
    [string]$GlobalConflictsMissions, [string]$SelectedMods, [switch]$UpdateMods, [string]$MissionWorld
)

$ErrorActionPreference = 'Stop'
$env:GIT_TERMINAL_PROMPT = '0'
$env:GCM_INTERACTIVE = 'Never'

function Git([string]$root, [string[]]$arguments) {
    $output = @(& git.exe -C $root @arguments 2>&1)
    if ($LASTEXITCODE -ne 0) { throw "Git failed ($($arguments[0])) in $root. $($output -join ' ')" }
    $output | ForEach-Object { "$_" }
}

function Get-Checkout([string]$project, [string]$guid) {
    if (!$project -or !(Test-Path -LiteralPath $project -PathType Leaf)) { throw "Source checkout missing. Expected $project" }
    if ((Get-Content -LiteralPath $project -Raw) -notmatch ('(?m)^\s*GUID "' + $guid + '"')) { throw 'Unexpected addon identity.' }
    $root = [IO.Path]::GetFullPath((Split-Path -LiteralPath $project))
    $top = Git $root @('rev-parse','--show-toplevel')
    if ([IO.Path]::GetFullPath($top) -ne [IO.Path]::GetFullPath($root)) { throw 'Addon must be its own Git checkout.' }
    foreach ($marker in 'MERGE_HEAD','CHERRY_PICK_HEAD','REVERT_HEAD','rebase-apply','rebase-merge','sequencer','BISECT_START') {
        $path = Git $root @('rev-parse','--path-format=absolute','--git-path',$marker)
        if (Test-Path -LiteralPath $path) { throw 'Finish the current Git operation first.' }
    }
    $root
}

function New-UpdatePlan($addon) {
    $root = Get-Checkout $addon.Project $addon.Guid
    $branch = Git $root @('symbolic-ref','--short','HEAD')
    if (Git $root @('status','--porcelain','--untracked-files=normal')) { throw "$branch has local changes." }
    $url = "https://github.com/$($addon.Repo).git"
    $head = Git $root @('ls-remote','--symref',$url,'HEAD')
    if ($head[0] -notmatch '^ref: refs/heads/(.+)\s+HEAD$') { throw 'Cannot identify the upstream default branch.' }
    if ($branch -cne $Matches[1]) { throw "Keeping working branch $branch." }
    if ($head.Count -lt 2 -or $head[1] -notmatch '^([0-9a-f]{40,64})\s+HEAD$') { throw 'Cannot identify the upstream revision.' }
    $target = $Matches[1]
    $null = Git $root @('fetch','--quiet','--no-tags',$url,"refs/heads/$branch")
    $before = Git $root @('rev-parse','HEAD')
    if ($before -eq $target) { throw "$branch is current." }
    $base = Git $root @('merge-base',$before,$target)
    if ($base -ne $before) { throw "$branch contains local commits. Keeping current revision." }
    [pscustomobject]@{ Addon=$addon; Root=$root; Branch=$branch; Before=$before; Target=$target }
}

function Show-Message([string]$message, [string]$buttons = 'OK') {
    [Windows.Forms.MessageBox]::Show($message,'GC Git', $buttons)
}

function Read-MissionBranch([string]$current, [string]$world) {
    Add-Type -AssemblyName Microsoft.VisualBasic
    $slug = ([IO.Path]::GetFileNameWithoutExtension($world).ToLowerInvariant() -replace '[^a-z0-9]+','-').Trim('-')
    [Microsoft.VisualBasic.Interaction]::InputBox("Current branch: $current`nCreate a local branch at the current commit. Cancel keeps the current branch.",'GC Mission Branch',"mission/$slug")
}

function Invoke-ModWorkflow {
    Add-Type -AssemblyName System.Windows.Forms
    $addons = @(
        @{Name='Lobby2'; Key='Lobby2'; Project=$Lobby2; Guid='6A115D5DFF7ED870'; Repo='burn0ut7/Lobby2'}
        @{Name='GC Core'; Key='GlobalConflictsCore'; Project=$GlobalConflictsCore; Guid='623555110E2B2CA0'; Repo='Global-Conflicts-ArmA/gc-reforger-core'}
        @{Name='GC Units'; Key='GlobalConflictsUnits'; Project=$GlobalConflictsUnits; Guid='6156F2F771D5D73D'; Repo='Global-Conflicts-ArmA/gc-reforger-units'}
        @{Name='GC Missions'; Key='GlobalConflictsMissions'; Project=$GlobalConflictsMissions; Guid='62D55563549DA97A'; Repo='Global-Conflicts-ArmA/gc-reforger-missions'}
    )
    $null = Get-Command git.exe -ErrorAction Stop
    # Workbench only resolves mounts loaded in the current project. Source checkouts
    # installed alongside Lobby2 can still be updated after their GUID is verified.
    $addonsDirectory = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
    foreach ($addon in $addons) {
        if (!$addon.Project) {
            $folder = ($addon.Repo -split '/')[-1]
            $addon.Project = Join-Path $addonsDirectory "$folder/addon.gproj"
        }
    }
    if ($UpdateMods -and !$SelectedMods) { $SelectedMods = $addons.Key -join ',' }
    if ($SelectedMods) {
        $selectedKeys = $SelectedMods -split ','
        $rows = @()
        $plans = @(foreach ($addon in $addons) {
            if ($addon.Key -notin $selectedKeys) { continue }
            try {
                $plan = New-UpdatePlan $addon
                $rows += "$($addon.Name) ($($plan.Branch)): update to $($plan.Target.Substring(0,8))."
                $plan
            } catch { $rows += "$($addon.Name): $_" }
        })
        $summary = $rows -join [Environment]::NewLine
        if ($plans.Count) {
            $answer = Show-Message "$summary`n`nHave you saved changes in all affected Workbench windows? Choose Yes to apply the listed updates. No skips updates. Cancel stops the workflow." 'YesNoCancel'
            if ($answer -eq 'Cancel') { return 2 }
            if ($answer -eq 'Yes') {
                $completed = @()
                try {
                    foreach ($plan in $plans) {
                        $root = Get-Checkout $plan.Addon.Project $plan.Addon.Guid
                        if ((Git $root @('symbolic-ref','--short','HEAD')) -cne $plan.Branch -or
                            (Git $root @('rev-parse','HEAD')) -ne $plan.Before -or
                            (Git $root @('status','--porcelain','--untracked-files=normal'))) { throw "Checkout changed: $root" }
                        $null = Git $root @('merge','--ff-only','--no-overwrite-ignore','--quiet',$plan.Target)
                        $completed += $plan.Addon.Name
                    }
                } finally {
                    if ($completed.Count) {
                        $null = Show-Message "Updated: $($completed -join ', ').`nRefresh the affected Workbench resources and reload scripts before starting a template."
                    }
                }
                return 2
            }
        } else {
            if ((Show-Message "$summary`n`nContinue without updates?" 'OKCancel') -eq 'Cancel') { return 2 }
        }
    }
    if ($MissionWorld) {
        $missions = $addons | Where-Object Key -eq GlobalConflictsMissions
        $root = Get-Checkout $missions.Project $missions.Guid
        $world = (Get-Item -LiteralPath $MissionWorld).FullName
        if (!$world.StartsWith($root.TrimEnd('\','/') + '\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Select a saved world in GC Missions to create its branch.' }
        $current = Git $root @('symbolic-ref','--short','HEAD')
        $branch = Read-MissionBranch $current $world
        if ($branch) {
            if ($branch -notmatch '^[a-zA-Z0-9][a-zA-Z0-9._/-]*$') { throw 'Use letters, numbers, slashes, dots, underscores or hyphens in the branch name.' }
            $null = Git $root @('check-ref-format','--branch',$branch)
            if ((Git $root @('symbolic-ref','--short','HEAD')) -cne $current) { throw 'The current branch changed. Try again.' }
            $null = Git $root @('switch','--quiet','-c',$branch)
            if ((Git $root @('symbolic-ref','--short','HEAD')) -cne $branch) { throw 'Could not verify the new branch.' }
        }
    }
    return 0
}

if ($MyInvocation.InvocationName -ne '.') {
    try { exit (Invoke-ModWorkflow) }
    catch { $null = Show-Message "$_"; exit 2 }
}
