#ifdef WORKBENCH
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestGearQuantity(string action, string value)
	{
		SCR_InventoryMenuUI menu = SCR_InventoryMenuUI.GetInventoryMenu();
		if (!menu)
			return "closed";
		return menu.GC_TestGearQuantity(action, value);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestGearWidgetParents()
	{
		SCR_InventoryMenuUI menu = SCR_InventoryMenuUI.GetInventoryMenu();
		if (!menu)
			return "closed";
		string result;
		array<string> names = {"OpenedStorages", "StoragesListSlot", "StorageGrid", "RightArea", "LeftArea", "playerRender"};
		foreach (string name : names)
		{
			Widget widget = menu.GetRootWidget().FindAnyWidget(name);
			result += "\n" + name + ": ";
			while (widget)
			{
				result += widget.GetName() + " / ";
				widget = widget.GetParent();
			}
		}
		return result;
	}

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
		else if (action == "create" || action == "nearby" || action == "crate" || action == "inventory")
		{
			SCR_InventoryMenuUI menu = SCR_InventoryMenuUI.GetInventoryMenu();
			if (!menu)
				return "inventory missing";
			int index;
			if (action != "inventory")
				index = 1;
			SCR_ComboBoxComponent combo = SCR_ComboBoxComponent.Cast(menu.GetRootWidget().FindAnyWidget("GearDestination").FindHandler(SCR_ComboBoxComponent));
			combo.SetCurrentItem(index, false, false, true);
			combo.m_OnChanged.Invoke(combo, index);
		}
		else if (action == "spawn")
			controller.GC_RequestGearItem(prefab, null, -1, true);
		else if (action == "native-drop" || action == "native-take")
			return controller.GC_TestGearNativeTransfer(action == "native-take");
		else if (action == "invalid")
			controller.GC_TestGearInvalid(prefab);
		else if (action == "denied-local")
			controller.GC_TestGearDenied(prefab);
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
	protected IEntity m_GC_GearTestDropped;

	//------------------------------------------------------------------------------------------------
	string GC_TestGearNativeTransfer(bool take)
	{
		SCR_InventoryStorageManagerComponent manager = SCR_InventoryStorageManagerComponent.Cast(GetControlledEntity().FindComponent(SCR_InventoryStorageManagerComponent));
		if (take)
		{
			BaseInventoryStorageComponent target = manager.FindStorageForItem(m_GC_GearTestDropped, EStoragePurpose.PURPOSE_DEPOSIT);
			return "take=" + manager.TryInsertItemInStorage(m_GC_GearTestDropped, target);
		}
		array<IEntity> items = {};
		manager.GetItems(items);
		foreach (IEntity item : items)
		{
			if (!SCR_ResourceNameUtils.GetPrefabName(item).Contains("FieldDressing"))
				continue;
			InventoryItemComponent inventory = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
			m_GC_GearTestDropped = item;
			return "drop=" + manager.TryRemoveItemFromInventory(item, inventory.GetParentSlot().GetStorage());
		}
		return "no bandage";
	}

	//------------------------------------------------------------------------------------------------
	void GC_TestGearDenied(ResourceName prefab)
	{
		GC_RpcAskGearOpen();
		GC_RpcAskGearCrate(true);
		GC_RpcAskGearItem(prefab, RplId.Invalid(), RplId.Invalid(), -1, true, false, RplId.Invalid());
		GC_RpcAskGearAmountItem(prefab, GC_GearEntityId(GetControlledEntity()), true, m_GC_GearCrate);
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
			storage.GetAll(crateItems, false);
			result += " crateItems=" + crateItems.Count();
			result += " crateId=" + m_GC_GearCrate;
			result += " unlimited=" + SCR_UniversalInventoryStorageComponent.Cast(storage).GC_HasAdminGearCapacity();
			result += " volume=" + storage.GetOccupiedSpace() + "/" + storage.GetMaxVolumeCapacity();
			if (!crateItems.IsEmpty())
				result += " canFitMore=" + storage.CanStoreResource(SCR_ResourceNameUtils.GetPrefabName(crateItems[0]), -1);
			int nestedCount;
			foreach (IEntity item : crateItems)
			{
				BaseInventoryStorageComponent nested = BaseInventoryStorageComponent.Cast(item.FindComponent(BaseInventoryStorageComponent));
				if (!nested)
					continue;
				array<IEntity> nestedItems = {};
				nested.GetAll(nestedItems);
				nestedCount += nestedItems.Count();
			}
			result += " nestedItems=" + nestedCount;
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

modded class SCR_InventoryMenuUI
{
	string GC_TestGearQuantity(string action, string value)
	{
		if (!m_GCCataloguePanel)
			return "no panel";
		string result = m_GCCataloguePanel.GC_TestGearQuantity(action, value);
		result += " nativeContent=" + m_widget.FindAnyWidget("InventoryContent").IsVisible();
		result += " opened=" + m_aOpenedStoragesUI.Count();
		return result;
	}
}

modded class GC_InventoryCataloguePanel
{
	string GC_TestGearQuantity(string action, string value)
	{
		if (action == "open")
		{
			if (m_aPage.IsEmpty())
				return "empty page";
			m_Destination.CloseList();
			Widget button = m_aCells[0].FindAnyWidget("SelectItem");
			float x, y, w, h;
			button.GetScreenPos(x, y);
			button.GetScreenSize(w, h);
			WidgetManager.ReportMouse(x + w / 2, y + h / 2, SCR_InventoryMenuUI.GetInventoryMenu().GetRootWidget());
			OnDoubleClick(WidgetManager.GetWidgetUnderCursor(), x + w / 2, y + h / 2, 0);
		}
		if (m_QuantityDialog)
			return m_QuantityDialog.GC_TestAmount(action, value);
		return "dialog=0 crateView=" + m_bCrateView;
	}
}

modded class GC_GearQuantityDialog
{
	string GC_TestAmount(string action, string value)
	{
		if (action == "text")
		{
			m_Editbox.SetValue(value);
			OnAmountChanged(value);
		}
		else if (action == "slider")
			m_Slider.SetValue(value.ToInt());
		else if (action == "confirm")
			OnConfirm();
		else if (action == "cancel")
			OnCancel();
		return "dialog=1 text=" + m_Editbox.GetValue() + " slider=" + m_Slider.GetValue() + " closing=" + m_bIsClosing;
	}
}
#endif
