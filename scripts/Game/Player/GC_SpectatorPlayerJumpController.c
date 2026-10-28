//! One-shot spectator navigation: authority resolves the target, owner moves only its camera.
modded class GC_LobbyPlayerController
{
	protected int m_iGC_SpectatorJumpRequest;
	protected GC_SpectatorManualCamera m_GC_SpectatorJumpCamera;

	//------------------------------------------------------------------------------------------------
	bool GC_CanJumpSpectatorCamera()
	{
		return GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this
			&& GetLobbyPlayerState() == GC_ELobbyPlayerState.SPECTATOR && m_SpectatorCamera
			&& m_bSpectatorPresentationRequested;
	}

	//------------------------------------------------------------------------------------------------
	void GC_RequestSpectatorPlayerPosition(int targetPlayerId)
	{
		if (!GC_CanJumpSpectatorCamera() || targetPlayerId <= 0)
			return;
		m_iGC_SpectatorJumpRequest++;
		m_GC_SpectatorJumpCamera = m_SpectatorCamera;
		Rpc(GC_RpcAskSpectatorPlayerPosition, targetPlayerId, m_iGC_SpectatorJumpRequest);
	}

	//------------------------------------------------------------------------------------------------
	protected string GC_ResolveSpectatorPlayerPosition(int targetPlayerId, out vector position)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (!Replication.IsServer() || !manager || GetLobbyPlayerState() != GC_ELobbyPlayerState.SPECTATOR)
			return "Spectator camera is unavailable.";
		SCR_EGameModeState phase = manager.GetLobbyPhase();
		if (phase != SCR_EGameModeState.GAME && phase != SCR_EGameModeState.BRIEFING)
			return "Spectator camera is unavailable.";
		if (phase == SCR_EGameModeState.BRIEFING && manager.GetSlotByPlayerId(GetPlayerId()))
			return "Spectator camera is unavailable.";
		PlayerManager players = GetGame().GetPlayerManager();
		if (targetPlayerId <= 0 || !players.IsPlayerConnected(targetPlayerId))
			return "Player is no longer connected.";

		// A dead player's controlled entity is now a spectator body. Use the retained
		// slot character instead; never jump to another spectator's hidden body.
		IEntity target;
		GC_Slot slot = manager.GetSlotByPlayerId(targetPlayerId);
		if (slot && slot.GetState() == GC_SlotState.KIA)
			target = slot.GetEntity();
		else
			target = players.GetPlayerControlledEntity(targetPlayerId);
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(target);
		if (!character || character.FindComponent(GC_LobbySpectatorBodyComponent))
			return "Player position is unavailable.";
		// Arrive above the character instead of inside its head or ragdoll.
		position = character.EyePosition() + "0 2 0";
		return "";
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GC_RpcAskSpectatorPlayerPosition(int targetPlayerId, int requestId)
	{
		vector position;
		string error = GC_ResolveSpectatorPlayerPosition(targetPlayerId, position);
		Rpc(GC_RpcDoSpectatorPlayerPosition, requestId, position, error);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDoSpectatorPlayerPosition(int requestId, vector position, string error)
	{
		if (!GC_CanJumpSpectatorCamera() || requestId != m_iGC_SpectatorJumpRequest
			|| m_GC_SpectatorJumpCamera != m_SpectatorCamera)
			return;
		m_GC_SpectatorJumpCamera = null;
		if (!error.IsEmpty())
		{
			SCR_HintManagerComponent.ShowCustomHint(error, "Spectator", 4);
			return;
		}
		// No entity lookup on the receiving client: moving the camera brings the
		// destination into streaming range, just as in Reforger Lobby's position RPC.
		m_SpectatorCamera.SetFreeMode();
		m_SpectatorCamera.SetOrigin(position);
	}
}
