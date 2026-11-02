#ifdef WORKBENCH
modded class GC_LobbyManager
{
	string GC_TestAIReport(string action)
	{
		GC_MissionStatsComponent stats = GC_MissionStatsComponent.GetInstance();
		if (!stats)
			return "UNAVAILABLE stats";
		return stats.GC_TestAIReport(action);
	}
}

modded class GC_MissionStatsComponent
{
	string GC_TestAIReport(string action)
	{
		if (action == "partial")
		{
			GC_MissionStatsAI original = m_AIStats;
			m_AIStats = new GC_MissionStatsAI();
			m_AIStats.MarkIncomplete();
			array<string> partialRows;
			int partialNext;
			string partialStatus;
			ReadReport(GetGame().GetPlayerController().GetPlayerId(), "faction", "USSR", 0, 0, partialRows, partialNext, partialStatus);
			m_AIStats = original;
			return string.Format("partialVisible=%1 rows=%2", partialStatus == "Partial report", partialRows.Count());
		}
		if (action == "player-exclusion")
		{
			string before = GC_TestAIReport("snapshot");
			m_AIStats.RecordShot(this, GetGame().GetPlayerController().GetControlledEntity(), null, null);
			return string.Format("playerExcluded=%1", before == GC_TestAIReport("snapshot"));
		}
		string result = m_AIStats.GC_TestSnapshot();
		result = string.Format("collecting=%1 final=%2 ", IsCollecting(), IsFinalized()) + result;
		int playerShots;
		foreach (GC_StatsPlayer player : m_Batch.m_aPlayers)
			playerShots += player.m_iShots;
		result += string.Format(" | playerShots=%1 persistentPlayers=%2", playerShots, m_Batch.m_aPlayers.Count());
		if (action == "snapshot" || action == "player-exclusion")
			return result;
		array<string> rows;
		int next;
		string status;
		ReadReport(GetGame().GetPlayerController().GetPlayerId(), "faction", action, 0, 0, rows, next, status);
		result += string.Format(" | reportRows=%1 status=%2", rows.Count(), status);
		if (rows.Count() == 2)
			result += " | " + rows[1];
		return result;
	}
}
modded class GC_MissionStatsAI
{
	string GC_TestSnapshot()
	{
		string result = string.Format("factions=%1 partial=%2", m_mGC_AIStats.Count(), m_bGC_AIPartial);
		foreach (string key, GC_StatsAssignment counters : m_mGC_AIStats)
		{
			int rounds;
			foreach (GC_StatsAmmo ammo : counters.m_aAmmo)
				rounds += ammo.m_iRounds;
			result += string.Format(" | %1 shots=%2 grenades=%3 playerKills=%4 aiKills=%5 deaths=%6 rounds=%7 teamKills=%8", key, counters.m_iShots, counters.m_iGrenades, counters.m_iKills, counters.m_iAIKills, counters.m_iDeaths, rounds, counters.m_iTeamKills);
		}
		return result;
	}
}
#endif
