#ifdef WORKBENCH
modded class GC_AdminStatusComponent
{
	//------------------------------------------------------------------------------------------------
	string GC_TestStatus(string action)
	{
		if (action == "bounds")
		{
			m_aFPS.Clear();
			for (int i; i < 100; i++)
			{
				m_iGraphSampleAt = -5000;
				OnSample(m_iRequest, 1, 1, i, 100, 0);
			}
			return string.Format("rolling count=%1 first=%2 last=%3 expected=60/40/99", m_aFPS.Count(), m_aFPS[0], m_aFPS[m_aFPS.Count() - 1]);
		}
		if (action == "long-hide")
		{
			SetActive(false);
			m_iReceivedAt -= 20000;
			SetActive(true);
		}
		if (action == "stale")
			OnSample(m_iRequest - 1, 999, 999, 999, 999, 999);
		if (action == "event")
			OnPopulationChanged(1);
		if (action == "history")
		{
			GetGame().GetCallqueue().Remove(Poll);
			m_aFPS.Clear();
			for (int n; n < 40; n++)
			{
				int fps = 30;
				if (n % 17 == 10)
					fps = 26;
				m_aFPS.Insert(fps);
			}
			SetText("StatusFPS", "30");
			DrawGraph();
		}
		return string.Format("active=%1 samples=%2 commands=%3 request=%4 received=%5 players=%6 fps=%7", m_bActive, m_aFPS.Count(), m_aCommands.Count(), m_iRequest, m_iReceivedAt, TextWidget.Cast(m_wRoot.FindAnyWidget("StatusPlayers")).GetText(), TextWidget.Cast(m_wRoot.FindAnyWidget("StatusFPS")).GetText()) + " feedback=" + TextWidget.Cast(m_wRoot.FindAnyWidget("StatusFeedback")).GetText() + " uptime=" + TextWidget.Cast(m_wRoot.FindAnyWidget("StatusUptime")).GetText();
	}
}

modded class GC_LobbyPlayerController
{
	//------------------------------------------------------------------------------------------------
	string GC_TestStatusDenied()
	{
		if (SCR_Global.IsAdmin(GetPlayerId()))
			return "REQUIRES logged-out requester";
		int before = m_iGC_StatusSampleTime;
		GC_RpcAsk_ServerStatus(-1);
		GC_RpcAsk_ServerStatus(-2);
		return string.Format("unauthorized: collection unchanged=%1; denialTime=%2", before == m_iGC_StatusSampleTime, m_iGC_StatusDenialTime);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestStatusCost()
	{
		int start = System.GetTickCount();
		int previous = m_iGC_StatusSampleTime;
		GC_RpcAsk_ServerStatus(-1);
		bool throttled = previous == m_iGC_StatusSampleTime;
		for (int i; i < 100; i++)
		{
			m_iGC_StatusSampleTime = System.GetTickCount() - 5000;
			GC_RpcAsk_ServerStatus(-1);
		}
		return string.Format("100 authorized collections and owner replies: %1 ms; immediate request throttled=%2", System.GetTickCount() - start, throttled);
	}
}

modded class GC_AdminToolsComponent
{
	//------------------------------------------------------------------------------------------------
	static string GC_TestServerStatus(string action)
	{
		if (!s_wPanel)
			return "Admin tools closed";
		GC_AdminStatusComponent status = GC_AdminStatusComponent.Cast(s_wPanel.FindAnyWidget("StatusFields").FindHandler(GC_AdminStatusComponent));
		return status.GC_TestStatus(action);
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestStatus(string action)
	{
		if (action == "denied")
			return GC_LobbyPlayerController.Cast(GetGame().GetPlayerController()).GC_TestStatusDenied();
		if (action == "cost")
			return GC_LobbyPlayerController.Cast(GetGame().GetPlayerController()).GC_TestStatusCost();
		return GC_AdminToolsComponent.GC_TestServerStatus(action);
	}
}
#endif
