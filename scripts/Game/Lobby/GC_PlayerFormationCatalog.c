//! Lobby-owned, resource-independent formation data.
//!
//! The source AI world is read once and discarded. Runtime and Workbench
//! spawning use only the copied names and vectors held here.
class GC_PlayerFormationData
{
	protected string m_sName;
	protected ref array<vector> m_aOffsets = {};

	void GC_PlayerFormationData(string name, array<vector> offsets)
	{
		m_sName = name;
		if (!offsets)
			return;

		foreach (vector offset : offsets)
			m_aOffsets.Insert(offset);
	}

	string GetName()
	{
		return m_sName;
	}

	bool TryGetOffset(int index, out vector offset)
	{
		if (index < 0 || index >= m_aOffsets.Count())
			return false;

		offset = m_aOffsets[index];
		return true;
	}

	int GetOffsetCount()
	{
		return m_aOffsets.Count();
	}
}

class GC_PlayerFormationCatalog
{
	static ResourceName GetDefaultSource()
	{
		return "{E0A05C76552E7F58}Prefabs/AI/SCR_AIWorld.et";
	}

	static string GetDefaultFormationName()
	{
		return "File";
	}

	protected ref map<string, ref GC_PlayerFormationData> m_mFormations = new map<string, ref GC_PlayerFormationData>();
	protected bool m_bLoaded;

	bool Load(ResourceName source)
	{
		if (m_bLoaded)
			return !m_mFormations.IsEmpty();

		m_bLoaded = true;
		m_mFormations.Clear();

		if (source == string.Empty)
		{
			Print("GC | Formation | source resource is empty", LogLevel.ERROR);
			return false;
		}

		Resource resource = Resource.Load(source);
		if (!resource || !resource.IsValid())
		{
			PrintFormat("GC | Formation | source resource failed to load: %1", source, level: LogLevel.ERROR);
			return false;
		}

		BaseResourceObject resourceObject = resource.GetResource();
		BaseContainer root = resourceObject.ToBaseContainer();
		if (!root)
		{
			PrintFormat("GC | Formation | source has no base container: %1", source, level: LogLevel.ERROR);
			return false;
		}

		BaseContainerList formationList = root.GetObjectArray("Formations");
		if (!formationList)
		{
			PrintFormat("GC | Formation | source has no Formations array: %1", source, level: LogLevel.WARNING);
			return false;
		}

		for (int formationIndex = 0; formationIndex < formationList.Count(); formationIndex++)
		{
			BaseContainer formationContainer = formationList.Get(formationIndex);
			if (!formationContainer)
				continue;

			string name;
			if (!formationContainer.Get("Name", name) || name == string.Empty)
			{
				PrintFormat("GC | Formation | ignored unnamed formation at index %1", formationIndex, level: LogLevel.WARNING);
				continue;
			}

			ref array<vector> offsets = {};
			if (!formationContainer.Get("Offsets", offsets) || !offsets || offsets.IsEmpty())
			{
				PrintFormat("GC | Formation | ignored formation with no offsets: %1", name, level: LogLevel.WARNING);
				continue;
			}

			if (m_mFormations.Contains(name))
			{
				PrintFormat("GC | Formation | ignored duplicate formation: %1", name, level: LogLevel.WARNING);
				continue;
			}

			m_mFormations.Set(name, new GC_PlayerFormationData(name, offsets));
		}

		PrintFormat("GC | Formation | loaded %1 formations from %2", m_mFormations.Count(), source);
		return !m_mFormations.IsEmpty();
	}

	bool TryGetOffset(string name, int index, out vector offset)
	{
		GC_PlayerFormationData formation = m_mFormations.Get(name);
		if (!formation)
			return false;

		return formation.TryGetOffset(index, offset);
	}

	bool HasFormation(string name)
	{
		return m_mFormations.Contains(name);
	}

	int GetFormationCount()
	{
		return m_mFormations.Count();
	}

	void Clear()
	{
		m_mFormations.Clear();
		m_bLoaded = false;
	}
}
