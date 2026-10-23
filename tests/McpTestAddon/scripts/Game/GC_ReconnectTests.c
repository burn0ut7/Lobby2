#ifdef WORKBENCH
//! Optional observations and arrangements. No automatic execution or permission grants.
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestReconnectTimeout(int seconds)
	{
		if (GetManager() != this || !Replication.IsServer() || seconds < 0)
			return "REJECTED authority/timeout";
		m_iSlotReservationTimeoutSeconds = seconds;
		return string.Format("timeout=%1", GetSlotReservationTimeoutSeconds());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestReconnectSnapshot(int slotId)
	{
		GC_Slot slot = GetSlot(slotId);
		if (!slot)
			return "UNAVAILABLE slot";
		IEntity entity = slot.GetEntity();
		PlayerManager players = GetGame().GetPlayerManager();
		bool possessed = entity && slot.GetPlayerId() > 0
			&& players.GetPlayerControlledEntity(slot.GetPlayerId()) == entity;
		string result = string.Format("slot=%1 state=%2 player=%3 claimed=%4 entity=%5 possessed=%6 timeout=%7",
			slotId, slot.GetState(), slot.GetPlayerId(), slot.HasClaim(), slot.GetEntityRplId(), possessed, GetSlotReservationTimeoutSeconds());
		if (entity)
		{
			SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(entity.FindComponent(SCR_CharacterDamageManagerComponent));
			result += string.Format(" position=%1", entity.GetOrigin());
			if (damage)
				result += string.Format(" health=%1", damage.GetHealthScaled());
		}
		SCR_ReconnectComponent reconnect = SCR_ReconnectComponent.GetInstance();
		if (reconnect)
			result += string.Format(" timer=%1", reconnect.m_mReconnectData.Contains(slot.GetUUID()));
		result += string.Format(" reservedSpectators=%1", m_mReservedSpectators.Count());
		return result;
	}
}

modded class GC_LobbyReconnectComponent
{
	//------------------------------------------------------------------------------------------------
	//! Exercise the production deadline comparison at equality and either side.
	string GC_TestReconnectDeadline()
	{
		SCR_ReconnectData data = new SCR_ReconnectData();
		WorldTimestamp now = GetGame().GetWorld().GetTimestamp();
		data.m_ValidUntil = now;
		bool equality = IsDataExpired(data, now);
		bool after = IsDataExpired(data, now.PlusSeconds(1));
		data.m_ValidUntil = now.PlusSeconds(1);
		bool before = IsDataExpired(data, now);
		return string.Format("equalExpired=%1 afterExpired=%2 beforeExpired=%3", equality, after, before);
	}
}
#endif
