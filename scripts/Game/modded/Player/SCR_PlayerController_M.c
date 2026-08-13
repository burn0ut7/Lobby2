enum PlayerState
{
    SPECTATOR,
    PLAYING
}

modded class SCR_PlayerController : PlayerController
{
	protected PlayerState m_ePlayerState;

	//! Sends a slot ID from the owning client to the server authority.
	void RequestLobbySlot(int slotId)
	{
		if (slotId < 0)
			return;

		Rpc(RpcAsk_RequestLobbySlot, slotId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestLobbySlot(int slotId)
	{
		GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
		if (!lobbyManager)
			return;

		lobbyManager.TryAssignPlayerToSlot(GetPlayerId(), slotId);
	}

	void SetSpectator(IEntity entity)
	{
		SetPlayerState(PlayerState.SPECTATOR);
		SetPossessedEntity(entity);
		Rpc(RplDo_SpectatorBrodcast);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RplDo_SpectatorBrodcast()
	{
		SetPlayerState(PlayerState.SPECTATOR);

		RplComponent rpl = RplComponent.Cast(FindComponent(RplComponent));
		if (rpl && rpl.IsOwner())
		{
			//Do camera shit
		}
	}

	void SetPlayerState(PlayerState state)
	{
		if(m_ePlayerState == state)
			return;
		
		m_ePlayerState = state;

		//OnPlayerStateChanged
	}
}
