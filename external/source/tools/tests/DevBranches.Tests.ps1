$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot '../GC-ModWorkflow.ps1')
function Assert($v,$m){if(!$v){throw $m}}
$fixture=Join-Path $env:TEMP ('gc-dev-base-'+[guid]::NewGuid())
$null=New-Item -ItemType Directory $fixture
$null=Git $fixture @('init','-q','-b','main')
$null=Git $fixture @('config','user.name','Fixture');$null=Git $fixture @('config','user.email','fixture@example.invalid')
[IO.File]::WriteAllText("$fixture/file",'release')
$null=Git $fixture @('add','.');$null=Git $fixture @('commit','-qm','Release')
$release=Git $fixture @('rev-parse','HEAD')
$null=Git $fixture @('switch','-qc','dev')
[IO.File]::WriteAllText("$fixture/file",'development')
$null=Git $fixture @('add','.');$null=Git $fixture @('commit','-qm','Development')
$dev=Git $fixture @('rev-parse','HEAD')
$null=Git $fixture @('switch','-q','main')
foreach($name in @('core','units','missions')) {
    $repo="Global-Conflicts-ArmA/gc-reforger-$name"
    $null=Git $fixture @('config','--add',"url.$fixture.insteadOf","https://github.com/$repo.git")
    $base=Get-ModBase $fixture $repo
    Assert ($base.Name -eq 'dev' -and $base.Commit -eq $dev -and $base.Commit -ne $release) "$name did not target dev"
}
$null=Git $fixture @('branch','-D','dev')
try {$null=Get-ModBase $fixture 'Global-Conflicts-ArmA/gc-reforger-units';throw 'Release fallback accepted'} catch {Assert ($_.Exception.Message -like 'The required dev branch*') $_.Exception.Message}
Write-Output 'PASS: Core, Units, Missions target dev despite default main; missing dev refuses release fallback.'
