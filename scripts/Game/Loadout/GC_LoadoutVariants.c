#ifdef WORKBENCH
//! Detached edit buffer for the native editable-character randomizer.
class GC_LoadoutVariants
{
	bool m_bIncludeDefault = true;
	int m_iDefaultWeight = 1;
	ref array<ref SCR_EditableEntityVariant> m_aEntries = {};

	//------------------------------------------------------------------------------------------------
	static GC_LoadoutVariants Read(BaseContainer source)
	{
		array<ref ContainerIdPathEntry> path = {};
		BaseContainer editable = GC_LoadoutMetadata.FindComponent(source, SCR_EditableCharacterComponent, path);
		if (!editable)
			return null;
		GC_LoadoutVariants result = new GC_LoadoutVariants();
		BaseContainer data = editable.GetObject("m_VariantData");
		if (!data)
			return result;
		data.Get("m_bRandomizeDefaultVariant", result.m_bIncludeDefault);
		data.Get("m_iDefaultVariantRandomizerWeight", result.m_iDefaultWeight);
		BaseContainerList entries = data.GetObjectArray("m_aVariants");
		for (int i; entries && i < entries.Count(); i++)
		{
			SCR_EditableEntityVariant entry = new SCR_EditableEntityVariant();
			entries.Get(i).Get("m_sVariantPrefab", entry.m_sVariantPrefab);
			entries.Get(i).Get("m_iRandomizerWeight", entry.m_iRandomizerWeight);
			entries.Get(i).Get("m_bEnabled", entry.m_bEnabled);
			result.m_aEntries.Insert(entry);
		}
		return result;
	}

	//------------------------------------------------------------------------------------------------
	static GC_LoadoutVariants Load(ResourceName prefab)
	{
		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return null;
		BaseContainer source = resource.GetResource().ToEntitySource();
		if (!source)
			return null;
		return Read(source);
	}

	//------------------------------------------------------------------------------------------------
	bool Equals(GC_LoadoutVariants other)
	{
		if (!other || m_bIncludeDefault != other.m_bIncludeDefault || m_iDefaultWeight != other.m_iDefaultWeight || m_aEntries.Count() != other.m_aEntries.Count())
			return false;
		foreach (int i, SCR_EditableEntityVariant entry : m_aEntries)
		{
			SCR_EditableEntityVariant comparison = other.m_aEntries[i];
			if (entry.m_sVariantPrefab != comparison.m_sVariantPrefab || entry.m_iRandomizerWeight != comparison.m_iRandomizerWeight || entry.m_bEnabled != comparison.m_bEnabled)
				return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool IsValid(ResourceName owner, bool allowCurrentGear = false)
	{
		array<ResourceName> used = {};
		foreach (SCR_EditableEntityVariant entry : m_aEntries)
		{
			if (entry && entry.m_sVariantPrefab.IsEmpty() && allowCurrentGear)
				continue;
			if (!entry || entry.m_sVariantPrefab.IsEmpty() || entry.m_sVariantPrefab == owner || used.Contains(entry.m_sVariantPrefab) || entry.m_iRandomizerWeight < 1)
				return false;
			Resource resource = Resource.Load(entry.m_sVariantPrefab);
			if (!resource || !resource.IsValid())
				return false;
			IEntitySource source = resource.GetResource().ToEntitySource();
			if (!source || !source.GetClassName().ToType().IsInherited(ChimeraCharacter) || !Load(entry.m_sVariantPrefab))
				return false;
			used.Insert(entry.m_sVariantPrefab);
		}
		return m_iDefaultWeight > 0;
	}
}
#endif
