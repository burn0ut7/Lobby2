//! Authored server actions use existing Lobby2 owners. No client request or admin authorization bypass.
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	bool GC_ExecuteFrameworkAction(GC_MissionRuleEngine source, GC_RuleAction action)
	{
		if (!HasMissionRuleAuthority() || !m_bMissionEventsEnabled || !source || source != m_MissionRuleEngine
			|| !source.IsRunning() || !action || !action.IsValid())
			return false;
		switch (action.m_eDo)
		{
			case GC_ERuleAction.MISSION_MESSAGE:
				return GC_SendMissionMessage(action);
			case GC_ERuleAction.END_MISSION:
				return GC_EndFrameworkMission(action);
			case GC_ERuleAction.RESPAWN_TICKETS:
				return GC_ChangeFrameworkTickets(action);
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected bool GC_SendMissionMessage(GC_RuleAction action)
	{
		array<int> recipients = {};
		GC_EAdminMessageAudience audience = GC_EAdminMessageAudience.ALL_PLAYERS;
		if (!action.m_aFlagNames || action.m_aFlagNames.IsEmpty())
			GetGame().GetPlayerManager().GetPlayers(recipients);
		else
		{
			audience = GC_EAdminMessageAudience.FACTION;
			foreach (string faction : action.m_aFlagNames)
			{
				array<int> players;
				array<string> identities;
				if (!GC_GetFactionPlayers(faction, players, identities))
					return false;
				foreach (int playerId : players)
				{
					if (!recipients.Contains(playerId))
						recipients.Insert(playerId);
				}
			}
		}
		m_iAdminMessageSequence++;
		foreach (int recipient : recipients)
		{
			GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(recipient));
			if (controller)
				controller.DeliverAdminMessage(m_iAdminMessageSequence, 0, "Mission", audience, action.m_sTextOrName, true, action.m_bFlagAnswer, true);
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool GC_EndFrameworkMission(GC_RuleAction action)
	{
		if (GetLobbyPhase() != SCR_EGameModeState.GAME)
			return false;
		int factionIndex = -1;
		EGameOverTypes reason = EGameOverTypes.EDITOR_NEUTRAL;
		if (action.m_iOperation > 0)
		{
			FactionManager factions = GetGame().GetFactionManager();
			Faction faction = factions.GetFactionByKey(action.m_sTextOrName);
			if (!faction)
				return false;
			factionIndex = factions.GetFactionIndex(faction);
			switch (action.m_iOperation)
			{
				case 1:
					reason = EGameOverTypes.EDITOR_FACTION_VICTORY;
					break;
				case 2:
					reason = EGameOverTypes.EDITOR_FACTION_DEFEAT;
					break;
				case 3:
					reason = EGameOverTypes.EDITOR_FACTION_DRAW;
					break;
			}
		}
		return EnterPostGame(SCR_GameModeEndData.CreateSimple(reason, -1, factionIndex));
	}

	//------------------------------------------------------------------------------------------------
	protected bool GC_ChangeFrameworkTickets(GC_RuleAction action)
	{
		int tickets = GC_GetFactionRespawnTickets(action.m_sTextOrName);
		if (action.m_iOperation == 0)
			tickets = action.m_iAmount;
		else if (action.m_iOperation == 1)
		{
			if (tickets > int.MAX - action.m_iAmount)
				return false;
			tickets += action.m_iAmount;
		}
		else
			tickets = Math.Max(0, tickets - action.m_iAmount);
		return GC_SetFactionRespawnTickets(action.m_sTextOrName, tickets);
	}
}
