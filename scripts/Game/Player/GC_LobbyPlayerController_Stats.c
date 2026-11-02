//! Finalization and mode teardown share the paged debrief transport.
modded class GC_LobbyPlayerController
{
	//------------------------------------------------------------------------------------------------
	void GC_SendDebrief()
	{
		GC_MissionStatsComponent stats = GC_MissionStatsComponent.GetInstance();
		if (!Replication.IsServer() || !stats)
			return;
		// Finalization joins the shared queue; no all-player/all-faction burst.
		if (m_sGC_ServerReportKey.IsEmpty())
			GC_SetServerReport("player", "", 0, 0);
		stats.QueueReport(this);
	}

	//------------------------------------------------------------------------------------------------
	void GC_ClearDebriefCache()
	{
		m_mGC_ReportPages.Clear();
		m_aGC_ReportPageOrder.Clear();
		m_aGC_ReportRequests.Clear();
		m_sGC_ActiveReportKey = string.Empty;
		m_sGC_ServerReportKey = string.Empty;
		m_GC_ServerReport = null;
	}

	//------------------------------------------------------------------------------------------------
	override void ShutdownLobbyPresentation()
	{
		GC_ClearDebriefCache();
		super.ShutdownLobbyPresentation();
	}
}
