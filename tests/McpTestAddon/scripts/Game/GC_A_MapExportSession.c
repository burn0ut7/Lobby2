#ifdef WORKBENCH
//! Explicit cartography session; never starts automatically in gameplay.
modded class GC_LobbyManager
{
	protected string m_sGC_ExportName;
	protected string m_sGC_ExportWorld;
	protected vector m_vGC_ExportSize;
	protected vector m_vGC_ExportPhoto;

	//------------------------------------------------------------------------------------------------
	string GC_TestMapExportConfigure(string name)
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!Replication.IsServer() || GetManager() != this || !player || !AuthorizeAdminAction(player.GetPlayerId(), "map-export-configure", name))
			return "denied";
		if (!m_sGC_ExportName.IsEmpty())
			return "already-configured-restart-play";
		if (name.Length() < 1 || name.Length() > 64)
			return "invalid-name";
		string allowed = "abcdefghijklmnopqrstuvwxyz0123456789-";
		for (int i; i < name.Length(); i++)
		{
			if (!allowed.Contains(name.Substring(i, 1)))
				return "invalid-name";
		}
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity)
			return "missing-map";
		vector size = mapEntity.Size();
		vector offset = mapEntity.Offset();
		// Current native TOPO reader is calibrated to zero-origin square maps.
		if (offset[0] != 0 || offset[2] != 0 || size[0] != size[2] || size[0] <= 0 || size[0] > 32768 || Math.Mod(size[0], 8) != 0)
			return "unsupported-map-calibration";
		m_sGC_ExportName = name;
		m_sGC_ExportWorld = GetGame().GetWorldFile();
		m_vGC_ExportSize = size;
		m_vGC_ExportPhoto = Vector(size[0] * 0.5, size[1] + offset[1] + 512, size[2] * 0.5);
		JsonSaveContext context = new JsonSaveContext();
		context.WriteValue("version", 1);
		context.WriteValue("name", name);
		context.WriteValue("world", m_sGC_ExportWorld);
		context.WriteValue("size", size);
		context.WriteValue("offset", offset);
		bool saved = context.SaveToFile("$profile:GC_MapExport_" + name + ".json");
		GC_AdminActionLogger.Log(player.GetPlayerId(), "map-export-configure", name, saved.ToString());
		return "configured=" + saved;
	}

	//------------------------------------------------------------------------------------------------
	protected bool GC_MapExportReady()
	{
		return GetManager() == this && Replication.IsServer() && !m_sGC_ExportName.IsEmpty() && GetGame().GetWorldFile() == m_sGC_ExportWorld;
	}
}
#endif

