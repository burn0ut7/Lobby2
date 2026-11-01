#ifdef WORKBENCH
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueContainerDrop(int row, bool header)
	{
		return SCR_InventoryMenuUI.GetInventoryMenu().GC_TestCatalogueContainerDrop(row, header);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueRejectDelete(bool disabled)
	{
		return SCR_InventoryMenuUI.GetInventoryMenu().GC_TestCatalogueRejectDelete(disabled);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueRightClick(int row, int equipmentRow)
	{
		return SCR_InventoryMenuUI.GetInventoryMenu().GC_TestCatalogueRightClick(row, equipmentRow);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueDeletionState()
	{
		return SCR_InventoryMenuUI.GetInventoryMenu().GC_TestCatalogueDeletionState();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueOwnedItems()
	{
		InventoryStorageManagerComponent manager = InventoryStorageManagerComponent.Cast(GetGame().GetPlayerController().GetControlledEntity().FindComponent(InventoryStorageManagerComponent));
		array<IEntity> items = {};
		manager.GetItems(items);
		string result;
		foreach (IEntity item : items)
		{
			InventoryItemComponent inventory = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
			if (inventory && inventory.GetParentSlot())
				result += string.Format("\n%1 in %2", item.GetPrefabData().GetPrefabName(), inventory.GetParentSlot().GetStorage().GetOwner().GetPrefabData().GetPrefabName());
		}
		return result;
	}

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

modded class SCR_InventoryStorageBaseUI
{
	//------------------------------------------------------------------------------------------------
	Widget GC_TestContainerTarget(bool header)
	{
		if (header)
			return m_widget.FindAnyWidget("titleLayout");
		return m_wGrid;
	}
}

modded class SCR_InventoryMenuUI
{
	protected IEntity m_GCTestOldHead;
	protected IEntity m_GCTestDeletedItem;
	protected ref array<IEntity> m_aGCTestDeletedContents = {};

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueContainerDrop(int row, bool header)
	{
		Widget source = m_GCCataloguePanel.GC_TestRow(row);
		Widget target;
		foreach (SCR_InventoryStorageBaseUI storageUI : m_aStorages)
		{
			if (!storageUI || !storageUI.GetCurrentNavigationStorage())
				continue;
			IEntity owner = storageUI.GetCurrentNavigationStorage().GetOwner();
			if (owner.GetPrefabData().GetPrefabName().Contains("Jacket_US_BDU.et"))
				target = storageUI.GC_TestContainerTarget(header);
		}
		if (!source || !target)
			return "missing target";
		GC_TestMouse(source);
		Action_DragDown();
		GC_TestMouse(target);
		m_pFocusedSlotUI = null;
		Action_OnDrag();
		Action_DragUp();
		return TextWidget.Cast(m_widget.FindAnyWidget("Status")).GetText();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueRejectDelete(bool disabled)
	{
		IEntity item;
		if (disabled)
		{
			array<SCR_InventorySlotUI> slots = {};
			m_pStorageListUI.GetSlots(slots);
			foreach (SCR_InventorySlotUI slot : slots)
			{
				if (GridSlot.GetRow(slot.GetWidget()) == 4 && slot.GetInventoryItemComponent())
					item = slot.GetInventoryItemComponent().GetOwner();
			}
			GC_WorkbenchItemBrowser.Disable();
		}
		else
			item = GetGame().SpawnEntityPrefab(Resource.Load("{0D9A5DCF89AE7AA9}Prefabs/Items/Medicine/MorphineInjection_01/MorphineInjection_01.et"), GetGame().GetWorld());
		if (!item)
			return "missing fixture";
		bool accepted = GC_CatalogueTransfer.Delete(item, m_Player, this);
		string result = string.Format("accepted=%1 exists=%2 contained=%3", accepted, item != null, m_InventoryManager.Contains(item));
		if (!disabled && item)
			SCR_EntityHelper.DeleteEntityAndChildren(item);
		return result;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueRightClick(int row, int equipmentRow)
	{
		Widget widget;
		m_GCTestDeletedItem = null;
		m_aGCTestDeletedContents.Clear();
		if (row >= 0)
			widget = m_GCCataloguePanel.GC_TestRow(row);
		else
		{
			array<SCR_InventorySlotUI> slots = {};
			if (equipmentRow == 200)
			{
				foreach (SCR_InventoryStorageBaseUI storageUI : m_aStorages)
				{
					if (storageUI && storageUI.GetCurrentNavigationStorage() && storageUI.GetCurrentNavigationStorage().GetOwner().GetPrefabData().GetPrefabName().Contains("Jacket_US_BDU.et"))
						storageUI.GetSlots(slots);
				}
			}
			else
				m_pStorageListUI.GetSlots(slots);
			foreach (SCR_InventorySlotUI slot : slots)
			{
				if (equipmentRow != 200 && GridSlot.GetRow(slot.GetWidget()) != equipmentRow)
					continue;
				if (equipmentRow == 200 && !slot.GetInventoryItemComponent())
					continue;
				widget = slot.GetButtonWidget();
				if (slot.GetInventoryItemComponent())
					m_GCTestDeletedItem = slot.GetInventoryItemComponent().GetOwner();
				break;
			}
			array<IEntity> items = {};
			m_InventoryManager.GetItems(items);
			foreach (IEntity item : items)
			{
				InventoryItemComponent inventory = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
				if (inventory && inventory.GetParentSlot() && inventory.GetParentSlot().GetStorage().GetOwner() == m_GCTestDeletedItem)
					m_aGCTestDeletedContents.Insert(item);
			}
		}
		if (!widget)
			return "target absent";
		GC_TestMouse(widget);
		GC_ActionCatalogueShortcut();
		// The existing native hint dispatch must not perform a second move/drop.
		OnAction(null, "Inventory_Drop");
		return TextWidget.Cast(m_widget.FindAnyWidget("Status")).GetText();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueDeletionState()
	{
		int surviving;
		foreach (IEntity item : m_aGCTestDeletedContents)
		{
			if (item)
				surviving++;
		}
		return string.Format("itemExists=%1 contents=%2 survivingContents=%3 locked=%4 listener=%5", m_GCTestDeletedItem != null, m_aGCTestDeletedContents.Count(), surviving, m_InventoryManager.IsInventoryLocked(), m_bGCShortcutBound);
	}

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
		return TextWidget.Cast(m_widget.FindAnyWidget("Status")).GetText();
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
		// Catalogue drops must use the actual hit handler, not native selection/focus left on the source.
		m_pFocusedSlotUI = null;
		Action_OnDrag();
		Action_DragUp();
		return TextWidget.Cast(m_widget.FindAnyWidget("Status")).GetText();
	}
}
#endif
