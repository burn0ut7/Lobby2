#ifdef WORKBENCH
modded class GC_LobbyManager
{
	protected IEntity m_GCInventoryCharacter;
	protected IEntity m_GCInventoryPreviousCharacter;

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueOpen()
	{
		return GC_WorkbenchItemBrowser.Open().ToString();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueState()
	{
		SCR_InventoryMenuUI menu = SCR_InventoryMenuUI.GetInventoryMenu();
		if (!menu)
			return "closed";
		return menu.GC_TestCatalogueState();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueSlot(int row)
	{
		return SCR_InventoryMenuUI.GetInventoryMenu().GC_TestSelectEquipment(row);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCataloguePrimary()
	{
		return SCR_InventoryMenuUI.GetInventoryMenu().GC_TestSelectPrimary();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueSearch(string value)
	{
		SCR_InventoryMenuUI menu = SCR_InventoryMenuUI.GetInventoryMenu();
		EditBoxWidget.Cast(menu.GetRootWidget().FindAnyWidget("CatalogueSearch")).SetText(value);
		return "input changed";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueButton(string name)
	{
		Widget root = SCR_InventoryMenuUI.GetInventoryMenu().GetRootWidget();
		Widget button = root.FindAnyWidget(name);
		if (!button)
			return "button absent";
		SCR_ButtonBaseComponent component = SCR_ButtonBaseComponent.Cast(button.FindHandler(SCR_ButtonBaseComponent));
		component.OnClick(button, 0, 0, 0);
		return GC_TestCatalogueState();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueOption(string name, int value)
	{
		Widget widget = SCR_InventoryMenuUI.GetInventoryMenu().GetRootWidget().FindAnyWidget(name);
		SCR_ComboBoxComponent combo = SCR_ComboBoxComponent.Cast(widget.FindHandler(SCR_ComboBoxComponent));
		combo.OpenList();
		array<Widget> elements = {};
		combo.GetElementWidgets(elements);
		if (!elements.IsIndexValid(value))
			return "option absent";
		SCR_ButtonTextComponent button = SCR_ButtonTextComponent.Cast(elements[value].FindHandler(SCR_ButtonTextComponent));
		button.OnClick(elements[value], 0, 0, 0);
		return GC_TestCatalogueState();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueClose()
	{
		SCR_InventoryStorageManagerComponent manager = SCR_InventoryStorageManagerComponent.Cast(m_GCInventoryCharacter.FindComponent(SCR_InventoryStorageManagerComponent));
		manager.CloseInventory();
		return "closed";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCataloguePage()
	{
		return SCR_InventoryMenuUI.GetInventoryMenu().GC_TestPage();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueOwnerLoss()
	{
		GetGame().GetPlayerController().SetControlledEntity(m_GCInventoryPreviousCharacter);
		return "owner restored";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueCleanup()
	{
		GC_WorkbenchItemBrowser.Disable();
		GC_TestCatalogueClose();
		GetGame().GetPlayerController().SetControlledEntity(m_GCInventoryPreviousCharacter);
		SCR_EntityHelper.DeleteEntityAndChildren(m_GCInventoryCharacter);
		m_GCInventoryCharacter = null;
		m_GCInventoryPreviousCharacter = null;
		return "restored";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueDisable()
	{
		GC_WorkbenchItemBrowser.Disable();
		return "disabled";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestNativeInventoryStart()
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller)
			return "no controller";
		if (!m_GCInventoryCharacter)
		{
			m_GCInventoryPreviousCharacter = controller.GetControlledEntity();
			EntitySpawnParams spawn = new EntitySpawnParams();
			spawn.Transform[3] = "0 0 0";
			m_GCInventoryCharacter = GetGame().SpawnEntityPrefab(Resource.Load("{26A9756790131354}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Rifleman.et"), GetGame().GetWorld(), spawn);
		}
		if (!m_GCInventoryCharacter || (controller.GetControlledEntity() != m_GCInventoryCharacter && !controller.SetControlledEntity(m_GCInventoryCharacter)))
			return "possession failed";
		SCR_InventoryStorageManagerComponent manager = SCR_InventoryStorageManagerComponent.Cast(m_GCInventoryCharacter.FindComponent(SCR_InventoryStorageManagerComponent));
		manager.OpenInventory();
		return "native inventory requested";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestNativeInventoryState()
	{
		SCR_InventoryMenuUI menu = SCR_InventoryMenuUI.GetInventoryMenu();
		if (!menu)
			return "inventory closed";
		Widget root = menu.GetRootWidget();
		string result = "menu=" + menu.Type();
		array<string> names = {"Left", "LeftArea", "StorageLootSlot", "OpenedStorages", "StorageGrid", "playerRender"};
		foreach (string name : names)
		{
			Widget widget = root.FindAnyWidget(name);
			if (!widget)
				continue;
			float x, y, width, height;
			widget.GetScreenPos(x, y);
			widget.GetScreenSize(width, height);
			result += string.Format("\n%1 visible=%2 x=%3 y=%4 w=%5 h=%6", name, widget.IsVisible(), x, y, width, height);
		}
		return result;
	}
}
modded class GC_InventoryCataloguePanel
{
	//------------------------------------------------------------------------------------------------
	string GC_TestPage()
	{
		string report = State();
		foreach (GC_ItemCatalogueEntry entry : m_aPage)
			report += "\n" + entry.m_sName + " | " + entry.m_sPrefab;
		return report;
	}
}
modded class SCR_InventoryMenuUI
{
	//------------------------------------------------------------------------------------------------
	string GC_TestPage()
	{
		return m_GCCataloguePanel.GC_TestPage();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueState()
	{
		if (!m_GCCataloguePanel)
			return "panel absent";
		return m_GCCataloguePanel.State();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestSelectPrimary()
	{
		array<SCR_InventorySlotUI> slots = {};
		m_pWeaponStorage.GetSlots(slots);
		foreach (SCR_InventorySlotUI slot : slots)
		{
			SCR_InventorySlotWeaponSlotsUI weapon = SCR_InventorySlotWeaponSlotsUI.Cast(slot);
			if (!weapon || weapon.GC_GetCatalogueWeaponType() != "primary")
				continue;
			float x, y, width, height;
			slot.GetButtonWidget().GetScreenPos(x, y);
			slot.GetButtonWidget().GetScreenSize(width, height);
			WidgetManager.ReportMouse(x + width * 0.5, y + height * 0.5, GetRootWidget());
			Action_SelectItem();
			return GC_TestCatalogueState();
		}
		return "primary missing";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestSelectEquipment(int row)
	{
		array<SCR_InventorySlotUI> slots = {};
		m_pStorageListUI.GetSlots(slots);
		foreach (SCR_InventorySlotUI slot : slots)
		{
			if (GridSlot.GetRow(slot.GetWidget()) != row)
				continue;
			float x, y, width, height;
			slot.GetButtonWidget().GetScreenPos(x, y);
			slot.GetButtonWidget().GetScreenSize(width, height);
			WidgetManager.ReportMouse(x + width * 0.5, y + height * 0.5, GetRootWidget());
			Action_SelectItem();
			return GC_TestCatalogueState();
		}
		return "slot missing";
	}
}
modded class GC_IsolationProbeComponent
{
	//------------------------------------------------------------------------------------------------
	string CatalogueOpenOutsideMode()
	{
		return GC_WorkbenchItemBrowser.Open().ToString();
	}
}
#endif
