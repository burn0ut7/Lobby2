enum GC_ELobbyPlayerState
{
	SPECTATOR,
	PLAYING
}

class GC_LobbyPlayerControllerClass : SCR_PlayerControllerClass
{
}

//! Lobby2-only owning controller. The Lobby2 game-mode prefab selects a
//! dedicated player-controller prefab using this class.
class GC_LobbyPlayerController : SCR_PlayerController
{
	protected GC_ELobbyPlayerState m_eLobbyPlayerState;

	void RequestLobbySlot(int slotId)
	{
		if (slotId < 0)
			return;

		Rpc(RpcAsk_RequestLobbySlot, slotId);
	}

	void RequestVoiceChannel(GC_VoiceChannelType channelType, string factionKey = "", RplId groupRplId = RplId.Invalid())
	{
		Rpc(RpcAsk_RequestVoiceChannel, channelType, factionKey, groupRplId);
	}

	void RequestVoiceDeafened(bool deafened)
	{
		Rpc(RpcAsk_RequestVoiceDeafened, deafened);
	}

	void RequestLobbyPhaseAdvance(GC_ELobbyPhase requestedPhase)
	{
		PrintFormat("GC | Phase RPC | request queued: player=%1 target=%2 value=%3", GetPlayerId(), GC_LobbyManager.GetPhaseName(requestedPhase), requestedPhase);
		Rpc(RpcAsk_RequestLobbyPhaseAdvance, requestedPhase);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestLobbySlot(int slotId)
	{
		GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
		if (lobbyManager)
			lobbyManager.TryAssignPlayerToSlot(GetPlayerId(), slotId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestVoiceChannel(GC_VoiceChannelType channelType, string factionKey, RplId groupRplId)
	{
		GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
		if (lobbyManager)
			lobbyManager.TrySetPlayerVoiceChannel(GetPlayerId(), channelType, factionKey, groupRplId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestVoiceDeafened(bool deafened)
	{
		GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
		if (lobbyManager)
			lobbyManager.TrySetPlayerVoiceDeafened(GetPlayerId(), deafened);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestLobbyPhaseAdvance(GC_ELobbyPhase requestedPhase)
	{
		GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
		if (lobbyManager)
			lobbyManager.TryAdvanceLobbyPhase(GetPlayerId(), requestedPhase);
	}

	void SetLobbySpectator(IEntity entity)
	{
		SetLobbyPlayerState(GC_ELobbyPlayerState.SPECTATOR);
		SetPossessedEntity(entity);
		Rpc(RpcDo_SetLobbySpectator);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_SetLobbySpectator()
	{
		SetLobbyPlayerState(GC_ELobbyPlayerState.SPECTATOR);

		RplComponent replication = RplComponent.Cast(FindComponent(RplComponent));
		if (replication && replication.IsOwner())
		{
			// Lobby spectator camera behavior is added by the future camera slice.
		}
	}

	protected void SetLobbyPlayerState(GC_ELobbyPlayerState state)
	{
		if (m_eLobbyPlayerState == state)
			return;

		m_eLobbyPlayerState = state;
	}
}
