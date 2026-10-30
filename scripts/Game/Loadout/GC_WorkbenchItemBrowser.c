#ifdef WORKBENCH
//! Explicit local development session. Never enabled by loading the addon or opening Tab.
class GC_WorkbenchItemBrowser
{
	protected static IEntity s_Owner;
	protected static SCR_BaseGameMode s_Mode;
	protected static SCR_PlayerController s_Controller;

	//------------------------------------------------------------------------------------------------
	static bool Open()
	{
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (RplSession.Mode() != RplMode.None || !GC_LobbyGameMode.GetGameMode() || !controller)
			return false;
		IEntity owner = controller.GetControlledEntity();
		if (!owner)
			return false;
		SCR_InventoryStorageManagerComponent manager = SCR_InventoryStorageManagerComponent.Cast(owner.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!manager)
			return false;
		Disable();
		s_Owner = owner;
		s_Mode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		s_Controller = controller;
		s_Controller.m_OnControlledEntityChanged.Insert(OnOwnerChanged);
		s_Mode.GetOnGameEnd().Insert(Disable);
		manager.OpenInventory();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsActive(IEntity owner)
	{
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!s_Owner || !s_Mode || RplSession.Mode() != RplMode.None || !GC_LobbyGameMode.GetGameMode() || GetGame().GetGameMode() != s_Mode || !controller || controller.GetControlledEntity() != s_Owner)
		{
			Disable();
			return false;
		}
		return owner == s_Owner;
	}

	//------------------------------------------------------------------------------------------------
	static void Disable()
	{
		if (s_Controller)
			s_Controller.m_OnControlledEntityChanged.Remove(OnOwnerChanged);
		if (s_Mode)
			s_Mode.GetOnGameEnd().Remove(Disable);
		s_Controller = null;
		s_Owner = null;
		s_Mode = null;
	}
	//------------------------------------------------------------------------------------------------
	protected static void OnOwnerChanged(IEntity from, IEntity to)
	{
		if (to != s_Owner)
			Disable();
	}
}
#endif
