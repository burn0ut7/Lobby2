# Git-only branch submission. Never saves, exports, or queries Workbench.
# Discover the contributor's one request, independent of their local branch.
function Get-ContributorMissionPr($auth,$repo,$owner,$base) {
    $forkRepo=$owner+'/'+($repo -split '/')[1]
    foreach ($state in @('open','closed')) {
        $found=@()
        for ($page=1; $page -le 100; $page++) {
            # Windows PowerShell REST responses can arrive as one array object.
            # Enumerate the response before filtering individual requests.
            $response=Invoke-SubmissionApi $auth GET "repos/$repo/pulls?state=$state&sort=created&direction=desc&per_page=100&page=$page"
            $items=@($response | ForEach-Object { $_ })
            $matches=@($items | Where-Object { $_.head.repo.full_name -ieq $forkRepo })
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
    @($pr.number,$pr.state,$pr.merged_at,$pr.title,$pr.body,$pr.head.ref,$pr.base.ref) | ConvertTo-Json -Compress
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
        $value=@(& (Get-GcGitExecutable) -C $root rev-parse --verify --quiet $ref 2>$null)
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
        Checks=@('Briefing, map, slotting screen','Setup timers / AO Limits','Custom prefabs','Key framework events and interactions (QRFs, etc)','End conditions','Playable in Dedicated Server Tool / Peer Tool')
    }
}

function Get-SubmissionDescriptionFields($body,$template) {
    $body=([string]$body).Replace("`r`n","`n")
    $body=[regex]::Replace($body,'(?ms)^## (?:Dependencies|Required units / loadouts)\n.*\z','').TrimEnd()
    $values=@{overview='';reviewer='';testingNotes='';checks=@{}}
    $pattern='(?ms)\A# Overview\r?\n(?<overview>.*?)^## Testing\r?\n(?<testing>.*?)^## Comments for the reviewer\r?\n(?<reviewer>.*)\z'
    if ($body -match $pattern) {
        $values.overview=$Matches.overview.Trim()
        $values.reviewer=$Matches.reviewer.Trim()
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
    $lines+=@('','## Comments for the reviewer',$(if([string]::IsNullOrWhiteSpace($values.reviewer)){$template.Reviewer}else{[string]$values.reviewer}))
    $lines -join "`n"
}

function Read-BranchSubmission($title,$existing,$files,$dependency=$null) {
    $template=Get-MissionSubmissionTemplate
    $saved=Get-SubmissionDescriptionFields $(if($existing -and !$existing.merged_at){[string]$existing.body}else{''}) $template
    $fields=@(
        @{Id='title';Label='Mission name';Required=$true;Value=$title},
        @{Id='overview';Label='Overview';Kind='multiline';Help=$template.Overview;Value=$saved.overview},
        @{Id='testingRules';Label='Testing';Kind='filelist';ReadOnly=$true;Value=("Read: https://github.com/Global-Conflicts-ArmA/gc-reforger-missions/wiki/Testing-your-mission`n`n"+$template.Question)}
    )
    for($index=0;$index -lt $template.Checks.Count;$index++) {
        $fields+=@{Id="tested$index";Label=$template.Checks[$index];Kind='check';Checked=[bool]$saved.checks["tested$index"]}
    }
    if($saved.testingNotes){$fields+=@{Id='testingNotes';Label='Testing notes';Kind='multiline';Value=$saved.testingNotes}}
    $fields+=@(
        @{Id='reviewer';Label='Comments for the reviewer';Kind='multiline';Help=$template.Reviewer;Value=$saved.reviewer}
    )
    if($dependency){$fields+=@{Id='unitsRequest';Label='Required units / loadouts';Kind='filelist';ReadOnly=$true;Value=$dependency.Url}}
    $fields+=@(Get-SubmissionFileFields $files)
    $answer=Show-GcReview @{
        Title='Submit mission'; Section=$(if($existing.state -eq 'closed' -and !$existing.merged_at){'Request closed - resubmit for approval'}else{'Submit for approval'}); PrimaryAction=$(if($existing.state -eq 'closed' -and !$existing.merged_at){'Resubmit'}elseif($existing.state -eq 'open'){'Update submission'}else{'Submit'}); SecondaryAction='Cancel'
        Fields=$fields
    }
    if (!$answer) { return }
    $name=([string]$answer.Values.title).Trim()
    if (!$name) { throw 'Enter a mission name.' }
    # The UI returns a PSCustomObject; normalize to a dictionary for checklist keys.
    $values=@{}
    if($answer.Values -is [System.Collections.IDictionary]) {foreach($key in $answer.Values.Keys){$values[$key]=$answer.Values[$key]}}
    else {foreach($property in $answer.Values.PSObject.Properties){$values[$property.Name]=$property.Value}}
    $body=New-SubmissionDescription $values $template
    if($dependency){$body+="`n`n## Required units / loadouts`nRequires: "+$dependency.Url}
    @{Title=$name;Body=$body}
}

function Get-SubmissionFileFields($files) {
    $fields=@()
    foreach ($group in @(@{Code='A';Label='Added'},@{Code='M';Label='Changed'},@{Code='D';Label='Deleted'})) {
        $paths=@($files | Where-Object { $_.Status -eq $group.Code } | ForEach-Object { $_.Path })
        if ($paths.Count) {
            $fields+=@{Id=('files'+$group.Code);Label=($group.Label+' ('+$paths.Count+')');Kind='filelist';ReadOnly=$true;Value=($paths -join "`n")}
        }
    }
    $fields
}

function Read-UnitsSubmission($title,$existing,$files) {
    $overview=if($existing -and !$existing.merged_at){[string]$existing.body}else{''}
    $comments=''
    if($overview -match '(?ms)\A# Overview\r?\n(?<overview>.*?)^## Comments for the reviewer\r?\n(?<comments>.*)\z') {
        $overview=$Matches.overview.Trim();$comments=$Matches.comments.Trim()
    }
    $answer=Show-GcReview @{
        Title='Submit units / loadouts'; Section='Review units / loadout changes'
        PrimaryAction=$(if($existing.state -eq 'closed' -and !$existing.merged_at){'Resubmit'}else{'Submit'});SecondaryAction='Cancel'
        Fields=@(
            @{Id='title';Label='Change name';Required=$true;Value=$title},
            @{Id='overview';Label='Overview';Kind='multiline';Value=$overview},
            @{Id='reviewer';Label='Comments for the reviewer';Kind='multiline';Value=$comments}
        )+@(Get-SubmissionFileFields $files)
    }
    if(!$answer){return}
    $name=([string]$answer.Values.title).Trim()
    if(!$name){throw 'Enter a name for the units / loadout changes.'}
    @{Title=$name;Body=("# Overview`n"+[string]$answer.Values.overview+"`n`n## Comments for the reviewer`n"+[string]$answer.Values.reviewer)}
}

function Assert-SubmissionDependency($dependency,$auth) {
    if(!$dependency){return}
    if((Git $dependency.Root @('rev-parse','HEAD')) -ne $dependency.Commit -or
        (Get-SubmissionSnapshot $dependency.Root $dependency.Commit) -ne $dependency.Tree) {
        throw 'Units / loadouts changed. Submit again to review those changes first.'
    }
    $status=Invoke-SubmissionApi $auth GET "repos/$($dependency.Repo)/pulls/$($dependency.Number)"
    if($status.base.ref -cne $dependency.Base) {throw 'The units / loadout request target changed. Submit again to target dev.'}
    if($status.head.sha -ne $dependency.Commit) {throw 'The units / loadout request changed. Submit again to review it first.'}
    if(!$status.merged_at -and $status.merged -ne $true -and ($status.state -ne 'open' -or $status.mergeable -eq $false)) {
        throw 'The units / loadout request is closed or has conflicts. Submit again to resolve it first.'
    }
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
    # Validate the mission checkout before any units upload.
    $null=Get-Checkout $missions.Project $missions.Guid
    $units=$addons | Where-Object Key -eq GlobalConflictsUnits
    $unitsReceipt=$null
    if($units) {
        $result=Invoke-AddonSubmission $units 'units' $null ([ref]$unitsReceipt)
        if($result -ne 0){return $result}
    }
    $dependency=if($unitsReceipt -and $unitsReceipt.Status -eq 'submitted'){$unitsReceipt}else{$null}
    $missionReceipt=$null
    try {
        $result=Invoke-AddonSubmission $missions 'missions' $dependency ([ref]$missionReceipt)
        if($dependency -and $result -ne 0) {
            $status=if($missionReceipt.Status -eq 'cancelled'){'Cancelled'}else{'Retry'}
            $detail='Units / loadouts were submitted. Mission submission: '+$missionReceipt.Outcome.Status+'. '+$missionReceipt.Outcome.Detail
            if($missionReceipt.Outcome.Url){$detail+=' '+$missionReceipt.Outcome.Url}
            Set-GcOutcome $status $detail $dependency.Url
        }
        return $result
    } catch {
        if(!$dependency){throw}
        $message=$_.Exception.Message
        if($message.StartsWith('Git failed')){$message='Check your connection and retry.'}
        Set-GcOutcome 'Retry' ("Units / loadouts were submitted. Mission submission stopped. "+$message) $dependency.Url
        return 2
    }
}

function Invoke-AddonSubmission($addon,$kind,$dependency,[ref]$receipt) {
    $root=Get-Checkout $addon.Project $addon.Guid
    $hash=[Security.Cryptography.SHA256]::Create()
    try { $key=([BitConverter]::ToString($hash.ComputeHash([Text.Encoding]::UTF8.GetBytes($root.ToUpperInvariant())))).Replace('-','') }
    finally { $hash.Dispose() }
    $mutex=New-Object Threading.Mutex($false,('Local\GCSubmission-'+$key))
    $locked=$false
    try {
        try {$locked=$mutex.WaitOne(0)} catch [Threading.AbandonedMutexException] {$locked=$true}
        if (!$locked) { throw 'A submission is already in progress.' }
        Invoke-MissionSubmissionCore $root $addon.Repo $kind $dependency $receipt
    } finally {
        if ($locked) {$mutex.ReleaseMutex()}
        $mutex.Dispose()
    }
}

function Set-SubmissionResult([ref]$receipt,$Status,$Detail='',$Url='') {
    $receipt.Value.Outcome=@{Status=$Status;Detail=$Detail;Url=$Url}
    Set-GcOutcome $Status $Detail $Url
}

function Invoke-MissionSubmissionCore($root,$repo,$kind='missions',$dependency=$null,[ref]$receipt) {
    if(!$receipt){$receipt=[ref]$null}
    $receipt.Value=@{Status='pending'}
    $sharedLabel=if($kind -eq 'units'){'Shared units / loadouts'}else{'Shared missions'}
    $auth=Get-SubmissionAuth
    $user=Invoke-SubmissionApi $auth GET 'user'
    $owner=$user.login
    if ($owner -notmatch '^[A-Za-z0-9-]+$' -or [string]$user.id -notmatch '^\d+$') { throw 'Sign in to GitHub first.' }
    $sourceBranch=Git $root @('symbolic-ref','--short','HEAD')
    $tip=Git $root @('rev-parse','HEAD')
    $base=Get-ModBase $root $repo
    $null=Git $root @('fetch','--quiet','--no-tags',"https://github.com/$repo.git",('refs/heads/'+$base.Name))
    if ((Git $root @('rev-parse','FETCH_HEAD')) -ne $base.Commit) { throw "$sharedLabel changed. Submit again to use the latest version." }
    $existing=Get-ContributorMissionPr $auth $repo $owner $base.Name
    $prRevision=Get-SubmissionPrRevision $existing
    $branch=if($existing){[string]$existing.head.ref}else{'gc-submissions'}
    $localBranch=$branch
    $localTip=Get-SubmissionRef $root ('refs/heads/'+$localBranch)
    $null=Git $root @('check-ref-format','--branch',$branch)
    $snapshot=Get-SubmissionSnapshot $root $tip
    $candidate=$tip
    if ($snapshot -ne (Git $root @('rev-parse',($tip+'^{tree}')))) {
        $candidate=New-SubmissionCommit $root $snapshot @($tip) "Save $kind changes" $user
    }
    if ($localTip -and $localBranch -cne $sourceBranch) {
        $localTree=Merge-GcReviewedTree $root $candidate $localTip 'Your saved submission'
        if (!$localTree) {$receipt.Value=@{Status='cancelled'};Set-SubmissionResult $receipt 'Cancelled';return 2}
        $candidate=New-SubmissionCommit $root $localTree @($candidate,$localTip) 'Combine saved submission' $user
    }
    $name=($repo -split '/')[1];$forkRepo="$owner/$name"
    $fork=Invoke-SubmissionApi $auth GET "repos/$forkRepo" $null -AllowMissing
    if ($fork -and (!$fork.fork -or $fork.parent.full_name -ine $repo)) { throw 'The repository on your GitHub account is not a fork of the shared mod.' }
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
            if (!$remoteTree) {$receipt.Value=@{Status='cancelled'};Set-SubmissionResult $receipt 'Cancelled';return 2}
            $candidate=New-SubmissionCommit $root $remoteTree @($candidate,$remoteTip) "Combine $kind changes" $user
        }
    }
    # Every local/online conflict and every conflict with latest main is resolved
    # before touching the checkout, pushing, or creating/updating a pull request.
    $tree=Merge-GcReviewedTree $root $candidate $base.Commit $sharedLabel
    if (!$tree) {$receipt.Value=@{Status='cancelled'};Set-SubmissionResult $receipt 'Cancelled';return 2}
    $files=@(Git $root @('-c','core.quotePath=false','diff','--name-status','--no-renames',$base.Commit,$tree) | ForEach-Object {
        $parts=$_ -split "`t",2
        if ($parts.Count -ne 2 -or $parts[0] -notin 'A','M','D','T') { throw 'A changed file could not be reviewed safely.' }
        @{Status=$(if($parts[0] -eq 'T'){'M'}else{$parts[0]});Path=$parts[1]}
    })
    if (!$files.Count) {
        if($kind -eq 'units'){$receipt.Value=@{Status='current'};return 0}
        throw 'Your files already match the shared version. Nothing to submit.'
    }
    $title=if($existing -and !$existing.merged_at){$existing.title}elseif($kind -eq 'units'){'Units / loadout updates'}else{'Mission updates'}
    if($kind -eq 'units'){$review=Read-UnitsSubmission $title $existing $files}
    else {
        Assert-SubmissionDependency $dependency $auth
        $review=Read-BranchSubmission $title $existing $files $dependency
    }
    if (!$review) {$receipt.Value=@{Status='cancelled'};Set-SubmissionResult $receipt 'Cancelled';return 2}
    Assert-SubmissionDependency $dependency $auth
    $latest=Get-ModBase $root $repo
    if ($latest.Commit -ne $base.Commit -or $latest.Name -cne $base.Name) { throw "$sharedLabel changed during review. Submit again to compare the latest version." }
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
    if ($latest.Commit -ne $base.Commit -or $latest.Name -cne $base.Name) { throw "$sharedLabel changed. Your work is saved; submit again to sync the latest version." }
    if ((Git $root @('rev-parse','HEAD')) -ne $commit -or (Git $root @('symbolic-ref','--short','HEAD')) -cne $localBranch -or (Get-SubmissionSnapshot $root $commit) -ne $tree) { throw 'Your files changed. Submit again to include the latest edits.' }
    $currentPr=Get-ContributorMissionPr $auth $repo $owner $base.Name
    if ((Get-SubmissionPrRevision $currentPr) -cne $prRevision) { throw 'Your approval request changed. Your files are saved; submit again to check its current status.' }
    Assert-SubmissionDependency $dependency $auth
    try {$null=Git $root @('push','--quiet','--no-follow-tags','--recurse-submodules=no',$url,"${commit}:refs/heads/$branch")}
    catch {throw 'Upload stopped. Your changes are saved. Retry Submit to compare the online version; nothing will be overwritten.'}
    $latest=Get-ModBase $root $repo
    if ($latest.Commit -ne $base.Commit -or $latest.Name -cne $base.Name) { throw "$sharedLabel changed during upload. Your upload is saved; an existing approval request now includes it. Submit again to sync the latest changes." }
    Assert-SubmissionDependency $dependency $auth
    $fields=@{title=$review.Title;body=$review.Body;base=$base.Name}
    $currentPr=Get-ContributorMissionPr $auth $repo $owner $base.Name
    if ((Get-SubmissionPrRevision $currentPr) -cne $prRevision) {
        throw 'Your approval request changed during upload. Your upload is saved; submit again to check its current status.'
    }
    if ($existing -and !$existing.merged_at) {
        if ($existing.state -eq 'closed') { $fields.state='open' }
        try { $pr=Invoke-SubmissionApi $auth PATCH "repos/$repo/pulls/$($existing.number)" $fields }
        catch {
            if ($fields.state -eq 'open') {
                Set-SubmissionResult $receipt 'Resubmit' 'GitHub could not reopen this request. Your files are saved. View submission for reviewer feedback or retry.' "https://github.com/$repo/pull/$($existing.number)"
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
    $receipt.Value=@{Status='submitted';Repo=$repo;Base=$base.Name;Number=$pr.number;Url="https://github.com/$repo/pull/$($pr.number)";Root=$root;Commit=$commit;Tree=$tree}
    if ($status.base.ref -cne $base.Name) { throw 'Upload saved. The approval request target could not be confirmed. Retry Submit.' }
    if ($status.merged -eq $true -or $status.merged_at) {
        Set-SubmissionResult $receipt 'Approved' 'This request has been merged. Submit again for any remaining changes.' "https://github.com/$repo/pull/$($pr.number)"
        return 0
    }
    if ($status.state -eq 'closed') {
        $receipt.Value.Status='closed'
        Set-SubmissionResult $receipt 'Resubmit' 'Request closed. Your files are saved. View submission for feedback, or resubmit for approval.' "https://github.com/$repo/pull/$($pr.number)"
        return 2
    }
    if ($status.state -ne 'open') { throw 'Upload saved. The approval request status could not be confirmed. Retry Submit.' }
    if($kind -eq 'units' -and $status.mergeable -eq $false) {
        Set-SubmissionResult $receipt 'Retry' 'Units / loadouts have conflicts. Resolve them before submitting the mission.' $receipt.Value.Url
        return 2
    }
    $state=if ($status.mergeable -eq $false) {"$sharedLabel changed - submit again to resolve conflicts"} elseif ($status.mergeable -eq $true) {'Awaiting approval'} else {'Awaiting approval - conflict check pending'}
    Set-SubmissionResult $receipt 'Submitted' $state "https://github.com/$repo/pull/$($pr.number)"
    return 0
}
