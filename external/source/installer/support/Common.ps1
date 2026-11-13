$ErrorActionPreference = 'Stop'
# Use this host's built-in hashing commands even when launched by another PowerShell version.
Import-Module (Join-Path $PSHOME 'Modules/Microsoft.PowerShell.Utility/Microsoft.PowerShell.Utility.psd1') -ErrorAction Stop
. (Join-Path $PSScriptRoot 'LobbyTools.ps1')

function Show-Message([string]$Text, [string]$Buttons = 'OK', [string]$Icon = 'Information') {
    Add-Type -AssemblyName System.Windows.Forms
    return [System.Windows.Forms.MessageBox]::Show($Text, 'GC Workbench integration', $Buttons, $Icon).ToString()
}

function Get-LatestGitInstaller {
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    $release = Invoke-RestMethod -Uri 'https://api.github.com/repos/git-for-windows/git/releases/latest' -Headers @{ 'User-Agent' = 'GC-Workbench-Installer' }
    $assets = @($release.assets | Where-Object { $_.name -match '^Git-[0-9][0-9.]*-64-bit\.exe$' })
    if ($assets.Count -ne 1) { throw 'The official Git release does not contain exactly one Windows x64 installer.' }
    $asset = $assets[0]
    if ($asset.browser_download_url -notmatch '^https://github\.com/git-for-windows/git/releases/download/[^/]+/Git-[0-9][0-9.]*-64-bit\.exe$' -or
        $asset.digest -notmatch '^sha256:[0-9a-fA-F]{64}$') { throw 'The official Git release URL or checksum is missing or unexpected.' }
    return $asset
}

