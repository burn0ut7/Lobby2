#ifdef WORKBENCH
// Enable only for the explicit test described in docs/MissionMetadata.md.
// [WorkbenchPluginAttribute(name: "Metadata event probe", wbModules: { "ResourceManager" }, category: "GC Tests")]
class GC_MetadataEventProbe : WorkbenchPlugin
{
	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		ResourceManager manager = Workbench.GetModule(ResourceManager);
		string first;
		string second;
		if (!manager || !Workbench.GetAbsolutePath("$Lobby2McpTests:Missions/MetadataEventA.conf", first, false)
			|| !Workbench.GetAbsolutePath("$Lobby2McpTests:Missions/MetadataEventB.conf", second, false))
			return;
		if (FileIO.FileExists(first) || FileIO.FileExists(second))
		{
			Print("GC_METADATA_EVENT_PROBE existing output; refusing replacement", LogLevel.WARNING);
			return;
		}
		Resource holder = BaseContainerTools.CreateContainer("GC_MissionHeader");
		if (!holder || !holder.IsValid())
			return;
		BaseContainer header = holder.GetResource().ToBaseContainer();
		header.Set("World", "{A79C4E0A43DEB850}worlds/MetadataTest.ent");
		header.Set("m_sName", "Event metadata test");
		header.Set("m_sAuthor", "Preserve authored information");
		array<string> tags = {"night", "author-custom"};
		header.Set("m_aManualTags", tags);
		if (!BaseContainerTools.SaveContainer(header, ResourceName.Empty, first)
			|| !BaseContainerTools.SaveContainer(header, ResourceName.Empty, second))
			return;
		manager.RegisterResourceFile(first, false);
		manager.RegisterResourceFile(second, false);
		Print("GC_METADATA_EVENT_PROBE registered two configs; production callback must coalesce refresh");
	}
}
#endif
