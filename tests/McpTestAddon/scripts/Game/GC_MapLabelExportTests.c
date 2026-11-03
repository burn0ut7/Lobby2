#ifdef WORKBENCH
modded class GC_LobbyManager
{
	protected ref array<string> m_aGC_LabelRows;
	//------------------------------------------------------------------------------------------------
	string GC_TestMapLabelsExport()
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!Replication.IsServer() || GetManager() != this || !player || !AuthorizeAdminAction(player.GetPlayerId(), "map-label-export", m_sGC_ExportName))
			return "denied";
		if (!GC_MapExportReady())
			return "unsupported-world";
		m_aGC_LabelRows = {};
		vector mins, maxs;
		GetGame().GetWorld().GetBoundBox(mins, maxs);
		if (!GetGame().GetWorld().QueryEntitiesByAABB(mins, maxs, GC_TestMapLabelEntity))
			return "incomplete-query";
		FileHandle output = FileIO.OpenFile("$profile:GC_MapLabels_" + m_sGC_ExportName + ".tsv", FileMode.WRITE);
		if (!output)
			return "open-failed";
		output.WriteLine("type\tx\tz\tname");
		foreach (string row : m_aGC_LabelRows)
			output.WriteLine(row);
		output.Close();
		string result = "exported=" + m_aGC_LabelRows.Count();
		m_aGC_LabelRows = null;
		GC_AdminActionLogger.Log(player.GetPlayerId(), "map-label-export", m_sGC_ExportName, result);
		return result;
	}
	//------------------------------------------------------------------------------------------------
	protected bool GC_TestMapLabelEntity(IEntity entity)
	{
		MapDescriptorComponent descriptor = MapDescriptorComponent.Cast(entity.FindComponent(MapDescriptorComponent));
		if (!descriptor)
			return true;
		int type = descriptor.GetBaseType();
		if (type < EMapDescriptorType.MDT_NAME_GENERIC || type > EMapDescriptorType.MDT_NAME_VALLEY)
			return true;
		MapItem item = descriptor.Item();
		if (!item)
			return true;
		string name = WidgetManager.Translate(item.GetDisplayName());
		if (name.IsEmpty())
			return true;
		vector position = entity.GetOrigin();
		m_aGC_LabelRows.Insert(string.Format("%1\t%2\t%3\t%4", typename.EnumToString(EMapDescriptorType, type), position[0], position[2], name));
		return true;
	}
}
#endif
