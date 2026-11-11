modded class GC_LobbyPlayerController
{
	ref ScriptInvoker m_GC_OnMissionResponse = new ScriptInvoker();
	protected ref GC_AdminMissionRequest m_GC_MissionRequest;
	protected string m_sGC_MissionResponse;
	protected int m_iGC_MissionRequest;

	//------------------------------------------------------------------------------------------------
	void GC_RequestMissions(string action, string query, int offset, string mission, string launchId, int requestId)
	{
		if (GetGame().GetPlayerController() != this || !GC_LobbyManager.GetManager())
			return;
		m_iGC_MissionRequest = requestId;
		m_sGC_MissionResponse = "";
		if (Replication.IsServer())
			GC_RpcAskMissions(action, query, offset, mission, launchId, requestId);
		else
			Rpc(GC_RpcAskMissions, action, query, offset, mission, launchId, requestId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GC_RpcAskMissions(string action, string query, int offset, string mission, string launchId, int requestId)
	{
		if (!Replication.IsServer())
			return;
		if (!m_GC_MissionRequest)
			m_GC_MissionRequest = new GC_AdminMissionRequest();
		m_GC_MissionRequest.Request(this, action, query, offset, mission, launchId, requestId);
	}

	//------------------------------------------------------------------------------------------------
	void GC_SendMissionPart(int requestId, string part, string status, bool complete)
	{
		if (!Replication.IsServer())
			return;
		if (GetGame().GetPlayerController() == this)
			GC_RpcDoMissionPart(requestId, part, status, complete);
		else
			Rpc(GC_RpcDoMissionPart, requestId, part, status, complete);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDoMissionPart(int requestId, string part, string status, bool complete)
	{
		if (GetGame().GetPlayerController() != this || !GC_LobbyManager.GetManager() || requestId != m_iGC_MissionRequest)
			return;
		if (m_sGC_MissionResponse.Length() + part.Length() > 524288)
		{
			m_sGC_MissionResponse = "";
			m_GC_OnMissionResponse.Invoke(requestId, "", "Mission response is too large.");
			return;
		}
		m_sGC_MissionResponse += part;
		if (complete)
		{
			m_GC_OnMissionResponse.Invoke(requestId, m_sGC_MissionResponse, status);
			m_sGC_MissionResponse = "";
		}
	}
}
