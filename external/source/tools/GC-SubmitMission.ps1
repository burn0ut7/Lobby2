# Git-only branch submission. Never saves, exports, or queries Workbench.
# Discover the contributor's one request, independent of their local branch.
function Get-ContributorMissionPr($auth,$repo,$owner,$base) {
    $baseName=[Uri]::EscapeDataString($base)
    $forkRepo=$owner+'/'+($repo -split '/')[1]
    foreach ($state in @('open','closed')) {
        $found=@()
        for ($page=1; $page -le 100; $page++) {
            # Windows PowerShell REST responses can arrive as one array object.
            # Enumerate the response before filtering individual requests.
            $response=Invoke-SubmissionApi $auth GET "repos/$repo/pulls?state=$state&base=$baseName&sort=created&direction=desc&per_page=100&page=$page"
            $items=@($response | ForEach-Object { $_ })
            $matches=@($items | Where-Object { $_.head.repo.full_name -ieq $forkRepo -and $_.base.ref -ceq $base })
            if ($state -eq 'closed' -and $matches.Count) { return $matches[0] }
            $found+= $matches
            if ($items.Count -lt 100) { break }
            if ($page -eq 100) { throw 'Could not finish checking existing submissions. Nothing was uploaded.' }
        }
        if ($found.Count -gt 1) { throw 'More than one approval request already exists. Ask a reviewer to keep one open before submitting.' }
        if ($found.Count) { return $found[0] }
    }
}

function Get-SubmissionPrRevision($pr) {
    if (!$pr) { return '' }
    @($pr.number,$pr.state,$pr.merged_at,$pr.title,$pr.body,$pr.head.ref) | ConvertTo-Json -Compress
}

function Get-SubmissionAuth { Get-GcGitHubAuth }

function Invoke-SubmissionApi($auth, $method, $path, $data, [switch]$AllowMissing) {
    $request = @{Uri="https://api.github.com/$path"; Headers=$auth; Method=$method; ErrorAction='Stop'; TimeoutSec=30}
    if ($null -ne $data) {
        $request.ContentType = 'application/json; charset=utf-8'
        $request.Body = [Text.Encoding]::UTF8.GetBytes(($data | ConvertTo-Json -Compress))
    }
    try { Invoke-RestMethod @request } catch {
        $status = 0
        if ($_.Exception.Response) { $status = [int]$_.Exception.Response.StatusCode }
        if ($AllowMissing -and $status -eq 404) { return }
        if ($status -in 401,403) { throw 'GitHub denied access. Check your sign-in and repository permissions.' }
        throw "GitHub could not complete the request ($status). Retry Submit Mission."
    }

}

function New-SubmissionCommit($root, $tree, $parents, $title, $user) {
    $arguments=@('-c',('user.name='+$user.login),'-c',('user.email='+$user.id+'+'+$user.login+'@users.noreply.github.com'),'-c','commit.gpgSign=false','commit-tree',$tree)
    foreach ($parent in @($parents | Select-Object -Unique)) { $arguments+=@('-p',$parent) }
    Git $root ($arguments+@('-m',$title))
}

function Get-SubmissionRef($root,$ref) {
    $previous=$ErrorActionPreference
    try {
        $ErrorActionPreference='Continue'
        $value=@(& git.exe -C $root rev-parse --verify --quiet $ref 2>$null)
        if ($LASTEXITCODE -eq 0) { return [string]$value[0] }
        if ($LASTEXITCODE -ne 1) { throw 'Could not read the saved submission.' }
    } finally { $ErrorActionPreference=$previous }
}

function Get-SubmissionSnapshot($root,$tip) {
    $index=Join-Path ([IO.Path]::GetTempPath()) ('gc-submit-'+[guid]::NewGuid()+'.index')
    $previous=$env:GIT_INDEX_FILE
    try {
        $env:GIT_INDEX_FILE=$index
        $null=Git $root @('read-tree',$tip)
        $null=Git $root @('add','-A','--','.')
        Git $root @('write-tree')
    } finally {
        $env:GIT_INDEX_FILE=$previous
        if ([IO.File]::Exists($index)) { [IO.File]::Delete($index) }
    }
}

