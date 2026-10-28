#ifdef WORKBENCH
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestPlatformIdRefresh()
	{
		int playerId = GetGame().GetPlayerController().GetPlayerId();
		if (!Replication.IsServer() || !SCR_Global.IsAdmin(playerId))
			return "UNAVAILABLE authority/admin";
		string expected = GetGame().GetBackendApi().GetPlayerPlatformId(playerId);
		bool unavailable = expected.IsEmpty() || expected.Length() > 20;
		for (int digit; !unavailable && digit < expected.Length(); digit++)
			unavailable = !"0123456789".Contains(expected.Substring(digit, 1));
		if (unavailable)
			expected = string.Empty;
		foreach (GC_AdminSessionPlayer prior : m_aModerationHistory)
		{
			if (prior.m_iPlayerId == playerId)
				prior.m_sSteamId = "stale-test-value";
		}
		for (int i; i < 20; i++)
		{
			GC_RefreshModerationPlayer(playerId);
			int matches;
			foreach (GC_AdminSessionPlayer record : m_aModerationHistory)
			{
				if (record.m_iPlayerId != playerId)
					continue;
				matches++;
				if (record.m_sSteamId != expected)
					return string.Format("FAIL refresh=%1 expected=%2 actual=%3", i, expected, record.m_sSteamId);
			}
			if (matches != 1)
				return string.Format("FAIL refresh=%1 matchingRecords=%2", i, matches);
		}
		if (unavailable)
			return "PASS 20 refreshes: malformed backend value omitted; valid ID unavailable in this process";
		return "PASS 20 refreshes platformId=" + expected;
	}
}
#endif
