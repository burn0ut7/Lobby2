//! Geometry is a detection hint, never authority for eligibility or consequences.
class GC_BoundaryDetectionPolygon
{
	ref array<vector> m_aPoints = {};
	int m_iLastState = -1;
}

modded class GC_LobbyPlayerController
{
	protected ref array<ref GC_BoundaryDetectionPolygon> m_aGC_BoundaryPolygons = {};
	protected IEntity m_GC_BoundaryBody;
	protected IEntity m_GC_BoundaryMover;
	protected vector m_vGC_BoundaryCheckpoint;
	protected float m_fGC_BoundaryCheckpointAt;
	protected int m_iGC_BoundaryRevision = -1;
	protected int m_iGC_BoundaryRetries;
	protected float m_fGC_BoundaryRequestAt = -1000;
	protected float m_fGC_BoundaryHintAt = -1000;
	protected IEntity m_GC_EligibilityBody;
	protected SCR_InventoryStorageManagerComponent m_GC_EligibilityInventory;
	protected SCR_CompartmentAccessComponent m_GC_EligibilityCompartments;

	//------------------------------------------------------------------------------------------------
	override void OnInit(IEntity owner)
	{
		super.OnInit(owner);
		GetGame().GetCallqueue().CallLater(GC_RequestBoundaryDetection, 0);
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnOwnershipChanged(bool changing, bool becameOwner)
	{
		super.OnOwnershipChanged(changing, becameOwner);
		if (changing)
			return;
		GC_StopBoundaryDetection();
		if (becameOwner)
			GetGame().GetCallqueue().CallLater(GC_RequestBoundaryDetection, 0);
	}

	//------------------------------------------------------------------------------------------------
	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		super.OnControlledEntityChanged(from, to);
		GC_StopBoundaryDetection();
		GC_BindBoundaryEligibility();
		GC_QueueBoundaryEligibility();
		GetGame().GetCallqueue().CallLater(GC_RequestBoundaryDetection, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_RequestBoundaryDetection()
	{
		if (GetGame().GetPlayerController() != this || !GC_LobbyGameMode.GetGameMode())
			return;
		if (Replication.IsServer())
			GC_RpcAskBoundaryDetection();
		else
			Rpc(GC_RpcAskBoundaryDetection);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GC_RpcAskBoundaryDetection()
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!Replication.IsServer() || !mode || !mode.GetBoundaries())
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now - m_fGC_BoundaryRequestAt < 100)
			return;
		m_fGC_BoundaryRequestAt = now;
		mode.GetBoundaries().SendSnapshot(GetPlayerId());
		mode.GetBoundaries().Observe(GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	void GC_SendBoundaryDetection(int revision, array<int> counts, array<vector> points)
	{
		if (!Replication.IsServer())
			return;
		GC_BindBoundaryEligibility();
		if (GetGame().GetPlayerController() == this)
			GC_RpcDoBoundaryDetection(revision, counts, points);
		else
			Rpc(GC_RpcDoBoundaryDetection, revision, counts, points);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDoBoundaryDetection(int revision, array<int> counts, array<vector> points)
	{
		if (GetGame().GetPlayerController() != this || !GC_LobbyGameMode.GetGameMode() || revision < m_iGC_BoundaryRevision)
			return;
		GC_StopBoundaryDetection();
		m_aGC_BoundaryPolygons.Clear();
		int offset;
		foreach (int count : counts)
		{
			if (count < 3 || count > points.Count() - offset)
				return;
			GC_BoundaryDetectionPolygon polygon = new GC_BoundaryDetectionPolygon();
			for (int i; i < count; i++)
				polygon.m_aPoints.Insert(points[offset++]);
			m_aGC_BoundaryPolygons.Insert(polygon);
		}
		m_iGC_BoundaryRevision = revision;
		GC_UpdateBoundaryDetection();
	}

	//------------------------------------------------------------------------------------------------
	void GC_UpdateBoundaryDetection()
	{
		GetGame().GetCallqueue().Remove(GC_DetectBoundaries);
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (GetGame().GetPlayerController() != this || !mode || mode.GetState() != SCR_EGameModeState.GAME || m_aGC_BoundaryPolygons.IsEmpty())
			return;
		GetGame().GetCallqueue().CallLater(GC_DetectBoundaries, 100, true);
	}

	//------------------------------------------------------------------------------------------------
	//! Only the owning client samples its own body. Unchanged state sends nothing.
	protected void GC_DetectBoundaries()
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		IEntity body = GetControlledEntity();
		if (GetGame().GetPlayerController() != this || !mode || mode.GetState() != SCR_EGameModeState.GAME
			|| !body || body.FindComponent(GC_LobbySpectatorBodyComponent) || !SCR_AIDamageHandling.IsAlive(body))
		{
			GC_StopBoundaryDetection();
			return;
		}
		IEntity mover = CompartmentAccessComponent.GetVehicleIn(body);
		if (!mover)
			mover = body;
		bool changed = body != m_GC_BoundaryBody || mover != m_GC_BoundaryMover;
		vector position = mover.GetOrigin();
		bool near;
		foreach (GC_BoundaryDetectionPolygon polygon : m_aGC_BoundaryPolygons)
		{
			int state;
			if (Math2D.IsPointInPolygonXZ(polygon.m_aPoints, position))
				state = 1;
			int band = GC_BoundaryRuntime.ProximityBand(polygon.m_aPoints, position);
			state += 2 * band;
			near = near || band >= 2;
			changed = changed || state != polygon.m_iLastState;
			polygon.m_iLastState = state;
		}
		float now = GetGame().GetWorld().GetWorldTime();
		// Moving along an edge also needs server-observed safe history. Only report
		// substantial local movement near an edge, never a stationary heartbeat.
		if (near && now - m_fGC_BoundaryCheckpointAt >= 250 && vector.DistanceSq(position, m_vGC_BoundaryCheckpoint) >= 25)
			changed = true;
		if (GC_LobbyPlayerController.Cast(this).GC_IsSetupFrozen() && vector.DistanceSq(position, m_vGC_BoundaryCheckpoint) > 0.0025)
			changed = true;
		m_GC_BoundaryBody = body;
		m_GC_BoundaryMover = mover;
		if (!changed)
			return;
		m_vGC_BoundaryCheckpoint = position;
		m_fGC_BoundaryCheckpointAt = now;
		GetGame().GetCallqueue().Remove(GC_SendBoundaryHint);
		// Reliable delivery does not order the movement stream. Two bounded follow-ups
		// allow authority position to catch up; no permanent network heartbeat.
		m_iGC_BoundaryRetries = 2;
		GC_SendBoundaryHint();
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_SendBoundaryHint()
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		IEntity body = GetControlledEntity();
		if (GetGame().GetPlayerController() != this || !mode || mode.GetState() != SCR_EGameModeState.GAME || !body)
			return;
		RplId bodyId = Replication.FindItemId(body);
		if (Replication.IsServer())
			GC_RpcAskBoundaryHint(bodyId);
		else
			Rpc(GC_RpcAskBoundaryHint, bodyId);
		if (m_iGC_BoundaryRetries > 0)
		{
			m_iGC_BoundaryRetries--;
			GetGame().GetCallqueue().CallLater(GC_SendBoundaryHint, 250);
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GC_RpcAskBoundaryHint(RplId bodyId)
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		IEntity body = GetControlledEntity();
		if (!Replication.IsServer() || !mode || mode.GetState() != SCR_EGameModeState.GAME || !mode.GetBoundaries()
			|| !body || bodyId == RplId.Invalid() || Replication.FindItemId(body) != bodyId || GetLobbyPlayerState() != GC_ELobbyPlayerState.PLAYING)
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now - m_fGC_BoundaryHintAt < 100)
			return;
		m_fGC_BoundaryHintAt = now;
		mode.GetBoundaries().Observe(GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	//! Native authority events cover stationary filter changes, including seat swaps.
	void GC_BindBoundaryEligibility()
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		IEntity body;
		if (Replication.IsServer() && mode && mode.GetState() == SCR_EGameModeState.GAME)
			body = GetControlledEntity();
		if (body == m_GC_EligibilityBody)
			return;
		GC_UnbindBoundaryEligibility();
		m_GC_EligibilityBody = body;
		if (!body)
			return;
		m_GC_EligibilityInventory = SCR_InventoryStorageManagerComponent.Cast(body.FindComponent(SCR_InventoryStorageManagerComponent));
		if (m_GC_EligibilityInventory)
		{
			m_GC_EligibilityInventory.m_OnItemAddedInvoker.Insert(GC_BoundaryInventoryChanged);
			m_GC_EligibilityInventory.m_OnItemRemovedInvoker.Insert(GC_BoundaryInventoryChanged);
		}
		m_GC_EligibilityCompartments = SCR_CompartmentAccessComponent.Cast(body.FindComponent(SCR_CompartmentAccessComponent));
		if (m_GC_EligibilityCompartments)
		{
			m_GC_EligibilityCompartments.GetOnCompartmentEntered().Insert(GC_BoundaryCompartmentChanged);
			m_GC_EligibilityCompartments.GetOnCompartmentLeft().Insert(GC_BoundaryCompartmentChanged);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_BoundaryInventoryChanged(IEntity item, BaseInventoryStorageComponent storage)
	{
		GC_QueueBoundaryEligibility();
	}

	protected void GC_BoundaryCompartmentChanged(IEntity vehicle, BaseCompartmentManagerComponent manager, int managerId, int slotId)
	{
		GC_QueueBoundaryEligibility();
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_QueueBoundaryEligibility()
	{
		if (!Replication.IsServer())
			return;
		GetGame().GetCallqueue().Remove(GC_ObserveBoundaryEligibility);
		GetGame().GetCallqueue().CallLater(GC_ObserveBoundaryEligibility, 0);
	}

	protected void GC_ObserveBoundaryEligibility()
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (Replication.IsServer() && mode && mode.GetBoundaries() && GetGame().GetPlayerManager().GetPlayerController(GetPlayerId()) == this)
			mode.GetBoundaries().Observe(GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_UnbindBoundaryEligibility()
	{
		if (m_GC_EligibilityInventory)
		{
			m_GC_EligibilityInventory.m_OnItemAddedInvoker.Remove(GC_BoundaryInventoryChanged);
			m_GC_EligibilityInventory.m_OnItemRemovedInvoker.Remove(GC_BoundaryInventoryChanged);
		}
		if (m_GC_EligibilityCompartments)
		{
			m_GC_EligibilityCompartments.GetOnCompartmentEntered().Remove(GC_BoundaryCompartmentChanged);
			m_GC_EligibilityCompartments.GetOnCompartmentLeft().Remove(GC_BoundaryCompartmentChanged);
		}
		m_GC_EligibilityBody = null;
		m_GC_EligibilityInventory = null;
		m_GC_EligibilityCompartments = null;
		GetGame().GetCallqueue().Remove(GC_ObserveBoundaryEligibility);
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_StopBoundaryDetection()
	{
		GetGame().GetCallqueue().Remove(GC_DetectBoundaries);
		GetGame().GetCallqueue().Remove(GC_SendBoundaryHint);
		m_GC_BoundaryBody = null;
		m_GC_BoundaryMover = null;
	}

	//------------------------------------------------------------------------------------------------
	override void ShutdownLobbyPresentation()
	{
		GetGame().GetCallqueue().Remove(GC_RequestBoundaryDetection);
		GC_StopBoundaryDetection();
		m_aGC_BoundaryPolygons.Clear();
		GC_UnbindBoundaryEligibility();
		super.ShutdownLobbyPresentation();
	}
}
