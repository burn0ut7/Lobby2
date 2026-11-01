#ifdef WORKBENCH
//! Editable-character presentation and native affiliation saved with a loadout.
class GC_LoadoutMetadata
{
	string m_sName;
	FactionKey m_sFaction;
	ResourceName m_sIcon;
	string m_sIconName;

	//------------------------------------------------------------------------------------------------
	bool IsValid()
	{
		FactionManager factions = GetGame().GetFactionManager();
		return !m_sName.IsEmpty() && factions && factions.GetFactionByKey(m_sFaction);
	}

	//------------------------------------------------------------------------------------------------
	static BaseContainer FindComponent(BaseContainer source, typename componentType, array<ref ContainerIdPathEntry> path)
	{
		BaseContainerList components = source.GetObjectArray("components");
		for (int i; components && i < components.Count(); i++)
		{
			BaseContainer component = components.Get(i);
			typename type = component.GetClassName().ToType();
			if (type && type.IsInherited(componentType))
			{
				path.Insert(new ContainerIdPathEntry("components", i));
				return component;
			}
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	static GC_LoadoutMetadata Read(ResourceName prefab)
	{
		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return null;
		BaseContainer source = resource.GetResource().ToEntitySource();
		if (!source)
			return null;
		array<ref ContainerIdPathEntry> path = {};
		BaseContainer editable = FindComponent(source, SCR_EditableCharacterComponent, path);
		if (!editable)
			return null;
		BaseContainer info = editable.GetObject("m_UIInfo");
		if (!info)
			return null;
		GC_LoadoutMetadata result = new GC_LoadoutMetadata();
		info.Get("Name", result.m_sName);
		info.Get("Icon", result.m_sIcon);
		info.Get("IconSetName", result.m_sIconName);
		path.Clear();
		BaseContainer affiliation = FindComponent(source, FactionAffiliationComponent, path);
		if (affiliation)
			affiliation.Get("faction affiliation", result.m_sFaction);
		return result;
	}
}
#endif
