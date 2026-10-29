//! Explicit owner-side cache/transport probe, also compiled in test peers.
modded class GC_LobbyPlayerController
{
	protected int m_iGC_TestFeedPackets;
	protected int m_iGC_TestFeedRows;
	protected int m_iGC_TestPersonalPackets;
	protected int m_iGC_TestSubscribeRequests;
	protected int m_iGC_TestHistoryRequests;

	string GC_TestEventLogPeer(string action)
	{
		if (action == "everyone" || action == "repeat" || action == "you")
			GC_EnsureSpectatorEvents(action != "you");
		else if (action == "more")
			GC_LoadPersonalEventPage();
		else if (action == "resubscribe")
			Rpc(GC_RpcAsk_SubscribeSpectatorEvents, m_iGC_EventEpoch + 100);
		else if (action == "open" || action == "release")
		{
			float value;
			if (action == "open")
				value = 1;
			GetGame().GetInputManager().SetActionValue("GC_ToggleSpectatorEventLog", value);
		}
		string result = string.Format("state=%1 subscribed=%2 liveLoaded=%3 live=%4 personalLoaded=%5 personal=%6 next=%7", GetLobbyPlayerState(), m_bGC_EventFeedRequested, m_bGC_LiveEventsLoaded, m_aGC_LiveEvents.Count() / 7, m_bGC_PersonalEventsLoaded, m_aGC_PersonalEvents.Count() / 7, m_iGC_PersonalNext);
		result += string.Format(" packets=%1 deltaRows=%2 personalPackets=%3 subscribeRequests=%4 historyRequests=%5 serverSubscribed=%6 epoch=%7", m_iGC_TestFeedPackets, m_iGC_TestFeedRows, m_iGC_TestPersonalPackets, m_iGC_TestSubscribeRequests, m_iGC_TestHistoryRequests, m_GC_EventSource != null, m_iGC_EventEpoch);
		if (m_aGC_PersonalEvents.Count() >= 7)
			result += " firstPersonal=" + m_aGC_PersonalEvents[1] + " " + m_aGC_PersonalEvents[2] + " " + m_aGC_PersonalEvents[3];
		return result;
	}

	override protected void GC_RpcAsk_SubscribeSpectatorEvents(int epoch)
	{
		m_iGC_TestSubscribeRequests++;
		super.GC_RpcAsk_SubscribeSpectatorEvents(epoch);
	}

	override protected void GC_RpcAsk_PersonalEvents(int epoch, int before)
	{
		m_iGC_TestHistoryRequests++;
		super.GC_RpcAsk_PersonalEvents(epoch, before);
	}

	override protected void GC_RpcDo_SpectatorFeed(int epoch, array<string> changed, array<int> order, string status)
	{
		m_iGC_TestFeedPackets++;
		m_iGC_TestFeedRows += changed.Count() / 7;
		super.GC_RpcDo_SpectatorFeed(epoch, changed, order, status);
	}

	override protected void GC_RpcDo_PersonalEvents(int epoch, int before, array<string> rows, int next, string status)
	{
		m_iGC_TestPersonalPackets++;
		super.GC_RpcDo_PersonalEvents(epoch, before, rows, next, status);
	}
}

#ifdef WORKBENCH
modded class GC_LobbyManager
{
	string GC_TestEventCache(int playerId, string action)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!controller)
			return "NO CONTROLLER";
		return controller.GC_TestEventLogPeer(action);
	}
}
#endif
