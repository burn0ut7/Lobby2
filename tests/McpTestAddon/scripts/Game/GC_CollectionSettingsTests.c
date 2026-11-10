#ifdef WORKBENCH
//! Explicit startup fixture: -1 leaves authored settings untouched; 0 = all off;
//! 1 = statistics only; 2 = all on. Change only for a deliberate Reload/Play test.
modded class GC_LobbyGameMode
{
	static const int GC_TEST_COLLECTION_PROFILE = -1;

	override void EOnInit(IEntity owner)
	{
		if (!SCR_Global.IsEditMode() && GC_TEST_COLLECTION_PROFILE >= 0)
		{
			m_bCollectionEnabled = GC_TEST_COLLECTION_PROFILE > 0;
			m_bPlaybackTelemetryEnabled = GC_TEST_COLLECTION_PROFILE == 2;
		}
		super.EOnInit(owner);
	}
}

modded class GC_SessionLogComponent
{
	//! Reads the actual component even when its public collection owner is disabled.
	string GC_TestCollectionSettings()
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!mode || !Replication.IsServer())
			return "UNAVAILABLE";
		GC_MissionStatsComponent stats = GC_MissionStatsComponent.Cast(mode.FindComponent(GC_MissionStatsComponent));
		return string.Format("collection=%1 playback=%2 session=%3 history=%4 stats=%5 recorder=%6 queued=%7 sequence=%8",
			mode.IsCollectionEnabled(), mode.IsPlaybackTelemetryEnabled(), m_sSessionId, m_History != null,
			stats && stats.IsCollecting(), m_Telemetry != null, m_aQueued.Count(), m_iSequence);
	}
}
#endif
