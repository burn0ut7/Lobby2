//! Explicit feedback-order fixture. Does not change roles or gameplay state.
modded class GC_LobbyPlayerController
{
	protected int m_iGCResultCount;
	protected int m_iGCBanPageCount;
	protected string m_sGCResult;

	//------------------------------------------------------------------------------------------------
	protected void GC_TestObserveResult(string outcome)
	{
		m_iGCResultCount++;
		m_sGCResult = outcome;
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_TestObserveBanPage(array<string> identities, array<string> labels, int page, int total)
	{
		m_iGCBanPageCount++;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestResultOrdering()
	{
		if (!Replication.IsServer() || GetGame().GetPlayerController() != this || !SCR_Global.IsAdmin(GetPlayerId()))
			return "requires-current-host-admin";
		m_iGCResultCount = 0;
		m_iGCBanPageCount = 0;
		m_OnAdminActionResult.Insert(GC_TestObserveResult);
		m_OnAdminBanPage.Insert(GC_TestObserveBanPage);
		RequestAdminFreeze(-1, "invalid-fixture-action", 0);
		int first = m_iAdminActionRequestId;
		RequestAdminFreeze(-1, "invalid-fixture-action", 0);
		int second = m_iAdminActionRequestId;
		int delivered = m_iGCResultCount;
		// Inject delayed presentation payloads through production owner receivers.
		// No gameplay operation or permission check is bypassed by these payloads.
		RpcDo_AdminActionResult("stale-fixture", first);
		array<string> empty = {};
		RpcDo_AdminBanPage(empty, empty, 0, 0, first);
		int afterStale = m_iGCResultCount;
		int stalePages = m_iGCBanPageCount;
		RpcDo_AdminActionResult("current-fixture", second);
		RpcDo_AdminBanPage(empty, empty, 0, 0, second);
		m_OnAdminActionResult.Remove(GC_TestObserveResult);
		m_OnAdminBanPage.Remove(GC_TestObserveBanPage);
		return string.Format("first=%1 second=%2 delivered=%3 afterStale=%4 stalePages=%5 finalResults=%6 finalPages=%7 last=%8", first, second, delivered, afterStale, stalePages, m_iGCResultCount, m_iGCBanPageCount, m_sGCResult);
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestAdminResultOrdering()
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return "no-local-controller";
		return controller.GC_TestResultOrdering();
	}
}
