//! Session-local admin input lock. Authority separately holds eligible character/vehicle bodies.
modded class GC_LobbyPlayerController
{
	protected bool m_bGC_AdminControlsFrozen;

	//------------------------------------------------------------------------------------------------
	bool GC_AreAdminControlsFrozen()
	{
		return GC_LobbyGameMode.GetGameMode() && m_bGC_AdminControlsFrozen && !SCR_Global.IsAdmin(GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	void GC_RequestAdminControlFreeze(GC_EAdminHealScope scope, array<int> players, array<string> identities, int requestId, int radiusMeters, string factionKey, bool frozen)
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this)
			Rpc(GC_RpcAskAdminControlFreeze, scope, players, identities, requestId, radiusMeters, factionKey, frozen);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GC_RpcAskAdminControlFreeze(GC_EAdminHealScope scope, array<int> players, array<string> identities, int requestId, int radiusMeters, string factionKey, bool frozen)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		string result = "Admin tools are unavailable in this mode.";
		if (manager)
			result = manager.GC_ExecuteAdminControlFreeze(GetPlayerId(), scope, players, identities, requestId, radiusMeters, factionKey, frozen);
		else
			GC_AdminActionLogger.Log(GetPlayerId(), "control-freeze", "", "rejected-inactive-mode");
		GC_LobbyPlayerController.Cast(this).GC_SendPlayerToolResult(requestId, result);
	}

	//------------------------------------------------------------------------------------------------
	//! Authority has already resolved and authenticated the audience.
	bool GC_SetAdminControlsFrozen(int requesterId, bool frozen)
	{
		if (!Replication.IsServer() || !GC_LobbyManager.GetManager() || !SCR_Global.IsAdmin(requesterId))
			return false;
		if (frozen && SCR_Global.IsAdmin(GetPlayerId()))
			return false;
		m_bGC_AdminControlsFrozen = frozen;
		Rpc(GC_RpcDoAdminControlFreeze, frozen);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDoAdminControlFreeze(bool frozen)
	{
		if (GetGame().GetPlayerController() == this && GC_LobbyManager.GetManager())
			m_bGC_AdminControlsFrozen = frozen;
	}

	//------------------------------------------------------------------------------------------------
	override void OnUpdate(float timeSlice)
	{
		super.OnUpdate(timeSlice);
		if (GetGame().GetPlayerController() != this || !GC_AreAdminControlsFrozen())
			return;
		// One-frame context ownership: unfreezing or leaving the mode stops activation.
		// This is an input-manager rule, not a menu/widget. Preserve pause and voice
		// events while preventing gameplay listeners from receiving blocked actions.
		InputManager input = GetGame().GetInputManager();
		if (input)
			input.ActivateContext("GC_AdminControlFreeze");
	}

	//------------------------------------------------------------------------------------------------
	override void ShutdownLobbyPresentation()
	{
		m_bGC_AdminControlsFrozen = false;
		super.ShutdownLobbyPresentation();
	}
}
