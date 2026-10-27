modded class GC_LobbyPlayerController
{
	ref ScriptInvoker m_OnSessionLogRows = new ScriptInvoker();
	protected float m_fLogRequestTime = -1000;
	protected float m_fLogDenialTime = -5000;
	protected bool m_bLogAccessAudited;
	protected float m_fGC_ViewDenialTime = -5000;

	//------------------------------------------------------------------------------------------------
	void GC_RequestAdminViewOpen(bool logsView)
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this)
			Rpc(GC_RpcAsk_AdminViewOpen, logsView);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GC_RpcAsk_AdminViewOpen(bool logsView)
	{
		if (!Replication.IsServer() || !GC_SessionLogComponent.GetInstance())
			return;
		if (!SCR_Global.IsAdmin(GetPlayerId()))
		{
			float now = GetGame().GetWorld().GetWorldTime();
			if (now - m_fGC_ViewDenialTime >= 5000)
			{
				m_fGC_ViewDenialTime = now;
				GC_AdminActionLogger.Log(GetPlayerId(), "admin-view", "open", "rejected: not authenticated");
			}
			return;
		}
		if (logsView)
			// Audit the next authorized read, once per actual view opening, not per poll.
			m_bLogAccessAudited = false;
		else
			GC_AdminActionLogger.Log(GetPlayerId(), "admin-tools", "workspace", "view opened");
	}

	//------------------------------------------------------------------------------------------------
	void RequestSessionLog(int requestId, string query, string type, int before)
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this)
			Rpc(RpcAsk_SessionLog, requestId, query, type, before);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SessionLog(int requestId, string query, string type, int before)
	{
		GC_SessionLogComponent logs = GC_SessionLogComponent.GetInstance();
		if (!Replication.IsServer() || !logs)
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		if (!SCR_Global.IsAdmin(GetPlayerId()))
		{
			// Preserve the first denial without letting request spam flood the archive.
			if (now - m_fLogDenialTime >= 5000)
			{
				m_fLogDenialTime = now;
				GC_AdminActionLogger.Log(GetPlayerId(), "session-log", "current mission", "rejected: not authenticated");
			}
			m_bLogAccessAudited = false;
			return;
		}
		if (now - m_fLogRequestTime < 250)
		{
			array<string> empty = {};
			Rpc(RpcDo_SessionLog, requestId, empty, -1, "Please wait a moment, then refresh.");
			return;
		}
		m_fLogRequestTime = now;
		if (!m_bLogAccessAudited)
		{
			GC_AdminActionLogger.Log(GetPlayerId(), "session-log", "current mission", "view opened");
			m_bLogAccessAudited = true;
		}
		array<string> rows;
		int next;
		string status;
		logs.ReadPage(GetPlayerId(), query, type, before, rows, next, status);
		if (SCR_Global.IsAdmin(GetPlayerId()))
			Rpc(RpcDo_SessionLog, requestId, rows, next, status);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_SessionLog(int requestId, array<string> rows, int next, string status)
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this && SCR_Global.IsAdmin(GetPlayerId()))
			m_OnSessionLogRows.Invoke(requestId, rows, next, status);
	}
}
