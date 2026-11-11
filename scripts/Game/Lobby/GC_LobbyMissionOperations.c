//! Authority-only mission operations. No client request or admin permission bypass.
modded class GC_LobbyManager
{
	protected ref map<string, int> m_mGC_FactionRespawnTickets = new map<string, int>();
	protected bool m_bGC_RespawnWaveActive;
	protected bool m_bGC_RespawnAttemptActive;
	protected ref array<ref GC_RespawnWaveJob> m_aGC_RespawnWaves = {};
	protected ref map<string, ref GC_RespawnLocation> m_mGC_RespawnLocations = new map<string, ref GC_RespawnLocation>();

	//------------------------------------------------------------------------------------------------
	//! Current server mission policy; null preserves each caller's normal original/current fallback.
	GC_RespawnLocation GC_GetRespawnLocation(FactionKey factionKey)
	{
		if (!Replication.IsServer() || GetManager() != this || m_bShutdown)
			return null;
		return m_mGC_RespawnLocations.Get(factionKey);
	}

	//------------------------------------------------------------------------------------------------
	//! Trusted server mission API. Validate the entire scope before replacing any faction defaults.
	bool GC_SetRespawnLocation(array<string> factionKeys, GC_RespawnLocation location)
	{
		if (!Replication.IsServer() || GetManager() != this || m_bShutdown
			|| GetLobbyPhase() == SCR_EGameModeState.POSTGAME || m_bGC_RespawnAttemptActive
			|| !factionKeys || factionKeys.IsEmpty() || !GC_RuleAction.ValidTargetNames(factionKeys)
			|| (location && !location.IsValid()))
			return false;
		foreach (string key : factionKeys)
		{
			if (!GetGame().GetFactionManager().GetFactionByKey(key))
				return false;
		}
		foreach (string factionKey : factionKeys)
		{
			if (location)
				m_mGC_RespawnLocations.Set(factionKey, location);
			else
				m_mGC_RespawnLocations.Remove(factionKey);
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	int GC_GetFactionRespawnTickets(FactionKey factionKey)
	{
		if (!Replication.IsServer() || GetManager() != this || m_bShutdown)
			return 0;
		return m_mGC_FactionRespawnTickets.Get(factionKey);
	}

	//------------------------------------------------------------------------------------------------
	bool GC_SetFactionRespawnTickets(FactionKey factionKey, int tickets)
	{
		if (!Replication.IsServer() || GetManager() != this || m_bShutdown
			|| GetLobbyPhase() == SCR_EGameModeState.POSTGAME || tickets < 0
			|| !GetGame().GetFactionManager().GetFactionByKey(factionKey))
			return false;

		m_mGC_FactionRespawnTickets.Set(factionKey, tickets);
		PrintFormat("GC | Respawn tickets | faction=%1 tickets=%2", factionKey, tickets);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Acceptance only; completion happens after bounded spawn batches.
	bool GC_QueueFactionRespawns(FactionKey factionKey)
	{
		return GC_QueueRespawnWave({factionKey});
	}

	//------------------------------------------------------------------------------------------------
	//! Queue one wave through the existing respawn owner. True includes an empty
	//! accepted wave; it is not a count of successful spawns.
	bool GC_QueueRespawnWave(array<string> factionKeys, int maximumPlayers = 0, bool useTickets = true, bool randomOrder = false, GC_RespawnLocation location = null, GC_MissionRuleEngine completion = null)
	{
		if (!Replication.IsServer() || GetManager() != this || m_bShutdown
			|| GetLobbyPhase() != SCR_EGameModeState.GAME || m_aGC_RespawnWaves.Count() >= 256
			|| maximumPlayers < 0 || maximumPlayers > 1000000 || (location && !location.IsValid())
			|| (completion && !GC_LobbyManager.Cast(this).GC_HasFrameworkSource(completion)))
			return false;
		if (factionKeys)
		{
			if (factionKeys.Count() > 128)
				return false;
			foreach (string key : factionKeys)
			{
				if (key.IsEmpty() || !GetGame().GetFactionManager().GetFactionByKey(key))
					return false;
			}
		}
		GC_RespawnWaveJob wave = new GC_RespawnWaveJob();
		if (factionKeys)
			wave.m_aFactions.Copy(factionKeys);
		wave.m_iMaximum = maximumPlayers;
		wave.m_bUseTickets = useTickets;
		wave.m_Location = location;
		wave.m_Completion = completion;
		// Policy replacements after acceptance apply to future requests. Each
		// location still resolves moving anchors immediately before the spawn.
		foreach (string defaultFaction, GC_RespawnLocation defaultLocation : m_mGC_RespawnLocations)
			wave.m_mDefaultLocations.Set(defaultFaction, defaultLocation);
		foreach (GC_Slot candidate : GetSlots())
		{
			string candidateFaction;
			if (GC_CanJoinRespawnWave(candidate, factionKeys, candidateFaction))
				wave.m_aCandidates.Insert(new GC_RespawnWaveCandidate(candidate));
		}
		if (randomOrder)
		{
			for (int i = wave.m_aCandidates.Count() - 1; i > 0; i--)
			{
				int selected = Math.RandomInt(0, i + 1);
				GC_RespawnWaveCandidate swap = wave.m_aCandidates[i];
				wave.m_aCandidates[i] = wave.m_aCandidates[selected];
				wave.m_aCandidates[selected] = swap;
			}
		}
		m_aGC_RespawnWaves.Insert(wave);
		if (!m_bGC_RespawnWaveActive)
		{
			m_bGC_RespawnWaveActive = true;
			GetGame().GetCallqueue().CallLater(GC_ProcessRespawnWaves, 1);
		}
		return true;
	}

	bool GC_IsRespawnWavePending()
	{
		return m_bGC_RespawnWaveActive;
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_ProcessRespawnWaves()
	{
		if (!Replication.IsServer() || GetManager() != this || m_bShutdown || GetLobbyPhase() != SCR_EGameModeState.GAME)
		{
			GC_CancelRespawnWaves();
			return;
		}
		if (m_aGC_RespawnWaves.IsEmpty())
		{
			m_bGC_RespawnWaveActive = false;
			return;
		}
		GC_RespawnWaveJob wave = m_aGC_RespawnWaves[0];
		if (wave.m_Completion && !GC_LobbyManager.Cast(this).GC_HasFrameworkSource(wave.m_Completion))
			wave.m_iCursor = wave.m_aCandidates.Count();
		int started = System.GetTickCount();
		for (int attempted; attempted < 4 && wave.m_iCursor < wave.m_aCandidates.Count()
			&& (wave.m_iMaximum == 0 || wave.m_iRespawned < wave.m_iMaximum)
			&& (attempted == 0 || System.GetTickCount(started) < 2); attempted++)
		{
			if (GetManager() != this || m_bShutdown || GetLobbyPhase() != SCR_EGameModeState.GAME)
				break;
			GC_RespawnWaveCandidate request = wave.m_aCandidates[wave.m_iCursor++];
			GC_Slot slot = GetSlot(request.m_iSlot);
			string factionKey;
			if (!slot || slot.GetPlayerId() != request.m_iPlayer || slot.GetUUID() != request.m_sIdentity
				|| !GC_CanJoinRespawnWave(slot, wave.m_aFactions, factionKey) || wave.m_aAttemptedPlayers.Contains(slot.GetPlayerId()))
				continue;
			int tickets = m_mGC_FactionRespawnTickets.Get(factionKey);
			if (wave.m_bUseTickets && tickets <= 0)
				continue;
			GC_RespawnLocation destination = wave.m_Location;
			if (!destination)
				destination = wave.m_mDefaultLocations.Get(factionKey);
			if (!destination)
				destination = wave.m_OriginalLocation;
			int locationIndex;
			if (destination)
			{
				locationIndex = wave.m_mLocationIndices.Get(destination);
				wave.m_mLocationIndices.Set(destination, locationIndex + 1);
			}
			wave.m_aAttemptedPlayers.Insert(slot.GetPlayerId());
			if (wave.m_bUseTickets)
				m_mGC_FactionRespawnTickets.Set(factionKey, tickets - 1);
			m_bGC_RespawnAttemptActive = true;
			string result = RespawnAdminSlot(slot, false, destination, locationIndex);
			m_bGC_RespawnAttemptActive = false;
			if (result == "applied")
			{
				wave.m_iRespawned++;
				UpdateSlot(slot);
			}
			else if (wave.m_bUseTickets)
				m_mGC_FactionRespawnTickets.Set(factionKey, m_mGC_FactionRespawnTickets.Get(factionKey) + 1);
			PrintFormat("GC | Respawn wave | faction=%1 slot=%2 result=%3 useTickets=%4 remaining=%5", factionKey, slot.GetID(), result, wave.m_bUseTickets, m_mGC_FactionRespawnTickets.Get(factionKey));
		}
		if (m_aGC_RespawnWaves.IsEmpty() || m_aGC_RespawnWaves[0] != wave)
			return;
		if (wave.m_iCursor >= wave.m_aCandidates.Count() || (wave.m_iMaximum > 0 && wave.m_iRespawned >= wave.m_iMaximum))
		{
			m_aGC_RespawnWaves.RemoveOrdered(0);
			PrintFormat("GC | Respawn wave | completed spawned=%1", wave.m_iRespawned);
			// Keep the scheduler owned while completion may enqueue another wave.
			if (wave.m_Completion && GC_LobbyManager.Cast(this).GC_HasFrameworkSource(wave.m_Completion))
				wave.m_Completion.ResumeAfterRespawnWave();
		}
		m_bGC_RespawnWaveActive = !m_aGC_RespawnWaves.IsEmpty();
		if (m_bGC_RespawnWaveActive)
			GetGame().GetCallqueue().CallLater(GC_ProcessRespawnWaves, 1);
	}

	void GC_CancelRespawnWaves()
	{
		GetGame().GetCallqueue().Remove(GC_ProcessRespawnWaves);
		foreach (GC_RespawnWaveJob wave : m_aGC_RespawnWaves)
		{
			if (wave.m_Completion)
				wave.m_Completion.CancelRespawnWaveWait();
		}
		m_aGC_RespawnWaves.Clear();
		m_bGC_RespawnWaveActive = false;
	}

	//------------------------------------------------------------------------------------------------
	protected bool GC_CanJoinRespawnWave(GC_Slot slot, array<string> factionKeys, out string factionKey)
	{
		if (!slot || !slot.IsKIA() || !slot.HasClaim())
			return false;
		int playerId = slot.GetPlayerId();
		if (playerId <= 0 || !GetGame().GetPlayerManager().IsPlayerConnected(playerId)
			|| GetSlotByPlayerId(playerId) != slot)
			return false;
		SCR_AIGroup group = ResolveSlotGroup(slot);
		if (!group || !group.GetFaction())
			return false;
		factionKey = group.GetFaction().GetFactionKey();
		if (factionKeys && !factionKeys.IsEmpty() && !factionKeys.Contains(factionKey))
			return false;
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!controller)
			return false;
		IEntity controlled = controller.GetControlledEntity();
		if (controlled && !controlled.FindComponent(GC_LobbySpectatorBodyComponent))
		{
			ChimeraCharacter character = ChimeraCharacter.Cast(controlled);
			if (!character || !character.GetCharacterController() || !character.GetCharacterController().IsDead())
				return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Preserve claimed/reserved slots and use the existing replicated removal path.
	void GC_RemoveOpenMissionSlots(notnull array<string> factionKeys)
	{
		if (!Replication.IsServer() || GetManager() != this || m_bShutdown
			|| GetLobbyPhase() == SCR_EGameModeState.POSTGAME)
			return;

		for (int index = GetSlots().Count() - 1; index >= 0; index--)
		{
			GC_Slot slot = GetSlots()[index];
			SCR_AIGroup group = ResolveSlotGroup(slot);
			if (slot.HasClaim() || !group || !group.GetFaction()
				|| (!factionKeys.IsEmpty() && !factionKeys.Contains(group.GetFaction().GetFactionKey())))
				continue;

			IEntity character = slot.GetEntity();
			RemoveRegisteredSlot(slot);
			Rpc(RpcDo_RemoveAdminSlot, slot.GetID());
			if (character)
				SCR_EntityHelper.DeleteEntityAndChildren(character);
		}
	}
}
