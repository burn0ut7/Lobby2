//! Authority-only mission operations. No client request or admin permission bypass.
modded class GC_LobbyManager
{
	protected ref map<string, int> m_mGC_FactionRespawnTickets = new map<string, int>();
	protected bool m_bGC_RespawnWaveActive;

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
	//! One ticket per successful connected KIA slot; the saved starting point is authoritative.
	int GC_RespawnFactionSlots(FactionKey factionKey)
	{
		return Math.Max(0, GC_RespawnWave({factionKey}));
	}

	//------------------------------------------------------------------------------------------------
	//! One synchronous wave through the existing slot respawn owner. Negative means rejected; zero is a valid empty wave.
	int GC_RespawnWave(array<string> factionKeys, int maximumPlayers = 0, bool useTickets = true, bool randomOrder = false)
	{
		if (!Replication.IsServer() || GetManager() != this || m_bShutdown
			|| GetLobbyPhase() != SCR_EGameModeState.GAME || m_bGC_RespawnWaveActive
			|| maximumPlayers < 0 || maximumPlayers > 1000000)
			return -1;
		// Validate the whole requested scope before spending tickets or spawning anyone.
		if (factionKeys)
		{
			if (factionKeys.Count() > 128)
				return -1;
			foreach (string key : factionKeys)
			{
				if (key.IsEmpty() || !GetGame().GetFactionManager().GetFactionByKey(key))
					return -1;
			}
		}

		m_bGC_RespawnWaveActive = true;
		array<int> candidates = {};
		foreach (GC_Slot candidate : GetSlots())
		{
			string candidateFaction;
			if (GC_CanJoinRespawnWave(candidate, factionKeys, candidateFaction))
				candidates.Insert(candidate.GetID());
		}
		if (randomOrder)
		{
			for (int i = candidates.Count() - 1; i > 0; i--)
			{
				int selected = Math.RandomInt(0, i + 1);
				int swap = candidates[i];
				candidates[i] = candidates[selected];
				candidates[selected] = swap;
			}
		}
		int respawned;
		array<int> attemptedPlayers = {};
		foreach (int slotId : candidates)
		{
			if (GetManager() != this || m_bShutdown || GetLobbyPhase() != SCR_EGameModeState.GAME
				|| (maximumPlayers > 0 && respawned >= maximumPlayers))
				break;
			GC_Slot slot = GetSlot(slotId);
			string factionKey;
			if (!GC_CanJoinRespawnWave(slot, factionKeys, factionKey) || attemptedPlayers.Contains(slot.GetPlayerId()))
				continue;
			int tickets = m_mGC_FactionRespawnTickets.Get(factionKey);
			if (useTickets && tickets <= 0)
				continue;
			SCR_AIGroup group = ResolveSlotGroup(slot);
			// Do not substitute the group's current location when the original anchor is unavailable.
			if (!slot.GetAdminSpawnPoint() && !FindGroupSpawnTransform(group))
			{
				PrintFormat("GC | Respawn tickets | slot=%1 rejected: original spawn unavailable", slot.GetID(), level: LogLevel.WARNING);
				continue;
			}

			attemptedPlayers.Insert(slot.GetPlayerId());
			if (useTickets)
				m_mGC_FactionRespawnTickets.Set(factionKey, tickets - 1);
			string result = RespawnAdminSlot(slot, false);
			if (result == "applied")
			{
				respawned++;
				UpdateSlot(slot);
			}
			else if (useTickets)
				m_mGC_FactionRespawnTickets.Set(factionKey, m_mGC_FactionRespawnTickets.Get(factionKey) + 1);
			PrintFormat("GC | Respawn wave | faction=%1 slot=%2 result=%3 useTickets=%4 remaining=%5", factionKey, slot.GetID(), result, useTickets, m_mGC_FactionRespawnTickets.Get(factionKey));
		}
		m_bGC_RespawnWaveActive = false;
		return respawned;
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
