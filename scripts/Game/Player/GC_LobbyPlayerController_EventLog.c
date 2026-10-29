modded class GC_LobbyPlayerController
{
	ref ScriptInvoker m_OnGC_SpectatorEvents = new ScriptInvoker();
	protected float m_fGC_EventLogRequestTime = -1000;
	protected int m_iGC_EventLogRequest;

	//------------------------------------------------------------------------------------------------
	int GC_ReserveSpectatorEventRequest()
	{
		return ++m_iGC_EventLogRequest;
	}

	//------------------------------------------------------------------------------------------------
	void GC_RequestSpectatorEvents(int request, bool everyone, int before)
	{
		if (GC_LobbyGameMode.GetGameMode() && GetGame().GetPlayerController() == this)
			Rpc(GC_RpcAsk_SpectatorEvents, request, everyone, before);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GC_RpcAsk_SpectatorEvents(int request, bool everyone, int before)
	{
		GC_SessionLogComponent logs = GC_SessionLogComponent.GetInstance();
		if (!Replication.IsServer() || !logs)
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now - m_fGC_EventLogRequestTime < 250)
			return;
		m_fGC_EventLogRequestTime = now;
		array<string> rows;
		int next;
		string status;
		logs.ReadSpectatorPage(GetPlayerId(), everyone, before, rows, next, status);
		Rpc(GC_RpcDo_SpectatorEvents, request, rows, next, status);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDo_SpectatorEvents(int request, array<string> rows, int next, string status)
	{
		if (GC_LobbyGameMode.GetGameMode() && GetGame().GetPlayerController() == this && GetLobbyPlayerState() == GC_ELobbyPlayerState.SPECTATOR)
			m_OnGC_SpectatorEvents.Invoke(request, rows, next, status);
	}
}