function Get-MissionSubmissionTemplate {
    @{
        Overview='Put a brief summary of the mission added or updated here.'
        Testing='If you have not already, please read [testing your mission](https://github.com/Global-Conflicts-ArmA/gc-reforger-missions/wiki/Testing-your-mission).'
        Question='Have you tested the following are all working as intended?'
        Reviewer='Put any specifics the mission reviewer needs to look out for or double check here.'
        Dependencies="If this mission is dependent on a pull request to GC units or core, or the TilW framework, link it here.`n`nIf this mission is dependent on a mod not currently on the GC mod list, put it here, though understand rejected or incompatible mods may require substantial changes to your mission."
        Checks=@('Briefing, map, slotting screen','Setup timers / AO Limits','Custom prefabs','Key framework events and interactions (QRFs, etc)','End conditions','Playable in Dedicated Server Tool / Peer Tool')
    }
}

function Get-SubmissionDescriptionFields($body,$template) {
    $body=([string]$body).Replace("`r`n","`n")
    $values=@{overview='';reviewer='';dependencies='';testingNotes='';checks=@{}}
    $pattern='(?ms)\A# Overview\r?\n(?<overview>.*?)^## Testing\r?\n(?<testing>.*?)^## Comments for the reviewer\r?\n(?<reviewer>.*?)^## Dependencies\r?\n(?<dependencies>.*)\z'
    if ($body -match $pattern) {
        $values.overview=$Matches.overview.Trim()
        $values.reviewer=$Matches.reviewer.Trim()
        $values.dependencies=$Matches.dependencies.Trim()
        $testing=$Matches.testing.Trim()
        for($index=0;$index -lt $template.Checks.Count;$index++) {
            $checkPattern='(?m)^- \[(?<checked>[ xX])\] '+[regex]::Escape($template.Checks[$index])+'\r?$'
            $match=[regex]::Match($testing,$checkPattern)
            $values.checks["tested$index"]=$match.Success -and $match.Groups['checked'].Value -match '[xX]'
            $testing=[regex]::Replace($testing,$checkPattern,'')
        }
        $values.testingNotes=$testing.Replace($template.Testing,'').Replace($template.Question,'').Trim()
        if($values.overview -eq $template.Overview){$values.overview=''}
        if($values.reviewer -eq $template.Reviewer){$values.reviewer=''}
        $values.dependencies=$values.dependencies.Replace($template.Dependencies,'').Trim()
    } elseif ($body) {
        # Preserve older/custom descriptions rather than discarding authored notes.
        $values.overview=$body
    }
    $values
}

function New-SubmissionDescription($values,$template) {
    $lines=@('# Overview',$(if([string]::IsNullOrWhiteSpace($values.overview)){$template.Overview}else{[string]$values.overview}),'','## Testing',$template.Testing,'',$template.Question)
    for($index=0;$index -lt $template.Checks.Count;$index++) {
        $mark=if($values["tested$index"] -is [bool] -and $values["tested$index"]){'x'}else{' '}
        $lines+='- ['+$mark+'] '+$template.Checks[$index]
    }
    if($values.testingNotes){$lines+=@('',[string]$values.testingNotes)}
    $lines+=@('','## Comments for the reviewer',$(if([string]::IsNullOrWhiteSpace($values.reviewer)){$template.Reviewer}else{[string]$values.reviewer}),'','## Dependencies',$template.Dependencies)
    if($values.dependencies){$lines+=@('',[string]$values.dependencies)}
    $lines -join "`n"
}

