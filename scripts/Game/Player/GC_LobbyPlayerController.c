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

	void RequestVoiceChannel(GC_VoiceChannelType channelType, string factionKey = "", RplId groupRplId = RplId.Invalid(), UUID personalOwnerIdentity = "")
	{
		Rpc(RpcAsk_RequestVoiceChannel, channelType, factionKey, groupRplId, personalOwnerIdentity);
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
	protected void RpcAsk_RequestVoiceChannel(GC_VoiceChannelType channelType, string factionKey, RplId groupRplId, string personalOwnerIdentity)
	{
		GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
		if (lobbyManager)
			lobbyManager.TrySetPlayerVoiceChannel(GetPlayerId(), channelType, factionKey, groupRplId, personalOwnerIdentity);
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
		SetLobbyControlledEntity(entity, true);
	}

	//! Assigns a Lobby2-owned main entity through the base controller lifecycle.
	//! This is authority-only and intentionally avoids temporary possession when
	//! the player has no pre-existing main entity.
	bool SetLobbyControlledEntity(IEntity entity, bool spectator)
	{
		if (!entity)
			return false;

		SetInitialMainEntity(entity);
		if (GetControlledEntity() != entity)
		{
			PrintFormat("GC | Lifecycle | controller assignment failed: player=%1 entity=%2", GetPlayerId(), entity, level: LogLevel.WARNING);
			return false;
		}

		if (spectator)
			SetLobbyPlayerState(GC_ELobbyPlayerState.SPECTATOR);
		else
			SetLobbyPlayerState(GC_ELobbyPlayerState.PLAYING);
		Rpc(RpcDo_SetLobbySpectator, spectator);
		return true;
	}

	GC_ELobbyPlayerState GetLobbyPlayerState()
	{
		return m_eLobbyPlayerState;
	}

	void SetLobbySpectatorState()
	{
		SetLobbyPlayerState(GC_ELobbyPlayerState.SPECTATOR);
		Rpc(RpcDo_SetLobbySpectator, true);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_SetLobbySpectator(bool spectator)
	{
		if (spectator)
			SetLobbyPlayerState(GC_ELobbyPlayerState.SPECTATOR);
		else
			SetLobbyPlayerState(GC_ELobbyPlayerState.PLAYING);

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
