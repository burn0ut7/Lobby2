#ifdef WORKBENCH
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestAdminGear(string action, string prefab)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return "no controller";
		if (action == "menu")
		{
			controller.GetControlledEntity().SetOrigin("120 1 120");
			SCR_InventoryMenuUI inventory = SCR_InventoryMenuUI.GetInventoryMenu();
			if (inventory)
				inventory.Close();
			GC_AdminToolsComponent.Open(-1, false);
		}
		else if (action == "open")
		{
			MenuBase menu = GetGame().GetMenuManager().FindMenuByPreset(ChimeraMenuPreset.GC_AdminToolsMenu);
			if (!menu)
				return "admin menu missing";
			Widget button = menu.GetRootWidget().FindAnyWidget("ToolGear");
			SCR_ButtonBaseComponent.Cast(button.FindHandler(SCR_ButtonBaseComponent)).m_OnClicked.Invoke(SCR_ButtonBaseComponent.Cast(button.FindHandler(SCR_ButtonBaseComponent)));
		}
		else if (action == "create" || action == "nearby" || action == "inventory")
		{
			SCR_InventoryMenuUI menu = SCR_InventoryMenuUI.GetInventoryMenu();
			if (!menu)
				return "inventory missing";
			int index;
			if (action == "nearby")
				index = 1;
			if (action == "create")
				index = 2;
			SCR_ComboBoxComponent combo = SCR_ComboBoxComponent.Cast(menu.GetRootWidget().FindAnyWidget("GearDestination").FindHandler(SCR_ComboBoxComponent));
			combo.SetCurrentItem(index, false, false, true);
			combo.m_OnChanged.Invoke(combo, index);
		}
		else if (action == "spawn")
			controller.GC_RequestGearItem(prefab, null, -1, true);
		else if (action == "invalid")
			controller.GC_TestGearInvalid(prefab);
		else if (action == "far")
			controller.GetControlledEntity().SetOrigin("140 1 120");
		else if (action == "return")
			controller.GetControlledEntity().SetOrigin("120 1 120");
		else if (action == "denied")
		{
			GC_LobbyPlayerController unresolved = GC_LobbyPlayerController.Cast(GetGame().SpawnEntityPrefab(Resource.Load("{FE3C731AAB2FB003}Prefabs/Characters/Core/LobbyPlayerController.et"), GetGame().GetWorld()));
			if (!unresolved || unresolved.GetPlayerId() > 0)
				return "invalid rejection fixture";
			unresolved.GC_TestGearDenied(prefab);
			SCR_EntityHelper.DeleteEntityAndChildren(unresolved);
		}
		else if (action == "close")
			SCR_InventoryMenuUI.GetInventoryMenu().Close();
		return controller.GC_TestGearState();
	}
}

modded class GC_LobbyPlayerController
{
	//------------------------------------------------------------------------------------------------
	void GC_TestGearDenied(ResourceName prefab)
	{
		GC_RpcAskGearOpen();
		GC_RpcAskGearCrate(true);
		GC_RpcAskGearItem(prefab, RplId.Invalid(), RplId.Invalid(), -1, true, false, RplId.Invalid());
	}

	//------------------------------------------------------------------------------------------------
	void GC_TestGearInvalid(ResourceName prefab)
	{
		GC_RpcAskGearItem(prefab, GC_GearEntityId(GetControlledEntity()), RplId.Invalid(), 0, false, false, RplId.Invalid());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestGearState()
	{
		IEntity owner = GetControlledEntity();
		array<IEntity> items = {};
		if (owner)
			InventoryStorageManagerComponent.Cast(owner.FindComponent(InventoryStorageManagerComponent)).GetItems(items);
		string result = string.Format("admin=%1 active=%2 crate=%3 pending=%4 items=%5", SCR_Global.IsAdmin(GetPlayerId()), GC_GearSession.IsAdmin(owner), GC_GearSession.s_bCrate, m_bGC_GearPending, items.Count());
		IEntity crate = GC_GearEntity(m_GC_GearCrate);
		if (crate)
		{
			BaseInventoryStorageComponent storage = GC_GetGearCrateStorage(crate);
			array<IEntity> crateItems = {};
			storage.GetAll(crateItems);
			result += " crateItems=" + crateItems.Count();
		}
		SCR_InventoryMenuUI menu = SCR_InventoryMenuUI.GetInventoryMenu();
		if (menu)
		{
			TextWidget status = TextWidget.Cast(menu.GetRootWidget().FindAnyWidget("Status"));
			if (status)
				result += " status=" + status.GetText();
			result += " files=" + (menu.GetRootWidget().FindAnyWidget("CatalogueFiles") != null);
		}
		return result;
	}
}
#endif
