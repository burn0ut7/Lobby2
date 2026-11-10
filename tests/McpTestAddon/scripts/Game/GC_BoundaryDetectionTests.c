//! Explicit production-path instrumentation. No automatic fixtures or permission grants.
class GC_BoundaryDetectionProbe
{
	static int s_iObservations;
	static int s_iBodies;
	static int s_iMaximumBatch;
	static int s_iClientSamples;
	static int s_iHints;
	static int s_iAudits;
}

modded class GC_BoundaryRuntime
{
	override protected void Process(array<int> players)
	{
		GC_BoundaryDetectionProbe.s_iObservations++;
		super.Process(players);
		GC_BoundaryDetectionProbe.s_iBodies += players.Count();
		GC_BoundaryDetectionProbe.s_iMaximumBatch = Math.Max(GC_BoundaryDetectionProbe.s_iMaximumBatch, players.Count());
	}

	override protected void Audit()
	{
		GC_BoundaryDetectionProbe.s_iAudits++;
		super.Audit();
	}

	string GC_TestDetectionState()
	{
		return string.Format("polygons=%1 active=%2 fastScheduled=%3 auditCursor=%4 auditRoster=%5 stopped=%6",
			m_aCounts.Count(), m_aActive.Count(), m_bActiveScheduled, m_iAuditCursor, m_aAudit.Count(), m_bStopped);
	}
}

modded class GC_LobbyPlayerController
{
	override protected void GC_DetectBoundaries()
	{
		GC_BoundaryDetectionProbe.s_iClientSamples++;
		super.GC_DetectBoundaries();
	}

	override protected void GC_SendBoundaryHint()
	{
		GC_BoundaryDetectionProbe.s_iHints++;
		super.GC_SendBoundaryHint();
	}

	string GC_TestBoundaryClient(bool enabled)
	{
		if (enabled)
			GC_UpdateBoundaryDetection();
		else
			GC_StopBoundaryDetection();
		return string.Format("clientEnabled=%1 revision=%2 polygons=%3 inventoryBound=%4 compartmentsBound=%5",
			enabled, m_iGC_BoundaryRevision, m_aGC_BoundaryPolygons.Count(), m_GC_EligibilityInventory != null, m_GC_EligibilityCompartments != null);
	}

	bool GC_TestRejectBoundaryBody()
	{
		int before = GC_BoundaryDetectionProbe.s_iObservations;
		GC_RpcAskBoundaryHint(RplId.Invalid());
		return before == GC_BoundaryDetectionProbe.s_iObservations;
	}
}

modded class GC_LobbyManager
{
	//! Uses a removable native item, rather than non-removable identity equipment.
	string GC_TestBoundaryInventory(string action)
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!Replication.IsServer() || !player || !player.GetControlledEntity() || m_aGC_AOFixtures.IsEmpty())
			return "UNAVAILABLE player/fixture";
		SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(player.GetControlledEntity().FindComponent(SCR_InventoryStorageManagerComponent));
		array<IEntity> items = {};
		if (inventory)
			inventory.GetItems(items);
		int removed;
		foreach (IEntity item : items)
		{
			ResourceName prefab = item.GetPrefabData().GetPrefabName();
			if (!prefab.Contains("FieldDressing"))
				continue;
			if (action == "filter")
				return "configured=" + m_aGC_AOFixtures[0].SetEligibility({}, GC_EAOVehicleState.ANY, GC_EAOListMode.IGNORE, {}, GC_EAOItemCondition.HAS_NONE, {prefab});
			InventoryItemComponent component = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
			if (component && component.GetParentSlot() && inventory.TryRemoveItemFromInventory(item, component.GetParentSlot().GetStorage()))
				removed++;
		}
		return "removed=" + removed;
	}

	string GC_TestBoundaryDetection(string action)
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		GC_LobbyPlayerController player = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!mode || !mode.GetBoundaries() || !player)
			return "UNAVAILABLE mode/authority/player";
		if (action == "reset")
		{
			GC_BoundaryDetectionProbe.s_iObservations = 0;
			GC_BoundaryDetectionProbe.s_iBodies = 0;
			GC_BoundaryDetectionProbe.s_iMaximumBatch = 0;
			GC_BoundaryDetectionProbe.s_iClientSamples = 0;
			GC_BoundaryDetectionProbe.s_iHints = 0;
			GC_BoundaryDetectionProbe.s_iAudits = 0;
		}
		if (action == "client-off")
			return player.GC_TestBoundaryClient(false);
		if (action == "client-on")
			return player.GC_TestBoundaryClient(true);
		if (action == "reject")
			return "invalidBodyRejected=" + player.GC_TestRejectBoundaryBody();
		return mode.GetBoundaries().GC_TestDetectionState() + string.Format(" observations=%1 bodies=%2 maxBatch=%3 clientSamples=%4 hints=%5 audits=%6",
			GC_BoundaryDetectionProbe.s_iObservations, GC_BoundaryDetectionProbe.s_iBodies, GC_BoundaryDetectionProbe.s_iMaximumBatch,
			GC_BoundaryDetectionProbe.s_iClientSamples, GC_BoundaryDetectionProbe.s_iHints, GC_BoundaryDetectionProbe.s_iAudits);
	}
}