function Install-LatestGit([switch]$Quiet) {
    $asset = Get-LatestGitInstaller
    $folder = Join-Path ([IO.Path]::GetTempPath()) ('GC-Git-' + [Guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $folder | Out-Null
    $file = Join-Path $folder $asset.name
    try {
        Invoke-WebRequest -UseBasicParsing -Uri $asset.browser_download_url -OutFile $file
        if ((Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash -ne $asset.digest.Substring(7)) { throw 'Git download checksum did not match the official release.' }
        if ((Get-AuthenticodeSignature -LiteralPath $file).Status -ne 'Valid') { throw 'Git installer Windows signature is not valid.' }
        # This is the official interactive Git installer; its own wizard requests elevation if needed.
        $arguments = if ($Quiet) { '/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /NOCANCEL /SP- /CURRENTUSER' } else { '/NORESTART' }
        $process = Start-Process -FilePath $file -ArgumentList $arguments -Wait -PassThru
        if ($process.ExitCode -notin @(0, 3010)) { throw "Git installation did not complete (exit $($process.ExitCode))." }
        return "Git installed using $($asset.name). Open a new terminal to use it."
    }
    finally {
        # Delete only the downloaded file, never a computed directory tree.
        if (Test-Path -LiteralPath $file) { Remove-Item -LiteralPath $file }
        if (Test-Path -LiteralPath $folder) { Remove-Item -LiteralPath $folder }
    }
}


function Assert-GcOwnedPath([string]$Path, [string]$Root) {
    $full = [IO.Path]::GetFullPath($Path).TrimEnd('\')
    $base = [IO.Path]::GetFullPath($Root).TrimEnd('\')
    if (!$full.StartsWith($base + '\', [StringComparison]::OrdinalIgnoreCase)) { throw "Path is outside installation root: $full" }
    $item = $full
    while ($item) {
        if ((Test-Path -LiteralPath $item) -and ((Get-Item -LiteralPath $item -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw "Refusing linked installation path: $item" }
        $item = Split-Path $item -Parent
    }
    return $full
}

function Get-GcCanonicalPath([string]$Path) {
    if ([string]::IsNullOrWhiteSpace($Path)) { throw 'Workbench did not supply a profile folder. Reopen GC Tools from Workbench.' }
    $full = [IO.Path]::GetFullPath($Path.Replace('/', '\'))
    $root = [IO.Path]::GetPathRoot($full)
    if ($full.Length -gt $root.Length) { $full = $full.TrimEnd('\') }
    return $full
}

function Test-GcSamePath([string]$First, [string]$Second) {
    return [string]::Equals((Get-GcCanonicalPath $First), (Get-GcCanonicalPath $Second), [StringComparison]::OrdinalIgnoreCase)
}

function Remove-GcLegacyStartup {
    $control = Assert-GcOwnedPath (Join-Path $env:LOCALAPPDATA 'GCWorkbench') $env:LOCALAPPDATA
    $automatic = Assert-GcOwnedPath (Join-Path $control 'automatic.json') $control
    $run = 'HKCU:/Software/Microsoft/Windows/CurrentVersion/Run'
    Remove-ItemProperty -LiteralPath $run -Name 'GCWorkbench' -ErrorAction SilentlyContinue
    if (Test-Path -LiteralPath $automatic) { Remove-Item -LiteralPath $automatic }
    $shortcut = Join-Path ([Environment]::GetFolderPath('Desktop')) 'Enable GC Workbench.lnk'
    if (Test-Path -LiteralPath $shortcut) { Remove-Item -LiteralPath $shortcut }
}

function Install-GcFiles([string]$Source, [string]$Destination, [string]$WorkbenchExecutable, [string]$Profile) {
    if (!(Test-Path -LiteralPath $WorkbenchExecutable -PathType Leaf) -or
        [IO.Path]::GetFileName($WorkbenchExecutable) -ne 'ArmaReforgerWorkbenchSteamDiag.exe') { throw 'Select ArmaReforgerWorkbenchSteamDiag.exe from your Workbench installation.' }
    if (!(Test-Path -LiteralPath (Join-Path $Source 'payload/helper/GCWorkbench.Loader.exe')) -or
        !(Test-Path -LiteralPath (Join-Path $Source 'payload/generic/gcworkbench.dll'))) { throw 'Extract the complete ZIP before running Install.bat.' }
    $null = Assert-GcOwnedPath $Destination (Join-Path $env:LOCALAPPDATA 'GCWorkbench/versions')
    if (Test-Path -LiteralPath $Destination) { throw 'The selected installation directory already exists.' }
    New-Item -ItemType Directory -Path $Destination -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $Source 'payload') -Destination $Destination -Recurse
    foreach ($name in @('Common.ps1', 'Uninstall.ps1', 'LobbyTools.ps1')) { Copy-Item -LiteralPath (Join-Path $Source $name) -Destination $Destination }
    $profilePath = Get-GcCanonicalPath $Profile
    if (!(Test-Path -LiteralPath $profilePath -PathType Container)) { throw 'Open GC Tools from your Workbench profile.' }
    # Read the editor's own exported preferences; never configure or control Workbench.
    $preferences = Join-Path $profilePath 'wbSettingsDump.ini'
    if (!(Test-Path -LiteralPath $preferences)) { throw 'Save Workbench preferences and reopen GC Tools.' }
    $settings = Get-Content -LiteralPath $preferences -Raw
    $enabled = [regex]::Match($settings, '(?m)^NetAPI_Enabled=(1|true)\r?$')
    $portMatch = [regex]::Match($settings, '(?m)^NetAPI_Port=(\d+)\r?$')
    if (!$enabled.Success -or !$portMatch.Success) { throw 'Enable NET API in Workbench preferences, save, then install again. GC Tools does not change editor preferences.' }
    $port = [int]$portMatch.Groups[1].Value
    if ($port -lt 1 -or $port -gt 65535) { throw 'Workbench NET API port is invalid.' }
    @{
        schema = 1; workbenchExecutable = [IO.Path]::GetFullPath($WorkbenchExecutable)
        profileDirectory = $profilePath; port = $port
        dllSha256 = (Get-FileHash -LiteralPath (Join-Path $Destination 'payload/generic/gcworkbench.dll')).Hash
    } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $Destination 'settings.json') -Encoding UTF8
    return $Destination
}

function Enable-GcStartup([string]$Destination) {
    $control = Assert-GcOwnedPath (Join-Path $env:LOCALAPPDATA 'GCWorkbench') $env:LOCALAPPDATA
    $setup = Get-Content -LiteralPath (Join-Path $Destination 'settings.json') -Raw | ConvertFrom-Json
    $profile = Get-GcCanonicalPath $setup.profileDirectory
    $script = Assert-GcOwnedPath (Join-Path $profile 'scripts/WorkbenchGame/GCWorkbench/GC_WorkbenchStartup.c') $profile
    $manifest = Assert-GcOwnedPath (Join-Path $control 'installation.json') $control
    if (Test-Path -LiteralPath $manifest) {
        $previous = Get-Content -LiteralPath $manifest -Raw | ConvertFrom-Json
        if (!(Test-GcSamePath $previous.startupPath $script)) { throw 'A different Workbench profile is installed. Uninstall that integration before changing profiles.' }
    }
    if (Test-Path -LiteralPath $script) {
        if (!(Test-Path -LiteralPath $manifest)) { throw "Unmanaged startup script preserved: $script" }
        $previous = Get-Content -LiteralPath $manifest -Raw | ConvertFrom-Json
        if (!(Test-GcSamePath $previous.startupPath $script) -or (Get-FileHash -LiteralPath $script -Algorithm SHA256).Hash -ne $previous.startupSha256) { throw "Modified startup script preserved: $script" }
    }
    $executable = [IO.Path]::GetFullPath((Join-Path $Destination 'payload/helper/GCWorkbench.Loader.exe')).Replace('\','/')
    if ($executable.Contains('"')) { throw 'Unsupported quote in installation path.' }
    $content = @"
// Installed GC Workbench bootstrap. Managed by Install.bat / Uninstall.bat.
[WorkbenchPluginAttribute(name: "GC Workbench integration", wbModules: { "ResourceManager" })]
class GC_WorkbenchStartup : WorkbenchPlugin
{
`tvoid GC_WorkbenchStartup()
`t{
`t`tProcessHandle helper = Workbench.RunProcess("\"$executable\"");
`t`tif (helper)
`t`t`tWorkbench.FinishProcess(helper);
`t}
}
"@
    New-Item -ItemType Directory -Path (Split-Path $script -Parent) -Force | Out-Null
    Install-GcLobbyTools $Destination $profile
    $uiPath = Assert-GcOwnedPath (Join-Path $control 'ui-path.txt') $control
    [IO.File]::WriteAllText($uiPath, (Join-Path $Destination 'payload/helper/GCWorkbench.UI.exe'), [Text.Encoding]::Unicode)
    # Deactivate the previous version before replacing its bootstrap. No sign-in task is installed.
    Remove-GcLegacyStartup
    $active = Assert-GcOwnedPath (Join-Path $control 'active.json') $control
    if (Test-Path -LiteralPath $active) { Remove-Item -LiteralPath $active }
    [IO.File]::WriteAllText($script, $content, (New-Object Text.UTF8Encoding($false)))
    @{ enabled = $true; startupPath = $script; startupSha256 = (Get-FileHash -LiteralPath $script -Algorithm SHA256).Hash; destination = $Destination; profileDirectory = $profile } |
        ConvertTo-Json | Set-Content -LiteralPath $manifest -Encoding UTF8

}

function Uninstall-GcIntegration {
    $control = Assert-GcOwnedPath (Join-Path $env:LOCALAPPDATA 'GCWorkbench') $env:LOCALAPPDATA
    Remove-GcLegacyStartup
    $active = Assert-GcOwnedPath (Join-Path $control 'active.json') $control
    if (Test-Path -LiteralPath $active) { Remove-Item -LiteralPath $active }
    $manifest = Assert-GcOwnedPath (Join-Path $control 'installation.json') $control
    if (Test-Path -LiteralPath $manifest) {
        $installed = Get-Content -LiteralPath $manifest -Raw | ConvertFrom-Json
        # Revoke loading before checking files that may have been edited. Keep
        # ownership metadata so a later uninstall can finish safely.
        $installed | Add-Member -NotePropertyName enabled -NotePropertyValue $false -Force
        $installed | ConvertTo-Json | Set-Content -LiteralPath $manifest -Encoding UTF8
        $expected = Join-Path $installed.profileDirectory 'scripts/WorkbenchGame/GCWorkbench/GC_WorkbenchStartup.c'
        if (!(Test-GcSamePath $installed.startupPath $expected)) { throw 'Installation record has an unexpected startup script path.' }
        $script = Assert-GcOwnedPath $installed.startupPath $installed.profileDirectory
        if (Test-Path -LiteralPath $script) {
            if ((Get-FileHash -LiteralPath $script -Algorithm SHA256).Hash -ne $installed.startupSha256) { throw "Automatic loading is disabled, but the modified startup script was preserved. Remove or relocate it, then run Uninstall.bat again: $script" }
            Remove-Item -LiteralPath $script
            $scriptDirectory = Split-Path $script -Parent
            if (!(Get-ChildItem -LiteralPath $scriptDirectory -Force)) { Remove-Item -LiteralPath $scriptDirectory }
        }
    }
    Uninstall-GcLobbyTools $control
    # A live DLL cannot safely unload. Disable first, then require a normal host exit.
    $loaded = @(Get-Process -Name 'ArmaReforgerWorkbenchSteamDiag' -ErrorAction SilentlyContinue)
    if ($loaded.Count) { throw 'Automatic loading is disabled and its startup script was removed. Close Workbench normally after saving your work, then run Uninstall.bat again to finish removing the installed files.' }
    # Old sign-in watchers exit when their active file disappears; bounded startup helpers do too.
    $deadline = [DateTime]::UtcNow.AddSeconds(15)
    do {
        $helpers = @(Get-Process -Name GCWorkbench.Loader -ErrorAction SilentlyContinue | Where-Object {
            try { $_.Path.StartsWith($control + '\', [StringComparison]::OrdinalIgnoreCase) } catch { $false }
        })
        if (!$helpers.Count) { break }
        Start-Sleep -Milliseconds 250
    } while ([DateTime]::UtcNow -lt $deadline)
    if ($helpers.Count) { throw 'Integration helpers are still finishing. Run Uninstall.bat again after they exit; no processes were force-terminated.' }
    if (Test-Path -LiteralPath $control) {
        $verified = Assert-GcOwnedPath $control ([IO.Path]::GetFullPath($env:LOCALAPPDATA))
        if (@(Get-ChildItem -LiteralPath $verified -Recurse -Force | Where-Object { $_.Attributes -band [IO.FileAttributes]::ReparsePoint }).Count) { throw 'Installation contains linked files; preserved without recursive deletion.' }
        Remove-Item -LiteralPath $verified -Recurse -Force
    }
    return 'GC Workbench integration was removed. Other tools, Workbench preferences, your projects and Git were preserved.'
}
