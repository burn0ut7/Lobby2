#ifdef WORKBENCH
[WorkbenchPluginAttribute(name: "Install External Dependencies", description: "Install the GC Workbench dropdown extension, Git and Git helpers.", wbModules: { "WorldEditor" }, category: "GC")]
class GC_InstallExternalDependenciesPlugin : WorkbenchPlugin
{
	override void Run()
	{
		string installer;
		if (!Workbench.GetAbsolutePath("$Lobby2:external/workbench/Install.js", installer) || !FileIO.FileExists(installer))
		{
			Workbench.Dialog("Install External Dependencies", "Open the Lobby2 source checkout with its external/workbench folder. The installer requires ordinary Windows files and cannot run from a packed Workshop addon.");
			return;
		}

		ProcessHandle process = Workbench.RunProcess("wscript.exe //nologo //E:JScript \"" + installer + "\"");
		if (!process)
		{
			Workbench.Dialog("Install External Dependencies", "Could not start setup. Run external/workbench/Install.js from your Lobby2 folder.");
			return;
		}
		// Setup may reload scripts; do not block the editor waiting for it.
		Workbench.FinishProcess(process);
	}
}
#endif
