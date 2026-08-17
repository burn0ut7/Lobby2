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
		Rpc(RpcAsk_RequestVoiceChannel, channelType, factionKey, groupRplId);
	}

	//! Requests replicated lobby presentation state only; this does not alter VoN audio.
	void RequestVoiceDeafened(bool deafened)
	{
		Rpc(RpcAsk_RequestVoiceDeafened, deafened);
	}

	//! Requests one exact next Lobby2 phase from the owning client.
	void RequestLobbyPhaseAdvance(SCR_EGameModeState requestedPhase)
	{
		PrintFormat("GC | Phase RPC | request queued: player=%1 target=%2 value=%3", GetPlayerId(), GC_LobbyManager.GetPhaseName(requestedPhase), requestedPhase);
		Rpc(RpcAsk_RequestLobbyPhaseAdvance, requestedPhase);
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

		lobbyManager.TrySetPlayerVoiceChannel(GetPlayerId(), channelType, factionKey, groupRplId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestVoiceDeafened(bool deafened)
	{
		GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
		if (!lobbyManager)
			return;

		lobbyManager.TrySetPlayerVoiceDeafened(GetPlayerId(), deafened);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestLobbyPhaseAdvance(SCR_EGameModeState requestedPhase)
	{
		GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
		if (!lobbyManager)
			return;

		lobbyManager.TryAdvanceLobbyPhase(GetPlayerId(), requestedPhase);
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
