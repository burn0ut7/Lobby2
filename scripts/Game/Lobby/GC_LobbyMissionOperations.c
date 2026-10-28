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
		if (!Replication.IsServer() || GetManager() != this || m_bShutdown
			|| GetLobbyPhase() != SCR_EGameModeState.GAME || m_bGC_RespawnWaveActive)
			return 0;

		m_bGC_RespawnWaveActive = true;
		int respawned;
		foreach (GC_Slot slot : GetSlots())
		{
			int tickets = m_mGC_FactionRespawnTickets.Get(factionKey);
			if (tickets <= 0)
				break;
			if (!slot || !slot.IsKIA() || !slot.HasClaim())
				continue;
			int playerId = slot.GetPlayerId();
			SCR_AIGroup group = ResolveSlotGroup(slot);
			if (playerId <= 0 || !GetGame().GetPlayerManager().IsPlayerConnected(playerId)
				|| !group || !group.GetFaction() || group.GetFaction().GetFactionKey() != factionKey)
				continue;
			// Do not substitute the group's current location when the original anchor is unavailable.
			if (!slot.GetAdminSpawnPoint() && !FindGroupSpawnTransform(group))
			{
				PrintFormat("GC | Respawn tickets | slot=%1 rejected: original spawn unavailable", slot.GetID(), level: LogLevel.WARNING);
				continue;
			}

			m_mGC_FactionRespawnTickets.Set(factionKey, tickets - 1);
			string result = RespawnAdminSlot(slot, false);
			if (result == "applied")
			{
				respawned++;
				UpdateSlot(slot);
			}
			else
				m_mGC_FactionRespawnTickets.Set(factionKey, m_mGC_FactionRespawnTickets.Get(factionKey) + 1);
			PrintFormat("GC | Respawn tickets | faction=%1 slot=%2 result=%3 remaining=%4", factionKey, slot.GetID(), result, m_mGC_FactionRespawnTickets.Get(factionKey));
		}
		m_bGC_RespawnWaveActive = false;
		return respawned;
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
