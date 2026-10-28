modded class GC_LobbyPlayerController
{
	ref ScriptInvoker m_OnGC_ServerStatus = new ScriptInvoker();
	protected int m_iGC_StatusSampleTime = -5000;
	protected int m_iGC_StatusDenialTime = -5000;
	protected bool m_bGC_StatusAudited;
	protected ref array<int> m_aGC_StatusValues = {};

	//------------------------------------------------------------------------------------------------
	void GC_RequestServerStatus(int requestId)
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this)
			Rpc(GC_RpcAsk_ServerStatus, requestId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GC_RpcAsk_ServerStatus(int requestId)
	{
		if (!Replication.IsServer() || !GC_LobbyManager.GetManager())
			return;
		int now = System.GetTickCount();
		if (!SCR_Global.IsAdmin(GetPlayerId()))
		{
			if (now - m_iGC_StatusDenialTime >= 5000)
			{
				m_iGC_StatusDenialTime = now;
				GC_AdminActionLogger.Log(GetPlayerId(), "server-status", "read", "rejected: not authenticated");
			}
			m_bGC_StatusAudited = false;
			return;
		}
		// Rate limit each requesting owner before collecting even the inexpensive counters.
		if (now - m_iGC_StatusSampleTime < 250)
		{
			GC_SendServerStatus(requestId);
			return;
		}
		m_iGC_StatusSampleTime = now;
		if (!m_bGC_StatusAudited)
		{
			GC_AdminActionLogger.Log(GetPlayerId(), "server-status", "read", "view opened");
			m_bGC_StatusAudited = true;
		}
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		int admins;
		foreach (int playerId : players)
		{
			if (SCR_Global.IsAdmin(playerId))
				admins++;
		}
		int activeAI = -1;
		if (GetGame().GetAIWorld())
			activeAI = GetGame().GetAIWorld().GetCurrentNumOfActiveAIs();
		// RPC arguments must exactly match the receiver's types; GetFPS returns float.
		int fps = Math.Round(System.GetFPS());
		m_aGC_StatusValues = {players.Count(), admins, fps, now / 1000, activeAI};
		GC_SendServerStatus(requestId);
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_SendServerStatus(int requestId)
	{
		if (m_aGC_StatusValues.Count() != 5)
			return;
		Rpc(GC_RpcDo_ServerStatus, requestId, m_aGC_StatusValues[0], m_aGC_StatusValues[1],
			m_aGC_StatusValues[2], m_aGC_StatusValues[3], m_aGC_StatusValues[4]);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDo_ServerStatus(int requestId, int players, int admins, int fps, int uptime, int activeAI)
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this && SCR_Global.IsAdmin(GetPlayerId()))
			m_OnGC_ServerStatus.Invoke(requestId, players, admins, fps, uptime, activeAI);
	}
}
