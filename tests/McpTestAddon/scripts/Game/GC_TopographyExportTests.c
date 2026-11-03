#ifdef WORKBENCH
//! Explicit offline map-build probe. Never runs during normal mission telemetry.
modded class GC_LobbyManager
{
	protected ref array<float> m_aGC_MapHeights;
	protected int m_iGC_MapRow;
	protected string m_sGC_MapExportStatus;

	//------------------------------------------------------------------------------------------------
	string GC_TestTopographyExport()
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!Replication.IsServer() || GetManager() != this || !player || !AuthorizeAdminAction(player.GetPlayerId(), "topography-export", m_sGC_ExportName))
			return "denied";
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!GC_MapExportReady())
			return "unsupported-world";
		if (!mapEntity)
			return "unsupported-map";
		if (m_aGC_MapHeights)
			return m_sGC_MapExportStatus;
		m_aGC_MapHeights = {};
		m_iGC_MapRow = 0;
		m_sGC_MapExportStatus = "sampling";
		GC_AdminActionLogger.Log(player.GetPlayerId(), "topography-export", m_sGC_ExportName + " 8m terrain grid", "started");
		GetGame().GetCallqueue().CallLater(GC_TestTopographyRow, 1);
		return m_sGC_MapExportStatus;
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_TestTopographyRow()
	{
		if (GetManager() != this || !Replication.IsServer())
			return;
		for (int x; x <= m_vGC_ExportSize[0] / 8; x++)
			m_aGC_MapHeights.Insert(GetGame().GetWorld().GetSurfaceY(x * 8, m_iGC_MapRow * 8));
		m_iGC_MapRow++;
		if (m_iGC_MapRow <= m_vGC_ExportSize[2] / 8)
		{
			GetGame().GetCallqueue().CallLater(GC_TestTopographyRow, 1);
			return;
		}
		JsonSaveContext context = new JsonSaveContext();
		context.SetMaxDecimalPlaces(3);
		context.WriteValue("spacing", 8);
		context.WriteValue("columns", (int)(m_vGC_ExportSize[0] / 8) + 1);
		context.WriteValue("rows", (int)(m_vGC_ExportSize[2] / 8) + 1);
		context.WriteValue("heights", m_aGC_MapHeights);
		bool saved = context.SaveToFile("$profile:GC_MapDetail_" + m_sGC_ExportName + ".json");
		m_sGC_MapExportStatus = "saved=" + saved + " samples=" + m_aGC_MapHeights.Count();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestTopographyStatus()
	{
		return m_sGC_MapExportStatus + " rows=" + m_iGC_MapRow;
	}
}
#endif
