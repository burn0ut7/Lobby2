modded class SCR_VoNComponent : VoNComponent
{
	protected bool m_bGC_LobbyCaptureActive;
	protected float m_fGC_NextSpectatorReceiveLog;

	//------------------------------------------------------------------------------------------------
	//! Explicit Lobby VON boundary; never changes normal game capture or social mute.
	bool GC_SetLobbyCapture(bool requested, int playerId)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (!manager)
			return false;
		bool allowed = requested;
		PlayerController controller = GetGame().GetPlayerManager().GetPlayerController(playerId);
		IEntity body;
		if (controller)
			body = controller.GetControlledEntity();
		if (!body || body.FindComponent(VoNComponent) != this)
			return false;
		if (body.FindComponent(GC_LobbySpectatorBodyComponent))
			allowed = requested && manager.CanTransmitLobbyVoice(playerId);
		if (allowed == m_bGC_LobbyCaptureActive)
			return m_bGC_LobbyCaptureActive;
		if (allowed)
			m_bGC_LobbyCaptureActive = SetCapture(true);
		else
			GC_EndLobbyCapture();
		return m_bGC_LobbyCaptureActive;
	}

	//------------------------------------------------------------------------------------------------
	//! Release only capture explicitly acquired through the Lobby VON entry point.
	void GC_EndLobbyCapture()
	{
		if (m_bGC_LobbyCaptureActive)
			SetCapture(false);
		m_bGC_LobbyCaptureActive = false;
	}

	//------------------------------------------------------------------------------------------------
	override protected event void OnVoNUsed(int senderId)
	{
		super.OnVoNUsed(senderId);
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (manager)
			manager.ObserveLobbyVoiceTransmission(senderId);
	}

	//------------------------------------------------------------------------------------------------
	override protected event void OnReceive(int playerId, bool isSenderEditor, BaseTransceiver receiver, int frequency, float quality)
	{
		super.OnReceive(playerId, isSenderEditor, receiver, frequency, quality);
		GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
		GC_LobbyPlayerController localController = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		float now = GetGame().GetWorld().GetWorldTime();
		if (lobbyManager && localController && localController.GC_CanJumpSpectatorCamera() && now >= m_fGC_NextSpectatorReceiveLog)
		{
			m_fGC_NextSpectatorReceiveLog = now + 2000;
			PrintFormat("GC | Spectator voice | received talker=%1 radio=%2 frequency=%3 quality=%4 editor=%5 %6",
				playerId, receiver != null, frequency, quality, isSenderEditor, localController.GC_GetSpectatorVoiceDiagnostics());
		}
		if (!lobbyManager || !GC_UIEventDiagnostics.ENABLED)
			return;

		GC_VoiceChannelAssignment assignment = lobbyManager.GetPlayerVoiceAssignment(playerId);
		if (!assignment)
			return;

		string channelName;
		switch (assignment.GetChannelType())
		{
			case GC_VoiceChannelType.GLOBAL:
				channelName = "GLOBAL";
				break;
			case GC_VoiceChannelType.FACTION:
				channelName = "FACTION";
				break;
			case GC_VoiceChannelType.GROUP:
				channelName = "GROUP";
				break;
			case GC_VoiceChannelType.PERSONAL:
				channelName = "PERSONAL";
				break;
		}
		
		PrintFormat(
			"GC | Voice receive | talker=%1 channel=%2 faction=%3 group=%4 personalOwner=%5",
			playerId,
			channelName,
			assignment.GetFactionKey(),
			assignment.GetGroupRplId(),
			assignment.GetPersonalOwnerIdentity()
		);
	}
}
