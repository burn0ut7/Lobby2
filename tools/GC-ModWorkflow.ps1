[CmdletBinding()]
param(
    [string]$Lobby2, [string]$GlobalConflictsCore, [string]$GlobalConflictsUnits,
    [string]$GlobalConflictsMissions, [string]$SelectedMods, [switch]$UpdateMods, [string]$MissionWorld, [switch]$SignIn
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

function Read-LocalChanges([string]$root) {
    Git $root @('status','--porcelain=v1','--untracked-files=all','--ignore-submodules=none')
}

function Confirm-Discard($plan) {
    $form = New-Object Windows.Forms.Form
    try {
        $form.Text = "Review local changes - $($plan.Addon.Name)"
        $form.StartPosition = 'CenterScreen'
        $form.ClientSize = New-Object Drawing.Size(700,460)
        $form.MinimumSize = New-Object Drawing.Size(540,380)
        $description = New-Object Windows.Forms.Label
        $description.Dock = 'Top'
        $description.Height = 100
        $description.Padding = New-Object Windows.Forms.Padding(10)
        $description.Text = "$($plan.Root)  [$($plan.Branch)]`r`nDiscard All adds this checkout to the final confirmation. Git will save staged, unstaged and untracked files in a local stash, then clear those changes. Ignored files stay in place.`r`n`r`nFirst column: staged. Second: working copy. M modified, A added, D deleted, R renamed, ?? untracked."
        $files = New-Object Windows.Forms.TextBox
        $files.Multiline = $true
        $files.ReadOnly = $true
        $files.WordWrap = $false
        $files.ScrollBars = 'Both'
        $files.Dock = 'Fill'
        $files.Lines = [string[]]$plan.Changes
        $buttons = New-Object Windows.Forms.FlowLayoutPanel
        $buttons.Dock = 'Bottom'
        $buttons.FlowDirection = 'RightToLeft'
        $buttons.Height = 42
        $ignore = New-Object Windows.Forms.Button
        $ignore.Text = 'Ignore'
        $ignore.DialogResult = 'Cancel'
        $discard = New-Object Windows.Forms.Button
        $discard.Text = 'Discard All'
        $discard.DialogResult = 'Yes'
        $buttons.Controls.AddRange(@($ignore,$discard))
        $form.Controls.AddRange(@($files,$description,$buttons))
        $form.AcceptButton = $ignore
        $form.CancelButton = $ignore
        $form.ActiveControl = $ignore
        return $form.ShowDialog() -eq 'Yes'
    } finally { $form.Dispose() }
}

function New-UpdatePlan($addon) {
    $root = Get-Checkout $addon.Project $addon.Guid
    $branch = Git $root @('symbolic-ref','--short','HEAD')
    $changes = @(Read-LocalChanges $root)
    $url = "https://github.com/$($addon.Repo).git"
    $head = Git $root @('ls-remote','--symref',$url,'HEAD')
    if ($head[0] -notmatch '^ref: refs/heads/(.+)\s+HEAD$') { throw 'Cannot identify the upstream default branch.' }
    $defaultBranch = $Matches[1]
    if ($head.Count -lt 2 -or $head[1] -notmatch '^([0-9a-f]{40,64})\s+HEAD$') { throw 'Cannot identify the upstream revision.' }
    $target = $Matches[1]
    $before = Git $root @('rev-parse','HEAD')
    if ($branch -cne $defaultBranch) {
        $note = "Keep working branch $branch at its current commit."
        $target = $before
    } elseif ($before -eq $target) {
        $note = "$branch is current."
    } else {
        $null = Git $root @('fetch','--quiet','--no-tags',$url,"refs/heads/$branch")
        $base = Git $root @('merge-base',$before,$target)
        if ($base -ne $before) {
            $note = "$branch contains local commits. Keep its current revision."
            $target = $before
        } else {
            $note = "Update $branch to $($target.Substring(0,8))."
        }
    }
    $plan = [pscustomobject]@{ Addon=$addon; Root=$root; Branch=$branch; Before=$before; Target=$target; Changes=$changes; Note=$note }
    if ($changes.Count) {
        if (!(Confirm-Discard $plan)) { throw "Skipped. Kept local changes on $branch." }
        $plan.Note = "Stash and clear the current contents of the reviewed paths on $branch. $note"
    } elseif ($before -eq $target) { throw $note }
    $plan
}

function Show-Message([string]$message, [string]$buttons = 'OK') {
    [Windows.Forms.MessageBox]::Show($message,'GC Git', $buttons)
}

function Connect-GitHub {
    $setupUrl = 'https://docs.github.com/en/get-started/git-basics/caching-your-github-credentials-in-git'
    $reason = 'Install Git for Windows with Git Credential Manager to connect an account.'
    $previous = $env:GCM_INTERACTIVE
    try {
        $null = Get-Command git.exe -ErrorAction Stop
        # Windows PowerShell treats native stderr as error records. GCM can write
        # browser instructions there on success, so use its exit code for the result.
        $ErrorActionPreference = 'Continue'
        $null = & git.exe credential-manager github login --help 2>&1
        if ($LASTEXITCODE -eq 0) {
            $reason = 'GitHub sign-in did not complete. You can try again from the GC menu.'
            $env:GCM_INTERACTIVE = 'true'
            $null = & git.exe credential-manager github login --browser 2>&1
            if ($LASTEXITCODE -eq 0) {
                $null = Show-Message 'Git Credential Manager completed GitHub sign-in.'
                return 0
            }
        }
    } catch {
        # Show setup guidance without exposing authentication output in Workbench.
    } finally {
        $env:GCM_INTERACTIVE = $previous
    }
    if ((Show-Message "$reason Open GitHub's setup guide?" 'YesNo') -eq 'Yes') {
        Start-Process $setupUrl
    }
    return 2
}

function Read-MissionBranch([string]$current, [string]$world) {
    Add-Type -AssemblyName Microsoft.VisualBasic
    $slug = ([IO.Path]::GetFileNameWithoutExtension($world).ToLowerInvariant() -replace '[^a-z0-9]+','-').Trim('-')
    [Microsoft.VisualBasic.Interaction]::InputBox("Current branch: $current`nCreate a local branch at the current commit. Cancel keeps the current branch.",'GC Mission Branch',"mission/$slug")
}

function Invoke-ModWorkflow {
    Add-Type -AssemblyName System.Windows.Forms
    if ($SignIn) { return Connect-GitHub }
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
                $rows += "$($addon.Name): $($plan.Note)"
                $plan
            } catch { $rows += "$($addon.Name): $_" }
        })
        $summary = $rows -join [Environment]::NewLine
        if ($plans.Count) {
            $answer = Show-Message "$summary`n`nApply these actions to the saved files on disk? If any affected editor has unsaved changes, choose Cancel and save them first. No skips all actions." 'YesNoCancel'
            if ($answer -eq 'Cancel') { return 2 }
            if ($answer -eq 'Yes') {
                $completed = @()
                try {
                    foreach ($plan in $plans) {
                        $root = Get-Checkout $plan.Addon.Project $plan.Addon.Guid
                        if ((Git $root @('symbolic-ref','--short','HEAD')) -cne $plan.Branch -or
                            (Git $root @('rev-parse','HEAD')) -ne $plan.Before -or
                            ((@(Read-LocalChanges $root) -join "`n") -cne ($plan.Changes -join "`n"))) { throw "Checkout changed since review: $root" }
                        if ($plan.Changes.Count) {
                            $label = "GC Update Mods - $($plan.Branch) - $([guid]::NewGuid())"
                            $null = Git $root @('-c','user.name=GC Workbench','-c','user.email=workbench@localhost','stash','push','--quiet','--include-untracked','--message',$label)
                            $backup = Git $root @('stash','list','-1','--format=%h %gs')
                            if (!$backup -or !$backup.EndsWith($label)) { throw "Could not verify the new stash in $root. Check git stash list before retrying." }
                            $completed += "$($plan.Addon.Name) saved local changes in stash $($backup.Split(' ')[0])"
                            if (Read-LocalChanges $root) { throw "Some changes remain in $root. Inspect the checkout before retrying." }
                        }
                        if ($plan.Before -ne $plan.Target) {
                            $null = Git $root @('merge','--ff-only','--no-overwrite-ignore','--quiet',$plan.Target)
                            $completed += "$($plan.Addon.Name) updated to $($plan.Target.Substring(0,8))"
                        }
                    }
                } finally {
                    if ($completed.Count) {
                        $null = Show-Message "$($completed -join [Environment]::NewLine)`n`nRefresh the affected Workbench resources and reload scripts before starting a template."
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
