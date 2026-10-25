modded class GC_LobbyPlayerController
{
	protected ref GC_MapShapeOverlay m_GC_MapShapes;
	protected float m_GC_fMapShapesRequestAt;

	//------------------------------------------------------------------------------------------------
	override protected void OnInit(IEntity owner)
	{
		super.OnInit(owner);
		if (GetGame().InPlayMode() && RplSession.Mode() != RplMode.Dedicated && GC_LobbyGameMode.GetGameMode())
			GetGame().GetCallqueue().CallLater(GC_UpdateMapShapes, 16, true);
	}

	//------------------------------------------------------------------------------------------------
	override void ShutdownLobbyPresentation()
	{
		GetGame().GetCallqueue().Remove(GC_UpdateMapShapes);
		if (m_GC_MapShapes)
			m_GC_MapShapes.Close();
		m_GC_MapShapes = null;
		super.ShutdownLobbyPresentation();
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_UpdateMapShapes()
	{
		if (GetGame().GetPlayerController() != this || !GC_LobbyGameMode.GetGameMode())
			return;
		if (!m_GC_MapShapes)
			m_GC_MapShapes = new GC_MapShapeOverlay();
		m_GC_MapShapes.Update(this);
	}

	//------------------------------------------------------------------------------------------------
	void GC_RequestMapShapes(int generation)
	{
		if (GetGame().GetPlayerController() != this || !GC_LobbyGameMode.GetGameMode())
			return;
		if (Replication.IsServer())
			GC_RpcAsk_MapShapes(generation);
		else
			Rpc(GC_RpcAsk_MapShapes, generation);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GC_RpcAsk_MapShapes(int generation)
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!Replication.IsServer() || !mode || !mode.GetMapShapes())
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now < m_GC_fMapShapesRequestAt)
			return;
		m_GC_fMapShapesRequestAt = now + 250;
		array<int> counts = {}, colors = {}, fills = {};
		array<bool> inverted = {};
		array<float> widths = {};
		array<vector> points = {};
		array<string> names = {};
		mode.GetMapShapes().GetSnapshot(GetPlayerId(), counts, colors, fills, inverted, widths, points, names);
		if (GetGame().GetPlayerController() == this)
			GC_RpcDo_MapShapes(generation, counts, colors, fills, inverted, widths, points, names);
		else
			Rpc(GC_RpcDo_MapShapes, generation, counts, colors, fills, inverted, widths, points, names);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDo_MapShapes(int generation, array<int> counts, array<int> colors, array<int> fills, array<bool> inverted, array<float> widths, array<vector> points, array<string> names)
	{
		if (GetGame().GetPlayerController() != this || !GC_LobbyGameMode.GetGameMode() || !m_GC_MapShapes)
			return;
		m_GC_MapShapes.Receive(generation, counts, colors, fills, inverted, widths, points, names);
	}
}
