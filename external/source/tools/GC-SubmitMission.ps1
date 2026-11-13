# Git-only branch submission. Never saves, exports, or queries Workbench.
function Get-OpenMissionPr($auth,$repo,$owner,$branch,$base) {
    $source=[Uri]::EscapeDataString("${owner}:$branch")
    $baseName=[Uri]::EscapeDataString($base)
    $items=Invoke-SubmissionApi $auth GET "repos/$repo/pulls?state=open&head=$source&base=$baseName"
    $items | Where-Object { $_.head.repo.owner.login -eq $owner -and $_.head.ref -ceq $branch -and $_.base.ref -ceq $base } | Select-Object -First 1
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

function Read-BranchSubmission($title,$existing,$files) {
    $answer=Show-GcReview @{
        Title='Submit mission'; Section='Submit for approval'; PrimaryAction='Submit'; SecondaryAction='Cancel'
        Fields=@(
            @{Id='title';Label='Mission name';Required=$true;Value=$title},
            @{Id='body';Label='Notes for the reviewer';Kind='multiline';Value=$(if($existing){[string]$existing.body}else{''})},
            @{Id='files';Label='Files to submit';Kind='multiline';ReadOnly=$true;Value=($files -join "`n")}
        )
    }
    if (!$answer) { return }
    $name=([string]$answer.Values.title).Trim()
    if (!$name) { throw 'Enter a mission name.' }
    @{Title=$name;Body=[string]$answer.Values.body}
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
    $branch=$sourceBranch
    if ($branch -ceq $base.Name) { $branch='mission/submission-'+[guid]::NewGuid().ToString('N').Substring(0,12) }
    $null=Git $root @('check-ref-format','--branch',$branch)
    $snapshot=Get-SubmissionSnapshot $root $tip
    $candidate=$tip
    if ($snapshot -ne (Git $root @('rev-parse',($tip+'^{tree}')))) {
        $candidate=New-SubmissionCommit $root $snapshot @($tip) 'Save mission changes' $user
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
            if (!$remoteTree) {return 2}
            $candidate=New-SubmissionCommit $root $remoteTree @($candidate,$remoteTip) 'Combine mission changes' $user
        }
    }
    # Every local/online conflict and every conflict with latest main is resolved
    # before touching the checkout, pushing, or creating/updating a pull request.
    $tree=Merge-GcReviewedTree $root $candidate $base.Commit 'Shared missions'
    if (!$tree) {return 2}
    $files=@(Git $root @('-c','core.quotePath=false','diff','--name-only','--no-renames',$base.Commit,$tree))
    if (!$files.Count) { throw 'Your files already match the shared version. Nothing to submit.' }
    $existing=Get-OpenMissionPr $auth $repo $owner $branch $base.Name
    $title=if($existing){$existing.title}elseif($sourceBranch -ceq $base.Name){'New mission'}else{$sourceBranch -replace '[-_/]',' '}
    $review=Read-BranchSubmission $title $existing $files
    if (!$review) {return 2}
    $latest=Get-ModBase $root $repo
    if ($latest.Commit -ne $base.Commit -or $latest.Name -cne $base.Name) { throw 'Shared missions changed during review. Submit again to compare the latest version.' }
    if ((Git $root @('symbolic-ref','--short','HEAD')) -cne $sourceBranch -or (Git $root @('rev-parse','HEAD')) -ne $tip -or (Get-SubmissionSnapshot $root $tip) -ne $snapshot) {
        throw 'Your files changed during review. Submit again.'
    }
    $currentPr=Get-OpenMissionPr $auth $repo $owner $branch $base.Name
    if ($currentPr.number -ne $existing.number -or $currentPr.body -cne $existing.body -or $currentPr.title -cne $existing.title) { throw 'Your submission changed during review. Submit again.' }
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
        if ($branch -cne $sourceBranch) { $null=Git $root @('switch','--quiet','-c',$branch) }
        $null=Git $root @('merge','--ff-only','--no-autostash','--no-overwrite-ignore','--quiet',$commit)
    } catch {
        # Restore dirty files only while HEAD is unchanged and the checkout is
        # still clean. Never overwrite edits made by another program.
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
    if ((Git $root @('rev-parse','HEAD')) -ne $commit -or (Git $root @('symbolic-ref','--short','HEAD')) -cne $branch -or (Get-SubmissionSnapshot $root $commit) -ne $tree) { throw 'Your files changed. Submit again to include the latest edits.' }
    try {$null=Git $root @('push','--quiet','--no-follow-tags','--recurse-submodules=no',$url,"${commit}:refs/heads/$branch")}
    catch {throw 'Upload stopped. Your changes are saved. Retry Submit to compare the online version; nothing will be overwritten.'}
    $latest=Get-ModBase $root $repo
    if ($latest.Commit -ne $base.Commit -or $latest.Name -cne $base.Name) { throw 'Shared missions changed during upload. Your upload is saved; an existing approval request now includes it. Submit again to sync the latest changes.' }
    $fields=@{title=$review.Title;body=$review.Body}
    if ($existing) {$pr=Invoke-SubmissionApi $auth PATCH "repos/$repo/pulls/$($existing.number)" $fields}
    else {
        $fields.head="${owner}:$branch";$fields.base=$base.Name;$fields.maintainer_can_modify=$true
        $pr=Invoke-SubmissionApi $auth POST "repos/$repo/pulls" $fields
    }
    if (!$pr.number) {throw 'Upload saved. Approval request could not be confirmed. Retry Submit.'}
    $status=Invoke-SubmissionApi $auth GET "repos/$repo/pulls/$($pr.number)"
    $state=if ($status.mergeable -eq $false) {'Shared missions changed - submit again to resolve conflicts'} elseif ($status.mergeable -eq $true) {'Awaiting approval'} else {'Awaiting approval - conflict check pending'}
    Set-GcOutcome 'Submitted' $state "https://github.com/$repo/pull/$($pr.number)"
    return 0
}
