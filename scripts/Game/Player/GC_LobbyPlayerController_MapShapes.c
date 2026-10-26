//! Authority publishes permitted shape changes; the local owner keeps presentation state.
modded class GC_LobbyPlayerController
{
	protected ref GC_MapShapeOverlay m_GC_MapShapes;
	protected int m_iGC_MapRevision = -1;
	protected string m_sGC_MapFaction;
	protected bool m_bGC_MapAdmin;
	protected bool m_bGC_MapSubscribed;
	protected float m_fGC_MapSubscribeAt;

	//------------------------------------------------------------------------------------------------
	override protected void OnInit(IEntity owner)
	{
		super.OnInit(owner);
		// Remote owners subscribe after ownership is established, when owner RPCs are ready.
		if (GetGame().InPlayMode() && Replication.IsServer() && RplSession.Mode() != RplMode.Dedicated)
			GetGame().GetCallqueue().CallLater(GC_InitMapShapes, 0);
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnOwnershipChanged(bool changing, bool becameOwner)
	{
		super.OnOwnershipChanged(changing, becameOwner);
		if (changing)
			return;
		if (becameOwner)
			GetGame().GetCallqueue().CallLater(GC_InitMapShapes, 0);
		else if (m_GC_MapShapes)
		{
			GC_MapClosed(null);
			SCR_MapEntity.GetOnMapOpenComplete().Remove(GC_MapOpened);
			SCR_MapEntity.GetOnMapClose().Remove(GC_MapClosed);
			m_GC_MapShapes = null;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_InitMapShapes()
	{
		if (m_GC_MapShapes || GetGame().GetPlayerController() != this || !GC_LobbyGameMode.GetGameMode())
			return;
		m_GC_MapShapes = new GC_MapShapeOverlay();
		SCR_MapEntity.GetOnMapOpenComplete().Insert(GC_MapOpened);
		SCR_MapEntity.GetOnMapClose().Insert(GC_MapClosed);
		GC_RequestMapShapes(0);
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (mapEntity && mapEntity.IsOpen())
			GC_MapOpened(null);
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_MapOpened(MapConfiguration config)
	{
		if (!m_GC_MapShapes || GetGame().GetPlayerController() != this || !GC_LobbyGameMode.GetGameMode())
			return;
		GetGame().GetCallqueue().Remove(GC_UpdateMapShapes);
		GetGame().GetCallqueue().CallLater(GC_UpdateMapShapes, 16, true);
		GC_UpdateMapShapes();
		m_GC_MapShapes.OnMapReady(config);
		GC_OpenMapBriefing(config);
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_MapClosed(MapConfiguration config)
	{
		GC_CloseMapBriefing();
		GetGame().GetCallqueue().Remove(GC_UpdateMapShapes);
		if (m_GC_MapShapes)
			m_GC_MapShapes.Close();
	}

	//------------------------------------------------------------------------------------------------
	override void ShutdownLobbyPresentation()
	{
		GetGame().GetCallqueue().Remove(GC_InitMapShapes);
		GetGame().GetCallqueue().Remove(GC_UpdateMapShapes);
		GetGame().GetCallqueue().Remove(GC_PublishMapChanges);
		SCR_MapEntity.GetOnMapOpenComplete().Remove(GC_MapOpened);
		SCR_MapEntity.GetOnMapClose().Remove(GC_MapClosed);
		if (m_GC_MapShapes)
			m_GC_MapShapes.Close();
		m_GC_MapShapes = null;
		m_bGC_MapSubscribed = false;
		super.ShutdownLobbyPresentation();
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_UpdateMapShapes()
	{
		if (GetGame().GetPlayerController() != this || !GC_LobbyGameMode.GetGameMode())
			return;
		if (m_GC_MapShapes)
			m_GC_MapShapes.Update(this);
	}

	//------------------------------------------------------------------------------------------------
	//! Initial owner subscription, not a recurring geometry request.
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
		if (!Replication.IsServer() || !GC_LobbyGameMode.GetGameMode())
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now < m_fGC_MapSubscribeAt)
			return;
		m_fGC_MapSubscribeAt = now + 250;
		if (!m_bGC_MapSubscribed)
			GetGame().GetCallqueue().CallLater(GC_PublishMapChanges, 500, true);
		m_bGC_MapSubscribed = true;
		m_iGC_MapRevision = -1;
		GC_PublishMapChanges();
	}

	//------------------------------------------------------------------------------------------------
	//! Recheck live faction/admin visibility cheaply; serialize only on a change.
	protected void GC_PublishMapChanges()
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (!Replication.IsServer() || !mode || !manager || !mode.GetMapShapes())
			return;
		Faction faction = manager.GetPlayerFaction(GetPlayerId());
		string factionKey;
		if (faction)
			factionKey = faction.GetFactionKey();
		bool showAdmin = GC_ShowBoundaryMap();
		int revision = mode.GetMapShapes().GetRevision();
		if (revision == m_iGC_MapRevision && factionKey == m_sGC_MapFaction && showAdmin == m_bGC_MapAdmin)
			return;
		m_iGC_MapRevision = revision;
		m_sGC_MapFaction = factionKey;
		m_bGC_MapAdmin = showAdmin;
		array<int> counts = {}, colors = {}, fills = {};
		array<bool> inverted = {};
		array<float> widths = {};
		array<vector> points = {};
		array<string> names = {};
		array<vector> frameBounds = {};
		mode.GetMapShapes().GetSnapshot(GetPlayerId(), counts, colors, fills, inverted, widths, points, names, frameBounds);
		JsonSaveContext metadata = new JsonSaveContext();
		metadata.WriteValue("names", names);
		metadata.WriteValue("frameBounds", frameBounds);
		string presentation = metadata.SaveToString();
		if (GetGame().GetPlayerController() == this)
			GC_RpcDo_MapShapes(revision, counts, colors, fills, inverted, widths, points, presentation);
		else
			Rpc(GC_RpcDo_MapShapes, revision, counts, colors, fills, inverted, widths, points, presentation);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDo_MapShapes(int generation, array<int> counts, array<int> colors, array<int> fills, array<bool> inverted, array<float> widths, array<vector> points, string presentation)
	{
		if (GetGame().GetPlayerController() != this || !GC_LobbyGameMode.GetGameMode() || !m_GC_MapShapes)
			return;
		array<string> names = {};
		array<vector> frameBounds = {};
		JsonLoadContext metadata = new JsonLoadContext(false);
		if (!metadata.LoadFromString(presentation) || !metadata.ReadValue("names", names) || !metadata.ReadValue("frameBounds", frameBounds))
			return;
		m_GC_MapShapes.Receive(generation, counts, colors, fills, inverted, widths, points, names, frameBounds);
	}
}
