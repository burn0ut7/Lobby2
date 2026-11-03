#ifdef WORKBENCH
modded class GC_LobbyManager
{
	protected ref array<string> m_aGC_SymbolRows;
	//------------------------------------------------------------------------------------------------
	string GC_TestMapSymbolsExport()
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!Replication.IsServer() || GetManager() != this || !player || !AuthorizeAdminAction(player.GetPlayerId(), "map-symbol-export", m_sGC_ExportName))
			return "denied";
		if (!GC_MapExportReady())
			return "unsupported-world";
		m_aGC_SymbolRows = {};
		vector mins, maxs;
		GetGame().GetWorld().GetBoundBox(mins, maxs);
		if (!GetGame().GetWorld().QueryEntitiesByAABB(mins, maxs, GC_TestMapSymbolEntity))
			return "incomplete-query";
		FileHandle output = FileIO.OpenFile("$profile:GC_MapSymbols_" + m_sGC_ExportName + ".tsv", FileMode.WRITE);
		if (!output)
			return "open-failed";
		output.WriteLine("type\tx\tz\tyaw\tresource");
		foreach (string row : m_aGC_SymbolRows)
			output.WriteLine(row);
		output.Close();
		string result = "exported=" + m_aGC_SymbolRows.Count();
		m_aGC_SymbolRows = null;
		GC_AdminActionLogger.Log(player.GetPlayerId(), "map-symbol-export", m_sGC_ExportName, result);
		return result;
	}
	//------------------------------------------------------------------------------------------------
	protected bool GC_TestMapSymbolEntity(IEntity entity)
	{
		MapDescriptorComponent descriptor = MapDescriptorComponent.Cast(entity.FindComponent(MapDescriptorComponent));
		if (!descriptor)
			return true;
		int type = descriptor.GetBaseType();
		if (type < EMapDescriptorType.MDT_CALVARY || (type >= EMapDescriptorType.MDT_NAME_GENERIC && type != EMapDescriptorType.MDT_PORT && type != EMapDescriptorType.MDT_AIRPORT && type != EMapDescriptorType.MDT_LANDMARK && type != EMapDescriptorType.MDT_CAVE))
			return true;
		vector position = entity.GetOrigin();
		vector angles = entity.GetAngles();
		string resource;
		if (entity.GetPrefabData())
			resource = entity.GetPrefabData().GetPrefabName();
		m_aGC_SymbolRows.Insert(string.Format("%1\t%2\t%3\t%4\t%5", typename.EnumToString(EMapDescriptorType, type), position[0], position[2], angles[1], resource));
		return true;
	}
}
#endif
