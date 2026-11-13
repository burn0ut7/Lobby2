#ifdef WORKBENCH
class GC_MissionSubmission
{
	static bool Prepare(out string path, out string error)
	{
		WorldEditor editor = Workbench.GetModule(WorldEditor);
		if (!editor)
		{
			error = "Open World Editor.";
			return false;
		}
		WorldEditorAPI api = editor.GetApi();
		if (!api || !api.IsEditMode() || api.IsGameMode() || api.IsPrefabEditMode() || api.IsDoingEditAction() || !api.GetWorld())
		{
			error = "Open your mission in edit mode.";
			return false;
		}
		int subscene = api.GetCurrentSubScene();
		string world = api.GetWorld().GetSubSceneName(subscene);
		if (!world.StartsWith("$GlobalConflictsMissions:") || !Workbench.GetAbsolutePath(world, path))
		{
			error = "Select a saved world in GC Missions.";
			return false;
		}
		ResourceManager manager = Workbench.GetModule(ResourceManager);
		MetaFile worldMeta;
		if (manager)
			worldMeta = manager.GetMetaFile(path);
		if (!worldMeta)
		{
			error = "Save and register the mission world first.";
			return false;
		}
		ResourceName worldResource = worldMeta.GetResourceID();
		ResourceName headerName;
		GC_MissionMetadataConfig metadata = new GC_MissionMetadataConfig();
		Resource header = metadata.FindHeader(world, worldResource, headerName, error);
		if (!header)
		{
			// Header lookup supplies the error.
			return false;
		}
		if (!editor.Save())
		{
			error = "Save your mission and try again.";
			return false;
		}
		GC_MissionMetadataExport exporter = new GC_MissionMetadataExport();
		WBProgressDialog progress;
		bool collected = exporter.Collect(api, subscene, progress);
		progress = null;
		if (!collected || !metadata.Save(header, headerName, exporter, worldResource, error))
		{
			error = "Could not refresh mission data. " + exporter.GetError() + error;
			return false;
		}
		return true;
	}
}
#endif