function Read-BranchSubmission($title,$existing,$files) {
    $template=Get-MissionSubmissionTemplate
    $saved=Get-SubmissionDescriptionFields $(if($existing -and !$existing.merged_at){[string]$existing.body}else{''}) $template
    $fields=@(
        @{Id='title';Label='Submission name';Required=$true;Value=$title},
        @{Id='overview';Label='Overview';Kind='multiline';Help=$template.Overview;Value=$saved.overview},
        @{Id='testingRules';Label='Testing';Kind='filelist';ReadOnly=$true;Value=("Read: https://github.com/Global-Conflicts-ArmA/gc-reforger-missions/wiki/Testing-your-mission`n`n"+$template.Question)}
    )
    for($index=0;$index -lt $template.Checks.Count;$index++) {
        $fields+=@{Id="tested$index";Label=$template.Checks[$index];Kind='check';Checked=[bool]$saved.checks["tested$index"]}
    }
    if($saved.testingNotes){$fields+=@{Id='testingNotes';Label='Testing notes';Kind='multiline';Value=$saved.testingNotes}}
    $fields+=@(
        @{Id='reviewer';Label='Comments for the reviewer';Kind='multiline';Help=$template.Reviewer;Value=$saved.reviewer},
        @{Id='dependencies';Label='Dependencies';Kind='multiline';Help=$template.Dependencies;Value=$saved.dependencies}
    )
    foreach ($group in @(@{Code='A';Label='Added'},@{Code='M';Label='Changed'},@{Code='D';Label='Deleted'})) {
        $paths=@($files | Where-Object { $_.Status -eq $group.Code } | ForEach-Object { $_.Path })
        if ($paths.Count) {
            $fields+=@{Id=('files'+$group.Code);Label=($group.Label+' ('+$paths.Count+')');Kind='filelist';ReadOnly=$true;Value=($paths -join "`n")}
        }
    }
    $answer=Show-GcReview @{
        Title='Submit mission'; Section=$(if($existing.state -eq 'closed' -and !$existing.merged_at){'Request closed - resubmit for approval'}else{'Submit for approval'}); PrimaryAction=$(if($existing.state -eq 'closed' -and !$existing.merged_at){'Resubmit'}elseif($existing.state -eq 'open'){'Update submission'}else{'Submit'}); SecondaryAction='Cancel'
        Fields=$fields
    }
    if (!$answer) { return }
    $name=([string]$answer.Values.title).Trim()
    if (!$name) { throw 'Enter a submission name.' }
    # The UI returns a PSCustomObject; normalize to a dictionary for checklist keys.
    $values=@{}
    if($answer.Values -is [System.Collections.IDictionary]) {foreach($key in $answer.Values.Keys){$values[$key]=$answer.Values[$key]}}
    else {foreach($property in $answer.Values.PSObject.Properties){$values[$property.Name]=$property.Value}}
    @{Title=$name;Body=(New-SubmissionDescription $values $template)}
}

function Get-SubmissionBackupTree($root,$stash) {
    $index=Join-Path ([IO.Path]::GetTempPath()) ('gc-backup-'+[guid]::NewGuid()+'.index')
    $previous=$env:GIT_INDEX_FILE
    try {
        $env:GIT_INDEX_FILE=$index
        $null=Git $root @('read-tree',$stash)
        $untracked=Get-SubmissionRef $root ($stash+'^3')
        if ($untracked) {
            foreach ($entry in @(Git $root @('-c','core.quotePath=false','ls-tree','-r',$untracked))) {
                if ($entry -notmatch '^(100644|100755|120000) blob ([a-f0-9]{40,64})\t([^"\r\n\t]+)$') { throw 'A filename could not be checked safely.' }
                $null=Git $root @('update-index','--add','--cacheinfo',$Matches[1],$Matches[2],$Matches[3])
            }
        }
        Git $root @('write-tree')
    } finally {
        $env:GIT_INDEX_FILE=$previous
        if ([IO.File]::Exists($index)) { [IO.File]::Delete($index) }
    }
}

function Invoke-MissionSubmission($addons) {
    $missions=$addons | Where-Object Key -eq GlobalConflictsMissions
    $root=Get-Checkout $missions.Project $missions.Guid
    $hash=[Security.Cryptography.SHA256]::Create()
    try { $key=([BitConverter]::ToString($hash.ComputeHash([Text.Encoding]::UTF8.GetBytes($root.ToUpperInvariant())))).Replace('-','') }
    finally { $hash.Dispose() }
    $mutex=New-Object Threading.Mutex($false,('Local\GCSubmission-'+$key))
    $locked=$false
    try {
        try {$locked=$mutex.WaitOne(0)} catch [Threading.AbandonedMutexException] {$locked=$true}
        if (!$locked) { throw 'A submission is already in progress.' }
        Invoke-MissionSubmissionCore $root $missions.Repo
    } finally {
        if ($locked) {$mutex.ReleaseMutex()}
        $mutex.Dispose()
    }
}

