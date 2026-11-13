$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot '../GC-ModWorkflow.ps1')
. (Join-Path $PSScriptRoot '../GC-SubmitMission.ps1')
function Assert($value,$message) {if(!$value){throw $message}}
$fixture=Join-Path ([IO.Path]::GetTempPath()) ('gc-submission-test-'+[guid]::NewGuid())
$upstream=Join-Path $fixture 'shared';$author=Join-Path $fixture 'author';$forkPath=Join-Path $fixture 'personal.git'
$null=New-Item -ItemType Directory -Path $upstream -Force
$null=Git $upstream @('init','-q','-b','main')
$null=Git $upstream @('config','user.name','Fixture')
$null=Git $upstream @('config','user.email','fixture@example.invalid')
$worldPath='worlds/Example.ent';$headerPath='Missions/Example.conf'
$null=New-Item -ItemType Directory -Path "$upstream/worlds/Example_Layers","$upstream/Missions" -Force
[IO.File]::WriteAllText("$upstream/addon.gproj",('GameProject {'+[Environment]::NewLine+' GUID "62D55563549DA97A"'+[Environment]::NewLine+'}'))
[IO.File]::WriteAllText("$upstream/$worldPath",'base world')
[IO.File]::WriteAllText("$upstream/$worldPath.meta",'MetaFileClass {'+"`n"+' Name "{ABCD123456789012}worlds/Example.ent"'+"`n"+'}')
[IO.File]::WriteAllText("$upstream/$headerPath",'GC_MissionHeader {'+"`n"+' World "{ABCD123456789012}worlds/Example.ent"'+"`n"+' m_sName "COOP Alpha"'+"`n"+'}')
[IO.File]::WriteAllText("$upstream/$headerPath.meta",'header meta')
[IO.File]::WriteAllText("$upstream/worlds/Example_Layers/default.layer",'base layer')
[IO.File]::WriteAllText("$upstream/readme.txt",'shared readme')
$null=Git $upstream @('add','.')
$null=Git $upstream @('commit','-qm','Base')
$null=Git $upstream @('clone','-q',$upstream,$author)
$null=Git $upstream @('clone','-q','--bare',$upstream,$forkPath)
$null=Git $author @('config','user.name','Author')
$null=Git $author @('config','user.email','author@example.invalid')
$repo='Global-Conflicts-ArmA/gc-reforger-missions'
$null=Git $author @('config',"url.$upstream.insteadOf","https://github.com/$repo.git")
$null=Git $author @('config',"url.$forkPath.insteadOf",'https://tester@github.com/tester/gc-reforger-missions.git')
$addons=@(@{Name='GC Missions';Key='GlobalConflictsMissions';Project="$author/addon.gproj";Guid='62D55563549DA97A';Repo=$repo})
$script:forkReady=$false;$script:created=0;$script:patched=0;$script:pr=$null;$script:cancel=$false;$script:choice='mine';$script:conflicts=0;$script:reviewMutation=$null
function Get-SubmissionAuth {@{}}
function Show-GcReview($spec) {
    if ($spec.Title -eq 'Resolve conflicting file') {
        $script:conflicts++
        Assert ($spec.Fields[0].ReadOnly -and $spec.Fields[1].ReadOnly) 'Conflict preview must be read-only'
        if ($script:choice -eq 'cancel') {return}
        return @{Action=$script:choice;Values=@{}}
    }
    if ($spec.Section -eq 'Submit for approval') {
        Assert ($spec.PrimaryAction -eq 'Submit') 'Submission action is unclear'
        Assert (@($spec.Fields | Where-Object Id -eq files).Count -eq 1) 'Changed files not reviewed'
        if ($script:cancel) {return}
        if ($script:reviewMutation) {& $script:reviewMutation;$script:reviewMutation=$null}
        return @{Action='confirm';Values=@{title='COOP Alpha';body='Mission changes'}}
    }
    throw "Unexpected review: $($spec.Title)"
}
function Invoke-SubmissionApi($auth,$method,$path,$data,[switch]$AllowMissing) {
    if($path -eq 'user'){return @{login='tester';id=42}}
    if($path -eq 'repos/tester/gc-reforger-missions') {if($script:forkReady){return @{fork=$true;parent=@{full_name=$repo};default_branch='main'}};return}
    if($method -eq 'POST' -and $path -eq "repos/$repo/forks") {$script:forkReady=$true;return @{full_name='tester/gc-reforger-missions'}}
    if($method -eq 'GET' -and $path.Contains('/pulls?')){return $script:pr}
    if($method -eq 'POST' -and $path.EndsWith('/pulls')) {
        $script:created++;$script:branch=$data.head.Substring('tester:'.Length)
        Assert ($data.base -eq 'main' -and $data.maintainer_can_modify) 'Wrong PR destination'
        $main=Git $upstream @('rev-parse','HEAD')
        Assert ((Git $forkPath @('merge-base',$script:branch,$main)) -eq $main) 'PR created before latest main was merged'
        $script:pr=@{number=12;body=$data.body;title=$data.title;mergeable=$true;head=@{repo=@{owner=@{login='tester'}};ref=$script:branch};base=@{ref='main'}}
        return $script:pr
    }
    if($path.EndsWith('/pulls/12')) {
        if($method -eq 'PATCH'){$script:patched++;$script:pr.body=$data.body;$script:pr.title=$data.title}
        return $script:pr
    }
    throw "Unexpected API: $method $path"
}
function Assert-Synced {
    $main=Git $upstream @('rev-parse','HEAD')
    Assert ((Git $author @('merge-base','HEAD',$main)) -eq $main) 'Local branch does not contain main'
    Assert ((Git $author @('rev-parse','HEAD')) -eq (Git $forkPath @('rev-parse',$script:branch))) 'Local and submitted branch differ'
    Assert (!(Git $author @('status','--porcelain'))) 'Checkout was not clean after submission'
}
$initial=Git $author @('rev-parse','HEAD')
[IO.File]::WriteAllText("$author/$worldPath",'my first world')
[IO.File]::WriteAllText("$author/new dependency.txt",'mission dependency')
[IO.File]::WriteAllText("$upstream/shared.txt",'latest main')
$null=Git $upstream @('add','.');$null=Git $upstream @('commit','-qm','Shared change')
$script:cancel=$true
Assert ((Invoke-MissionSubmission $addons) -eq 2) 'Cancel not honored'
Assert ((Git $author @('rev-parse','HEAD')) -eq $initial -and $created -eq 0 -and !$forkReady) 'Cancel mutated branch or GitHub'
Assert ([IO.File]::ReadAllText("$author/$worldPath") -eq 'my first world') 'Cancel changed files'
$script:cancel=$false
Assert ((Invoke-MissionSubmission $addons) -eq 0) 'Initial submission failed'
Assert ($created -eq 1 -and $script:branch -like 'mission/submission-*') 'Automatic branch/PR missing'
Assert ((Git $author @('rev-parse','main')) -eq $initial) 'Original main branch was modified'
Assert ((Git $author @('branch','--show-current')) -eq $script:branch) 'Automatic mission branch not selected'
Assert ((Git $forkPath @('show',($script:branch+':new dependency.txt'))) -eq 'mission dependency') 'Saved new files missing'
Assert-Synced
Assert (@(Git $author @('for-each-ref','--format=%(refname)','refs/gc/backups')).Count -ge 2) 'Recovery refs missing'
[IO.File]::WriteAllText("$author/$worldPath",'local conflict')
[IO.File]::WriteAllText("$upstream/$worldPath",'shared conflict')
$null=Git $upstream @('add','.');$null=Git $upstream @('commit','-qm','Shared conflict')
$before=Git $author @('rev-parse','HEAD');$remoteBefore=Git $forkPath @('rev-parse',$script:branch)
$script:choice='cancel'
Assert ((Invoke-MissionSubmission $addons) -eq 2) 'Conflict cancel not honored'
Assert ((Git $author @('rev-parse','HEAD')) -eq $before -and (Git $forkPath @('rev-parse',$script:branch)) -eq $remoteBefore) 'Conflict cancellation changed a branch'
$script:choice='mine'
Assert ((Invoke-MissionSubmission $addons) -eq 0 -and $conflicts -ge 2 -and $created -eq 1 -and $patched -eq 1) 'Conflict resolution/PR reuse failed'
Assert ([IO.File]::ReadAllText("$author/$worldPath") -eq 'local conflict') 'Mine choice lost'
Assert-Synced
[IO.File]::WriteAllText("$author/$worldPath",'local next')
[IO.File]::WriteAllText("$upstream/$worldPath",'shared next')
$null=Git $upstream @('add','.');$null=Git $upstream @('commit','-qm','Shared next')
$script:choice='shared'
Assert ((Invoke-MissionSubmission $addons) -eq 0) 'Shared choice failed'
Assert ([IO.File]::ReadAllText("$author/$worldPath") -eq 'shared next') 'Shared choice not applied locally'
Assert-Synced
[IO.File]::WriteAllText("$author/$worldPath",'before review')
$before=Git $author @('rev-parse','HEAD');$remoteBefore=Git $forkPath @('rev-parse',$script:branch)
$script:reviewMutation={ [IO.File]::WriteAllText("$author/$worldPath",'edited during review') }
try {$null=Invoke-MissionSubmission $addons;throw 'Stale local review accepted'} catch {Assert ($_.Exception.Message -eq 'Your files changed during review. Submit again.') $_.Exception.Message}
Assert ((Git $author @('rev-parse','HEAD')) -eq $before -and (Git $forkPath @('rev-parse',$script:branch)) -eq $remoteBefore) 'Stale review uploaded'
$script:reviewMutation={ [IO.File]::WriteAllText("$upstream/shared.txt",'changed during review');$null=Git $upstream @('add','.');$null=Git $upstream @('commit','-qm','Moving main') }
try {$null=Invoke-MissionSubmission $addons;throw 'Stale main accepted'} catch {Assert ($_.Exception.Message -like 'Shared missions changed during review.*') $_.Exception.Message}
[IO.File]::WriteAllText("$forkPath/hooks/pre-receive", "#!/bin/sh`nexit 1`n", (New-Object Text.UTF8Encoding($false)))
try {$null=Invoke-MissionSubmission $addons;throw 'Rejected push accepted'} catch {Assert ($_.Exception.Message -like 'Upload stopped.*') $_.Exception.Message}
Assert ((Git $forkPath @('rev-parse',$script:branch)) -eq $remoteBefore) 'Rejected push changed remote'
[IO.File]::Delete("$forkPath/hooks/pre-receive")
Assert ((Invoke-MissionSubmission $addons) -eq 0) 'Retry failed'
Assert-Synced
# Advance the personal branch independently, then resolve its conflict locally.
$online=Join-Path $fixture 'online'
$null=Git $upstream @('clone','-q','--branch',$script:branch,$forkPath,$online)
$null=Git $online @('config','user.name','Online')
$null=Git $online @('config','user.email','online@example.invalid')
[IO.File]::WriteAllText("$online/$worldPath",'online conflict')
[IO.File]::WriteAllText("$online/online.txt",'remote addition')
$null=Git $online @('add','.')
$null=Git $online @('commit','-qm','Online changes')
$onlineTip=Git $online @('rev-parse','HEAD')
$null=Git $online @('push','-q','origin',$script:branch)
[IO.File]::WriteAllText("$author/$worldPath",'local divergent conflict')
$conflictsBefore=$script:conflicts
$script:choice='shared'
Assert ((Invoke-MissionSubmission $addons) -eq 0) 'Divergent online submission failed'
Assert ($script:conflicts -gt $conflictsBefore) 'Online conflict was not reviewed'
Assert ([IO.File]::ReadAllText("$author/$worldPath") -eq 'online conflict') 'Online choice lost'
Assert ([IO.File]::ReadAllText("$author/online.txt") -eq 'remote addition') 'Online addition lost'
Assert ((Git $author @('merge-base','HEAD',$onlineTip)) -eq $onlineTip) 'Online history overwritten'
Assert-Synced
Write-Output "PASS: Git-only submission, automatic branch, saved changes/dependencies, latest-main merge, both conflict choices, no mutation on cancel/stale review, backup refs, safe retry, existing PR reuse. Fixture: $fixture"
