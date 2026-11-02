//! Per-connection permission; the controller survives character replacement and respawn.
modded class GC_LobbyPlayerController
{
	[RplProp(onRplName: "GC_OnClothingPermissionChanged")]
	protected bool m_bGC_ClothingChangesAllowed;
	protected GC_LobbyManager m_GC_ClothingPermissionManager;

	//------------------------------------------------------------------------------------------------
	bool GC_AreClothingChangesAllowed()
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (!manager || (Replication.IsServer() && m_GC_ClothingPermissionManager != manager))
			return false;
		return m_bGC_ClothingChangesAllowed;
	}

	//------------------------------------------------------------------------------------------------
	bool GC_SetClothingChangesAllowed(int requesterId, bool allowed)
	{
		if (!Replication.IsServer() || !GC_LobbyManager.GetManager() || !SCR_Global.IsAdmin(requesterId))
			return false;
		m_bGC_ClothingChangesAllowed = allowed;
		m_GC_ClothingPermissionManager = GC_LobbyManager.GetManager();
		Replication.BumpMe();
		GC_OnClothingPermissionChanged();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_OnClothingPermissionChanged()
	{
		if (GetGame().GetPlayerController() != this || !GC_LobbyManager.GetManager() || !GetControlledEntity())
			return;
		SCR_CharacterInventoryStorageComponent storage = SCR_CharacterInventoryStorageComponent.Cast(GetControlledEntity().FindComponent(SCR_CharacterInventoryStorageComponent));
		if (storage)
			storage.GC_RefreshClothingPermission();
	}

	//------------------------------------------------------------------------------------------------
	//! An outgoing mode cannot clear a replacement mode's later permission.
	void GC_ReleaseClothingPermission(GC_LobbyManager manager)
	{
		if (!Replication.IsServer() || !manager || m_GC_ClothingPermissionManager != manager)
			return;
		m_bGC_ClothingChangesAllowed = false;
		m_GC_ClothingPermissionManager = null;
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	void GC_RequestForcedClothing(GC_EAdminHealScope scope, array<int> players, array<string> identities, int requestId, int radiusMeters, string factionKey, bool allowed)
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this)
			Rpc(GC_RpcAskForcedClothing, scope, players, identities, requestId, radiusMeters, factionKey, allowed);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GC_RpcAskForcedClothing(GC_EAdminHealScope scope, array<int> players, array<string> identities, int requestId, int radiusMeters, string factionKey, bool allowed)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		string result = "Admin tools are unavailable in this mode.";
		if (manager)
			result = manager.GC_ExecuteForcedClothing(GetPlayerId(), scope, players, identities, requestId, radiusMeters, factionKey, allowed);
		else
			GC_AdminActionLogger.Log(GetPlayerId(), "forced-clothing", "", "rejected-inactive-mode");
		GC_LobbyPlayerController.Cast(this).GC_SendPlayerToolResult(requestId, result);
	}

	//------------------------------------------------------------------------------------------------
	override void ShutdownLobbyPresentation()
	{
		m_bGC_ClothingChangesAllowed = false;
		m_GC_ClothingPermissionManager = null;
		super.ShutdownLobbyPresentation();
	}
}
