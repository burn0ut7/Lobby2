param([switch]$Inspect)

Import-Module (Join-Path $PSHOME "Modules/Microsoft.PowerShell.Utility/Microsoft.PowerShell.Utility.psd1") -ErrorAction Stop

function Get-GcGitInstallations {
    foreach ($entry in @(
        @{Key='HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\Git_is1';Scope='CurrentUser'},
        @{Key='HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\Git_is1';Scope='AllUsers'},
        @{Key='HKLM:\Software\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\Git_is1';Scope='AllUsers'})) {
        $record = Get-ItemProperty -LiteralPath $entry.Key -ErrorAction SilentlyContinue
        if (!$record -or [string]::IsNullOrWhiteSpace($record.InstallLocation)) { continue }
        $record | Add-Member -NotePropertyName Scope -NotePropertyValue $entry.Scope -Force
        $record | Add-Member -NotePropertyName GitExecutable -NotePropertyValue (Join-Path $record.InstallLocation 'cmd/git.exe') -Force
        $record
    }
}

function Get-GcGitExecutable {
    foreach ($record in @(Get-GcGitInstallations)) {
        if (Test-Path -LiteralPath $record.GitExecutable -PathType Leaf) { return $record.GitExecutable }
    }
    return $null
}

if ($Inspect) {
    $git = Get-GcGitExecutable
    if (!$git) { @{Installed=$false;Version='';CredentialManager=$false} | ConvertTo-Json -Compress; exit 0 }
    $version = & $git --version 2>$null
    if ($LASTEXITCODE -ne 0) { throw 'The registered Git installation could not run.' }
    $null = & $git credential-manager --version 2>$null
    @{Installed=$true;Version=($version -join ' ');CredentialManager=($LASTEXITCODE -eq 0)} | ConvertTo-Json -Compress
}
