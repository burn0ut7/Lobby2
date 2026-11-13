$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '../../installer/support/Common.ps1')
# Never mutate real registrations or run a real installer in these fixtures.
function Get-ItemProperty { param($LiteralPath,$ErrorAction) return $null }
if (Get-GcGitExecutable) { throw 'Unregistered bundled Git was detected as installed.' }
if ((Uninstall-GcGit) -ne 'Git is not installed.') { throw 'Uninstall did not ignore unrelated Git.' }
$script:fixture = Join-Path $env:TEMP ('GC-git-test-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory $fixture | Out-Null
try {
    $exe = Join-Path $fixture 'cmd/git.exe'
    New-Item -ItemType Directory (Split-Path $exe) | Out-Null
    Set-Content $exe 'fixture'
    function Get-ItemProperty { param($LiteralPath,$ErrorAction)
        if ($LiteralPath -like 'HKLM:*' -and $LiteralPath -notlike '*WOW6432Node*') { [pscustomobject]@{InstallLocation=$fixture} }
    }
    if ((Get-GcGitExecutable) -ne $exe -or @(Get-GcGitInstallations)[0].Scope -ne 'AllUsers') { throw 'Registered installation was not selected.' }
    function Get-LatestGitInstaller { [pscustomobject]@{name='fixture.exe';browser_download_url='fixture';digest=('sha256:' + (Get-FileHash $exe).Hash);ExpectedVersion='2.56.1.windows.1'} }
    function Invoke-WebRequest { param([switch]$UseBasicParsing,$Uri,$OutFile) Copy-Item $exe $OutFile }
    function Get-AuthenticodeSignature { param($LiteralPath) [pscustomobject]@{Status='Valid'} }
    function Start-Process { param($FilePath,$ArgumentList,$WindowStyle,[switch]$Wait,[switch]$PassThru)
        $script:installerArguments=$ArgumentList
        [pscustomobject]@{ExitCode=0}
    }
    function Get-GcGitExecutable { 'Invoke-FixtureGit' }
    function Invoke-FixtureGit {
        $global:LASTEXITCODE=0
        if ($args[0] -eq '--version') { return $script:version }
        $global:LASTEXITCODE=$script:gcmExit
    }
    $script:version='git version 2.56.1.windows.1'; $script:gcmExit=0
    Install-LatestGit -Quiet | Out-Null
    if ($installerArguments -notmatch '/ALLUSERS' -or !$installerArguments.Contains('/DIR="'+$fixture+'"')) { throw 'Update did not preserve machine installation.' }
    function Get-GcGitInstallations { }
    Install-LatestGit -Quiet | Out-Null
    if ($installerArguments -notmatch '/CURRENTUSER') { throw 'New installation has wrong scope.' }
    $script:version='git version 2.54.0.windows.1'
    $rejected=$false
    try { Install-LatestGit -Quiet } catch { $rejected=$_.Exception.Message -match 'expected version' }
    if (!$rejected) { throw 'Old version reported updated.' }
    $script:version='git version 2.56.1.windows.1'; $script:gcmExit=1
    $rejected=$false
    try { Install-LatestGit -Quiet } catch { $rejected=$_.Exception.Message -match 'Credential Manager is missing' }
    if (!$rejected) { throw 'Missing credential manager reported success.' }
    'PASS: registered Git discovery, bundled Git exclusion, install scope, update version and credential manager verification.'
} finally {
    $resolved=[IO.Path]::GetFullPath($fixture)
    if (!$resolved.StartsWith([IO.Path]::GetFullPath($env:TEMP).TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase) -or [IO.Path]::GetFileName($resolved) -notlike 'GC-git-test-*') { throw 'Unsafe cleanup path.' }
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
