//! Parameter deadlines share the existing replicated phase clock. Respawns use
//! the mission-operation ticket owner and its authoritative slot spawn path.
modded class GC_LobbyManager
{
	protected bool m_bGC_ParameterWarningSent;
	protected bool m_bGC_ParameterLockSent;
	protected SCR_EGameModeState m_eGC_ParameterPhase = -1;
	protected ref map<string, int> m_mGC_LastParameterWave = new map<string, int>();

	//------------------------------------------------------------------------------------------------
	override void OnGameStateChanged(SCR_EGameModeState state)
	{
		super.OnGameStateChanged(state);
		if (Replication.IsServer())
			GC_OnParameterPhaseChanged(state);
	}

	//------------------------------------------------------------------------------------------------
	override bool TryApplyScenarioParameters(int playerId, int baseRevision, array<ref GC_LobbyParameterValue> values)
	{
		array<ref GC_LobbyParameterValue> previousValues = {};
		if (m_ParameterCatalog)
		{
			foreach (GC_LobbyParameterValue previous : m_ParameterCatalog.GetValues())
				previousValues.Insert(previous);
		}
		if (!super.TryApplyScenarioParameters(playerId, baseRevision, values))
			return false;
		GC_ApplyParameterBudgetChanges(previousValues);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override protected void Shutdown()
	{
		GetGame().GetCallqueue().Remove(GC_TickParameters);
		super.Shutdown();
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_OnParameterPhaseChanged(SCR_EGameModeState phase)
	{
		if (m_eGC_ParameterPhase == phase)
			return;
		m_eGC_ParameterPhase = phase;
		GetGame().GetCallqueue().Remove(GC_TickParameters);
		if (!EnsureParameterCatalog() || !m_ParameterCatalog.HasDefinitions())
			return;
		if (phase == SCR_EGameModeState.BRIEFING)
		{
			GC_AnnounceParameters(string.Format("COs: choose your side's parameters within %1 seconds. After that, only admins can make changes.", GC_GetParameterLockRemaining()));
			GetGame().GetCallqueue().CallLater(GC_TickParameters, 1000, true);
		}
		else if (phase == SCR_EGameModeState.GAME)
		{
			if (!m_bGC_ParameterLockSent)
				GC_AnnounceParameters("Parameters locked. Only admins can make changes.");
			m_bGC_ParameterLockSent = true;
			if (!EnsureParameterCatalog())
				return;
			foreach (GC_LobbyParameterValue value : m_ParameterCatalog.GetValues())
			{
				GC_LobbyParameterDefinition definition = m_ParameterCatalog.FindDefinition(value.GetDefinitionIndex());
				if (definition.GetEffect() == GC_ELobbyParameterEffect.RESPAWN_BUDGET)
					GC_SetFactionRespawnTickets(definition.GetOwnerFactionKey(), value.GetIntValue());
			}
			m_mGC_LastParameterWave.Clear();
			GetGame().GetCallqueue().CallLater(GC_TickParameters, 1000, true);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_TickParameters()
	{
		if (!Replication.IsServer() || GetManager() != this || m_bShutdown)
		{
			GetGame().GetCallqueue().Remove(GC_TickParameters);
			return;
		}
		if (GetLobbyPhase() == SCR_EGameModeState.BRIEFING)
		{
			int remaining = GC_GetParameterLockRemaining();
			if (remaining > 0 && remaining <= 30 && !m_bGC_ParameterWarningSent)
			{
				m_bGC_ParameterWarningSent = true;
				GC_AnnounceParameters(string.Format("Parameters lock in %1 seconds. Apply your changes now.", remaining));
			}
			if (remaining == 0 && !m_bGC_ParameterLockSent)
			{
				m_bGC_ParameterLockSent = true;
				GC_AnnounceParameters("Parameters locked. Only admins can make changes.");
				GetGame().GetCallqueue().Remove(GC_TickParameters);
			}
			return;
		}
		if (GetLobbyPhase() != SCR_EGameModeState.GAME || !m_ParameterCatalog)
			return;
		int elapsed = GetPhaseElapsedSeconds();
		foreach (GC_LobbyParameterValue value : m_ParameterCatalog.GetValues())
		{
			GC_LobbyParameterDefinition definition = m_ParameterCatalog.FindDefinition(value.GetDefinitionIndex());
			if (definition.GetEffect() != GC_ELobbyParameterEffect.RESPAWN_INTERVAL || value.GetIntValue() <= 0)
				continue;
			string factionKey = definition.GetOwnerFactionKey();
			int previous = m_mGC_LastParameterWave.Get(factionKey);
			if (elapsed - previous < value.GetIntValue() * 60)
				continue;
			if (GC_QueueFactionRespawns(factionKey))
				m_mGC_LastParameterWave.Set(factionKey, elapsed);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! During play, apply the configured budget delta to current tickets.
	//! Reductions clamp at zero; later increases grant their full delta.
	protected void GC_ApplyParameterBudgetChanges(array<ref GC_LobbyParameterValue> values)
	{
		if (GetLobbyPhase() != SCR_EGameModeState.GAME)
			return;
		foreach (GC_LobbyParameterValue value : values)
		{
			GC_LobbyParameterDefinition definition = m_ParameterCatalog.FindDefinition(value.GetDefinitionIndex());
			GC_LobbyParameterValue current = m_ParameterCatalog.FindValue(value.GetDefinitionIndex());
			if (definition.GetEffect() != GC_ELobbyParameterEffect.RESPAWN_BUDGET || !current)
				continue;
			int difference = current.GetIntValue() - value.GetIntValue();
			if (difference == 0)
				continue;
			string factionKey = definition.GetOwnerFactionKey();
			int remaining = GC_GetFactionRespawnTickets(factionKey);
			if (difference > 0)
				remaining = Math.Min(remaining, int.MAX - difference);
			GC_SetFactionRespawnTickets(factionKey, Math.Max(0, remaining + difference));
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_AnnounceParameters(string message)
	{
		m_iAdminMessageSequence++;
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		foreach (int playerId : players)
		{
			GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
			if (controller)
				controller.DeliverAdminMessage(m_iAdminMessageSequence, 0, "Parameters", GC_EAdminMessageAudience.ALL_PLAYERS, message, true);
		}
	}
}
