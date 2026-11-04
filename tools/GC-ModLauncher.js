// Use the Windows GUI script host so launching PowerShell never allocates a visible terminal.
var shell = new ActiveXObject("WScript.Shell");
var files = new ActiveXObject("Scripting.FileSystemObject");
var helper = files.BuildPath(files.GetParentFolderName(WScript.ScriptFullName), "GC-ModWorkflow.ps1");
var executable = files.BuildPath(files.GetSpecialFolder(1), "WindowsPowerShell\\v1.0\\powershell.exe");
function quote(value) {
    // Windows paths and the fixed option values cannot contain double quotes.
    if (value.indexOf('"') !== -1) throw new Error("Unexpected quote in argument.");
    return '"' + value + '"';
}
var command = quote(executable) + " -NoProfile -NonInteractive -STA -ExecutionPolicy Bypass -File " + quote(helper);
for (var i = 0; i < WScript.Arguments.length; i++) command += " " + quote(WScript.Arguments(i));
WScript.Quit(shell.Run(command, 0, true));
