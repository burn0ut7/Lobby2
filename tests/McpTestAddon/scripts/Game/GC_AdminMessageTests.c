#ifdef WORKBENCH
modded class GC_AdminMessagePopupComponent
{
	static string GC_TestWarningState()
	{
		if (s_aVisible.IsEmpty())
			return "UNAVAILABLE popup";
		GC_AdminMessagePopupComponent popup = s_aVisible[s_aVisible.Count() - 1];
		return string.Format("warning=%1 icon=%2 accent=%3 soundValid=%4 playing=%5 title=%6", popup.m_Message.m_bWarning,
			popup.m_wRoot.FindAnyWidget("AdminWarningIcon").IsVisible(), popup.m_wRoot.FindAnyWidget("AdminWarningAccent").IsVisible(),
			popup.m_Sound != AudioHandle.Invalid, AudioSystem.IsSoundPlayed(popup.m_Sound),
			TextWidget.Cast(popup.m_wRoot.FindAnyWidget("AdminMessageTitle")).GetText());
	}
}

modded class GC_LobbyManager
{
	string GC_TestAdminWarning(string message)
	{
		int playerId = GetGame().GetPlayerController().GetPlayerId();
		return GC_ExecuteAdminWarn(playerId, playerId, GetPlayerVoiceIdentity(playerId), message)
			+ " | " + GC_AdminMessagePopupComponent.GC_TestWarningState();
	}

	string GC_TestAdminWarningState()
	{
		return GC_AdminMessagePopupComponent.GC_TestWarningState();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAdminMessage(int audience, string message)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return "UNAVAILABLE controller";
		array<string> recipients = {GetPlayerVoiceIdentity(controller.GetPlayerId())};
		controller.RequestAdminMessage(audience, message, recipients);
		return "DISPATCHED inspect message snapshot and audit log";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAdminMessageSnapshot()
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return "UNAVAILABLE controller";
		array<ref GC_AdminMessage> messages = controller.GetAdminMessages();
		if (messages.IsEmpty())
			return "count=0";
		GC_AdminMessage latest = messages[messages.Count() - 1];
		return string.Format("count=%1 id=%2 sender=%3 audience=%4 text=%5", messages.Count(), latest.m_iMessageId, latest.m_iSenderId, latest.m_eAudience, latest.m_sText);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAdminMessageReject()
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller || !Replication.IsServer())
			return "UNAVAILABLE authority/controller";
		int before = controller.GetAdminMessages().Count();
		array<string> recipients = {"00000000-0000-0000-0000-000000000001"};
		string outcome = SendAdminMessage(-1, GC_EAdminMessageAudience.ALL_PLAYERS, "unauthorized test", recipients);
		if (outcome != "rejected-not-logged-in")
			return "FAIL invalid requester accepted";
		outcome = SendAdminMessage(controller.GetPlayerId(), GC_EAdminMessageAudience.SELECTED_PLAYERS, "stale recipient test", recipients);
		if (outcome != "rejected-stale-recipient")
			return "FAIL stale recipient accepted";
		if (controller.GetAdminMessages().Count() != before)
			return "FAIL rejected message delivered";
		return "PASS requester and stale-recipient rejection without delivery";
	}
}
#endif
