param([string]$RepositoryRoot = (Join-Path $PSScriptRoot '../../..'))
$ErrorActionPreference = 'Stop'
$taskRoot = (Resolve-Path -LiteralPath $RepositoryRoot).Path
$taskScripts = Join-Path $taskRoot 'scripts'

# String/FactionKey fields and arrays use the same DLL provider contract.
# Match faction-bearing names as well as descriptions explicitly saying faction key.
# Quoted attribute arguments may contain brackets or escaped quotes.
$taskPattern = '(?s)\[Attribute\((?<attribute>(?:"(?:\\.|[^"\\])*"|[^\]"])*?)\)\]\s*(?:(?:protected|private|public|static|ref)\s+)*(?<type>array\s*<\s*(?:string|FactionKey)\s*>|string|FactionKey)\s+(?<field>\w+)\s*(?:=[^;]*)?;'
$taskRegex = [regex]::new($taskPattern, [Text.RegularExpressions.RegexOptions]::None, [TimeSpan]::FromSeconds(2))
$taskFailures = [Collections.Generic.List[string]]::new()
$taskCount = 0
foreach ($taskFile in Get-ChildItem -LiteralPath $taskScripts -Filter '*.c' -Recurse -File) {
    $taskText = [IO.File]::ReadAllText($taskFile.FullName)
    $taskRelative = $taskFile.FullName.Substring($taskRoot.TrimEnd('\', '/').Length + 1).Replace('\', '/')
    foreach ($taskMatch in $taskRegex.Matches($taskText)) {
        $taskAttribute = $taskMatch.Groups['attribute'].Value
        $taskField = $taskMatch.Groups['field'].Value
        if ($taskField -notmatch '(?i)faction' -and $taskAttribute -notmatch '(?i)faction\s+keys?\b') { continue }
        if ($taskAttribute -match 'UIWidgets\.Hidden\b') { continue }
        # Generated evidence beneath the hidden m_GC_GeneratedMetadata object is
        # serialized for submission, not an editable faction selection.
        if ($taskRelative -eq 'scripts/Game/Mission/GC_MissionHeader.c' -and
            $taskField -in @('m_aFactions', 'm_aConfiguredFactions', 'm_sFaction', 'm_sLoadoutFaction')) { continue }
        $taskCount++
        if ($taskAttribute -notmatch 'UIWidgets\.EditComboBox\b' -or
            $taskAttribute -notmatch '"gc-choices:1:GC_FactionChoices"') {
            $taskLine = 1 + ([regex]::Matches($taskText.Substring(0, $taskMatch.Index), "`n")).Count
            $taskFailures.Add("${taskRelative}:${taskLine}: $taskField must use EditComboBox and gc-choices:1:GC_FactionChoices")
        }
    }
}
if ($taskFailures.Count) { throw ($taskFailures -join "`n") }
if (!$taskCount) { throw 'No authored faction-key fields found; check the repository path and scanner.' }
Write-Output "Faction dropdown audit passed: $taskCount authored fields use the shared world-faction provider."
