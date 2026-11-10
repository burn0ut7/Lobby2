#ifdef WORKBENCH
[WorkbenchPluginAttribute(name: "List weather presets", description: "Show every weather preset in the loaded map for Time and Weather Action.", wbModules: {"WorldEditor"}, category: "GC")]
class GC_ListWeatherPresetsPlugin : WorkbenchPlugin
{
	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		WorldEditor editor = Workbench.GetModule(WorldEditor);
		if (!editor)
			return;
		WorldEditorAPI api = editor.GetApi();
		if (!api || !api.GetWorld())
		{
			Workbench.Dialog("Weather presets", "Open a mission world first.");
			return;
		}
		array<string> names = GC_WeatherAction.GetPresetNames(api.GetWorld());
		if (names.IsEmpty())
		{
			Workbench.Dialog("Weather presets", "No weather presets were found in this map's Time and Weather Manager.");
			return;
		}
		string message = "Use an exact name below in Time and Weather Action:\n";
		foreach (string name : names)
		{
			message += "\n" + name;
			Print("GC weather preset: " + name);
		}
		Workbench.Dialog("Weather presets", message);
	}
}
#endif
