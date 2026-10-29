//! Explicit owner-side transport probe, also compiled in test peers.
modded class GC_LobbyPlayerController
{
	protected string m_sGC_EventPeerResult = "No response";
	protected int m_iGC_EventPeerRequest;

	//------------------------------------------------------------------------------------------------
	string GC_TestEventLogPeer(string action)
	{
		if (action == "everyone" || action == "you")
		{
			m_OnGC_SpectatorEvents.Remove(GC_TestEventPeerRows);
			m_OnGC_SpectatorEvents.Insert(GC_TestEventPeerRows);
			m_iGC_EventPeerRequest = GC_ReserveSpectatorEventRequest();
			m_sGC_EventPeerResult = "Pending";
			GC_RequestSpectatorEvents(m_iGC_EventPeerRequest, action == "everyone", 0);
		}
		else if (action == "open" || action == "release")
		{
			float value;
			if (action == "open")
				value = 1;
			GetGame().GetInputManager().SetActionValue("GC_ToggleSpectatorEventLog", value);
		}
		return string.Format("state=%1 admin=%2 %3", GetLobbyPlayerState(), SCR_Global.IsAdmin(GetPlayerId()), m_sGC_EventPeerResult);
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_TestEventPeerRows(int request, array<string> rows, int next, string status)
	{
		if (request != m_iGC_EventPeerRequest)
			return;
		m_sGC_EventPeerResult = string.Format("rows=%1 next=%2 status=%3", rows.Count() / 7, next, status);
		if (rows.Count() >= 7)
			m_sGC_EventPeerResult += " first=" + rows[1] + " " + rows[2] + " " + rows[3] + " " + rows[4];
		m_OnGC_SpectatorEvents.Remove(GC_TestEventPeerRows);
	}
}
