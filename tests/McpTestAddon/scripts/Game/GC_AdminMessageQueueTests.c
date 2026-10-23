//! Optional test addon observations, available on the receiving peer too.
modded class GC_AdminMessagePopupComponent
{
	//------------------------------------------------------------------------------------------------
	static string GC_TestQueueSnapshot()
	{
		if (!s_CurrentMessage)
			return "active=0";
		string rendered;
		if (s_wPopup)
		{
			TextWidget body = TextWidget.Cast(s_wPopup.FindAnyWidget("AdminMessageText"));
			if (body)
				rendered = body.GetText();
		}
		return string.Format("active=1 current=%1 rendered=%2", s_CurrentMessage.m_sText, rendered);
	}
}

modded class GC_LobbyManager
{
	protected int m_iGC_TestQueuedMessages;
	protected string m_sGC_TestQueueCompleted;

	//------------------------------------------------------------------------------------------------
	string GC_TestMessageQueuePaced()
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller || !Replication.IsServer() || !controller.GetAdminMessages().IsEmpty())
			return "UNAVAILABLE requires fresh local authority/controller/history";
		if (m_iGC_TestQueuedMessages > 0)
			return "UNAVAILABLE already started";
		GC_TestMessageQueueTick();
		return "STARTED 120 production requests at 550 ms intervals";
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_TestMessageQueueTick()
	{
		if (GC_LobbyManager.GetManager() != this || !Replication.IsServer())
			return;
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return;
		m_iGC_TestQueuedMessages++;
		array<string> recipients = {};
		controller.RequestAdminMessage(GC_EAdminMessageAudience.ALL_PLAYERS,
			string.Format("paced-%1", m_iGC_TestQueuedMessages), recipients);
		if (m_iGC_TestQueuedMessages < 120)
			GetGame().GetCallqueue().CallLater(GC_TestMessageQueueTick, 550);
		else
			m_sGC_TestQueueCompleted = GC_TestMessageQueueSnapshot();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestMessageQueueResult()
	{
		return string.Format("sent=%1 completed=%2 current=%3", m_iGC_TestQueuedMessages,
			m_sGC_TestQueueCompleted, GC_TestMessageQueueSnapshot());
	}

	//------------------------------------------------------------------------------------------------
	//! Sends through the production controller; never edits history or popup state.
	string GC_TestMessageQueueBurst()
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller || !Replication.IsServer())
			return "UNAVAILABLE local authority/controller";
		if (!controller.GetAdminMessages().IsEmpty())
			return "UNAVAILABLE requires fresh message history";
		array<string> recipients = {};
		for (int i = 1; i <= 102; i++)
			controller.RequestAdminMessage(GC_EAdminMessageAudience.ALL_PLAYERS,
				string.Format("queue-%1", i), recipients);
		return GC_TestMessageQueueSnapshot();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestMessageQueueSnapshot()
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return "UNAVAILABLE controller";
		array<ref GC_AdminMessage> messages = controller.GetAdminMessages();
		if (messages.IsEmpty())
			return "history=0 " + GC_AdminMessagePopupComponent.GC_TestQueueSnapshot();
		return string.Format("history=%1 first=%2 last=%3 %4", messages.Count(),
			messages[0].m_sText, messages[messages.Count() - 1].m_sText,
			GC_AdminMessagePopupComponent.GC_TestQueueSnapshot());
	}
}
