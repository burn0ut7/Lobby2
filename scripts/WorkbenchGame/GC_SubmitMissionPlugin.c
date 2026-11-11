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
		ResourceManager manager = Workbench.GetModule(ResourceManager);
		MetaFile worldMeta;
		if (manager)
			worldMeta = manager.GetMetaFile(path);
		if (!worldMeta)
		{
			Workbench.Dialog("Submit Mission", "Save and register the mission world first.");
			return;
		}
		ResourceName worldResource = worldMeta.GetResourceID();
		ResourceName headerName;
		string error;
		GC_MissionMetadataConfig metadata = new GC_MissionMetadataConfig();
		Resource header = metadata.FindHeader(world, worldResource, headerName, error);
		if (!header)
		{
			Workbench.Dialog("Submit Mission", error);
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
		if (!collected || !metadata.Save(header, headerName, exporter, worldResource, error))
		{
			Workbench.Dialog("Submit Mission", "Could not refresh mission data. " + exporter.GetError() + error);
			return;
		}
		GC_ModWorkflow.Launch(GC_ModWorkflow.ProjectArguments() + " -SubmitMission -MissionWorld \"" + path + "\"");
	}
}
#endif
