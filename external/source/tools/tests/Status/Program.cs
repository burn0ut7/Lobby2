using System.IO.Compression;
using System.Security.Cryptography;
using System.Text.Json;
using GCWorkbench.UI;
var root=Path.Combine(Path.GetTempPath(),"gc-status-"+Guid.NewGuid());
var control=Path.Combine(root,"control");var install=Path.Combine(root,"installed");var profile=Path.Combine(root,"profile");var package=Path.Combine(root,"package");var ui=Path.Combine(root,"ui");
foreach(var folder in new[]{control,install,profile,package,ui})Directory.CreateDirectory(folder);
void Check(bool condition,string reason){if(!condition)throw new Exception(reason);}
Check(ToolStatus.CheckWorkbench(control,package,ui).Status=="Not installed","Missing installation");
File.WriteAllText(Path.Combine(control,"installation.json"),JsonSerializer.Serialize(new{destination=install,profileDirectory=profile}));
Check(ToolStatus.CheckWorkbench(control,package,ui).Status=="Installation incomplete","Missing native DLL");
File.WriteAllText(Path.Combine(control,"installation.json"),JsonSerializer.Serialize(new{destination=install,profileDirectory=profile,enabled=false}));
Check(ToolStatus.CheckWorkbench(control,package,ui).Status=="Not installed","Disabled integration reported installed");
File.WriteAllText(Path.Combine(control,"installation.json"),JsonSerializer.Serialize(new{destination=install,profileDirectory=profile}));
var paths=new Dictionary<string,string>{["support/payload/generic/gcworkbench.dll"]=Path.Combine(install,"payload/generic/gcworkbench.dll"),["support/payload/helper/GC Tools.dll"]=Path.Combine(install,"payload/helper/GC Tools.dll"),["support/payload/lobby2-tools/GC-GitHub.ps1"]=Path.Combine(profile,"GCWorkbench/tools/GC-GitHub.ps1")};
foreach(var target in paths.Values){Directory.CreateDirectory(Path.GetDirectoryName(target)!);File.WriteAllText(target,"expected");}
var zipPath=Path.Combine(package,"GCWorkbench.zip");
using(var zip=ZipFile.Open(zipPath,ZipArchiveMode.Create))using(var writer=new StreamWriter(zip.CreateEntry("support/files.json").Open()))writer.Write(JsonSerializer.Serialize(paths.Select(p=>new{path=p.Key,sha256=Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(p.Value)))})));
File.WriteAllText(zipPath+".sha256",Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(zipPath)))+"  GCWorkbench.zip");
File.WriteAllText(Path.Combine(ui,"GC Tools.dll"),"expected");
Check(ToolStatus.CheckWorkbench(control,package,ui).Status=="Up to date","Matching package");
File.WriteAllText(Path.Combine(ui,"GC Tools.dll"),"old running UI");
var restart=ToolStatus.CheckWorkbench(control,package,ui);
Check(restart.State=="ready"&&restart.Status=="Restart Workbench","Updated installed UI mistaken for another required install");
File.WriteAllText(paths.Values.Last(),"old helper");
Check(ToolStatus.CheckWorkbench(control,package,ui).Status=="Update or repair available","Changed helper");
File.Delete(paths.Values.Last());
Check(ToolStatus.CheckWorkbench(control,package,ui).Status.StartsWith("Installation incomplete"),"Missing helper");
File.WriteAllText(zipPath+".sha256",new string('0',64));
Check(ToolStatus.CheckWorkbench(control,package,ui).Status=="Dependency package needs repair","Corrupt package");
Check(ToolStatus.ParseGitVersion("git version 2.54.0.windows.1")!>ToolStatus.ParseGitVersion("v2.54.0.windows.0"),"Windows revision comparison");
Check(ToolStatus.ParseGitVersion("v2.55.0.windows.1")!>ToolStatus.ParseGitVersion("git version 2.54.9.windows.9"),"New version comparison");
Check(ToolStatus.ParseGitVersion("unavailable")==null,"Invalid version");
var missing=await ToolStatus.RunAsync(Path.Combine(root,"absent.exe"),[],1,CancellationToken.None);
Check(missing.Code==-1,"Missing process not contained");
using(var cancelled=new CancellationTokenSource()) {
 cancelled.Cancel();
 var stopped=await ToolStatus.RunAsync("git.exe",["--version"],1,cancelled.Token);
 Check(stopped.Code==-1&&stopped.Output=="","Queued cancellation escaped status contract");
}
var slow=await ToolStatus.RunAsync(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System),"WindowsPowerShell/v1.0/powershell.exe"),["-NoProfile","-NonInteractive","-Command","Start-Sleep -Seconds 10"],1,CancellationToken.None);
Check(slow.Code==-1,"Slow check did not time out safely");
Console.WriteLine("PASS: missing/incomplete/current/outdated/corrupt installation, Git version comparison, unavailable helper.");
var currentMod=ToolStatus.ModStatusField(new InputField{Id="lobby",Label="GC Lobby",Kind="check",Checked=true},0,"{\"Status\":\"Up to date\",\"UpdateNeeded\":false}");
Check(!currentMod.Enabled&&!currentMod.Checked,"Current mod eligibility");
var missingMod=ToolStatus.ModStatusField(currentMod,0,"{\"Status\":\"Not installed\",\"UpdateNeeded\":true}");
Check(missingMod.Enabled&&missingMod.Checked&&missingMod.Help=="Not installed","Missing mods must be selected and actionable");
Check(ToolPages.Updates().PrimaryAction=="Update","Update button label");
var oldMod=ToolStatus.ModStatusField(currentMod,0,"{\"Status\":\"Update available\",\"UpdateNeeded\":true}");
Check(oldMod.Enabled&&oldMod.Checked,"Outdated mod eligibility");
Check(!ToolStatus.ModStatusField(oldMod,-1,"").Enabled,"Failed mod check eligibility");
Console.WriteLine("PASS: per-mod status eligibility mapping.");
// First-launch routing uses local presence, not network/update-check results.
Check(ToolPages.Initial(true).Title=="External dependencies","Missing dependency start page");
Check(ToolPages.Initial(false).Title=="Submit mission","Installed start page");
Check(ToolStatus.MissingStartupDependencies(control),"Missing helpers not detected");
var helper=Path.Combine(install,"payload/helper/GCWorkbench.Loader.exe");Directory.CreateDirectory(Path.GetDirectoryName(helper)!);File.WriteAllText(helper,"installed older version");
foreach(var name in new[]{"GC-GitInstallation.ps1","GC-ModLauncher.js","GC-ModWorkflow.ps1","GC-SubmitMission.ps1","GC-Review.ps1","GC-GitHub.ps1","GC-ModStatus.ps1"})File.WriteAllText(Path.Combine(profile,"GCWorkbench/tools",name),"installed");
Check(!ToolStatus.MissingStartupDependencies(control),"Present components mistaken for missing because updates exist");

File.Delete(Path.Combine(profile,"GCWorkbench/tools/GC-ModStatus.ps1"));
Check(ToolStatus.MissingStartupDependencies(control),"Missing profile helper not detected");
File.WriteAllText(Path.Combine(control,"installation.json"),"broken record");
Check(ToolStatus.MissingStartupDependencies(control),"Broken install not routed to dependencies");
Console.WriteLine("PASS: first-launch routing, present/older components, missing helpers, damaged installation.");

Check(ToolStatus.RegisteredGitStatus(0,"{\"Installed\":false}").Status=="Not installed","Bundled PATH Git must not count as Git for Windows");
Check(ToolStatus.RegisteredGitStatus(0,"{\"Installed\":true,\"Version\":\"git version 2.56.0.windows.4\",\"CredentialManager\":true}").State=="ready","Registered Git version lost");
Check(ToolStatus.RegisteredGitStatus(-1,"").Status=="Could not check installation","Failed inspection reported absent");
