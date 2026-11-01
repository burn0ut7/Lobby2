#ifdef WORKBENCH
//! Same native inventory journey with and without Lobby2 loaded.
modded class GC_IsolationProbeComponent
{
	//------------------------------------------------------------------------------------------------
	string InventoryInputSnapshot()
	{
		return SCR_InventoryMenuUI.GetInventoryMenu().GC_ProbeInventoryInput();
	}

	//------------------------------------------------------------------------------------------------
	string OpenInventory()
	{
		string possession = PossessCharacter();
		IEntity character = GetGame().GetPlayerController().GetControlledEntity();
		if (!character)
			return possession;
		SCR_InventoryStorageManagerComponent manager = SCR_InventoryStorageManagerComponent.Cast(character.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!manager)
			return "missing inventory";
		manager.OpenInventory();
		return "native inventory requested";
	}

	//------------------------------------------------------------------------------------------------
	string InventorySnapshot()
	{
		SCR_InventoryMenuUI menu = SCR_InventoryMenuUI.GetInventoryMenu();
		if (!menu)
			return "closed";
		Widget root = menu.GetRootWidget();
		string result = "menu=" + menu.Type();
		result += string.Format(" customPanel=%1", root.FindAnyWidget("GCCatalogue") != null);
		array<string> names = {"StorageLootSlot", "OpenedStorages", "StorageGrid", "playerRender"};
		foreach (string name : names)
		{
			Widget widget = root.FindAnyWidget(name);
			float x, y, width, height;
			widget.GetScreenPos(x, y);
			widget.GetScreenSize(width, height);
			result += string.Format("\n%1 visible=%2 x=%3 y=%4 w=%5 h=%6", name, widget.IsVisible(), x, y, width, height);
		}
		return result;
	}
}

modded class SCR_InventoryMenuUI
{
	//------------------------------------------------------------------------------------------------
	string GC_ProbeInventoryInput()
	{
		array<SCR_InventorySlotUI> slots = {};
		m_pStorageListUI.GetSlots(slots);
		Widget button = slots[0].GetButtonWidget();
		float x, y, width, height;
		button.GetScreenPos(x, y);
		button.GetScreenSize(width, height);
		x += width * 0.5;
		y += height * 0.5;
		WidgetManager.ReportMouse(x, y, m_widget);
		Action_SelectItem();
		string result = string.Format("selected=%1", m_pSelectedSlotUI != null);
		Action_DragDown();
		WidgetManager.ReportMouse(x + 20, y, m_widget);
		Action_OnDrag();
		result += string.Format(" dragging=%1", m_bDraggingEnabled);
		WidgetManager.ReportMouse(x, y, m_widget);
		Action_DragUp();
		Action_DeselectItem();
		result += string.Format(" released=%1", !m_bDraggingEnabled);
		return result;
	}
}
#endif
