//! Portable shared-map snapshot. Session marker/player IDs are deliberately omitted.
class GC_MapPlanData
{
	ref array<ref array<int>> m_aMarkers = {};
	ref array<string> m_aTexts = {};
	ref array<ref array<int>> m_aDrawings = {};
	ref array<string> m_aFactions = {};
	string m_sWorld;
	int m_iLastActive;
	int m_iGeneration;

	//------------------------------------------------------------------------------------------------
	string Encode()
	{
		JsonSaveContext context = new JsonSaveContext();
		context.WriteValue("version", 1);
		context.WriteValue("world", m_sWorld);
		context.WriteValue("lastActive", m_iLastActive);
		context.WriteValue("generation", m_iGeneration);
		context.WriteValue("factions", m_aFactions);
		context.WriteValue("markers", m_aMarkers);
		context.WriteValue("texts", m_aTexts);
		context.WriteValue("drawings", m_aDrawings);
		return context.SaveToString();
	}

	//------------------------------------------------------------------------------------------------
	bool Decode(string input)
	{
		if (input.IsEmpty() || input.Length() > 131072)
			return false;
		JsonLoadContext context = new JsonLoadContext(false);
		int version;
		return context.LoadFromString(input) && context.ReadValue("version", version) && version == 1
			&& context.ReadValue("world", m_sWorld) && context.ReadValue("lastActive", m_iLastActive)
			&& context.ReadValue("generation", m_iGeneration)
			&& context.ReadValue("factions", m_aFactions) && context.ReadValue("markers", m_aMarkers)
			&& context.ReadValue("texts", m_aTexts) && context.ReadValue("drawings", m_aDrawings);
	}

	//------------------------------------------------------------------------------------------------
	static bool ReadFile(string path, out string data)
	{
		FileHandle file = FileIO.OpenFile(path, FileMode.READ);
		if (!file)
			return false;
		int size = file.GetLength();
		if (size <= 0 || size > 131072)
		{
			file.Close();
			return false;
		}
		int read = file.Read(data, size);
		file.Close();
		return read == size;
	}

	//------------------------------------------------------------------------------------------------
	static bool WriteFile(string path, string data)
	{
		if (data.IsEmpty() || data.Length() > 131072)
			return false;
		FileHandle file = FileIO.OpenFile(path, FileMode.WRITE);
		if (!file)
			return false;
		int written = file.Write(data);
		file.Close();
		string check;
		return written == data.Length() && ReadFile(path, check) && check == data;
	}
}
