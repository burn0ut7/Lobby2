param([switch]$Quiet)
. (Join-Path $PSScriptRoot 'Common.ps1')
try {
    $messages = @()
    $currentDependency = 'tools'
    if ($env:GC_UNINSTALL_TOOLS -ne '0') {
        Write-Output 'GC_PROGRESS:{"Dependency":"tools","Status":"Uninstalling...","State":"pending"}'
        $messages += Uninstall-GcIntegration
        Write-Output 'GC_PROGRESS:{"Dependency":"tools","Status":"Not installed","State":"warning"}'
    }
    $currentDependency = 'git'
    # Standalone Uninstall.bat keeps its historical GC-only scope.
    if ($env:GC_UNINSTALL_GIT -eq '1') {
        Write-Output 'GC_PROGRESS:{"Dependency":"git","Status":"Uninstalling...","State":"pending"}'
        $messages += Uninstall-GcGit
        Write-Output 'GC_PROGRESS:{"Dependency":"git","Status":"Not installed","State":"warning"}'
    }
    $result = $messages -join ' '

    if ($env:GC_TOOLS_OUTCOME) {
        [IO.File]::WriteAllText($env:GC_TOOLS_OUTCOME, (@{Status='Uninstalled';Detail=$result} | ConvertTo-Json -Compress), (New-Object Text.UTF8Encoding($false)))
    } elseif ($Quiet) { Write-Output $result } else { Show-Message $result | Out-Null }
}
catch {
    Write-Output ('GC_PROGRESS:' + (@{Dependency=$currentDependency;Status="Uninstall failed";State="error"} | ConvertTo-Json -Compress))
    if ($env:GC_TOOLS_OUTCOME) {
        [IO.File]::WriteAllText($env:GC_TOOLS_OUTCOME, (@{Status='Uninstall stopped';Detail=$_.Exception.Message} | ConvertTo-Json -Compress), (New-Object Text.UTF8Encoding($false)))
        exit 1
    }
    if ($Quiet) { throw }
    Show-Message $_.Exception.Message 'OK' 'Warning' | Out-Null
    exit 1
}
