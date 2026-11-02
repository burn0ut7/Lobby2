#ifdef WORKBENCH
//! Automatic, opt-in creation prototype. No file watchers, periodic scans or runtime hooks.
class GC_MissionMetadataRegistration
{
	protected static bool s_bWriting;

	//------------------------------------------------------------------------------------------------
	static void Initialize(string path, ResourceName resourceName)
	{
		if (s_bWriting)
			return;
		// Registration is still in progress: its GUID is not in the resource database yet.
		// Resolve by mounted path, but only when it resolves to this exact newly registered file.
		string sourcePath;
		if (!Workbench.GetAbsolutePath(resourceName.GetPath(), sourcePath))
			return;
		string normalizedPath = path;
		normalizedPath.Replace("\\", "/");
		normalizedPath.ToLower();
		sourcePath.Replace("\\", "/");
		sourcePath.ToLower();
		if (sourcePath != normalizedPath)
			return;
		Resource resource = BaseContainerTools.LoadContainer(resourceName.GetPath());
		if (!resource || !resource.IsValid())
			return;
		BaseContainer header = resource.GetResource().ToBaseContainer();
		if (!header)
			return;
		typename type = header.GetClassName().ToType();
		if (!type || !type.IsInherited(GC_MissionHeader) || header.GetObject("m_GC_GeneratedMetadata"))
			return;
		WorldEditor editor = Workbench.GetModule(WorldEditor);
		if (!editor)
			return;
		WorldEditorAPI api = editor.GetApi();
		if (!api || !api.IsEditMode() || api.IsGameMode() || api.IsPrefabEditMode() || api.IsDoingEditAction() || !api.GetWorld())
			return;
		int subscene = api.GetCurrentSubScene();
		IEntitySource terrain = SCR_WorldEditorToolHelper.GetTerrainEntitySource();
		if (terrain && terrain.GetSubScene() == subscene)
			return;
		string worldPath = api.GetWorld().GetSubSceneName(subscene);
		string absoluteWorld;
		if (worldPath.IsEmpty() || !Workbench.GetAbsolutePath(worldPath, absoluteWorld))
			return;
		ResourceName world = Workbench.GetResourceName(absoluteWorld);
		ResourceName configuredWorld;
		header.Get("World", configuredWorld);
		if (world.IsEmpty() || configuredWorld.IsEmpty() || configuredWorld != world)
		{
			Print("GC | Mission Metadata | Creation snapshot skipped: config World does not match the active mission: " + path, LogLevel.WARNING);
			return;
		}
		GC_MissionMetadataExport exporter = new GC_MissionMetadataExport();
		exporter.Collect(api, subscene);
		GC_GeneratedMissionMetadata snapshot = exporter.CreateSnapshot(world);
		Resource snapshotResource = BaseContainerTools.CreateContainerFromInstance(snapshot);
		if (!snapshotResource || !snapshotResource.IsValid())
			return;
		BaseContainer metadata = snapshotResource.GetResource().ToBaseContainer();
		if (!metadata)
			return;
		s_bWriting = true;
		bool saved = header.SetObject("m_GC_GeneratedMetadata", metadata)
			&& BaseContainerTools.SaveContainer(header, ResourceName.Empty, path);
		if (!saved)
			header.SetObject("m_GC_GeneratedMetadata", null);
		s_bWriting = false;
		if (saved)
			PrintFormat("GC | Mission Metadata | Automatically initialized %1 entities in %2", exporter.GetEntityCount(), path);
		else
			Print("GC | Mission Metadata | Could not initialize config: " + path, LogLevel.ERROR);
	}
}
#endif
