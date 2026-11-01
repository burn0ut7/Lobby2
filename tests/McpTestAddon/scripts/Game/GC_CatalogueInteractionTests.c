#ifdef WORKBENCH
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestCataloguePager(bool next)
	{
		Widget pager = SCR_InventoryMenuUI.GetInventoryMenu().GetRootWidget().FindAnyWidget("CataloguePaging");
		string name = "ButtonLeft";
		if (next)
			name = "ButtonRight";
		Widget widget = pager.FindAnyWidget(name);
		SCR_PagingButtonComponent.Cast(widget.FindHandler(SCR_PagingButtonComponent)).OnClick(widget, 0, 0, 0);
		return SCR_InventoryMenuUI.GetInventoryMenu().GC_TestCataloguePagingState();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueSearchFocus(bool focus)
	{
		Widget root = SCR_InventoryMenuUI.GetInventoryMenu().GetRootWidget();
		Widget widget = root.FindAnyWidget("CategoryAll");
		if (focus)
			widget = root.FindAnyWidget("CatalogueSearch");
		GetGame().GetWorkspace().SetFocusedWidget(widget);
		return "focus changed";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueOldHead(bool capture)
	{
		return SCR_InventoryMenuUI.GetInventoryMenu().GC_TestCatalogueOldHead(capture);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueRejectDisabled()
	{
		return SCR_InventoryMenuUI.GetInventoryMenu().GC_TestCatalogueRejectDisabled();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueTransferProbe(int row)
	{
		return SCR_InventoryMenuUI.GetInventoryMenu().GC_TestCatalogueTransferProbe(row);
	}
	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueClick(int row)
	{
		return SCR_InventoryMenuUI.GetInventoryMenu().GC_TestCatalogueClick(row);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueWheel(int direction)
	{
		return SCR_InventoryMenuUI.GetInventoryMenu().GC_TestCatalogueWheel(direction);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueDrag(int row, int equipmentRow)
	{
		return SCR_InventoryMenuUI.GetInventoryMenu().GC_TestCatalogueDrag(row, equipmentRow);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueInventory()
	{
		InventoryStorageManagerComponent manager = InventoryStorageManagerComponent.Cast(GetGame().GetPlayerController().GetControlledEntity().FindComponent(InventoryStorageManagerComponent));
		array<BaseInventoryStorageComponent> storages = {};
		manager.GetStorages(storages);
		string report;
		foreach (BaseInventoryStorageComponent storage : storages)
		{
			for (int i; i < storage.GetSlotsCount(); i++)
			{
				IEntity item = storage.GetSlot(i).GetAttachedEntity();
				if (item)
					report += string.Format("\n%1 %2 %3", storage.Type(), i, item.GetPrefabData().GetPrefabName());
			}
		}
		return report;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueGeometry()
	{
		Widget root = SCR_InventoryMenuUI.GetInventoryMenu().GetRootWidget();
		array<string> names = {"GCCatalogue", "CatalogueContent", "CataloguePaging", "Results", "CatalogueCell", "ItemPreview", "ItemCategory", "ItemName", "StorageGrid", "playerRender"};
		string result;
		foreach (string name : names)
		{
			Widget widget = root.FindAnyWidget(name);
			float x, y, width, height;
			widget.GetScreenPos(x, y);
			widget.GetScreenSize(width, height);
			result += string.Format("\n%1 x=%2 y=%3 w=%4 h=%5", name, x, y, width, height);
		}
		return result;
	}
}

modded class GC_InventoryCataloguePanel
{
	Widget GC_TestRow(int row)
	{
		if (!m_aPage.IsIndexValid(row))
			return null;
		return m_aCells[row].FindAnyWidget("SelectItem");
	}
}

modded class SCR_InventoryMenuUI
{
	protected IEntity m_GCTestOldHead;

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueRejectDisabled()
	{
		GC_ItemCatalogueEntry entry = m_GCCataloguePanel.EntryAt(m_GCCataloguePanel.GC_TestRow(0));
		GC_WorkbenchItemBrowser.Disable();
		return GC_CatalogueTransfer.Insert(entry, m_Player, this).ToString();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueOldHead(bool capture)
	{
		if (capture)
		{
			array<BaseInventoryStorageComponent> storages = {};
			m_InventoryManager.GetStorages(storages, EStoragePurpose.PURPOSE_LOADOUT_PROXY);
			m_GCTestOldHead = storages[0].GetSlot(0).GetAttachedEntity();
		}
		if (!m_GCTestOldHead)
			return "absent";
		InventoryItemComponent item = InventoryItemComponent.Cast(m_GCTestOldHead.FindComponent(InventoryItemComponent));
		return string.Format("exists=1 attached=%1 prefab=%2", item.GetParentSlot() != null, m_GCTestOldHead.GetPrefabData().GetPrefabName());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCataloguePagingState()
	{
		return m_GCCataloguePanel.State();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueTransferProbe(int row)
	{
		GC_ItemCatalogueEntry entry = m_GCCataloguePanel.EntryAt(m_GCCataloguePanel.GC_TestRow(row));
		ChimeraWorld world = GetGame().GetWorld();
		IEntity preview = world.GetItemPreviewManager().ResolvePreviewEntityForPrefab(entry.m_sPrefab);
		string result = string.Format("active=%1 portable=%2 locked=%3 ready=%4 preview=%5 prefab=%6", GC_WorkbenchItemBrowser.IsActive(m_Player), entry.m_bPortable, m_InventoryManager.IsInventoryLocked(), m_InventoryManager.IsAnimationReady(), preview != null, entry.m_sPrefab);
		BaseInventoryStorageComponent target;
		int slotID;
		result += " found=" + GC_CatalogueTransfer.FindSlot(entry, m_InventoryManager, preview, target, slotID);
		array<BaseInventoryStorageComponent> storages = {};
		m_InventoryManager.GetStorages(storages, EStoragePurpose.PURPOSE_LOADOUT_PROXY);
		foreach (BaseInventoryStorageComponent storage : storages)
			result += string.Format(" loadout=%1 slots=%2 replace0=%3 insert0=%4", storage.Type().ToString(), storage.GetSlotsCount(), m_InventoryManager.CanReplaceItem(preview, storage, 0), m_InventoryManager.CanInsertResourceInStorage(entry.m_sPrefab, storage, 0));
		return result;
	}

	//------------------------------------------------------------------------------------------------
	void GC_TestMouse(Widget widget)
	{
		float x, y, width, height;
		widget.GetScreenPos(x, y);
		widget.GetScreenSize(width, height);
		WidgetManager.ReportMouse(x + width * 0.5, y + height * 0.5, m_widget);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueClick(int row)
	{
		Widget widget = m_GCCataloguePanel.GC_TestRow(row);
		if (!widget)
			return "row absent";
		GC_TestMouse(widget);
		Action_DragDown();
		Action_DragUp();
		SCR_ButtonBaseComponent.Cast(widget.FindHandler(SCR_ButtonBaseComponent)).OnClick(widget, 0, 0, 0);
		return TextWidget.Cast(m_widget.FindAnyWidget("Detail")).GetText();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueWheel(int direction)
	{
		Widget row = m_GCCataloguePanel.GC_TestRow(0);
		GC_TestMouse(row);
		m_GCCataloguePanel.OnMouseWheel(row, 0, 0, direction);
		return m_GCCataloguePanel.State();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueDrag(int row, int equipmentRow)
	{
		Widget widget = m_GCCataloguePanel.GC_TestRow(row);
		if (!widget)
			return "row absent";
		GC_TestMouse(widget);
		Action_DragDown();
		array<SCR_InventorySlotUI> slots = {};
		Widget destination;
		if (equipmentRow >= 100)
		{
			m_pWeaponStorage.GetSlots(slots);
			foreach (SCR_InventorySlotUI slot : slots)
			{
				SCR_InventorySlotWeaponSlotsUI weapon = SCR_InventorySlotWeaponSlotsUI.Cast(slot);
				if (weapon && weapon.GetWeaponSlotIndex() == equipmentRow - 100)
					destination = slot.GetButtonWidget();
			}
		}
		else
		{
			m_pStorageListUI.GetSlots(slots);
			foreach (SCR_InventorySlotUI slot : slots)
			{
				if (GridSlot.GetRow(slot.GetWidget()) == equipmentRow)
					destination = slot.GetButtonWidget();
			}
		}
		if (destination)
			GC_TestMouse(destination);
		else
			WidgetManager.ReportMouse(1, 1, m_widget);
		Action_OnDrag();
		Action_DragUp();
		return TextWidget.Cast(m_widget.FindAnyWidget("Detail")).GetText();
	}
}
#endif
