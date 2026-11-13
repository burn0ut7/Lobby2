// Keep Workbench responsive while GC Tools runs.
var shell = new ActiveXObject("WScript.Shell");
var files = new ActiveXObject("Scripting.FileSystemObject");
var script = files.BuildPath(files.GetParentFolderName(WScript.ScriptFullName), "Install.ps1");
var executable = files.BuildPath(files.GetSpecialFolder(1), "WindowsPowerShell\\v1.0\\powershell.exe");
shell.Environment("Process")("GC_WORKBENCH_INSTALL_SCRIPT") = script;
shell.Environment("Process")("GC_WORKBENCH_PACKAGE") = files.GetParentFolderName(WScript.ScriptFullName);
var tools = WScript.Arguments.length >= 1 && WScript.Arguments(0) === "--tools";
// Always open the package belonging to this Lobby checkout. Optional addons and
// a previous installation are not required; Install.ps1 verifies and caches it.
if (tools) {
    for (var argument = 1; argument < WScript.Arguments.length; argument += 2) {
        var key = WScript.Arguments(argument);
        if (!/^-(Lobby2|GlobalConflictsCore|GlobalConflictsUnits|GlobalConflictsMissions|WorkbenchExecutable|WorkbenchProfile)$/.test(key) || argument + 1 >= WScript.Arguments.length) throw new Error("Invalid tool argument.");
        shell.Environment("Process")("GC_WORKBENCH_MOD_" + key.substring(1)) = WScript.Arguments(argument + 1);
    }
    // The checksum selects this checkout's already-verified extraction. No archive
    // is consumed on this path; install/update still verifies the ZIP in Install.ps1.
    var checksum = files.BuildPath(files.GetParentFolderName(script), "GCWorkbench.zip.sha256");
    if (files.FileExists(checksum)) {
        var input = files.OpenTextFile(checksum, 1);
        var expected = input.ReadAll().replace(/^\s+|\s+$/g, "").split(/\s+/)[0].toLowerCase();
        input.Close();
        if (/^[0-9a-f]{64}$/.test(expected)) {
            var cache = files.BuildPath(shell.ExpandEnvironmentStrings("%LOCALAPPDATA%"), "GCWorkbench\\ui\\" + expected);
            var complete = files.BuildPath(cache, "complete");
            var ui = files.BuildPath(cache, "support\\payload\\helper\\GC Tools.exe");
            if (files.FileExists(complete) && files.FileExists(ui)) {
                input = files.OpenTextFile(complete, 1);
                var receipt = input.ReadAll().replace(/^\s+|\s+$/g, "").toLowerCase();
                input.Close();
                if (receipt === expected) WScript.Quit(shell.Run('"' + ui + '" --tools', 1, true));
            }
        }
    }
}
var mode = tools ? " -Tools" : WScript.Arguments.length === 1 && WScript.Arguments(0) === "--preview" ? " -Preview" : "";
WScript.Quit(shell.Run('"' + executable + '" -NoProfile -STA -ExecutionPolicy Bypass -File "' + script + '"' + mode, 0, tools));
