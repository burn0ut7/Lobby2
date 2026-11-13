using System.ComponentModel;
using System.Diagnostics;
using System.Reflection.PortableExecutable;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;

namespace GCWorkbench.Loader;

// This is a fixed GC Lobby dependency bootstrap, not an editor-control service.
// There are no command-line targets, arbitrary DLLs, scripts or commands.
internal static class Program
{
    private const string HostHash = "456a3e4e3b55f47d89c09e4364df78d9a0b1f7348bdc2354cce3b34e1db70b28";
    private const string Protocol = "4df00e70851952d140c187dfefe9c59d274c81d0d9e30a12cd06d87b28accd8f";
    private static readonly string Support = Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "..", ".."));

    private static int Main(string[] args)
    {
        string? resultPath = null;
        try
        {
            if (args.Length != 0) throw new InvalidOperationException("Start GC dependencies from Workbench.");
            using var settings = JsonDocument.Parse(File.ReadAllText(Path.Combine(Support, "settings.json")));
            using var installed = JsonDocument.Parse(File.ReadAllText(Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "GCWorkbench", "installation.json")));
            if (!installed.RootElement.TryGetProperty("enabled", out var enabled) || !enabled.GetBoolean() ||
                !SamePath(installed.RootElement.GetProperty("destination").GetString()!, Support))
                throw new InvalidOperationException("This installation is no longer active.");
            using var parent = Process.GetProcessById(ParentId());
            var host = parent.MainModule?.FileName ?? throw new IOException("Workbench is unavailable.");
            if (!SamePath(host, settings.RootElement.GetProperty("workbenchExecutable").GetString()!) || Hash(host) != HostHash)
                throw new InvalidOperationException("Unsupported Workbench. Update GC dependencies for this version.");
            var identity = parent.Id + "-" + parent.StartTime.ToUniversalTime().Ticks;
            using var mutex = new Mutex(false, @"Local\GCWorkbench.Loader." + identity);
            bool locked;
            try { locked = mutex.WaitOne(0); } catch (AbandonedMutexException) { locked = true; }
            if (!locked) return 0;
            try
            {
                var state = Path.Combine(Support, "state");
                Directory.CreateDirectory(state);
                resultPath = Path.Combine(state, identity + ".json");
                // A previous attempt may have dispatched. Never load again in the same process.
                if (File.Exists(resultPath)) return 0;
                var dll = Path.Combine(Support, "payload", "generic", "gcworkbench.dll");
                using var pinned = new FileStream(dll, FileMode.Open, FileAccess.Read, FileShare.Read);
                if (!Convert.ToHexString(SHA256.HashData(pinned)).Equals(settings.RootElement.GetProperty("dllSha256").GetString(), StringComparison.OrdinalIgnoreCase))
                    throw new IOException("GC dropdown DLL changed. Reinstall dependencies.");
                pinned.Position = 0;
                var export = ExportRva(pinned);
                if (parent.Modules.Cast<ProcessModule>().Any(m => m.ModuleName.Equals("gcworkbench.dll", StringComparison.OrdinalIgnoreCase) || m.ModuleName.Equals("rstauthoring.dll", StringComparison.OrdinalIgnoreCase)))
                    throw new InvalidOperationException("An earlier dropdown DLL is already loaded. Save and restart Workbench.");
                var port = settings.RootElement.GetProperty("port").GetInt32();
                if (port is < 1 or > 65535) throw new IOException("Invalid Workbench connection setting.");
                var report = Path.Combine(state, identity + ".ndjson");
                var config = Path.Combine(state, identity + "-choices.json");
                File.WriteAllText(config, JsonSerializer.Serialize(new { schema = 1, mode = "choices", port, adapterBuild = Protocol, reportPath = report }));
                WriteResult(resultPath, "starting", "", parent.Id, report);
                using var remote = new RemoteProcess(parent.Id);
                // Resolve the actual Windows module containing LoadLibraryW (forwarders included).
                var localLoad = GetProcAddress(GetModuleHandleW("kernel32.dll"), "LoadLibraryW");
                if (localLoad == 0) throw new Win32Exception(Marshal.GetLastWin32Error());
                using var self = Process.GetCurrentProcess();
                var owner = self.Modules.Cast<ProcessModule>().Single(m => localLoad >= m.BaseAddress && localLoad < m.BaseAddress + m.ModuleMemorySize);
                var targetOwner = parent.Modules.Cast<ProcessModule>().Single(m => m.ModuleName.Equals(owner.ModuleName, StringComparison.OrdinalIgnoreCase));
                remote.Call(targetOwner.BaseAddress + (localLoad - owner.BaseAddress), dll);
                parent.Refresh();
                var module = parent.Modules.Cast<ProcessModule>().SingleOrDefault(m => SamePath(m.FileName, dll))
                    ?? throw new IOException("The GC dropdown DLL did not load. Save and restart Workbench before retrying.");
                if (remote.Call(module.BaseAddress + export, "@" + config) != 1)
                    throw new IOException("GC dropdown initialization could not be queued. Save and restart Workbench.");
                var timer = Stopwatch.StartNew();
                while (timer.Elapsed < TimeSpan.FromSeconds(30) && !parent.HasExited)
                {
                    if (File.Exists(report))
                    {
                        // Native reporting may be between records; retry reads, never mutations.
                        using var reportFile = new FileStream(report, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
                        using var reportReader = new StreamReader(reportFile);
                        var lines = reportReader.ReadToEnd().Split('\n');
                        foreach (var line in lines.Take(lines.Length - 1).Where(l => l.EndsWith('}')))
                        {
                            using var item = JsonDocument.Parse(line);
                            var root = item.RootElement;
                            if (root.GetProperty("event").GetString() == "propertyHook")
                            {
                                if (root.GetProperty("state").GetString() != "installed") throw new IOException(root.GetProperty("error").GetString());
                                WriteResult(resultPath, "ready", "", parent.Id, report);
                                return 0;
                            }
                            if (root.TryGetProperty("state", out var status) && status.GetString() == "incompatible")
                                throw new IOException(root.GetProperty("reason").GetString());
                        }
                    }
                    Thread.Sleep(100);
                }
                throw new IOException("Dropdown startup was not confirmed. Save and restart Workbench; no automatic retry was made.");
            }
            finally { mutex.ReleaseMutex(); }
        }
        catch (Exception error)
        {
            try
            {
                Directory.CreateDirectory(Path.Combine(Support, "state"));
                WriteResult(resultPath ?? Path.Combine(Support, "state", "startup-error.json"), "unavailable", error.Message, 0, "");
            }
            catch { /* Cannot write an installation that is inaccessible; never open a popup. */ }
            return 1;
        }
    }

    private static void WriteResult(string path, string state, string detail, int pid, string report) =>
        File.WriteAllText(path, JsonSerializer.Serialize(new { state, detail, pid, report, utc = DateTime.UtcNow }));
    private static bool SamePath(string first, string second) => Path.GetFullPath(first).TrimEnd(Path.DirectorySeparatorChar)
        .Equals(Path.GetFullPath(second).TrimEnd(Path.DirectorySeparatorChar), StringComparison.OrdinalIgnoreCase);
    private static string Hash(string path) { using var file = File.OpenRead(path); return Convert.ToHexString(SHA256.HashData(file)).ToLowerInvariant(); }

    private static int ExportRva(Stream file)
    {
        using var pe = new PEReader(file, PEStreamOptions.LeaveOpen);
        if (pe.PEHeaders.CoffHeader.Machine != Machine.Amd64 || !pe.PEHeaders.CoffHeader.Characteristics.HasFlag(Characteristics.Dll))
            throw new IOException("Invalid GC dropdown DLL architecture.");
        var directory = pe.PEHeaders.PEHeader!.ExportTableDirectory;
        var data = pe.GetSectionData(directory.RelativeVirtualAddress).GetReader();
        data.Offset = 20;
        var functions = data.ReadInt32(); var names = data.ReadInt32();
        var functionTable = data.ReadInt32(); var nameTable = data.ReadInt32(); var ordinalTable = data.ReadInt32();
        if (names is < 1 or > 4096 || functions is < 1 or > 4096) throw new IOException("Invalid DLL exports.");
        for (var i = 0; i < names; ++i)
        {
            var nameRva = pe.GetSectionData(nameTable + i * 4).GetReader().ReadInt32();
            var bytes = pe.GetSectionData(nameRva).GetContent();
            var end = bytes.IndexOf((byte)0);
            if (end < 0 || end > 256) throw new IOException("Invalid DLL export name.");
            if (Encoding.ASCII.GetString(bytes.AsSpan(0, end)) != "GC_WorkbenchQueueStart_v1") continue;
            var ordinal = pe.GetSectionData(ordinalTable + i * 2).GetReader().ReadUInt16();
            if (ordinal >= functions) throw new IOException("Invalid DLL export ordinal.");
            var rva = pe.GetSectionData(functionTable + ordinal * 4).GetReader().ReadInt32();
            if (rva <= 0 || rva >= pe.PEHeaders.PEHeader.SizeOfImage ||
                (rva >= directory.RelativeVirtualAddress && rva < directory.RelativeVirtualAddress + directory.Size)) throw new IOException("Invalid DLL initializer.");
            return rva;
        }
        throw new IOException("GC dropdown initializer is missing.");
    }

    private static int ParentId()
    {
        var snapshot = CreateToolhelp32Snapshot(2, 0);
        if (snapshot == -1) throw new Win32Exception(Marshal.GetLastWin32Error());
        try
        {
            var entry = new ProcessEntry { Size = (uint)Marshal.SizeOf<ProcessEntry>(), Name = "" };
            if (Process32FirstW(snapshot, ref entry))
                do { if (entry.Id == Environment.ProcessId) return (int)entry.Parent; } while (Process32NextW(snapshot, ref entry));
            throw new IOException("Could not identify the launching Workbench.");
        }
        finally { CloseHandle(snapshot); }
    }

    private sealed class RemoteProcess : IDisposable
    {
        private readonly nint handle;
        internal RemoteProcess(int pid)
        {
            handle = OpenProcess(0x043A, false, pid);
            if (handle == 0) throw new Win32Exception(Marshal.GetLastWin32Error());
        }
        internal uint Call(nint function, string argument)
        {
            var bytes = Encoding.Unicode.GetBytes(argument + '\0');
            var memory = VirtualAllocEx(handle, 0, (nuint)bytes.Length, 0x3000, 4);
            if (memory == 0) throw new Win32Exception(Marshal.GetLastWin32Error());
            bool release = true;
            try
            {
                if (!WriteProcessMemory(handle, memory, bytes, (nuint)bytes.Length, out var written) || written != (nuint)bytes.Length)
                    throw new Win32Exception(Marshal.GetLastWin32Error());
                var thread = CreateRemoteThread(handle, 0, 0, function, memory, 0, out _);
                if (thread == 0) throw new Win32Exception(Marshal.GetLastWin32Error());
                try
                {
                    // On an uncertain wait the argument must remain valid until host exit.
                    release = false;
                    if (WaitForSingleObject(thread, 15000) != 0) throw new IOException("DLL loading is still pending. Save and restart Workbench before retrying.");
                    release = true;
                    if (!GetExitCodeThread(thread, out var code)) throw new Win32Exception(Marshal.GetLastWin32Error());
                    return code;
                }
                finally { CloseHandle(thread); }
            }
            finally { if (release) VirtualFreeEx(handle, memory, 0, 0x8000); }
        }
        public void Dispose() => CloseHandle(handle);
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)] private struct ProcessEntry
    {
        internal uint Size, Usage, Id; internal nuint Heap; internal uint Module, Threads, Parent; internal int Priority; internal uint Flags;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)] internal string Name;
    }
    [DllImport("kernel32.dll", SetLastError = true)] private static extern nint CreateToolhelp32Snapshot(uint flags, uint pid);
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode)] private static extern bool Process32FirstW(nint handle, ref ProcessEntry entry);
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode)] private static extern bool Process32NextW(nint handle, ref ProcessEntry entry);
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode)] private static extern nint GetModuleHandleW(string name);
    [DllImport("kernel32.dll", CharSet = CharSet.Ansi, ExactSpelling = true, SetLastError = true)] private static extern nint GetProcAddress(nint handle, string name);
    [DllImport("kernel32.dll", SetLastError = true)] private static extern nint OpenProcess(uint access, bool inherit, int pid);
    [DllImport("kernel32.dll", SetLastError = true)] private static extern nint VirtualAllocEx(nint process, nint address, nuint size, uint type, uint protection);
    [DllImport("kernel32.dll")] private static extern bool VirtualFreeEx(nint process, nint address, nuint size, uint type);
    [DllImport("kernel32.dll", SetLastError = true)] private static extern bool WriteProcessMemory(nint process, nint address, byte[] bytes, nuint size, out nuint written);
    [DllImport("kernel32.dll", SetLastError = true)] private static extern nint CreateRemoteThread(nint process, nint attributes, nuint stack, nint start, nint parameter, uint flags, out uint id);
    [DllImport("kernel32.dll")] private static extern uint WaitForSingleObject(nint handle, uint timeout);
    [DllImport("kernel32.dll", SetLastError = true)] private static extern bool GetExitCodeThread(nint thread, out uint code);
    [DllImport("kernel32.dll")] private static extern bool CloseHandle(nint handle);
}
