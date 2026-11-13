$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot '../GC-SubmitMission.ps1')
function Assert($value,$message){if(!$value){throw $message}}
$script:edit=$true;$script:form=$null
function Show-GcReview($spec) {
    $script:form=$spec
    $values=@{}
    foreach($field in $spec.Fields){$values[$field.Id]=if($field.Kind -eq 'check'){[bool]$field.Checked}else{[string]$field.Value}}
    if($script:edit){$values.overview='Two new missions';$values.tested0=$true;$values.tested5=$true;$values.reviewer='Check the extraction';$values.dependencies='https://github.com/example/core/pull/1'}
    @{Action='confirm';Values=$values} | ConvertTo-Json -Depth 8 | ConvertFrom-Json
}
$files=@(@{Status='A';Path='worlds/Example.ent'})
$first=Read-BranchSubmission 'Mission updates' $null $files
$checks=@($script:form.Fields | Where-Object Kind -eq check)
Assert ($checks.Count -eq 6 -and !@($checks | Where-Object Checked).Count) 'New checklist is not six unchecked items'
Assert (($script:form.Fields | Where-Object Id -eq testingRules).Value -match 'Testing-your-mission') 'Testing guide not shown'
Assert (($script:form.Fields | Where-Object Id -eq dependencies).Help -match 'rejected or incompatible mods') 'Dependency rules not shown'
Assert ($first.Body -match '(?m)^- \[x\] Briefing, map, slotting screen$') 'Checked item lost'
Assert ($first.Body -match '(?m)^- \[ \] Setup timers / AO Limits$') 'Unchecked item marked tested'
Assert ($first.Body -match '(?m)^## Comments for the reviewer$' -and $first.Body -match 'Check the extraction') 'Reviewer section missing'
Assert ($first.Body -match 'https://github.com/example/core/pull/1') 'Dependency link missing'
$script:edit=$false
$repeat=Read-BranchSubmission 'Mission updates' @{body=$first.Body.Replace("`n","`r`n");state='open'} $files
Assert ($repeat.Body -ceq $first.Body) 'Updating request duplicates template or loses values'
Assert (@($script:form.Fields | Where-Object { $_.Kind -eq 'check' -and $_.Checked }).Count -eq 2) 'Prior checkbox selections not restored'
$legacy='Old reviewer instructions and links'
$preserved=Read-BranchSubmission 'Mission updates' @{body=$legacy;state='open'} $files
Assert ($preserved.Body.Contains($legacy)) 'Legacy description lost'
$extra=$first.Body.Replace('## Comments for the reviewer',"Additional testing notes`n`n## Comments for the reviewer")
$preserved=Read-BranchSubmission 'Mission updates' @{body=$extra;state='open'} $files
Assert ($preserved.Body.Contains('Additional testing notes')) 'Custom testing notes lost'
$fresh=Read-BranchSubmission 'New work' @{body=$first.Body;merged_at='2026-11-13'} $files
Assert (!($fresh.Body -match '(?m)^- \[x\]')) 'Merged request testing carried into new submission'
Write-Output 'PASS: rules visible, six opt-in checks, GitHub template, JSON response, repeated updates, CRLF, legacy/custom notes, fresh post-merge checklist.'
