#ifdef WORKBENCH
modded class GC_PlayerHistory
{
	//------------------------------------------------------------------------------------------------
	//! Explicit fixture changes only the in-memory history result, never database or stats.
	void GC_TestHistory(string identity, float priorSeconds, bool known, bool stale)
	{
		m_mSeconds.Set(identity, priorSeconds);
		m_mKnown.Set(identity, known);
		m_fReceivedAt = GetGame().GetWorld().GetWorldTime() * 0.001;
		if (stale)
			m_fReceivedAt -= 100;
		Tick();
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestPlayerHistory(string action)
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!Replication.IsServer() || !controller || !m_GC_PlayerHistory)
			return "UNAVAILABLE authority/controller/history";
		return GC_TestPlayerHistoryForPlayer(controller.GetPlayerId(), action);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestPlayerHistoryForPlayer(int playerId, string action)
	{
		if (!Replication.IsServer() || !m_GC_PlayerHistory)
			return "UNAVAILABLE authority/history";
		string identity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		if (action == "check")
			return string.Format("new=%1 admin=%2 identity=%3", IsNewPlayer(playerId), SCR_Global.IsAdmin(playerId), identity);
		if (action == "new")
			m_GC_PlayerHistory.GC_TestHistory(identity, 0, true, false);
		else if (action == "threshold")
			m_GC_PlayerHistory.GC_TestHistory(identity, 10800, true, false);
		else if (action == "below")
			m_GC_PlayerHistory.GC_TestHistory(identity, 10799, true, false);
		else if (action == "unknown")
			m_GC_PlayerHistory.GC_TestHistory(identity, 0, false, false);
		else if (action == "stale")
			m_GC_PlayerHistory.GC_TestHistory(identity, 0, true, true);
		else if (action == "identity")
			PublishNewPlayer(playerId, "wrong-identity", true);
		else
			return "UNSUPPORTED action";
		return GC_TestPlayerHistoryForPlayer(playerId, "check");
	}
}
#endif

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestPlayerHistoryRoster()
	{
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		string result;
		foreach (int playerId : players)
			result += string.Format("player=%1 new=%2 admin=%3; ", playerId, IsNewPlayer(playerId), SCR_Global.IsAdmin(playerId));
		return result;
	}
}
