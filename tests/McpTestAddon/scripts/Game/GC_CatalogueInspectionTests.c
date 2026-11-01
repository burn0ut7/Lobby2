#ifdef WORKBENCH
modded class GC_LobbyManager
{
	string GC_TestMoveStored(string prefab, int equipmentRow)
	{
		return SCR_InventoryMenuUI.GetInventoryMenu().GC_TestMoveStored(prefab, equipmentRow);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestMoveWeapon(int fromSlot, int toSlot)
	{
		return SCR_InventoryMenuUI.GetInventoryMenu().GC_TestMoveWeapon(fromSlot, toSlot);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestInspectWeapon(int slotID)
	{
		return SCR_InventoryMenuUI.GetInventoryMenu().GC_TestInspectWeapon(slotID);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestInspectionPoint(int point, string action, int row)
	{
		return SCR_InventoryMenuUI.GetInventoryMenu().GC_TestInspectionPoint(point, action, row);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestInspectionState()
	{
		return SCR_InventoryMenuUI.GetInventoryMenu().GC_TestInspectionState();
	}
}

modded class SCR_InventoryMenuUI
{
	string GC_TestMoveStored(string prefab, int equipmentRow)
	{
		array<SCR_InventorySlotUI> items = {};
		foreach (SCR_InventoryStorageBaseUI storage : m_aStorages)
		{
			if (storage)
				storage.GetSlots(items);
		}
		foreach (SCR_InventorySlotUI source : items)
		{
			if (!source.GetInventoryItemComponent())
				continue;
			IEntity item = source.GetInventoryItemComponent().GetOwner();
			if (!item.GetPrefabData().GetPrefabName().Contains(prefab))
				continue;
			GC_TestInspectionMouse(source.GetButtonWidget());
			if (!m_bDraggingEnabled)
				Action_DragDown();
			if (equipmentRow == -2)
			{
				int x, y;
				WidgetManager.GetMousePos(x, y);
				WidgetManager.ReportMouse(x - 100, y + 70, m_widget);
				Action_OnDrag();
				return string.Format("nativeSource=%1 selected=%2 dragging=%3 previewVisible=%4", m_DraggedSlot == source, m_pSelectedSlotUI == source, m_bDraggingEnabled, m_wDragDropContainer.IsVisible());
			}
			array<SCR_InventorySlotUI> equipment = {};
			m_pStorageListUI.GetSlots(equipment);
			foreach (SCR_InventorySlotUI target : equipment)
			{
				if (GridSlot.GetRow(target.GetWidget()) != equipmentRow)
					continue;
				GC_TestInspectionMouse(target.GetButtonWidget());
				Action_OnDrag();
				Action_DragUp();
				InventoryStorageSlot parent = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent)).GetParentSlot();
				if (parent)
					return parent.GetStorage().GetOwner().GetPrefabData().GetPrefabName();
				return "no parent";
			}
		}
		return "source absent";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestMoveWeapon(int fromSlot, int toSlot)
	{
		array<SCR_InventorySlotUI> slots = {};
		m_pWeaponStorage.GetSlots(slots);
		SCR_InventorySlotUI source;
		SCR_InventorySlotUI target;
		foreach (SCR_InventorySlotUI slot : slots)
		{
			SCR_InventorySlotWeaponSlotsUI weapon = SCR_InventorySlotWeaponSlotsUI.Cast(slot);
			if (!weapon)
				continue;
			if (weapon.GetWeaponSlotIndex() == fromSlot)
				source = slot;
			if (weapon.GetWeaponSlotIndex() == toSlot)
				target = slot;
		}
		if (!source || (!target && toSlot >= 0))
			return "missing slot";
		IEntity moved = source.GetInventoryItemComponent().GetOwner();
		GC_TestInspectionMouse(source.GetButtonWidget());
		Action_DragDown();
		bool captured = m_GCExistingGestureSlot != null;
		if (target)
			GC_TestInspectionMouse(target.GetButtonWidget());
		else
			WidgetManager.ReportMouse(1, 1, m_widget);
		Action_OnDrag();
		bool started = m_bDraggingEnabled;
		Widget hit = WidgetManager.GetWidgetUnderCursor();
		string hitName;
		if (hit)
			hitName = hit.GetName();
		Action_DragUp();
		string result = string.Format("alive=%1 owned=%2 dragging=%3 captured=%4 started=%5 hit=%6 canMove=%7", moved != null, m_InventoryManager.Contains(moved), m_bDraggingEnabled, captured, started, hitName, m_InventoryManager.CanMoveItem(moved));
		for (int i; i < m_pWeaponStorageComp.GetSlotsCount(); i++)
		{
			IEntity item = m_pWeaponStorageComp.GetSlot(i).GetAttachedEntity();
			if (item)
				result += "\n" + i.ToString() + " " + item.GetPrefabData().GetPrefabName();
		}
		return result;
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_TestInspectionMouse(Widget widget)
	{
		float x, y, width, height;
		widget.GetScreenPos(x, y);
		widget.GetScreenSize(width, height);
		WidgetManager.ReportMouse(x + width * 0.5, y + height * 0.5, m_widget);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestInspectWeapon(int slotID)
	{
		array<SCR_InventorySlotUI> slots = {};
		m_pWeaponStorage.GetSlots(slots);
		foreach (SCR_InventorySlotUI slot : slots)
		{
			SCR_InventorySlotWeaponSlotsUI weapon = SCR_InventorySlotWeaponSlotsUI.Cast(slot);
			if (!weapon || weapon.GetWeaponSlotIndex() != slotID)
				continue;
			GC_TestInspectionMouse(slot.GetButtonWidget());
			GC_ActionCatalogueInspect();
			return GC_TestInspectionState();
		}
		return "weapon absent";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestInspectionState()
	{
		string result = string.Format("inspection=%1 interact=%2", m_pGearInspectionPointUI != null, GetCanInteract());
		result += string.Format(" nativeContext=%1 blockingContext=%2", GetGame().GetInputManager().IsContextActive("InventoryContext"), GetGame().GetInputManager().IsContextActive("GC_CatalogueInventory"));
		array<string> keys = {};
		array<BaseContainer> filters = {};
		result += string.Format(" binding=%1", GetGame().GetInputManager().GetActionKeybinding("GC_CatalogueInspect", keys, filters));
		foreach (string key : keys)
			result += " " + key;
		if (!m_pGearInspectionPointUI)
			return result;
		array<SCR_InventorySlotUI> slots = {};
		m_pGearInspectionPointUI.GetSlots(slots);
		foreach (int i, SCR_InventorySlotUI slot : slots)
		{
			IEntity item;
			if (slot.GetInventoryItemComponent())
				item = slot.GetInventoryItemComponent().GetOwner();
			string prefab;
			if (item)
				prefab = item.GetPrefabData().GetPrefabName();
			result += string.Format("\n%1 visible=%2 %3", i, slot.GetWidget().IsVisible(), prefab);
		}
		return result;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestInspectionPoint(int point, string action, int row)
	{
		if (!m_pGearInspectionPointUI)
			return "inspection absent";
		array<SCR_InventorySlotUI> slots = {};
		m_pGearInspectionPointUI.GetSlots(slots);
		if (!slots.IsIndexValid(point))
			return "point absent";
		SCR_InventorySlotUI target = slots[point];
		GC_TestInspectionMouse(target.GetWidget());
		string gesture;
		if (action == "select")
		{
			Action_SelectItem();
			target.OnClick(target.GetWidget(), 0, 0, 0);
		}
		else if (action == "right")
			GC_ActionCatalogueShortcut();
		else if (action == "catalogue")
		{
			Widget source = m_GCCataloguePanel.GC_InspectionTestRow(row);
			GC_TestInspectionMouse(source);
			Action_DragDown();
			GC_TestInspectionMouse(target.GetWidget());
			Action_OnDrag();
			Action_DragUp();
		}
		else if (action == "out")
		{
			gesture = string.Format(" hitButton=%1 focus=%2", WidgetManager.GetWidgetUnderCursor() == target.GetButtonWidget(), m_pFocusedSlotUI == target);
			Action_DragDown();
			int x, y;
			WidgetManager.GetMousePos(x, y);
			WidgetManager.ReportMouse(x + 20, y, m_widget);
			Action_OnDrag();
			gesture += string.Format(" started=%1", m_bDraggingEnabled);
			array<SCR_InventorySlotUI> equipment = {};
			m_pStorageListUI.GetSlots(equipment);
			foreach (SCR_InventorySlotUI slot : equipment)
			{
				if (GridSlot.GetRow(slot.GetWidget()) != row)
					continue;
				GC_TestInspectionMouse(slot.GetButtonWidget());
				Action_OnDrag();
				Action_DragUp();
				break;
			}
		}
		else if (action == "in" || action == "rightStored")
		{
			array<SCR_InventorySlotUI> items = {};
			foreach (SCR_InventoryStorageBaseUI storageUI : m_aStorages)
			{
				if (storageUI && storageUI.GetCurrentNavigationStorage() && storageUI.GetCurrentNavigationStorage().GetOwner().GetPrefabData().GetPrefabName().Contains("Jacket_US_BDU.et"))
					storageUI.GetSlots(items);
			}
			foreach (SCR_InventorySlotUI source : items)
			{
				if (!source.GetInventoryItemComponent())
					continue;
				GC_TestInspectionMouse(source.GetButtonWidget());
				if (action == "rightStored")
				{
					GC_ActionCatalogueShortcut();
					break;
				}
				Action_DragDown();
				int x, y;
				WidgetManager.GetMousePos(x, y);
				WidgetManager.ReportMouse(x + 20, y, m_widget);
				Action_OnDrag();
				gesture = string.Format(" started=%1", m_bDraggingEnabled);
				GC_TestInspectionMouse(target.GetWidget());
				Action_OnDrag();
				Action_DragUp();
				break;
			}
		}
		return GC_TestInspectionState() + gesture + "\n" + m_GCCataloguePanel.State();
	}
}

modded class GC_InventoryCataloguePanel
{
	//------------------------------------------------------------------------------------------------
	Widget GC_InspectionTestRow(int row)
	{
		if (row < 0 || row >= m_aPage.Count())
			return null;
		return m_aCells[row].FindAnyWidget("SelectItem");
	}
}
#endif
