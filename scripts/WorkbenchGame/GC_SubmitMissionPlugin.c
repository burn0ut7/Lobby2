#ifdef WORKBENCH
[WorkbenchPluginAttribute(name: "Submit Mission", description: "Save this GC mission and submit it for review.", wbModules: { "WorldEditor" }, category: "GC")]
class GC_SubmitMissionPlugin : WorkbenchPlugin
{
	override void Run()
	{
		WorldEditor editor = Workbench.GetModule(WorldEditor);
		if (!editor)
			return;
		WorldEditorAPI api = editor.GetApi();
		if (!api || !api.IsEditMode() || api.IsGameMode() || api.IsPrefabEditMode() || api.IsDoingEditAction() || !api.GetWorld())
		{
			Workbench.Dialog("Submit Mission", "Open your mission in edit mode.");
			return;
		}
		int subscene = api.GetCurrentSubScene();
		string world = api.GetWorld().GetSubSceneName(subscene);
		string path;
		if (!world.StartsWith("$GlobalConflictsMissions:") || !Workbench.GetAbsolutePath(world, path))
		{
			Workbench.Dialog("Submit Mission", "Select a saved world in GC Missions.");
			return;
		}
		if (!editor.Save())
		{
			Workbench.Dialog("Submit Mission", "Save your mission and try again.");
			return;
		}
		GC_MissionMetadataExport exporter = new GC_MissionMetadataExport();
		WBProgressDialog progress = new WBProgressDialog("Refreshing mission data", editor);
		bool collected = exporter.Collect(api, subscene, progress);
		progress = null;
		if (!collected || !exporter.Save(world + ".metadata.json", world))
		{
			Workbench.Dialog("Submit Mission", "Could not refresh mission data. " + exporter.GetError());
			return;
		}
		GC_ModWorkflow.Launch(GC_ModWorkflow.ProjectArguments() + " -SubmitMission -MissionWorld \"" + path + "\"");
	}
}
#endif
