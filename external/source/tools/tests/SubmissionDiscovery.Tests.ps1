$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot '../GC-SubmitMission.ps1')
function Assert($value,$message) {if(!$value){throw $message}}
function Request($number,$owner,$state='open') {
    @{number=$number;state=$state;head=@{repo=@{full_name="$owner/missions"};ref="branch-$number"};base=@{ref='main'}}
}
$script:mode='paged';$script:queries=@()
function Invoke-SubmissionApi($auth,$method,$path) {
    $script:queries+=$path
    if($script:mode -eq 'paged') {
        if($path.EndsWith('page=1')) {return ,@(1..100 | ForEach-Object {Request $_ 'someone-else'})}
        return ,@((Request 101 'tester'))
    }
    if($script:mode -eq 'closed') {
        if($path.Contains('state=open&')) {return ,@()}
        return ,@((Request 103 'someone-else' 'closed'),(Request 102 'tester' 'closed'),(Request 101 'tester' 'closed'))
    }
    if($script:mode -eq 'duplicate') {return ,@((Request 1 'tester'),(Request 2 'tester'))}
    if($script:mode -eq 'empty') {return ,@()}
}
$found=Get-ContributorMissionPr @{} 'official/missions' 'tester' 'main'
Assert ($found.number -eq 101 -and @($found).Count -eq 1) 'Pagination did not return one owned request'
Assert ($script:queries.Count -eq 2) 'A full REST array did not advance pagination'
$script:mode='closed'
$found=Get-ContributorMissionPr @{} 'official/missions' 'tester' 'main'
Assert ($found.number -eq 102 -and @($found).Count -eq 1) 'Closed request filtering returned unrelated requests'
$script:mode='duplicate'
try {Get-ContributorMissionPr @{} 'official/missions' 'tester' 'main';throw 'Duplicate requests accepted'} catch {Assert ($_.Exception.Message -like 'More than one approval request*') $_.Exception.Message}
$script:mode='empty'
Assert ($null -eq (Get-ContributorMissionPr @{} 'official/missions' 'tester' 'main')) 'Empty REST arrays produced a request'
Write-Output 'PASS: Windows PowerShell REST arrays, pagination, ownership, closed ordering, duplicates, empty responses.'
