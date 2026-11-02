#ifdef WORKBENCH
// Disabled experiment: see docs/MissionMetadata.md before enabling.
// [WorkbenchPluginAttribute(name: "Metadata callback probe", wbModules: { "ResourceManager" }, category: "GC Tests")]
class GC_MetadataCallbackProbe : ResourceManagerPlugin
{
	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		if (!CreateFixture())
			return;
		string savedPath;
		Workbench.GetAbsolutePath("$Lobby2McpTests:Missions/MetadataAutomatic4.conf", savedPath);
		Resource resource = BaseContainerTools.LoadContainer(Workbench.GetResourceName(savedPath));
		if (!resource || !resource.IsValid())
			return;
		BaseContainer header = resource.GetResource().ToBaseContainer();
		if (!header || !header.GetObject("m_GC_GeneratedMetadata"))
		{
			Print("GC_METADATA_PROBE FAIL missing native snapshot", LogLevel.ERROR);
			return;
		}
		GC_GeneratedMissionMetadata snapshot = GC_GeneratedMissionMetadata.Cast(BaseContainerTools.CreateInstanceFromContainer(header.GetObject("m_GC_GeneratedMetadata")));
		if (!snapshot || snapshot.m_iEntityCount != 9 || snapshot.m_aAssets.Count() != 3 || snapshot.m_aPrefabs.Count() != 8 || snapshot.m_aFactions.Count() != 2)
		{
			Print("GC_METADATA_PROBE FAIL native snapshot readback", LogLevel.ERROR);
			return;
		}
		PrintFormat("GC_METADATA_PROBE PASS native snapshot readback: entities=%1, assets=%2, prefabs=%3, addons=%4", snapshot.m_iEntityCount, snapshot.m_aAssets.Count(), snapshot.m_aPrefabs.Count(), snapshot.m_aLoadedAddons.Count());
	}

	//------------------------------------------------------------------------------------------------
	protected bool CreateFixture()
	{
		ResourceManager manager = Workbench.GetModule(ResourceManager);
		string path;
		if (!manager || !Workbench.GetAbsolutePath("$Lobby2McpTests:Missions/MetadataAutomatic4.conf", path, false))
			return false;
		if (FileIO.FileExists(path))
		{
			Print("GC_METADATA_PROBE existing output; refusing replacement");
			return false;
		}
		Resource holder = BaseContainerTools.CreateContainer("GC_MissionHeader");
		if (!holder || !holder.IsValid())
			return false;
		BaseContainer header = holder.GetResource().ToBaseContainer();
		if (!header)
			return false;
		header.Set("World", "{A79C4E0A43DEB850}worlds/MetadataTest.ent");
		header.Set("m_sName", "Automatic metadata prototype");
		header.Set("m_sAuthor", "Preserve this author");
		if (!BaseContainerTools.SaveContainer(header, ResourceName.Empty, path))
			return false;
		manager.RegisterResourceFile(path, false);
		manager.SetOpenedResource("$Lobby2McpTests:Missions/MetadataAutomatic4.conf");
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void OnRegisterResource(string absFileName, BaseContainer metaFile)
	{
		// Explicit fixture only: never load unrelated configs during their registration.
		string fixture;
		Workbench.GetAbsolutePath("$Lobby2McpTests:Missions/MetadataAutomatic4.conf", fixture, false);
		if (absFileName != fixture)
			return;
		ResourceName resourceName;
		if (metaFile && metaFile.Get("Name", resourceName) && !resourceName.IsEmpty())
			GC_MissionMetadataRegistration.Initialize(absFileName, resourceName);
	}

	//------------------------------------------------------------------------------------------------
	override BuildResourceContext OnBuildResource(string absSrcFileName, string absDestFileName, BaseContainer metaFile, BaseContainer configuration, GeneratedResources generatedResources)
	{
		if (absSrcFileName.Contains("Metadata"))
			Print("GC_METADATA_PROBE build " + absSrcFileName);
		return null;
	}
}
#endif
