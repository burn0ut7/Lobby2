//! Explicit lease integration tests without microphone capture or fabricated roles.
#ifdef WORKBENCH
modded class GC_LobbyPlayerController
{
	//------------------------------------------------------------------------------------------------
	string GC_TestVoiceLease(string action)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (!Replication.IsServer() || !manager || GetGame().GetPlayerController() != this)
			return "requires-local-authority";
		if (m_bLobbyVoiceHeld || m_LobbyCapture)
			return "refused-active-user-capture";
		if (action == "grant")
		{
			// Exercise production server authorization. With no local held input,
			// its owner grant cannot open native capture.
			RpcAsk_LobbyVoice(GC_LobbyRadio.CHAT, m_iLobbyVoiceRequest);
		}
		else if (action == "observe")
			manager.ObserveLobbyVoiceTransmission(GetPlayerId());
		else if (action == "release")
			RpcAsk_ReleaseLobbyVoice();
		else if (action != "snapshot")
			return "rejected-action";
		GC_VoiceChannelAssignment assignment = manager.GetPlayerVoiceAssignment(GetPlayerId());
		if (!assignment)
			return "unavailable-assignment";
		return string.Format("authorized=%1 priority=%2 muted=%3 transmitting=%4 held=%5 capture=%6", HasAuthorizedLobbyCapture(), assignment.IsPrioritySpeaker(), assignment.IsAdminMuted(), assignment.IsPriorityTransmitting(), m_bLobbyVoiceHeld, m_LobbyCapture != null);
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestVoiceLease(string action)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return "no-local-controller";
		return controller.GC_TestVoiceLease(action);
	}
}
#endif
