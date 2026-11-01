#ifdef WORKBENCH
//! Prefab file access is independent of the inventory catalogue's transfer session.
class GC_PrefabAuthoringSession
{
	protected static IEntity s_Owner;
	protected static ResourceName s_sEditingPrefab;

	//------------------------------------------------------------------------------------------------
	static bool IsAvailable(IEntity owner)
	{
		PlayerController controller = GetGame().GetPlayerController();
		return owner && controller && controller.GetControlledEntity() == owner
			&& GC_LobbyGameMode.GetGameMode() && Workbench.GetModule(WorldEditor)
			&& owner.FindComponent(SCR_InventoryStorageManagerComponent);
	}

	//------------------------------------------------------------------------------------------------
	static ResourceName GetEditingPrefab()
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller || !IsAvailable(controller.GetControlledEntity()))
			return string.Empty;
		IEntity owner = controller.GetControlledEntity();
		if (s_Owner != owner)
		{
			s_Owner = owner;
			s_sEditingPrefab = SCR_ResourceNameUtils.GetPrefabName(owner);
		}
		return s_sEditingPrefab;
	}

	//------------------------------------------------------------------------------------------------
	static void SetEditingPrefab(ResourceName prefab)
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller || !IsAvailable(controller.GetControlledEntity()))
			return;
		s_Owner = controller.GetControlledEntity();
		s_sEditingPrefab = prefab;
	}

	//------------------------------------------------------------------------------------------------
	static bool LoadPrefab(ResourceName prefab)
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller || !IsAvailable(controller.GetControlledEntity()) || RplSession.Mode() != RplMode.None)
			return false;
		if (!GC_WorkbenchItemBrowser.IsActive(controller.GetControlledEntity()) && !GC_WorkbenchItemBrowser.Open(false))
			return false;
		if (!GC_WorkbenchItemBrowser.LoadPrefab(prefab))
			return false;
		SetEditingPrefab(prefab);
		return true;
	}
}
#endif
