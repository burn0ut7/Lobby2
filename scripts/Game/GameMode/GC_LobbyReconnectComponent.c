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
	//------------------------------------------------------------------------------------------------
	override protected void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (manager)
			m_iReconnectTime = manager.GetSlotReservationTimeoutSeconds();
	}

	//------------------------------------------------------------------------------------------------
	override bool HandlePlayerDisconnect(int playerId, KickCauseCode cause)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (!Replication.IsServer() || !manager || manager.GetOwner() != GetOwner())
			return false;

		const KickCauseGroup2 group = KickCauseCodeAPI.GetGroup(cause);
		if (group == RplKickCauseGroup.REPLICATION && KickCauseCodeAPI.GetReason(cause) == RplError.SHUTDOWN)
			return false;

		GC_LobbyReconnectData data = new GC_LobbyReconnectData();
		data.m_ReservedEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		GC_Slot slot = manager.GetSlotByPlayerId(playerId);
		// A pre-match claim or a lost character still owns a reconnect window.
		if (!slot && !IsDataRelevant(data))
			return false;

		data.m_PlayerUUID = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		if (data.m_PlayerUUID.IsEmpty())
			return false;

		m_iReconnectTime = manager.GetSlotReservationTimeoutSeconds();
		if (m_iReconnectTime > 0)
			data.m_ValidUntil = GetGame().GetWorld().GetTimestamp().PlusSeconds(m_iReconnectTime);
		else
		{
			// The native game mode calls us before the manager disconnect callback.
			// That callback releases a zero-timeout claim after marking it offline.
			return true;
		}

		m_mReconnectData.Set(data.m_PlayerUUID, data);
		UpdateExpieryCheck();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Lobby catch-up owns possession and phase presentation, including pre-match
	//! spectators. Do not run native ApplyData's reconnect dialog/second spawn path.
	override bool HandlePlayerReconnect(int playerId)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (!Replication.IsServer() || !manager || manager.GetOwner() != GetOwner())
			return false;

		UUID identity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		SCR_ReconnectData data;
		if (!m_mReconnectData.Take(identity, data))
			return false;

		// Check the actual deadline here as well as on the native one-second timer.
		// Otherwise a late reconnect could consume the record just before expiry.
		if (IsDataExpired(data, GetGame().GetWorld().GetTimestamp()))
			HandleDataExpiery(data);
		UpdateExpieryCheck();
		return GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId) != null;
	}

	//------------------------------------------------------------------------------------------------
	override protected bool IsDataExpired(notnull SCR_ReconnectData data, WorldTimestamp timestamp)
	{
		return data.m_ValidUntil && !timestamp.Less(data.m_ValidUntil);
	}

	//------------------------------------------------------------------------------------------------
	override void OnGameEnd()
	{
		GetGame().GetCallqueue().Remove(CheckExpiery);
		m_mReconnectData.Clear();
		if (s_Instance == this)
			s_Instance = null;
		super.OnGameEnd();
	}

	//------------------------------------------------------------------------------------------------
	override protected void HandleDataExpiery(notnull SCR_ReconnectData data)
	{
		GC_LobbyReconnectData lobbyData = GC_LobbyReconnectData.Cast(data);
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (manager && lobbyData)
			manager.OnLobbyReservationExpired(lobbyData.m_PlayerUUID, lobbyData.m_ReservedEntity);
	}
}
