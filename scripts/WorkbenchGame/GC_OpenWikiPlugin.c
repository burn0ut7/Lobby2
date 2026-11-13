#ifdef WORKBENCH
[WorkbenchPluginAttribute(name: "Open Wiki", description: "Open the GC mission-making wiki in your default browser.", wbModules: { "WorldEditor" }, category: "GC")]
class GC_OpenWikiPlugin : WorkbenchPlugin
{
	override void Run()
	{
		ProcessHandle process = Workbench.RunProcess("explorer.exe \"https://github.com/Global-Conflicts-ArmA/gc-reforger-missions/wiki\"");
		if (!process)
		{
			Workbench.Dialog("Open Wiki", "Could not open your browser. Visit https://github.com/Global-Conflicts-ArmA/gc-reforger-missions/wiki");
			return;
		}
		Workbench.FinishProcess(process);
	}
}
#endif
