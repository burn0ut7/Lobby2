#ifdef WORKBENCH
[WorkbenchPluginAttribute(name: "GC Tools", description: "Mission submission, mod updates and external dependencies.", wbModules: { "WorldEditor" }, category: "GC", awesomeFontCode: 0xF0AD)]
class GC_ToolsPlugin : WorkbenchPlugin
{
	override void Run()
	{
		string launcher;
		if (!Workbench.GetAbsolutePath("$Lobby2:external/workbench/Install.js", launcher) || !FileIO.FileExists(launcher))
		{
			Workbench.Dialog("GC Tools", "Open the Lobby2 source checkout with its external/workbench folder.");
			return;
		}
		ProcessHandle process = Workbench.RunProcess("wscript.exe //nologo //E:JScript \"" + launcher + "\" --tools");
		if (!process)
		{
			Workbench.Dialog("GC Tools", "Could not open GC Tools.");
			return;
		}
		// Only the picker waits here; it never calls back into Workbench.
		int selected = Workbench.WaitProcess(process);
		Workbench.FinishProcess(process);
		if (selected > 100 && selected < 116)
		{
			int mask = selected - 100;
			GC_ModSelection mods = new GC_ModSelection();
			mods.m_bLobby2 = (mask & 1) != 0;
			mods.m_bGCCore = (mask & 2) != 0;
			mods.m_bGCUnits = (mask & 4) != 0;
			mods.m_bGCMissions = (mask & 8) != 0;
			GC_ModWorkflow.Launch(GC_ModWorkflow.ProjectArguments() + " -SelectedMods " + mods.SelectedMods());
			return;
		}
		switch (selected)
		{
			case 10:
				GC_MissionSubmission.Run();
				break;
			case 12:
				GC_ExternalDependencies.Run();
				break;
			case 2:
				break;
			default:
				Workbench.Dialog("GC Tools", "GC Tools could not finish. Check the external/workbench package.");
		}
	}
}
#endif
