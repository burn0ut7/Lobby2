#ifdef WORKBENCH
[WorkbenchPluginAttribute(name: "GC Tools", description: "Mission submission, mod updates and external dependencies.", wbModules: { "WorldEditor" }, category: "GC", awesomeFontCode: 0xF0AD)]
class GC_ToolsPlugin : WorkbenchPlugin
{
	static string s_sSession;
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
		s_sSession = Workbench.GenerateGloballyUniqueID64() + Workbench.GenerateGloballyUniqueID64();
		string executable;
		Workbench.GetExecutableFilePath(executable);
		string profile;
		Workbench.GetAbsolutePath("$profile:", profile);
		s_Process = Workbench.RunProcess("wscript.exe //nologo //E:JScript \"" + launcher + "\" --tools" + GC_ModWorkflow.ProjectArguments() + " -ToolsSession " + s_sSession + " -WorkbenchExecutable \"" + executable + "\" -WorkbenchProfile \"" + profile + "\"");
		if (!s_Process)
			Print("GC Tools could not start.", LogLevel.ERROR);
	}
}

class GC_ToolsRequest : JsonApiStruct
{
	string session;
	string action;
	void GC_ToolsRequest()
	{
		RegV("session");
		RegV("action");
	}
}

class GC_ToolsResponse : JsonApiStruct
{
	string session;
	string path;
	string error;
	bool ready;
	void GC_ToolsResponse()
	{
		RegV("session");
		RegV("path");
		RegV("error");
		RegV("ready");
	}
}

// Only the current, explicitly opened GC Tools session may save/export.
// Requests never contain script code or file paths.
class GC_ToolsBridge : NetApiHandler
{
	override JsonApiStruct GetRequest()
	{
		return new GC_ToolsRequest();
	}

	override JsonApiStruct GetResponse(JsonApiStruct request)
	{
		GC_ToolsRequest input = GC_ToolsRequest.Cast(request);
		GC_ToolsResponse result = new GC_ToolsResponse();
		if (!input || input.session.IsEmpty() || input.session != GC_ToolsPlugin.s_sSession)
		{
			result.error = "Reopen GC Tools from Workbench.";
			return result;
		}
		result.session = input.session;
		if (input.action == "submit")
			result.ready = GC_MissionSubmission.Prepare(result.path, result.error);
		else if (input.action == "update")
		{
			WorldEditor editor = Workbench.GetModule(WorldEditor);
			WorldEditorAPI api;
			if (editor)
				api = editor.GetApi();
			if (api && api.IsEditMode() && !api.IsGameMode() && !api.IsDoingEditAction())
				result.ready = editor.Save();
			if (!result.ready)
				result.error = "Stop Play and save your work before updating.";
		}
		else
			result.error = "Unsupported GC Tools action.";
		return result;
	}
}
#endif
