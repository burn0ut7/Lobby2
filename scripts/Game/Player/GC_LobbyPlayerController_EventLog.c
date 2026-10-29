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
	void GC_RequestSpectatorEvents(int request, bool everyone, int before, int knownRevision = -1)
	{
		if (GC_LobbyGameMode.GetGameMode() && GetGame().GetPlayerController() == this)
			Rpc(GC_RpcAsk_SpectatorEvents, request, everyone, before, knownRevision);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GC_RpcAsk_SpectatorEvents(int request, bool everyone, int before, int knownRevision)
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
		int revision;
		bool unchanged;
		logs.ReadSpectatorPage(GetPlayerId(), everyone, before, rows, next, status, revision, unchanged, knownRevision);
		Rpc(GC_RpcDo_SpectatorEvents, request, rows, next, status, revision, unchanged);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDo_SpectatorEvents(int request, array<string> rows, int next, string status, int revision, bool unchanged)
	{
		if (GC_LobbyGameMode.GetGameMode() && GetGame().GetPlayerController() == this && GetLobbyPlayerState() == GC_ELobbyPlayerState.SPECTATOR)
			m_OnGC_SpectatorEvents.Invoke(request, rows, next, status, revision, unchanged);
	}
}
