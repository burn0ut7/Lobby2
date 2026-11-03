//! Explicit lifecycle probe. Uses production start/authorization handlers; no phase bypass.
modded class GC_SessionLogComponent
{
	//------------------------------------------------------------------------------------------------
	string GC_TestTelemetryPhase(string action)
	{
		if (!Replication.IsServer() || GetInstance() != this)
			return "unavailable";
		GC_MissionStatsComponent stats = GC_MissionStatsComponent.GetInstance();
		if (action == "start-handler")
		{
			OnGameModeStart();
			if (stats)
				stats.OnGameModeStart();
		}
		if (action == "reject")
		{
			string before = GetTelemetryReport();
			if (SetTelemetry(-1, false) || before != GetTelemetryReport())
				return "FAIL unauthorized mutation";
		}
		int joins;
		foreach (GC_SessionEvent entry : m_aRecent)
		{
			if (entry.m_sType == "joined")
				joins++;
		}
		bool collecting = stats && stats.IsCollecting();
		return string.Format("phase=%1 session=%2 sessionMs=%3 joins=%4 stats=%5 telemetry=[%6]",
			GC_LobbyGameMode.GetGameMode().GetState(), m_sSessionId, GetSessionMilliseconds(), joins, collecting, GetTelemetryReport());
	}
}
