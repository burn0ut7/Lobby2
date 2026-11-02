class GC_DebriefPage
{
	ref array<string> m_aRows = {};
	string m_sStatus;
	int m_iNext = -1;
}

class GC_DebriefRequest
{
	string m_sKind;
	string m_sSubject;
	int m_iCursor;
	int m_iFilter;

	string Key()
	{
		return string.Format("%1|%2|%3|%4", m_sKind, m_sSubject, m_iCursor, m_iFilter);
	}
}

modded class GC_LobbyPlayerController
{
	ref ScriptInvoker m_GC_OnReportPage = new ScriptInvoker();
	protected ref map<string, ref GC_DebriefPage> m_mGC_ReportPages = new map<string, ref GC_DebriefPage>();
	protected ref array<string> m_aGC_ReportPageOrder = {};
	protected ref array<ref GC_DebriefRequest> m_aGC_ReportRequests = {};
	protected string m_sGC_ActiveReportKey;
	protected string m_sGC_ServerReportKey;
	protected ref GC_DebriefRequest m_GC_ServerReport;

	//------------------------------------------------------------------------------------------------
	void GC_RequestReport(string kind, string subject = "", int cursor = 0, int filter = 0)
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!mode || mode.GetState() != SCR_EGameModeState.POSTGAME || GetGame().GetPlayerController() != this)
			return;
		GC_DebriefRequest request = new GC_DebriefRequest();
		request.m_sKind = kind;
		request.m_sSubject = subject;
		request.m_iCursor = cursor;
		request.m_iFilter = filter;
		string key = request.Key();
		GC_DebriefPage cached = m_mGC_ReportPages.Get(key);
		if (cached)
		{
			m_GC_OnReportPage.Invoke(key, cached);
			return;
		}
		if (key == m_sGC_ActiveReportKey)
			return;
		foreach (GC_DebriefRequest queued : m_aGC_ReportRequests)
		{
			if (queued.Key() == key)
				return;
		}
		// UI keeps at most one foreground request plus one incremental directory page.
		if (m_aGC_ReportRequests.Count() >= 8)
			m_aGC_ReportRequests.RemoveOrdered(0);
		m_aGC_ReportRequests.Insert(request);
		GC_PumpReportRequests();
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_PumpReportRequests()
	{
		if (!m_sGC_ActiveReportKey.IsEmpty() || m_aGC_ReportRequests.IsEmpty())
			return;
		GC_DebriefRequest request = m_aGC_ReportRequests[0];
		m_aGC_ReportRequests.RemoveOrdered(0);
		m_sGC_ActiveReportKey = request.Key();
		Rpc(GC_RpcAsk_ReportPage, request.m_sKind, request.m_sSubject, request.m_iCursor, request.m_iFilter);
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_SetServerReport(string kind, string subject, int cursor, int filter)
	{
		m_GC_ServerReport = new GC_DebriefRequest();
		m_GC_ServerReport.m_sKind = kind;
		m_GC_ServerReport.m_sSubject = subject;
		m_GC_ServerReport.m_iCursor = cursor;
		m_GC_ServerReport.m_iFilter = filter;
		m_sGC_ServerReportKey = m_GC_ServerReport.Key();
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GC_RpcAsk_ReportPage(string kind, string subject, int cursor, int filter)
	{
		GC_MissionStatsComponent stats = GC_MissionStatsComponent.GetInstance();
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!Replication.IsServer() || !stats || !mode || mode.GetState() != SCR_EGameModeState.POSTGAME)
			return;
		if ((kind != "player" && kind != "faction" && kind != "ai" && kind != "players" && kind != "combat") || subject.Length() > 80 || cursor < 0 || filter < 0 || filter > 4)
			return;
		// One slot per owning controller; queued requests never multiply server work.
		GC_SetServerReport(kind, subject, cursor, filter);
		stats.QueueReport(this);
	}

	//------------------------------------------------------------------------------------------------
	int GC_DeliverReport(GC_MissionStatsComponent stats)
	{
		if (!Replication.IsServer() || !m_GC_ServerReport || m_sGC_ServerReportKey.IsEmpty())
			return 0;
		array<string> rows;
		int next;
		string status;
		stats.ReadReport(GetPlayerId(), m_GC_ServerReport.m_sKind, m_GC_ServerReport.m_sSubject, m_GC_ServerReport.m_iCursor, m_GC_ServerReport.m_iFilter, rows, next, status);
		string key = m_sGC_ServerReportKey;
		m_sGC_ServerReportKey = string.Empty;
		int payloadSize = key.Length() + status.Length() + 64;
		foreach (string field : rows)
			payloadSize += field.Length() + 8;
		if (payloadSize > GC_MissionStatsComponent.GC_REPORT_PAYLOAD_LIMIT)
		{
			rows.Clear();
			next = -1;
			status = "Report too large";
			payloadSize = 128;
		}
		Rpc(GC_RpcDo_ReportPage, key, rows, next, status);
		return payloadSize;
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDo_ReportPage(string key, array<string> rows, int next, string status)
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!mode || mode.GetState() != SCR_EGameModeState.POSTGAME || GetGame().GetPlayerController() != this)
			return;
		GC_DebriefPage page = new GC_DebriefPage();
		page.m_aRows.Copy(rows);
		page.m_iNext = next;
		page.m_sStatus = status;
		if (!m_mGC_ReportPages.Contains(key))
		{
			if (m_aGC_ReportPageOrder.Count() == 128)
			{
				m_mGC_ReportPages.Remove(m_aGC_ReportPageOrder[0]);
				m_aGC_ReportPageOrder.RemoveOrdered(0);
			}
			m_aGC_ReportPageOrder.Insert(key);
		}
		m_mGC_ReportPages.Set(key, page);
		if (m_sGC_ActiveReportKey == key)
			m_sGC_ActiveReportKey = string.Empty;
		m_GC_OnReportPage.Invoke(key, page);
		GC_PumpReportRequests();
	}
}
