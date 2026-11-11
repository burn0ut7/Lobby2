#ifdef WORKBENCH
//! Updates only generated evidence in the mission's registered native header.
class GC_MissionMetadataConfig
{
	protected ref array<ResourceName> m_aHeaders = {};

	//------------------------------------------------------------------------------------------------
	Resource FindHeader(string worldPath, ResourceName world, out ResourceName name, out string error)
	{
		m_aHeaders.Clear();
		name = string.Empty;
		int mountEnd = worldPath.IndexOf(":");
		if (mountEnd < 2 || !worldPath.StartsWith("$"))
		{
			error = "Select a saved mission world in a local addon.";
			return null;
		}
		string mount = worldPath.Substring(0, mountEnd + 1);
		array<string> folders = {"Missions", "Configs/Missions"};
		foreach (string folder : folders)
		{
			string absolute;
			if (!Workbench.GetAbsolutePath(mount + folder, absolute))
				continue;
			SearchResourcesFilter filter = new SearchResourcesFilter();
			filter.rootPath = mount + folder;
			filter.fileExtensions = {"conf"};
			if (!ResourceDatabase.SearchResources(filter, OnHeader))
			{
				error = "Could not search the mission headers.";
				return null;
			}
		}
		Resource selected;
		foreach (ResourceName candidate : m_aHeaders)
		{
			Resource resource = BaseContainerTools.LoadContainer(candidate);
			if (!resource || !resource.IsValid())
			{
				error = "Could not read mission header: " + candidate;
				return null;
			}
			BaseContainer header = resource.GetResource().ToBaseContainer();
			ResourceName headerWorld;
			if (!header || !header.Get("World", headerWorld) || headerWorld != world)
				continue;
			if (selected)
			{
				error = "Multiple mission headers reference this world. Keep one header before refreshing or submitting.";
				return null;
			}
			selected = resource;
			name = candidate;
		}
		if (!selected)
			error = "No registered mission header references this world. Create Mission Data first.";
		return selected;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnHeader(ResourceName name, string exactPath)
	{
		if (!m_aHeaders.Contains(name))
			m_aHeaders.Insert(name);
	}

	//------------------------------------------------------------------------------------------------
	bool Apply(BaseContainer header, GC_MissionMetadataExport exporter, ResourceName world, out string error)
	{
		typename type;
		if (header)
			type = header.GetClassName().ToType();
		if (!type || !type.IsInherited(GC_MissionHeader))
		{
			error = "This mission needs a GC_MissionHeader to store generated metadata.";
			return false;
		}
		GC_GeneratedMissionMetadata snapshot = exporter.CreateSnapshot(world);
		if (!snapshot)
		{
			error = "Metadata collection did not complete.";
			return false;
		}
		Resource data = BaseContainerTools.CreateContainerFromInstance(snapshot);
		if (!data || !data.IsValid() || !header.SetObject("m_GC_GeneratedMetadata", data.GetResource().ToBaseContainer()))
		{
			error = "Could not prepare native mission metadata.";
			return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool Save(Resource resource, ResourceName name, GC_MissionMetadataExport exporter, ResourceName world, out string error)
	{
		if (!resource || !resource.IsValid())
			return false;
		BaseContainer header = resource.GetResource().ToBaseContainer();
		if (!Apply(header, exporter, world, error))
			return false;
		if (!BaseContainerTools.SaveContainer(header, name))
		{
			error = "Could not save the mission header: " + name;
			return false;
		}
		return true;
	}
}
#endif
