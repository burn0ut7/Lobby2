$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot '../GC-ModWorkflow.ps1')
function Assert($value,$message){if(!$value){throw $message}}
$fixture=Join-Path $env:TEMP ('gc-units-submit-'+[guid]::NewGuid())
$script:repos=@{};$addons=@()
foreach($kind in @('units','missions')) {
    $repo="Global-Conflicts-ArmA/gc-reforger-$kind"
    $shared=Join-Path $fixture "$kind-shared";$author=Join-Path $fixture "$kind-author";$fork=Join-Path $fixture "$kind-fork.git"
    $guid=if($kind -eq 'units'){'6156F2F771D5D73D'}else{'62D55563549DA97A'}
    $null=New-Item -ItemType Directory $shared -Force
    $null=Git $shared @('init','-q','-b','main')
    $null=Git $shared @('config','user.name','Fixture');$null=Git $shared @('config','user.email','fixture@example.invalid')
    [IO.File]::WriteAllText("$shared/addon.gproj",("GameProject {`n GUID `"$guid`"`n}"))
    $null=Git $shared @('add','.');$null=Git $shared @('commit','-qm','Base')
    $null=Git $shared @('clone','-q',$shared,$author);$null=Git $shared @('clone','-q','--bare',$shared,$fork)
    $null=Git $author @('config',"url.$shared.insteadOf","https://github.com/$repo.git")
    $null=Git $author @('config',"url.$fork.insteadOf","https://tester@github.com/tester/gc-reforger-$kind.git")
    $script:repos[$repo]=@{Kind=$kind;Shared=$shared;Author=$author;Fork=$fork;Ready=$false;Pr=$null;Created=0;Forks=0;Patched=0}
    $addons+=@{Key=$(if($kind -eq 'units'){'GlobalConflictsUnits'}else{'GlobalConflictsMissions'});Project="$author/addon.gproj";Guid=$guid;Repo=$repo}
}
$script:reviews=@();$script:cancel='';$script:mutation=$null
function Get-SubmissionAuth {@{}}
function Set-GcOutcome($Status,$Detail,$Url){$script:outcome=@{Status=$Status;Detail=$Detail;Url=$Url}}
function Invoke-SubmissionApi($auth,$method,$path,$data,[switch]$AllowMissing) {
    if($path -eq 'user'){return @{login='tester';id=42}}
    if($path -notmatch '^repos/(?:Global-Conflicts-ArmA|tester)/(gc-reforger-(?:units|missions))(.*)$'){throw "Unexpected API $path"}
    $name=$Matches[1];$suffix=$Matches[2];$r=$script:repos["Global-Conflicts-ArmA/$name"]
    if($suffix -eq ''){if($r.Ready){return @{fork=$true;parent=@{full_name="Global-Conflicts-ArmA/$name"};default_branch='main'}};return}
    if($suffix -eq '/forks'){$r.Ready=$true;$r.Forks++;return @{full_name="tester/$name"}}
    if($suffix.StartsWith('/pulls?')){
        $items=@();if($r.Pr -and $suffix.Contains('state='+$r.Pr.state+'&')){$items+=($r.Pr|ConvertTo-Json -Depth 10|ConvertFrom-Json)}
        return ,$items
    }
    if($method -eq 'POST' -and $suffix -eq '/pulls'){
        $r.Created++
        $r.Pr=@{number=$(if($r.Kind -eq 'units'){21}else{31});state='open';merged_at=$null;mergeable=$true;title=$data.title;body=$data.body;head=@{ref='gc-submissions';repo=@{full_name="tester/$name"}};base=@{ref='main'}}
        Assert ($data.base -eq 'main') 'Wrong target branch'
    } elseif($method -eq 'PATCH'){if($script:denyMissionReopen -and $r.Kind -eq 'missions' -and $data.state -eq 'open'){throw 'Fixture reopen denied'};$r.Patched++;$r.Pr.title=$data.title;$r.Pr.body=$data.body;if($data.state){$r.Pr.state=$data.state}}
    if($script:closeUnitsOnStatus -and $r.Kind -eq 'units' -and $method -eq 'GET'){$r.Pr.state='closed';$script:closeUnitsOnStatus=$false}
    $r.Pr.head.sha=Git $r.Fork @('rev-parse','gc-submissions')
    return $r.Pr
}
function Show-GcReview($spec) {
    $script:reviews+=$spec.Title
    if($spec.Title -eq $script:cancel){return}
    if($spec.Title -eq 'Submit mission' -and $script:repos['Global-Conflicts-ArmA/gc-reforger-units'].Pr -and !$script:repos['Global-Conflicts-ArmA/gc-reforger-units'].Pr.merged_at){
        $link=$spec.Fields|Where-Object Id -eq unitsRequest
        Assert ($link.ReadOnly -and $link.Value -eq 'https://github.com/Global-Conflicts-ArmA/gc-reforger-units/pull/21') 'Mission review missing units dependency'
    }
    if($spec.Title -eq 'Submit mission' -and $script:mutation){& $script:mutation;$script:mutation=$null}
    return @{Action='confirm';Values=@{title='Fixture changes';overview='Test changes';reviewer='Review changes'}}
}
$units=$script:repos['Global-Conflicts-ArmA/gc-reforger-units'];$missions=$script:repos['Global-Conflicts-ArmA/gc-reforger-missions']
[IO.File]::WriteAllText(($missions.Author+'/Mission.ent'),'first mission')
Assert ((Invoke-MissionSubmission $addons) -eq 0) 'Unchanged units blocked mission'
Assert ($units.Created -eq 0 -and $units.Forks -eq 0 -and ($script:reviews -join '|') -eq 'Submit mission') 'Unchanged units created a request or review'
[IO.File]::WriteAllText(($units.Author+'/Loadout.et'),'new loadout')
[IO.File]::WriteAllText(($missions.Author+'/Mission.ent'),'uses new loadout')
$script:reviews=@()
Assert ((Invoke-MissionSubmission $addons) -eq 0) 'Units then mission failed'
Assert (($script:reviews -join '|') -eq 'Submit units / loadouts|Submit mission') 'Reviews not in units-first order'
Assert ($units.Created -eq 1 -and $missions.Created -eq 1 -and $units.Forks -eq 1 -and $missions.Forks -eq 1) 'Fork/request reuse failed'
Assert ($missions.Pr.body -match 'Requires: https://github.com/Global-Conflicts-ArmA/gc-reforger-units/pull/21') 'Mission PR dependency missing'
$missionTip=Git $missions.Fork @('rev-parse','gc-submissions')
$script:cancel='Submit units / loadouts';$script:reviews=@()
Assert ((Invoke-MissionSubmission $addons) -eq 2) 'Units cancellation ignored'
Assert ($script:reviews.Count -eq 1 -and (Git $missions.Fork @('rev-parse','gc-submissions')) -eq $missionTip) 'Mission uploaded after units cancellation'
$script:cancel='Submit mission'
Assert ((Invoke-MissionSubmission $addons) -eq 2) 'Mission cancellation ignored'
Assert ($script:outcome.Detail -like 'Units / loadouts were submitted.*') 'Partial completion missing'
Assert ($script:outcome.Url -eq 'https://github.com/Global-Conflicts-ArmA/gc-reforger-units/pull/21') 'Partial completion lost units link'
$script:cancel=''
Assert ((Invoke-MissionSubmission $addons) -eq 0) 'Retry after mission cancellation failed'
Assert ($units.Created -eq 1 -and $missions.Created -eq 1) 'Retry duplicated requests'
Assert ([regex]::Matches($missions.Pr.body,'## Required units / loadouts').Count -eq 1) 'Retry duplicated dependency section'
$missionTip=Git $missions.Fork @('rev-parse','gc-submissions')
$missions.Pr.state='closed';$script:denyMissionReopen=$true
Assert ((Invoke-MissionSubmission $addons) -eq 2) 'Failed mission reopen reported success'
Assert ($script:outcome.Detail -like '*GitHub could not reopen*' -and $script:outcome.Url -eq 'https://github.com/Global-Conflicts-ArmA/gc-reforger-units/pull/21') 'Mission reopen failure lost units receipt or failure detail'
$script:denyMissionReopen=$false
Assert ((Invoke-MissionSubmission $addons) -eq 0) 'Failed mission reopen retry stuck'
$missionTip=Git $missions.Fork @('rev-parse','gc-submissions')
$script:closeUnitsOnStatus=$true;$script:reviews=@()
Assert ((Invoke-MissionSubmission $addons) -eq 2) 'Closed units request allowed mission'
Assert ($script:reviews.Count -eq 1 -and (Git $missions.Fork @('rev-parse','gc-submissions')) -eq $missionTip) 'Mission continued after units request closed'
Assert ((Invoke-MissionSubmission $addons) -eq 0 -and $units.Created -eq 1) 'Closed units request retry duplicated or failed'
$missionTip=Git $missions.Fork @('rev-parse','gc-submissions')
$script:mutation={ [IO.File]::WriteAllText(($script:repos['Global-Conflicts-ArmA/gc-reforger-units'].Author+'/Loadout.et'),'edited during mission review') }
Assert ((Invoke-MissionSubmission $addons) -eq 2) 'Stale units accepted'
Assert ($script:outcome.Detail -like '*Units / loadouts changed*' -and (Git $missions.Fork @('rev-parse','gc-submissions')) -eq $missionTip) ('Stale dependency check: '+$script:outcome.Detail+'; remote='+ (Git $missions.Fork @('rev-parse','gc-submissions'))+' expected='+$missionTip)
Assert ((Invoke-MissionSubmission $addons) -eq 0) 'Stale dependency retry failed'
$null=Git $units.Shared @('fetch','-q',$units.Fork,'gc-submissions');$null=Git $units.Shared @('merge','--ff-only','-q','FETCH_HEAD')
$units.Pr.state='closed';$units.Pr.merged_at='2026-11-13T12:00:00Z';$script:reviews=@()
Assert ((Invoke-MissionSubmission $addons) -eq 0) 'Merged units blocked mission'
Assert (($script:reviews -join '|') -eq 'Submit mission' -and $missions.Pr.body -notmatch '## Required units / loadouts') 'Merged dependency was not cleared'
Write-Output "PASS: unchanged units skipped, units-first review, per-repo reuse, automatic dependency, cancellations, partial receipt, stale units, merged units. Fixture: $fixture"
