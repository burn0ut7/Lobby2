//! Client presentation ownership; every live creation is separately authorized by the server.
class GC_GearSession
{
	protected static IEntity s_Owner;
	protected static SCR_BaseGameMode s_Mode;
	protected static GC_LobbyPlayerController s_AdminController;
	protected static int s_iRevision;
	static bool s_bCrate;

	//------------------------------------------------------------------------------------------------
	static int GetRevision()
	{
		return s_iRevision;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsWorkbench(IEntity owner)
	{
#ifdef WORKBENCH
		return !s_Owner && GC_WorkbenchItemBrowser.IsActive(owner);
#else
		return false;
#endif
	}

	//------------------------------------------------------------------------------------------------
	static bool IsAdmin(IEntity owner)
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!s_Owner || s_Mode != GC_LobbyGameMode.GetGameMode() || !s_Mode || !controller || controller.GetControlledEntity() != s_Owner || !SCR_Global.IsAdmin(controller.GetPlayerId()))
		{
			Close();
			return false;
		}
		return owner == s_Owner;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsActive(IEntity owner)
	{
		return IsAdmin(owner) || IsWorkbench(owner);
	}

	//------------------------------------------------------------------------------------------------
	static bool Open()
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller || !GC_LobbyGameMode.GetGameMode() || !SCR_Global.IsAdmin(controller.GetPlayerId()))
			return false;
		IEntity owner = controller.GetControlledEntity();
		if (!owner)
			return false;
		SCR_InventoryStorageManagerComponent manager = SCR_InventoryStorageManagerComponent.Cast(owner.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!manager)
			return false;
#ifdef WORKBENCH
		GC_WorkbenchItemBrowser.Disable();
#endif
		s_Owner = owner;
		s_AdminController = GC_LobbyPlayerController.Cast(controller);
		s_iRevision++;
		s_Mode = GC_LobbyGameMode.GetGameMode();
		s_bCrate = false;
		GC_AdminToolsComponent.Close();
		manager.OpenInventory();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	static void Close()
	{
		if (s_Owner)
			s_iRevision++;
		if (s_AdminController)
			s_AdminController.GC_RequestGearClosed();
		s_AdminController = null;
		s_Owner = null;
		s_Mode = null;
		s_bCrate = false;
	}

	//------------------------------------------------------------------------------------------------
	static void Disable()
	{
		Close();
#ifdef WORKBENCH
		GC_WorkbenchItemBrowser.Disable();
#endif
	}
}
