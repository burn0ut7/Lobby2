#ifdef WORKBENCH
[WorkbenchPluginAttribute(name: "Save Mission and Metadata", description: "Save the mission, then refresh its website metadata. Only the active mission subscene is scanned; base terrain and other subscenes are excluded.", wbModules: { "WorldEditor" }, category: "GC", awesomeFontCode: 0xF0C7)]
class GC_MissionMetadataPlugin : ResourceManagerPlugin
{
	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		WorldEditor editor = Workbench.GetModule(WorldEditor);
		if (!editor)
			return;
		WorldEditorAPI api = editor.GetApi();
		if (!api || !api.IsEditMode() || api.IsGameMode() || api.IsPrefabEditMode() || api.IsDoingEditAction() || !api.GetWorld())
		{
			Report("Open a mission world in edit mode first.");
			return;
		}

		int subscene = api.GetCurrentSubScene();
		IEntitySource terrain = SCR_WorldEditorToolHelper.GetTerrainEntitySource();
		if (terrain && terrain.GetSubScene() == subscene)
		{
			Report("Select your mission subscene, not the base terrain scene.");
			return;
		}
		string worldPath = api.GetWorld().GetSubSceneName(subscene);
		string absolutePath;
		if (worldPath.IsEmpty() || !Workbench.GetAbsolutePath(worldPath, absolutePath))
		{
			Report("The active mission needs an existing local world file before metadata can be exported.");
			return;
		}

		if (!editor.Save())
		{
			Report("The mission could not be saved. Metadata was not replaced.");
			return;
		}
		WBProgressDialog progress = new WBProgressDialog("Refreshing mission metadata", editor);
		GC_MissionMetadataExport exporter = new GC_MissionMetadataExport();
		if (!exporter.Collect(api, subscene, progress))
		{
			progress = null;
			Report("Metadata was not replaced: " + exporter.GetError());
			return;
		}
		string output = absolutePath + ".metadata.json";
		if (!exporter.Save(output, worldPath))
		{
			progress = null;
			Report("Metadata could not be written: " + output);
			return;
		}
		progress = null;
		string message = string.Format("Saved mission and refreshed metadata for %1 entities.\n%2", exporter.GetEntityCount(), output);
		Print("GC | Mission Metadata | " + message);
		Workbench.Dialog("GC Mission Metadata", message);
	}

	//------------------------------------------------------------------------------------------------
	protected void Report(string message)
	{
		Print("GC | Mission Metadata | " + message, LogLevel.WARNING);
		Workbench.Dialog("GC Mission Metadata", message);
	}
}
#endif
