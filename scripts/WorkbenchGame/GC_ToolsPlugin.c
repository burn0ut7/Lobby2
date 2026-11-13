#ifdef WORKBENCH
[WorkbenchPluginAttribute(name: "GC Tools", description: "Mission submission, mod updates and external dependencies.", wbModules: { "WorldEditor" }, category: "GC", awesomeFontCode: 0xF0AD)]
class GC_ToolsPlugin : WorkbenchPlugin
{
	static ProcessHandle s_Process;

	override void Run()
	{
		if (s_Process)
		{
			if (Workbench.IsRunningProcess(s_Process))
				return;
			Workbench.FinishProcess(s_Process);
			s_Process = null;
		}
		string launcher;
		if (!Workbench.GetAbsolutePath("$Lobby2:external/workbench/Install.js", launcher) || !FileIO.FileExists(launcher))
		{
			Print("GC Tools: external/workbench/Install.js is missing.", LogLevel.ERROR);
			return;
		}
		string executable;
		Workbench.GetExecutableFilePath(executable);
		string profile;
		Workbench.GetAbsolutePath("$profile:", profile);
		s_Process = Workbench.RunProcess("wscript.exe //nologo //E:JScript \"" + launcher + "\" --tools" + GC_ModWorkflow.ProjectArguments() + " -WorkbenchExecutable \"" + executable + "\" -WorkbenchProfile \"" + profile + "\"");
		if (!s_Process)
			Print("GC Tools could not start.", LogLevel.ERROR);
	}
}

#endif
