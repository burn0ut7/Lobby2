//! Voice and replication use the controlled body, not just the local camera.
modded class GC_LobbyPlayerController
{
	protected float m_fGC_NextBodyFollowRequest;
	protected float m_fGC_NextBodyFollowLog;

	//------------------------------------------------------------------------------------------------
	void GC_StartSpectatorBodyFollow()
	{
		GC_StopSpectatorBodyFollow();
		GetGame().GetCallqueue().CallLater(GC_UpdateSpectatorBodyPosition, 250, true);
	}

	//------------------------------------------------------------------------------------------------
	void GC_StopSpectatorBodyFollow()
	{
		GetGame().GetCallqueue().Remove(GC_UpdateSpectatorBodyPosition);
	}

	//------------------------------------------------------------------------------------------------
	string GC_GetSpectatorVoiceDiagnostics()
	{
		IEntity body = GetControlledEntity();
		if (!body || !m_SpectatorCamera)
			return "body or camera unavailable";
		return string.Format("body=%1 camera=%2 separation=%3 listening=%4", body.GetOrigin(),
			m_SpectatorCamera.GetOrigin(), vector.Distance(body.GetOrigin(), m_SpectatorCamera.GetOrigin()), GC_LobbyPlayerController.Cast(this).GC_IsListeningToRadios());
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_UpdateSpectatorBodyPosition()
	{
		IEntity body = GetControlledEntity();
		if (!GC_LobbyPlayerController.Cast(this).GC_CanJumpSpectatorCamera() || !body || !body.FindComponent(GC_LobbySpectatorBodyComponent))
			return;
		vector position = m_SpectatorCamera.GetOrigin();
		if (vector.DistanceSq(body.GetOrigin(), position) <= 4)
			return;
		Rpc(GC_RpcAskSpectatorBodyPosition, Replication.FindItemId(body), position);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Unreliable, RplRcver.Server)]
	protected void GC_RpcAskSpectatorBodyPosition(RplId bodyId, vector position)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		IEntity body = GetControlledEntity();
		if (!Replication.IsServer() || !manager || GetLobbyPlayerState() != GC_ELobbyPlayerState.SPECTATOR
			|| !body || !body.FindComponent(GC_LobbySpectatorBodyComponent) || Replication.FindItemId(body) != bodyId)
			return;
		SCR_EGameModeState phase = manager.GetLobbyPhase();
		if (phase != SCR_EGameModeState.GAME && phase != SCR_EGameModeState.BRIEFING)
			return;
		if (!SCR_Global.IsPositionWithinTerrainBounds(position))
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now < m_fGC_NextBodyFollowRequest)
			return;
		m_fGC_NextBodyFollowRequest = now + 100;
		// Native character teleport executes on the owner, as in the stock editor.
		Rpc(GC_RpcDoSpectatorBodyPosition, bodyId, position);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Unreliable, RplRcver.Owner)]
	protected void GC_RpcDoSpectatorBodyPosition(RplId bodyId, vector position)
	{
		IEntity body = GetControlledEntity();
		if (!GC_LobbyPlayerController.Cast(this).GC_CanJumpSpectatorCamera() || !body || !body.FindComponent(GC_LobbySpectatorBodyComponent)
			|| Replication.FindItemId(body) != bodyId)
			return;
		bool moved = SCR_Global.TeleportPlayer(GetPlayerId(), position);
		float now = GetGame().GetWorld().GetWorldTime();
		if (now >= m_fGC_NextBodyFollowLog)
		{
			m_fGC_NextBodyFollowLog = now + 5000;
			PrintFormat("GC | Spectator voice | body-follow accepted=%1 %2", moved, GC_GetSpectatorVoiceDiagnostics());
		}
	}
}
