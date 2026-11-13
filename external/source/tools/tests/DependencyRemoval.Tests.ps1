$ErrorActionPreference = 'Stop'
$fixture = Join-Path ([IO.Path]::GetTempPath()) ('GC-removal-test-' + [guid]::NewGuid().ToString('N'))
$keys = @('GC_UNINSTALL_TOOLS','GC_UNINSTALL_GIT','GC_TOOLS_OUTCOME')
$previous = @{}
foreach ($key in $keys) { $previous[$key] = [Environment]::GetEnvironmentVariable($key) }
try {
    New-Item -ItemType Directory -Path $fixture | Out-Null
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot '../../installer/support/Uninstall.ps1') -Destination $fixture
    @'
function Uninstall-GcIntegration { $global:gcRemovalFixtureCalls.Add('tools'); return 'GC removed' }
function Uninstall-GcGit { $global:gcRemovalFixtureCalls.Add('git'); return 'Git removed' }
'@ | Set-Content -LiteralPath (Join-Path $fixture 'Common.ps1')
    $env:GC_TOOLS_OUTCOME = Join-Path $fixture 'outcome.json'
    foreach ($selection in @('tools','git','both','none')) {
        $global:gcRemovalFixtureCalls = New-Object 'Collections.Generic.List[string]'
        $env:GC_UNINSTALL_TOOLS = if ($selection -in @('tools','both')) { '1' } else { '0' }
        $env:GC_UNINSTALL_GIT = if ($selection -in @('git','both')) { '1' } else { '0' }
        & (Join-Path $fixture 'Uninstall.ps1') -Quiet | Out-Null
        if (('tools' -in $global:gcRemovalFixtureCalls) -ne ($env:GC_UNINSTALL_TOOLS -eq '1') -or
            ('git' -in $global:gcRemovalFixtureCalls) -ne ($env:GC_UNINSTALL_GIT -eq '1')) { throw 'Uninstaller ignored selection.' }
    }
    $env:GC_UNINSTALL_TOOLS = $null
    $env:GC_UNINSTALL_GIT = $null
    $global:gcRemovalFixtureCalls.Clear()
    & (Join-Path $fixture 'Uninstall.ps1') -Quiet | Out-Null
    if (($global:gcRemovalFixtureCalls -join ',') -ne 'tools') { throw 'Standalone uninstall changed its GC-only scope.' }
    . (Join-Path $PSScriptRoot '../../installer/support/Common.ps1')
    $exe = Join-Path $fixture 'unins001.exe'
    Set-Content -LiteralPath $exe -Value 'fixture - never executed'
    $script:registration = [pscustomobject]@{UninstallString=('"' + $exe + '"');InstallLocation=$fixture;PSPath=(Join-Path $fixture 'removed-registration')}
    function Get-ItemProperty { param($LiteralPath,$ErrorAction) return $script:registration }
    function Start-Process { param($FilePath,$ArgumentList,$WindowStyle,[switch]$Wait,[switch]$PassThru)
        if ($FilePath -ne $exe -or $ArgumentList -notmatch '/NORESTART' -or !$Wait) { throw 'Incorrect Git uninstaller dispatch.' }
        $script:launched = $true
        return [pscustomobject]@{ExitCode=$script:fakeExitCode}
    }
    $script:fakeExitCode = 0
    $script:launched = $false
    $result = Uninstall-GcGit
    if (!$script:launched -or $result -ne 'Git removed.') { throw 'Git uninstall not confirmed.' }
    $script:fakeExitCode = 1
    $rejected = $false
    try { Uninstall-GcGit } catch { $rejected = $_.Exception.Message -match 'Git was not removed' }
    if (!$rejected) { throw 'Failed uninstaller reported success.' }
    $script:fakeExitCode = 0
    $script:registration.PSPath = $exe
    $rejected = $false
    try { Uninstall-GcGit } catch { $rejected = $_.Exception.Message -match 'Git was not removed' }
    if (!$rejected) { throw 'Surviving Git registration reported removed.' }
    $script:registration.InstallLocation = Join-Path $fixture 'different'
    $script:launched = $false
    $rejected = $false
    try { Uninstall-GcGit } catch { $rejected = $_.Exception.Message -match 'could not be verified' }
    if (!$rejected -or $script:launched) { throw 'Unexpected uninstaller path was executed.' }
    Write-Output 'PASS: all uninstall selections, registered Git dispatch and mismatched-path rejection without removing real dependencies.'
} finally {
    Remove-Variable gcRemovalFixtureCalls -Scope Global -ErrorAction SilentlyContinue
    foreach ($key in $keys) { [Environment]::SetEnvironmentVariable($key, $previous[$key]) }
    $resolved = [IO.Path]::GetFullPath($fixture)
    $temp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\') + '\'
    if (!$resolved.StartsWith($temp, [StringComparison]::OrdinalIgnoreCase) -or [IO.Path]::GetFileName($resolved) -notlike 'GC-removal-test-*') { throw 'Unsafe fixture cleanup.' }
    if (Test-Path -LiteralPath $resolved) { Remove-Item -LiteralPath $resolved -Recurse -Force }
}
