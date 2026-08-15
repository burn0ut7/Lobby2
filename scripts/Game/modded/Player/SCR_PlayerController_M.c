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

	//! Requests one exclusive active voice channel from the server authority.
	void RequestVoiceChannel(GC_VoiceChannelType channelType, string factionKey = "", RplId groupRplId = RplId.Invalid())
	{
		PrintFormat("GC | Voice channel request sent: player=%1 type=%2 faction=%3 group=%4", GetPlayerId(), channelType, factionKey, groupRplId, level: LogLevel.WARNING);
		Rpc(RpcAsk_RequestVoiceChannel, channelType, factionKey, groupRplId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestLobbySlot(int slotId)
	{
		GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
		if (!lobbyManager)
			return;

		lobbyManager.TryAssignPlayerToSlot(GetPlayerId(), slotId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestVoiceChannel(GC_VoiceChannelType channelType, string factionKey, RplId groupRplId)
	{
		GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
		if (!lobbyManager)
			return;

		bool changed = lobbyManager.TrySetPlayerVoiceChannel(GetPlayerId(), channelType, factionKey, groupRplId);
		PrintFormat("GC | Voice channel request server result: player=%1 type=%2 changed=%3", GetPlayerId(), channelType, changed, level: LogLevel.WARNING);
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