function Invoke-MissionSubmissionCore($root,$repo) {
    $auth=Get-SubmissionAuth
    $user=Invoke-SubmissionApi $auth GET 'user'
    $owner=$user.login
    if ($owner -notmatch '^[A-Za-z0-9-]+$' -or [string]$user.id -notmatch '^\d+$') { throw 'Sign in to GitHub first.' }
    $sourceBranch=Git $root @('symbolic-ref','--short','HEAD')
    $tip=Git $root @('rev-parse','HEAD')
    $base=Get-ModBase $root $repo
    $null=Git $root @('fetch','--quiet','--no-tags',"https://github.com/$repo.git",('refs/heads/'+$base.Name))
    if ((Git $root @('rev-parse','FETCH_HEAD')) -ne $base.Commit) { throw 'Shared missions changed. Submit again to use the latest version.' }
    $existing=Get-ContributorMissionPr $auth $repo $owner $base.Name
    $prRevision=Get-SubmissionPrRevision $existing
    $branch=if($existing){[string]$existing.head.ref}else{'gc-submissions'}
    $localBranch=$branch
    $localTip=Get-SubmissionRef $root ('refs/heads/'+$localBranch)
    $null=Git $root @('check-ref-format','--branch',$branch)
    $snapshot=Get-SubmissionSnapshot $root $tip
    $candidate=$tip
    if ($snapshot -ne (Git $root @('rev-parse',($tip+'^{tree}')))) {
        $candidate=New-SubmissionCommit $root $snapshot @($tip) 'Save mission changes' $user
    }
    if ($localTip -and $localBranch -cne $sourceBranch) {
        $localTree=Merge-GcReviewedTree $root $candidate $localTip 'Your saved submission'
        if (!$localTree) {Set-GcOutcome 'Cancelled';return 2}
        $candidate=New-SubmissionCommit $root $localTree @($candidate,$localTip) 'Combine saved submission' $user
    }
    $name=($repo -split '/')[1];$forkRepo="$owner/$name"
    $fork=Invoke-SubmissionApi $auth GET "repos/$forkRepo" $null -AllowMissing
    if ($fork -and (!$fork.fork -or $fork.parent.full_name -ine $repo)) { throw 'The repository on your GitHub account is not a fork of GC Missions.' }
    if ($fork -and $fork.default_branch -ceq $branch) { throw 'Choose a mission branch before submitting; the shared branch will not be overwritten.' }
    $url="https://$owner@github.com/$forkRepo.git"
    $remoteTip=''
    if ($fork) {
        $remote=@(Git $root @('ls-remote','--heads',$url,"refs/heads/$branch"))
        if ($remote.Count) {
            $remoteTip=($remote[0] -split '\s+')[0]
            $null=Git $root @('fetch','--quiet','--no-tags',$url,"refs/heads/$branch")
            if ((Git $root @('rev-parse','FETCH_HEAD')) -ne $remoteTip) { throw 'Your online submission changed. Submit again to compare it.' }
            $remoteTree=Merge-GcReviewedTree $root $candidate $remoteTip 'Your online submission'
            if (!$remoteTree) {Set-GcOutcome 'Cancelled';return 2}
            $candidate=New-SubmissionCommit $root $remoteTree @($candidate,$remoteTip) 'Combine mission changes' $user
        }
    }
    # Every local/online conflict and every conflict with latest main is resolved
    # before touching the checkout, pushing, or creating/updating a pull request.
    $tree=Merge-GcReviewedTree $root $candidate $base.Commit 'Shared missions'
    if (!$tree) {Set-GcOutcome 'Cancelled';return 2}
    $files=@(Git $root @('-c','core.quotePath=false','diff','--name-status','--no-renames',$base.Commit,$tree) | ForEach-Object {
        $parts=$_ -split "`t",2
        if ($parts.Count -ne 2 -or $parts[0] -notin 'A','M','D','T') { throw 'A changed file could not be reviewed safely.' }
        @{Status=$(if($parts[0] -eq 'T'){'M'}else{$parts[0]});Path=$parts[1]}
    })
    if (!$files.Count) { throw 'Your files already match the shared version. Nothing to submit.' }
    $title=if($existing -and !$existing.merged_at){$existing.title}else{'Mission updates'}
    $review=Read-BranchSubmission $title $existing $files
    if (!$review) {Set-GcOutcome 'Cancelled';return 2}
    $latest=Get-ModBase $root $repo
    if ($latest.Commit -ne $base.Commit -or $latest.Name -cne $base.Name) { throw 'Shared missions changed during review. Submit again to compare the latest version.' }
    if ((Git $root @('symbolic-ref','--short','HEAD')) -cne $sourceBranch -or (Git $root @('rev-parse','HEAD')) -ne $tip -or (Get-SubmissionSnapshot $root $tip) -ne $snapshot) {
        throw 'Your files changed during review. Submit again.'
    }
    $currentPr=Get-ContributorMissionPr $auth $repo $owner $base.Name
    if ((Get-SubmissionPrRevision $currentPr) -cne $prRevision) { throw 'Your submission changed during review. Submit again.' }
    if ((Get-SubmissionRef $root ('refs/heads/'+$localBranch)) -ne $localTip) { throw 'Your saved submission changed during review. Submit again.' }
    $commit=New-SubmissionCommit $root $tree @($candidate,$base.Commit) $review.Title $user
    # Keep a named recovery reference before applying the reviewed changes.
    $backup='refs/gc/backups/'+[guid]::NewGuid().ToString('N')
    $null=Git $root @('update-ref',$backup,$tip)
    $dirty=@(Git $root @('status','--porcelain','--untracked-files=all'))
    $stash=''
    try {
        if ($dirty.Count) {
            $null=Git $root @('-c','user.name=GC Tools','-c','user.email=tools@localhost','stash','push','--quiet','--include-untracked','--message',"GC submission backup $backup")
            $stash=Git $root @('rev-parse','refs/stash')
            $null=Git $root @('update-ref',($backup+'-files'),$stash)
            if ((Get-SubmissionBackupTree $root $stash) -ne $snapshot) { throw 'Files changed while backing up the update.' }
        }
        if ((Git $root @('symbolic-ref','--short','HEAD')) -cne $sourceBranch -or (Git $root @('rev-parse','HEAD')) -ne $tip -or @(Git $root @('status','--porcelain','--untracked-files=all')).Count) { throw 'Files changed while preparing the update.' }
        if ((Get-SubmissionRef $root ('refs/heads/'+$localBranch)) -ne $localTip) { throw 'Your saved submission changed while preparing the update.' }
        if ($localBranch -cne $sourceBranch) {
            if ($localTip) { $null=Git $root @('switch','--quiet','--no-overwrite-ignore',$localBranch) }
            else { $null=Git $root @('switch','--quiet','--no-overwrite-ignore','-c',$localBranch) }
        }
        $null=Git $root @('merge','--ff-only','--no-autostash','--no-overwrite-ignore','--quiet',$commit)
    } catch {
        # Restore dirty files only while HEAD is unchanged and the checkout is
        # still clean. Never overwrite edits made by another program.
        if ($stash -and $localBranch -cne $sourceBranch -and (Git $root @('symbolic-ref','--short','HEAD')) -ceq $localBranch -and
            (Git $root @('rev-parse','HEAD')) -eq $localTip -and (Get-SubmissionRef $root ('refs/heads/'+$sourceBranch)) -eq $tip -and
            !@(Git $root @('status','--porcelain','--untracked-files=all')).Count) {
            $null=Git $root @('switch','--quiet','--no-overwrite-ignore',$sourceBranch)
        }
        if ($stash -and (Git $root @('rev-parse','HEAD')) -eq $tip -and !@(Git $root @('status','--porcelain','--untracked-files=all')).Count) {
            $null=Git $root @('stash','apply','--index',$stash)
        }
        throw 'The update could not finish. Your changes have been backed up; nothing was uploaded.'
    }
    if (!$fork) {
        $created=Invoke-SubmissionApi $auth POST "repos/$repo/forks" @{name=$name;default_branch_only=$true}
        if ($created.full_name -ine $forkRepo) { throw 'GitHub is preparing your account. Retry Submit shortly.' }
        $ready=$false
        for ($attempt=0;$attempt -lt 10;$attempt++) {
            $check=Invoke-SubmissionApi $auth GET "repos/$forkRepo" $null -AllowMissing
            if ($check -and $check.fork -and $check.parent.full_name -ieq $repo) {$ready=$true;break}
            Start-Sleep -Milliseconds 1000
        }
        if (!$ready) { throw 'GitHub is still preparing your account. Retry Submit shortly.' }
    }
    $latest=Get-ModBase $root $repo
    if ($latest.Commit -ne $base.Commit -or $latest.Name -cne $base.Name) { throw 'Shared missions changed. Your work is saved; submit again to sync the latest version.' }
    if ((Git $root @('rev-parse','HEAD')) -ne $commit -or (Git $root @('symbolic-ref','--short','HEAD')) -cne $localBranch -or (Get-SubmissionSnapshot $root $commit) -ne $tree) { throw 'Your files changed. Submit again to include the latest edits.' }
    $currentPr=Get-ContributorMissionPr $auth $repo $owner $base.Name
    if ((Get-SubmissionPrRevision $currentPr) -cne $prRevision) { throw 'Your approval request changed. Your files are saved; submit again to check its current status.' }
    try {$null=Git $root @('push','--quiet','--no-follow-tags','--recurse-submodules=no',$url,"${commit}:refs/heads/$branch")}
    catch {throw 'Upload stopped. Your changes are saved. Retry Submit to compare the online version; nothing will be overwritten.'}
    $latest=Get-ModBase $root $repo
    if ($latest.Commit -ne $base.Commit -or $latest.Name -cne $base.Name) { throw 'Shared missions changed during upload. Your upload is saved; an existing approval request now includes it. Submit again to sync the latest changes.' }
    $fields=@{title=$review.Title;body=$review.Body}
    $currentPr=Get-ContributorMissionPr $auth $repo $owner $base.Name
    if ((Get-SubmissionPrRevision $currentPr) -cne $prRevision) {
        throw 'Your approval request changed during upload. Your upload is saved; submit again to check its current status.'
    }
    if ($existing -and !$existing.merged_at) {
        if ($existing.state -eq 'closed') { $fields.state='open' }
        try { $pr=Invoke-SubmissionApi $auth PATCH "repos/$repo/pulls/$($existing.number)" $fields }
        catch {
            if ($fields.state -eq 'open') {
                Set-GcOutcome 'Resubmit' 'GitHub could not reopen this request. Your files are saved. View submission for reviewer feedback or retry.' "https://github.com/$repo/pull/$($existing.number)"
                return 2
            }
            throw
        }
    }
    else {
        $fields.head="${owner}:$branch";$fields.base=$base.Name;$fields.maintainer_can_modify=$true
        $pr=Invoke-SubmissionApi $auth POST "repos/$repo/pulls" $fields
    }
    if (!$pr.number) {throw 'Upload saved. Approval request could not be confirmed. Retry Submit.'}
    $status=Invoke-SubmissionApi $auth GET "repos/$repo/pulls/$($pr.number)"
    if ($status.merged -eq $true -or $status.merged_at) {
        Set-GcOutcome 'Approved' 'This request has been merged. Submit again for any remaining changes.' "https://github.com/$repo/pull/$($pr.number)"
        return 0
    }
    if ($status.state -eq 'closed') {
        Set-GcOutcome 'Resubmit' 'Request closed. Your files are saved. View submission for feedback, or resubmit for approval.' "https://github.com/$repo/pull/$($pr.number)"
        return 2
    }
    if ($status.state -ne 'open') { throw 'Upload saved. The approval request status could not be confirmed. Retry Submit.' }
    $state=if ($status.mergeable -eq $false) {'Shared missions changed - submit again to resolve conflicts'} elseif ($status.mergeable -eq $true) {'Awaiting approval'} else {'Awaiting approval - conflict check pending'}
    Set-GcOutcome 'Submitted' $state "https://github.com/$repo/pull/$($pr.number)"
    return 0
}
