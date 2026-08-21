class GC_LobbyReconnectData : SCR_ReconnectData
{
	UUID m_PlayerUUID;
}

class GC_LobbyReconnectComponentClass : SCR_ReconnectComponentClass
{
}

//! Lobby2 reconnect policy. It keeps slot entities alive for the configured
//! lobby grace period and lets the manager decide what to release at expiry.
class GC_LobbyReconnectComponent : SCR_ReconnectComponent
{
	override protected void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (manager)
			m_iReconnectTime = manager.GetSlotReservationTimeoutSeconds();
	}

	override bool HandlePlayerDisconnect(int playerId, KickCauseCode cause)
	{
		const KickCauseGroup2 group = KickCauseCodeAPI.GetGroup(cause);
		if (group == RplKickCauseGroup.REPLICATION && KickCauseCodeAPI.GetReason(cause) == RplError.SHUTDOWN)
			return false;

		GC_LobbyReconnectData data = new GC_LobbyReconnectData();
		data.m_ReservedEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		GC_Slot slot;
		if (manager)
			slot = manager.GetSlotByPlayerId(playerId);
		if (!IsDataRelevant(data) && !(slot && slot.IsKIA()))
			return false;

		data.m_PlayerUUID = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		if (data.m_PlayerUUID.IsEmpty())
			return false;

		if (m_iReconnectTime > 0)
			data.m_ValidUntil = GetGame().GetWorld().GetTimestamp().PlusSeconds(m_iReconnectTime);
		else
		{
			if (manager)
				manager.OnLobbyReservationExpired(data.m_PlayerUUID, data.m_ReservedEntity);
			return true;
		}

		m_mReconnectData.Set(data.m_PlayerUUID, data);
		UpdateExpieryCheck();
		return true;
	}

	override protected void HandleDataExpiery(notnull SCR_ReconnectData data)
	{
		GC_LobbyReconnectData lobbyData = GC_LobbyReconnectData.Cast(data);
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (manager && lobbyData)
			manager.OnLobbyReservationExpired(lobbyData.m_PlayerUUID, lobbyData.m_ReservedEntity);
	}
}
