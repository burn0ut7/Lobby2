param([switch]$Quiet)
. (Join-Path $PSScriptRoot 'Common.ps1')
try {
    $result = Uninstall-GcIntegration
    if ($env:GC_TOOLS_OUTCOME) {
        [IO.File]::WriteAllText($env:GC_TOOLS_OUTCOME, (@{Status='Uninstalled';Detail=$result} | ConvertTo-Json -Compress), (New-Object Text.UTF8Encoding($false)))
    } elseif ($Quiet) { Write-Output $result } else { Show-Message $result | Out-Null }
}
catch {
    if ($env:GC_TOOLS_OUTCOME) {
        [IO.File]::WriteAllText($env:GC_TOOLS_OUTCOME, (@{Status='Uninstall stopped';Detail=$_.Exception.Message} | ConvertTo-Json -Compress), (New-Object Text.UTF8Encoding($false)))
        exit 1
    }
    if ($Quiet) { throw }
    Show-Message $_.Exception.Message 'OK' 'Warning' | Out-Null
    exit 1
}
