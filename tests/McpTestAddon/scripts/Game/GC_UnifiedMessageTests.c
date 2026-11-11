#ifdef WORKBENCH
modded class GC_MissionRuleTests
{
	//------------------------------------------------------------------------------------------------
	string RunUnifiedMessages()
	{
		GC_MessageAction authored = new GC_MessageAction();
		authored.m_sMessage = "  Mission notice  ";
		authored.m_aFactions = {"US", "USSR"};
		GC_RuleAction action = authored.BuildAction();
		Check(action.IsValid() && action.m_eDo == GC_ERuleAction.MISSION_MESSAGE && action.m_iOperation == 0 && action.m_aFlagNames.Count() == 2, "existing player delivery and faction filtering preserved");
		authored.m_bWarning = true;
		Check(authored.BuildAction().m_bFlagAnswer, "existing warning setting preserved");
		authored.m_iDelivery = 1;
		authored.m_aFactions = {""};
		action = authored.BuildAction();
		Check(action.IsValid() && action.m_iOperation == 1 && action.m_aFlagNames.IsEmpty() && action.m_bFlagAnswer, "admin warning ignores player faction settings");
		authored.m_iDelivery = 2;
		authored.m_sTitle = "<unused>";
		action = authored.BuildAction();
		Check(action.IsValid() && action.m_eDo == GC_ERuleAction.WRITE_LOG && action.m_sTextOrName == "Mission notice", "log uses existing log command and ignores popup-only settings");
		GC_LogMessageAction legacy = new GC_LogMessageAction();
		legacy.m_sMessage = "Legacy notice";
		Check(legacy.BuildAction().IsValid() && legacy.BuildAction().m_eDo == GC_ERuleAction.WRITE_LOG, "serialized legacy log remains usable");
		authored.m_iDelivery = 3;
		Check(!authored.BuildAction().IsValid(), "unknown delivery rejected");
		authored.m_iDelivery = -1;
		Check(!authored.BuildAction().IsValid(), "negative delivery rejected");
		authored.m_iDelivery = 1;
		authored.m_sTitle = "";
		authored.m_sMessage = "<markup>";
		Check(!authored.BuildAction().IsValid(), "admin popup rejects markup");
		authored.m_iDelivery = 2;
		authored.m_sMessage = " \t\n ";
		Check(!authored.BuildAction().IsValid(), "blank log rejected");
		return string.Format("Unified message checks: %1; failures: %2", m_iChecks, m_iFailures);
	}
}

modded class GC_AdminMessagePopupComponent
{
	//------------------------------------------------------------------------------------------------
	static string GC_TestUnifiedPopupState()
	{
		int admins;
		foreach (GC_AdminMessagePopupComponent popup : s_aVisible)
		{
			if (popup.m_Message.m_eAudience == GC_EAdminMessageAudience.ADMINS)
				admins++;
		}
		return string.Format("visible=%1 queued=%2 admins=%3 %4", s_aVisible.Count(), s_aWaiting.Count(), admins, GC_TestWarningState());
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestUnifiedMessageValidation()
	{
		GC_MissionRuleTests tests = new GC_MissionRuleTests();
		return tests.RunUnifiedMessages();
	}

	//------------------------------------------------------------------------------------------------
	//! Explicit disposable Play test through a real event and the production authority gate.
	string GC_TestUnifiedMessage(int delivery, bool warning, string message)
	{
		if (!HasMissionRuleAuthority())
			return "UNAVAILABLE active server";
		GC_MessageAction authored = new GC_MessageAction();
		authored.m_iDelivery = delivery;
		authored.m_bWarning = warning;
		authored.m_sMessage = message;
		GC_RuleAction action = authored.BuildAction();
		GC_CompiledMissionRule rule = GC_MissionRuleTests.EventRule("unified-message-test");
		rule.m_aThen = {action};
		GC_MissionRuleEngine previous = m_MissionRuleEngine;
		bool enabled = m_bMissionEventsEnabled;
		GC_MissionRuleEngine engine = new GC_MissionRuleEngine();
		m_MissionRuleEngine = engine;
		m_bMissionEventsEnabled = true;
		engine.Start({rule}, null, GetLobbyPhase(), null, null, GetGame().GetWorld(), this);
		bool rejected = !GC_ExecuteFrameworkAction(null, action);
		engine.SendTestEvent("unified-message-test");
		int runs = engine.GetRunCount();
		engine.Stop();
		m_MissionRuleEngine = previous;
		m_bMissionEventsEnabled = enabled;
		return string.Format("runs=%1 invalidSourceRejected=%2 admin=%3 %4", runs, rejected, SCR_Global.IsAdmin(), GC_AdminMessagePopupComponent.GC_TestUnifiedPopupState());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestUnifiedMessageLogout()
	{
		if (!HasMissionRuleAuthority() || !SCR_Global.IsAdmin())
			return "UNAVAILABLE local authenticated server";
		PlayerManager players = GetGame().GetPlayerManager();
		int playerId = GetGame().GetPlayerController().GetPlayerId();
		bool administrator = players.HasPlayerRole(playerId, EPlayerRole.ADMINISTRATOR);
		bool sessionAdministrator = players.HasPlayerRole(playerId, EPlayerRole.SESSION_ADMINISTRATOR);
		if (administrator)
			players.ClearPlayerRole(playerId, EPlayerRole.ADMINISTRATOR);
		if (sessionAdministrator)
			players.ClearPlayerRole(playerId, EPlayerRole.SESSION_ADMINISTRATOR);
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		bool loggedOut = !SCR_Global.IsAdmin(playerId);
		int before = controller.GetAdminMessages().Count();
		string dispatch;
		if (loggedOut)
			dispatch = GC_TestUnifiedMessage(1, false, "Must not reach logged-out player");
		bool unchanged = before == controller.GetAdminMessages().Count();
		string popups = GC_AdminMessagePopupComponent.GC_TestUnifiedPopupState();
		GetGame().GetCallqueue().CallLater(GC_TestUnifiedLogoutSettled, 100);
		if (administrator)
			players.GivePlayerRole(playerId, EPlayerRole.ADMINISTRATOR);
		if (sessionAdministrator)
			players.GivePlayerRole(playerId, EPlayerRole.SESSION_ADMINISTRATOR);
		return string.Format("loggedOut=%1 historyUnchanged=%2 restored=%3 %4 dispatch=[%5]", loggedOut, unchanged, SCR_Global.IsAdmin(playerId), popups, dispatch);
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_TestUnifiedLogoutSettled()
	{
		Print("GC_UNIFIED_LOGOUT settled admin=" + SCR_Global.IsAdmin() + " " + GC_AdminMessagePopupComponent.GC_TestUnifiedPopupState());
	}
}
#endif
