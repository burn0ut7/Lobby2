//! The mission framework's only Lobby2 integration boundary. Native player and
//! faction ownership are queried afresh; the maps are one evaluation's observations.
class GC_LobbyMissionContext
{
	protected GC_LobbyManager m_Manager;
	protected ref map<string, int> m_mLivingPlayers = new map<string, int>();
	protected ref set<string> m_ObservedFactions = new set<string>();

	//------------------------------------------------------------------------------------------------
	void GC_LobbyMissionContext(GC_LobbyManager manager)
	{
		m_Manager = manager;
	}

	//------------------------------------------------------------------------------------------------
	bool IsOwned()
	{
		return Replication.IsServer() && m_Manager && GC_LobbyManager.GetManager() == m_Manager;
	}

	//------------------------------------------------------------------------------------------------
	bool IsRunning()
	{
		return IsOwned() && m_Manager.GetLobbyPhase() == SCR_EGameModeState.GAME;
	}

	//------------------------------------------------------------------------------------------------
	bool HasEnded()
	{
		return !IsOwned() || m_Manager.GetLobbyPhase() == SCR_EGameModeState.POSTGAME;
	}

	//------------------------------------------------------------------------------------------------
	bool IsKnownFaction(string factionKey)
	{
		return factionKey != string.Empty && GetGame().GetFactionManager().GetFactionByKey(factionKey) != null;
	}

	//------------------------------------------------------------------------------------------------
	//! Called once per evaluation, only during GAME. Unconscious living players
	//! count; AI, disconnected players, missing bodies and spectator bodies do not.
	void CapturePlayers()
	{
		m_mLivingPlayers.Clear();
		PlayerManager players = GetGame().GetPlayerManager();
		SCR_FactionManager factions = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		array<int> playerIds = {};
		players.GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(players.GetPlayerControlledEntity(playerId));
			if (!character || character.FindComponent(GC_LobbySpectatorBodyComponent) || !SCR_AIDamageHandling.IsAlive(character))
				continue;
			Faction faction = factions.GetPlayerFaction(playerId);
			if (!faction)
				continue;
			string factionKey = faction.GetFactionKey();
			m_mLivingPlayers.Set(factionKey, m_mLivingPlayers.Get(factionKey) + 1);
			m_ObservedFactions.Insert(factionKey);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! A never-populated faction is not a defeated faction. After one living
	//! player has been observed in GAME, zero is a valid count for this session.
	bool TryGetLivingPlayers(string factionKey, out int count)
	{
		count = m_mLivingPlayers.Get(factionKey);
		return m_ObservedFactions.Contains(factionKey);
	}

	//------------------------------------------------------------------------------------------------
	bool EndMission(string winningFactionKey)
	{
		if (!IsRunning() || !IsKnownFaction(winningFactionKey))
			return false;
		Faction winner = GetGame().GetFactionManager().GetFactionByKey(winningFactionKey);
		int winnerIndex = GetGame().GetFactionManager().GetFactionIndex(winner);
		ref SCR_GameModeEndData endData = SCR_GameModeEndData.CreateSimple(EGameOverTypes.EDITOR_FACTION_VICTORY, -1, winnerIndex);
		return m_Manager.EnterPostGame(endData);
	}
}
