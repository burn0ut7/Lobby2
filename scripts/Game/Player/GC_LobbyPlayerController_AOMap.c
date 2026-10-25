modded class GC_LobbyPlayerController
{
	protected ref GC_AOMapOverlay m_GC_AOMap;
	protected float m_GC_fAOMapRequestAt;

	//------------------------------------------------------------------------------------------------
	override protected void OnInit(IEntity owner)
	{
		super.OnInit(owner);
		if (GetGame().InPlayMode() && RplSession.Mode() != RplMode.Dedicated && GC_LobbyGameMode.GetGameMode())
			GetGame().GetCallqueue().CallLater(GC_UpdateAOMap, 16, true);
	}

	//------------------------------------------------------------------------------------------------
	override void ShutdownLobbyPresentation()
	{
		GetGame().GetCallqueue().Remove(GC_UpdateAOMap);
		if (m_GC_AOMap)
			m_GC_AOMap.Close();
		m_GC_AOMap = null;
		super.ShutdownLobbyPresentation();
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_UpdateAOMap()
	{
		if (GetGame().GetPlayerController() != this || !GC_LobbyGameMode.GetGameMode())
			return;
		if (!m_GC_AOMap)
			m_GC_AOMap = new GC_AOMapOverlay();
		m_GC_AOMap.Update(this);
	}

	//------------------------------------------------------------------------------------------------
	void GC_RequestAOMap(int generation)
	{
		if (GetGame().GetPlayerController() != this || !GC_LobbyGameMode.GetGameMode())
			return;
		if (Replication.IsServer())
			GC_RpcAsk_AOMap(generation);
		else
			Rpc(GC_RpcAsk_AOMap, generation);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GC_RpcAsk_AOMap(int generation)
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!Replication.IsServer() || !mode || !mode.GetAOLimits())
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now < m_GC_fAOMapRequestAt)
			return;
		m_GC_fAOMapRequestAt = now + 250;
		array<int> counts = {}, colors = {};
		array<float> widths = {};
		array<vector> points = {};
		mode.GetAOLimits().GetMapOutlines(GetPlayerId(), counts, colors, widths, points);
		if (GetGame().GetPlayerController() == this)
			GC_RpcDo_AOMap(generation, counts, colors, widths, points);
		else
			Rpc(GC_RpcDo_AOMap, generation, counts, colors, widths, points);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDo_AOMap(int generation, array<int> counts, array<int> colors, array<float> widths, array<vector> points)
	{
		if (GetGame().GetPlayerController() != this || !GC_LobbyGameMode.GetGameMode() || !m_GC_AOMap)
			return;
		m_GC_AOMap.Receive(generation, counts, colors, widths, points);
	}
}
