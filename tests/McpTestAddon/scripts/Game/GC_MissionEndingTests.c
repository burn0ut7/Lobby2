#ifdef WORKBENCH
modded class GC_LobbyManager
{
	//! Explicit test-only configuration; production authority and action dispatch remain intact.
	string GC_TestEndingPrepare()
	{
		if (!HasMissionRuleAuthority() || GetLobbyPhase() != SCR_EGameModeState.PREGAME)
			return "FAIL requires authoritative preview";
		StopMissionRules();
		m_bMissionRulesStarted = false;
		m_bMissionEventsEnabled = true;
		m_aMissionFlags = {};
		m_aMissionEvents = {};
		StartMissionRules();
		GC_EndMissionAction authored = new GC_EndMissionAction();
		authored.m_iResult = 1;
		authored.m_sFaction = "US";
		GC_RuleAction action = authored.BuildAction();
		if (GC_ExecuteFrameworkAction(m_MissionRuleEngine, action))
			return "FAIL ended before match";
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!mode.GC_SetLobbyState(SCR_EGameModeState.SLOTTING) || !mode.GC_SetLobbyState(SCR_EGameModeState.BRIEFING) || !StartLobbyGame())
			return "FAIL start";
		if (GC_ExecuteFrameworkAction(new GC_MissionRuleEngine(), action))
			return "FAIL accepted wrong engine";
		authored.m_aWinningFactions = {"missing-faction"};
		if (GC_ExecuteFrameworkAction(m_MissionRuleEngine, authored.BuildAction()) || GetLobbyPhase() != SCR_EGameModeState.GAME)
			return "FAIL invalid faction mutated mission";
		authored.m_iResult = 4;
		if (authored.BuildAction().IsValid())
			return "FAIL failure allowed winners";
		authored.m_aWinningFactions = {};
		authored.m_sMessage = " \t\n ";
		if (!authored.BuildAction().m_sEndMessage.IsEmpty())
			return "FAIL blank normalization";
		return "PASS pregame, engine and faction rejection; failed-winner rejection; blank normalization; phase GAME";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestEndingRun(int result, string message)
	{
		GC_EndMissionAction authored = new GC_EndMissionAction();
		authored.m_iResult = result;
		authored.m_sFaction = "US";
		authored.m_sMessage = message;
		if (result == 1)
			authored.m_aWinningFactions = {"USSR", "US", "USSR"};
		GC_RuleAction action = authored.BuildAction();
		int sequence = m_iAdminMessageSequence;
		bool ended = GC_ExecuteFrameworkAction(m_MissionRuleEngine, action);
		int notifications = m_iAdminMessageSequence - sequence;
		bool repeated = GC_ExecuteFrameworkAction(m_MissionRuleEngine, action);
		return string.Format("ended=%1 phase=%2 notifications=%3 repeated=%4 message=%5", ended, GetLobbyPhase(), notifications, repeated, GetEndMessage());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestEndingReport()
	{
		GC_MissionStatsComponent stats = GC_MissionStatsComponent.GetInstance();
		if (!stats || !GetGame().GetPlayerController())
			return "FAIL unavailable";
		array<string> rows;
		int next;
		string status;
		stats.ReadReport(GetGame().GetPlayerController().GetPlayerId(), "ending", "", 0, 0, rows, next, status);
		string report = "status=" + status;
		foreach (string row : rows)
			report += " | " + row;
		return report;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestEndingControl(string action, int seconds, bool unauthorized)
	{
		int requester = GetGame().GetPlayerController().GetPlayerId();
		if (unauthorized)
			requester = -1;
		GC_BoundaryAdmin owner = GC_LobbyGameMode.GetGameMode().GetBoundaryAdmin();
		string key = owner.GC_TestEndingRevision().ToString();
		if (action == "stale")
		{
			key = "-1";
			action = "ending-force";
		}
		string result = owner.Execute(requester, action, key, "", seconds, {});
		return result + " | " + GC_TestEndingSnapshot();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestEndingSnapshot()
	{
		GC_MissionStatsComponent stats = GC_MissionStatsComponent.GetInstance();
		return string.Format("phase=%1 collecting=%2 finalized=%3 notifications=%4 %5", GetLobbyPhase(),
			stats.IsCollecting(), stats.IsFinalized(), m_iAdminMessageSequence,
			GC_LobbyGameMode.GetGameMode().GetBoundaryAdmin().GC_TestEndingState());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestEndingAdminOpen()
	{
		if (!SCR_Global.IsAdmin(GetGame().GetPlayerController().GetPlayerId()))
			return "rejected: admin login required";
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.GC_AdminToolsMenu);
		return "requested native admin menu";
	}
}

modded class GC_BoundaryAdmin
{
	int GC_TestEndingRevision()
	{
		return m_iEndingRevision;
	}

	string GC_TestEndingState()
	{
		int reason = -1;
		array<int> factions = {};
		if (m_PendingEndData)
		{
			reason = m_PendingEndData.GetEndReason();
			m_PendingEndData.GetFactionWinnerIds(factions);
		}
		return string.Format("pending=%1 remaining=%2 revision=%3 reason=%4 factions=%5", m_PendingEndData != null,
			Math.Max(0, m_fAnnouncementDeadline - GetGame().GetWorld().GetWorldTime() * 0.001), m_iEndingRevision, reason, factions.Count());
	}
}
#endif
