using System.Diagnostics;
using System.IO;
using System.IO.Pipes;
using System.Text;
using System.Text.Json;


namespace GCWorkbench.UI;

internal sealed record OperationResult(string Status, string Detail = "", string Url = "");

internal static class ToolOperations
{
    internal static async Task<OperationResult> RunAsync(string action, Dictionary<string, object> values,
        Func<DialogSpec, Task<DialogResult>> review)
    {
        string world = "";
        if (action is "submit" or "update")
        {
            world = await Task.Run(() => Prepare(action));
        }
        var script = action == "install"
            ? Environment.GetEnvironmentVariable("GC_WORKBENCH_INSTALL_SCRIPT")
            : Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "..", "lobby2-tools", "GC-ModWorkflow.ps1"));
        if (string.IsNullOrEmpty(script) || !File.Exists(script)) throw new IOException("Install the current GC Tools package.");
        var pipeName = "GC-Tools-" + Guid.NewGuid().ToString("N");
        var outcome = Path.Combine(Path.GetTempPath(), pipeName + ".json");
        using var reviewLifetime = new CancellationTokenSource();
        using var pipe = new NamedPipeServerStream(pipeName, PipeDirection.InOut, 1, PipeTransmissionMode.Byte,
            PipeOptions.Asynchronous | PipeOptions.CurrentUserOnly);
        var serve = ServeReviewsAsync(pipe, review, reviewLifetime.Token);
        try
        {
            var start = new ProcessStartInfo(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "WindowsPowerShell", "v1.0", "powershell.exe"))
            { UseShellExecute = false, CreateNoWindow = true, RedirectStandardOutput = true, RedirectStandardError = true };
            foreach (var arg in new[] { "-NoProfile", "-NonInteractive", "-STA", "-ExecutionPolicy", "Bypass", "-File", script }) start.ArgumentList.Add(arg);
            start.Environment["GC_TOOLS_PIPE"] = pipeName;
            start.Environment["GC_TOOLS_OUTCOME"] = outcome;
            start.Environment["GCM_TRACE_SECRETS"] = "0";
            if (action != "install")
            {
                foreach (var key in new[] { "Lobby2", "GlobalConflictsCore", "GlobalConflictsUnits", "GlobalConflictsMissions" })
                    if (Environment.GetEnvironmentVariable("GC_WORKBENCH_MOD_" + key) is { Length: > 0 } project)
                    { start.ArgumentList.Add("-" + key); start.ArgumentList.Add(project); }
                if (action == "submit")
                { start.ArgumentList.Add("-SubmitMission"); start.ArgumentList.Add("-MissionWorld"); start.ArgumentList.Add(world); }
                else
                {
                    var keys = new[] { ("lobby", "Lobby2"), ("core", "GlobalConflictsCore"), ("units", "GlobalConflictsUnits"), ("missions", "GlobalConflictsMissions") };
                    var selected = keys.Where(k => values.TryGetValue(k.Item1, out var v) && v is true).Select(k => k.Item2).ToArray();
                    if (selected.Length == 0) throw new InvalidOperationException("Select a mod to update.");
                    start.ArgumentList.Add("-SelectedMods"); start.ArgumentList.Add(string.Join(",", selected));
                }
            }
            using var process = await Task.Run(() => Process.Start(start)) ?? throw new IOException("Could not start the action.");
            // Drain without exposing credentials or raw child-process diagnostics in the UI.
            var output = process.StandardOutput.ReadToEndAsync();
            var error = process.StandardError.ReadToEndAsync();
            await process.WaitForExitAsync();
            await Task.WhenAll(output, error);
            if (!File.Exists(outcome)) return new("Action stopped", "Completion could not be confirmed. Your files were not automatically retried.");
            if (new FileInfo(outcome).Length > 128 * 1024) throw new IOException("Invalid action result.");
            return JsonSerializer.Deserialize<OperationResult>(await File.ReadAllTextAsync(outcome), Program.Json)
                ?? throw new IOException("Missing action result.");
        }
        finally
        {
            reviewLifetime.Cancel();
            try { await serve; } catch (OperationCanceledException) { }
            if (File.Exists(outcome)) File.Delete(outcome);
        }
    }

    private static string Prepare(string action)
    {
        var session = Environment.GetEnvironmentVariable("GC_WORKBENCH_MOD_ToolsSession");
        if (string.IsNullOrEmpty(session)) throw new InvalidOperationException("Reopen GC Tools from Workbench.");
        var control = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "GCWorkbench");
        using var installation = JsonDocument.Parse(File.ReadAllText(Path.Combine(control, "installation.json")));
        using var config = JsonDocument.Parse(File.ReadAllText(Path.Combine(installation.RootElement.GetProperty("destination").GetString()!, "settings.json")));
        var port = config.RootElement.GetProperty("port").GetInt32();
        var response = WorkbenchNet.Call(port, new { APIFunc = "GC_ToolsBridge", session, action });
        if (response.GetProperty("session").GetString() != session) throw new InvalidOperationException("Reopen GC Tools from Workbench.");
        if (!response.GetProperty("ready").GetBoolean()) throw new InvalidOperationException(response.GetProperty("error").GetString());
        return response.GetProperty("path").GetString() ?? "";
    }

    private static async Task ServeReviewsAsync(NamedPipeServerStream pipe, Func<DialogSpec, Task<DialogResult>> review, CancellationToken token)
    {
        while (!token.IsCancellationRequested)
        {
            await pipe.WaitForConnectionAsync(token);
            try
            {
                using var reader = new StreamReader(pipe, Encoding.UTF8, false, 4096, leaveOpen: true);
                using var writer = new StreamWriter(pipe, new UTF8Encoding(false), 4096, leaveOpen: true) { AutoFlush = true };
                var json = await reader.ReadLineAsync(token) ?? throw new IOException("Review request missing.");
                if (json.Length > 128 * 1024) throw new IOException("Review request too large.");
                DialogResult answer;
                try
                {
                    var request = JsonSerializer.Deserialize<DialogSpec>(json, Program.Json) ?? throw new IOException("Invalid review request.");
                    request.Validate();
                    answer = await review(request).WaitAsync(token);
                }
                catch (Exception error) when (error is not OperationCanceledException) { answer = new("cancel", []); }
                await writer.WriteLineAsync(JsonSerializer.Serialize(answer, new JsonSerializerOptions { WriteIndented = false }));
            }
            catch (IOException) { /* A disconnected helper cannot receive a review answer. */ }
            finally { if (pipe.IsConnected) pipe.Disconnect(); }
        }
    }
}
