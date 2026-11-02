#ifdef WORKBENCH
[WorkbenchPluginAttribute(name: "Export Mission Metadata", description: "Export factions, placed prefab addons and asset categories from all layers of the active mission subscene. Replaces its .metadata.json file; does not modify or save the world.", wbModules: { "WorldEditor" }, category: "GC", awesomeFontCode: 0xF15C)]
class GC_MissionMetadataPlugin : WorkbenchPlugin
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

		GC_MissionMetadataExport exporter = new GC_MissionMetadataExport();
		exporter.Collect(api, subscene);
		string output = absolutePath + ".metadata.json";
		if (!exporter.Save(output, worldPath))
		{
			Report("Metadata could not be written: " + output);
			return;
		}
		string message = string.Format("Exported %1 entities from the active mission subscene.\n%2\n\nIncludes current editor edits and nested prefab entities. Other subscenes and runtime spawns are excluded. Save the mission and export again before publishing.", exporter.GetEntityCount(), output);
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
