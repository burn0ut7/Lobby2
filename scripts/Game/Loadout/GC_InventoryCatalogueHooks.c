#ifdef WORKBENCH
//! Read the same configuration used to build occupied AND empty native equipment slots.
modded class SCR_InventoryStoragesListUI
{
	string GC_GetCatalogueArea(SCR_InventorySlotUI slot)
	{
		if (!slot || slot.GetStorageUI() != this || !m_pInventoryUIConfig)
			return string.Empty;
		array<SCR_LoadoutArea> areas = {};
		m_pInventoryUIConfig.GetValidLoaddoutAreas(areas);
		int row = GridSlot.GetRow(slot.GetWidget());
		if (row < 0 || row >= areas.Count() || !areas[row].m_LoadoutArea)
			return string.Empty;
		return areas[row].m_LoadoutArea.Type().ToString();
	}
}

modded class SCR_InventorySlotWeaponSlotsUI
{
	string GC_GetCatalogueWeaponType()
	{
		return m_sWeaponSlotType;
	}
}

//! Native preset and all right-hand inventory behavior remain owned by the game.
modded class SCR_InventoryMenuUI
{
	protected ref GC_InventoryCataloguePanel m_GCCataloguePanel;
	protected float m_fGCPreviewRefresh;
	protected GC_ItemCatalogueEntry m_GCGestureItem;
	protected SCR_InventorySlotUI m_GCExistingGestureSlot;
	protected bool m_bGCShortcutBound;
	protected bool m_bGCDragged;

	//------------------------------------------------------------------------------------------------
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		if (!GC_WorkbenchItemBrowser.IsActive(m_Player))
		{
			GC_ReleaseCatalogue();
			return;
		}
		if (m_bProcessInitQueue || !m_widget)
			return;
		if (!m_GCCataloguePanel)
		{
			m_GCCataloguePanel = new GC_InventoryCataloguePanel();
			if (!m_GCCataloguePanel.Attach(m_widget))
			{
				GC_ReleaseCatalogue();
				GC_WorkbenchItemBrowser.Disable();
				return;
			}
			GetGame().GetInputManager().AddActionListener("Inventory_Drop", EActionTrigger.DOWN, GC_ActionCatalogueShortcut);
			GetGame().GetInputManager().AddActionListener("GC_CatalogueInspect", EActionTrigger.DOWN, GC_ActionCatalogueInspect);
			GetGame().GetInputManager().AddActionListener("GC_LoadoutFiles", EActionTrigger.DOWN, GC_ActionLoadoutFiles);
			m_bGCShortcutBound = true;
		}
		m_GCCataloguePanel.Tick(tDelta);
		// The native animated preview expires while idle in Workbench. Renew through its owner.
		m_fGCPreviewRefresh += tDelta;
		if (m_fGCPreviewRefresh >= 10 && !m_InspectionScreen)
		{
			UpdatePreview();
			m_fGCPreviewRefresh = 0;
		}
	}

	//------------------------------------------------------------------------------------------------
	override protected void Action_SelectItem()
	{
		if (m_GCCataloguePanel && (m_GCGestureItem || m_GCCataloguePanel.EntryAt(WidgetManager.GetWidgetUnderCursor())))
			return;
		if (m_GCCataloguePanel && GC_WorkbenchItemBrowser.IsActive(m_Player) && GetCanInteract())
			GC_FilterCatalogueSlot(m_pFocusedSlotUI);
		super.Action_SelectItem();
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_FilterCatalogueSlot(SCR_InventorySlotUI slot)
	{
		if (!slot)
			return;
		SCR_InventorySlotGearInspectionUI inspection = SCR_InventorySlotGearInspectionUI.Cast(slot);
		if (inspection)
		{
			GC_SelectInspectionSlot(inspection);
			return;
		}
		SCR_InventorySlotWeaponSlotsUI weapon = SCR_InventorySlotWeaponSlotsUI.Cast(slot);
		if (weapon)
		{
			m_GCCataloguePanel.SetWeaponSlot(m_pWeaponStorageComp, weapon.GetWeaponSlotIndex(), weapon.GC_GetCatalogueWeaponType());
			return;
		}
		SCR_InventoryStoragesListUI clothing = SCR_InventoryStoragesListUI.Cast(slot.GetStorageUI());
		if (clothing)
		{
			string area = clothing.GC_GetCatalogueArea(slot);
			if (!area.IsEmpty())
				m_GCCataloguePanel.SetSlot(area);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_ReleaseCatalogue()
	{
		GC_CancelCatalogueDrag();
		if (m_bGCShortcutBound)
		{
			GetGame().GetInputManager().RemoveActionListener("Inventory_Drop", EActionTrigger.DOWN, GC_ActionCatalogueShortcut);
			GetGame().GetInputManager().RemoveActionListener("GC_CatalogueInspect", EActionTrigger.DOWN, GC_ActionCatalogueInspect);
			GetGame().GetInputManager().RemoveActionListener("GC_LoadoutFiles", EActionTrigger.DOWN, GC_ActionLoadoutFiles);
		}
		m_bGCShortcutBound = false;
		if (m_GCCataloguePanel)
			m_GCCataloguePanel.Detach();
		m_GCCataloguePanel = null;
	}

	//------------------------------------------------------------------------------------------------
	void GC_ActionLoadoutFiles()
	{
		if (!GC_WorkbenchItemBrowser.IsActive(m_Player) || EditBoxWidget.Cast(GetGame().GetWorkspace().GetFocusedWidget()))
			return;
		if (GetGame().GetMenuManager().FindMenuByPreset(ChimeraMenuPreset.GC_LoadoutFilesMenu))
			return;
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.GC_LoadoutFilesMenu, 0, true, false);
	}

	//------------------------------------------------------------------------------------------------
	void GC_EquipCatalogueItem(GC_ItemCatalogueEntry entry)
	{
		if (!GC_WorkbenchItemBrowser.IsActive(m_Player))
			return;
		if (m_GCCataloguePanel && m_GCCataloguePanel.HasInspectionContext())
		{
			InventoryStorageSlot target = m_GCCataloguePanel.GetInspectionTarget();
			if (!target || !GC_CatalogueTransfer.Insert(entry, m_Player, this, target.GetStorage(), target.GetID(), false))
				GC_CatalogueTransferResult(false);
			return;
		}
		if (m_GCCataloguePanel && m_GCCataloguePanel.HasWeaponContext())
		{
			if (!GC_CatalogueTransfer.InsertWeapon(entry, m_Player, this, m_GCCataloguePanel.GetWeaponTarget()))
				GC_CatalogueTransferResult(false);
			return;
		}
		if (!GC_CatalogueTransfer.Insert(entry, m_Player, this))
			GC_CatalogueTransferResult(false);
	}

	//------------------------------------------------------------------------------------------------
	void GC_ClickWeaponCatalogueItem(GC_ItemCatalogueEntry entry)
	{
		if (m_bGCDragged)
		{
			m_bGCDragged = false;
			return;
		}
		if (m_GCCataloguePanel && m_GCCataloguePanel.HasWeaponContext())
			GC_EquipCatalogueItem(entry);
	}

	//------------------------------------------------------------------------------------------------
	void GC_CatalogueTransferResult(bool success, bool deleting = false)
	{
		if (!m_GCCataloguePanel || !GC_WorkbenchItemBrowser.IsActive(m_Player))
			return;
		m_GCCataloguePanel.TransferResult(success, deleting);
		if (success)
			UpdatePreview();
		else
			SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_INV_DROP_ERROR);
	}

	//------------------------------------------------------------------------------------------------
	void GC_PrepareCatalogueRefresh()
	{
		// Native completion rebuilds equipment widgets. Do not preserve a pointer into the removed tree.
		Action_DeselectItem();
		m_pFocusedSlotUI = null;
		m_pActiveHoveredStorageUI = null;
	}

	//------------------------------------------------------------------------------------------------
	//! One scoped listener owns RMB / the native Inventory_Drop shortcut; suppress its old action.
	override void OnAction(SCR_InputButtonComponent comp, string action, SCR_InventoryStorageBaseUI pParentStorage = null, int traverseStorageIndex = -1)
	{
		if (action == "Inventory_Drop" && m_GCCataloguePanel && GC_WorkbenchItemBrowser.IsActive(m_Player))
			return;
		super.OnAction(comp, action, pParentStorage, traverseStorageIndex);
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_ActionCatalogueShortcut()
	{
		if (!m_GCCataloguePanel || !GC_WorkbenchItemBrowser.IsActive(m_Player) || !GetCanInteract() || m_GCGestureItem || m_bDraggingEnabled)
			return;
		Widget cursor = WidgetManager.GetWidgetUnderCursor();
		GC_ItemCatalogueEntry entry = m_GCCataloguePanel.EntryAt(cursor);
		if (entry)
		{
			GC_EquipCatalogueItem(entry);
			return;
		}
		SCR_InventorySlotUI slot = GC_CatalogueSlotAt(cursor);
		if (!slot || SCR_InventoryStorageQuickSlotsUI.Cast(slot.GetStorageUI()))
			return;
		InventoryItemComponent inventory = slot.GetInventoryItemComponent();
		if (!inventory)
			return;
		IEntity item = inventory.GetOwner();
		Action_DeselectItem();
		if (m_pGearInspectionPointUI && !SCR_InventorySlotWeaponSlotsUI.Cast(slot))
		{
			if (!GC_CatalogueTransfer.MoveAvailable(item, m_Player, this))
				GC_CatalogueTransferResult(false);
			return;
		}
		if (!GC_CatalogueTransfer.Delete(item, m_Player, this))
			GC_CatalogueTransferResult(false, true);
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_ActionCatalogueInspect()
	{
		if (!m_GCCataloguePanel || !GC_WorkbenchItemBrowser.IsActive(m_Player) || !GetCanInteract() || m_bDraggingEnabled || m_GCGestureItem)
			return;
		SCR_InventorySlotUI slot = GC_CatalogueSlotAt(WidgetManager.GetWidgetUnderCursor());
		if (!slot || !slot.GetInventoryItemComponent())
			return;
		IEntity item = slot.GetInventoryItemComponent().GetOwner();
		if (!item.FindComponent(WeaponAttachmentsStorageComponent) || !m_InventoryManager.Contains(item))
			return;
		GC_FilterCatalogueSlot(slot);
		if (m_pInspectedSlot == slot)
			InspectItem(null);
		else
			InspectItem(slot);
	}

	//------------------------------------------------------------------------------------------------
	void GC_SelectInspectionSlot(SCR_InventorySlotGearInspectionUI slot)
	{
		if (!m_GCCataloguePanel || !GC_WorkbenchItemBrowser.IsActive(m_Player) || !slot)
			return;
		SCR_InventoryStorageBaseUI storageUI = slot.GetStorageUI();
		BaseInventoryStorageComponent storage = storageUI.GetCurrentNavigationStorage();
		int id = storageUI.GetSlotId(slot);
		if (storage && id >= 0 && id < storage.GetSlotsCount())
			m_GCCataloguePanel.SetInspectionSlot(storage.GetSlot(id), slot.m_tAttachmentType);
	}

	//------------------------------------------------------------------------------------------------
	//! Native focus can remain on the source during a data-only drag. Resolve the actual hit handler.
	protected SCR_InventorySlotUI GC_CatalogueSlotAt(Widget cursor)
	{
		while (cursor && cursor != m_widget)
		{
			SCR_InventorySlotUI slot = SCR_InventorySlotUI.Cast(cursor.FindHandler(SCR_InventorySlotUI));
			if (slot)
				return slot;
			cursor = cursor.GetParent();
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected SCR_InventoryStorageBaseUI GC_CatalogueStorageAt(Widget cursor)
	{
		while (cursor && cursor != m_widget)
		{
			SCR_InventoryStorageBaseUI storage = SCR_InventoryStorageBaseUI.Cast(cursor.FindHandler(SCR_InventoryStorageBaseUI));
			if (storage)
				return storage;
			cursor = cursor.GetParent();
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Native drag input, threshold and cursor preview; catalogue items have no live source slot.
	override void Action_DragDown()
	{
		m_bGCDragged = false;
		m_GCExistingGestureSlot = null;
		if (m_GCCataloguePanel && GC_WorkbenchItemBrowser.IsActive(m_Player))
		{
			GC_ItemCatalogueEntry entry = m_GCCataloguePanel.EntryAt(WidgetManager.GetWidgetUnderCursor());
			if (!entry)
			{
				SCR_InventorySlotUI source = GC_CatalogueSlotAt(WidgetManager.GetWidgetUnderCursor());
				if (source && source.IsDraggable() && source.GetInventoryItemComponent() && !SCR_InventoryStorageQuickSlotsUI.Cast(source.GetStorageUI()))
				{
					IEntity item = source.GetInventoryItemComponent().GetOwner();
					if (m_InventoryManager.Contains(item))
					{
						m_GCExistingGestureSlot = source;
					}
				}
			}
			if (entry)
			{
				m_GCGestureItem = entry;
				WidgetManager.GetMousePos(m_iMouseX, m_iMouseY);
				GetGame().GetInputManager().AddActionListener("Inventory_Drag", EActionTrigger.PRESSED, Action_OnDrag);
				GetGame().GetInputManager().AddActionListener("Inventory_Drag", EActionTrigger.UP, Action_DragUp);
				return;
			}
		}
		super.Action_DragDown();
	}

	//------------------------------------------------------------------------------------------------
	override void Action_OnDrag()
	{
		if (!m_GCGestureItem)
		{
			// Retain the pressed source while native code owns preview, highlights and drag state.
			SCR_InventorySlotUI focused = m_pFocusedSlotUI;
			if (m_GCExistingGestureSlot && !m_bDraggingEnabled && GC_WorkbenchItemBrowser.IsActive(m_Player))
			{
				m_pFocusedSlotUI = m_GCExistingGestureSlot;
				m_pSelectedSlotUI = m_GCExistingGestureSlot;
			}
			super.Action_OnDrag();
			m_pFocusedSlotUI = focused;
			return;
		}
		if (!GC_WorkbenchItemBrowser.IsActive(m_Player))
		{
			GC_CancelCatalogueDrag();
			return;
		}
		int x, y;
		WidgetManager.GetMousePos(x, y);
		if (!m_bDraggingEnabled)
		{
			if (Math.AbsInt(x - m_iMouseX) < DRAG_THRESHOLD && Math.AbsInt(y - m_iMouseY) < DRAG_THRESHOLD)
				return;
			m_bDraggingEnabled = true;
			m_bGCDragged = true;
			m_wDragDropContainer.SetVisible(true);
			m_pPreviewManager.SetPreviewItemFromPrefab(m_pDragDropPreviewImage, m_GCGestureItem.m_sPrefab);
		}
		ContainerSetPos(x, y);
	}

	//------------------------------------------------------------------------------------------------
	override void Action_DragUp()
	{
		if (!m_GCGestureItem)
		{
			super.Action_DragUp();
			m_GCExistingGestureSlot = null;
			return;
		}
		GC_ItemCatalogueEntry entry = m_GCGestureItem;
		bool dragged = m_bDraggingEnabled;
		GC_CancelCatalogueDrag();
		if (dragged && !GC_DropCatalogueItem(entry))
			GC_CatalogueTransferResult(false);
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_CancelCatalogueDrag()
	{
		m_GCExistingGestureSlot = null;
		if (!m_GCGestureItem)
			return;
		GetGame().GetInputManager().RemoveActionListener("Inventory_Drag", EActionTrigger.PRESSED, Action_OnDrag);
		GetGame().GetInputManager().RemoveActionListener("Inventory_Drag", EActionTrigger.UP, Action_DragUp);
		m_pPreviewManager.SetPreviewItem(m_pDragDropPreviewImage, null);
		m_wDragDropContainer.SetVisible(false);
		m_bDraggingEnabled = false;
		m_GCGestureItem = null;
	}

	//------------------------------------------------------------------------------------------------
	override protected void Action_Drop()
	{
		if (!m_GCCataloguePanel || !GC_WorkbenchItemBrowser.IsActive(m_Player) || !m_pSelectedSlotUI || !m_pSelectedSlotUI.GetInventoryItemComponent())
		{
			super.Action_Drop();
			return;
		}
		IEntity item = m_pSelectedSlotUI.GetInventoryItemComponent().GetOwner();
		Widget cursor = WidgetManager.GetWidgetUnderCursor();
		SCR_InventorySlotUI slot = GC_CatalogueSlotAt(cursor);
		SCR_InventoryStorageBaseUI storageUI = GC_CatalogueStorageAt(cursor);
		if (slot)
			storageUI = slot.GetStorageUI();
		bool nativePreviewTarget = cursor == m_wPlayerRender || (cursor && cursor.GetName() == "HitZoneButton");
		if (!storageUI && !nativePreviewTarget && m_InventoryManager.Contains(item) && !SCR_InventoryStorageQuickSlotsUI.Cast(m_pSelectedSlotUI.GetStorageUI()))
		{
			if (!GC_CatalogueTransfer.Drop(item, m_Player, this))
				GC_CatalogueTransferResult(false);
			return;
		}
		if (SCR_InventorySlotGearInspectionUI.Cast(slot))
		{
			if (!GC_CatalogueTransfer.MoveTo(item, m_Player, this, storageUI.GetCurrentNavigationStorage(), storageUI.GetSlotId(slot)))
				GC_CatalogueTransferResult(false);
			return;
		}
		// All ordinary container, equipment and quick-slot destinations remain native.
		m_pFocusedSlotUI = slot;
		m_pActiveHoveredStorageUI = storageUI;
		super.Action_Drop();
	}

	//------------------------------------------------------------------------------------------------
	protected bool GC_DropCatalogueItem(GC_ItemCatalogueEntry entry)
	{
		if (!GC_WorkbenchItemBrowser.IsActive(m_Player))
			return false;
		Widget cursor = WidgetManager.GetWidgetUnderCursor();
		if (cursor == m_wPlayerRender)
			return GC_CatalogueTransfer.Insert(entry, m_Player, this);
		SCR_InventorySlotUI slot = GC_CatalogueSlotAt(cursor);
		SCR_InventoryStorageBaseUI storageUI = GC_CatalogueStorageAt(cursor);
		if (slot)
			storageUI = slot.GetStorageUI();
		if (!storageUI || SCR_InventoryStorageQuickSlotsUI.Cast(storageUI))
			return false;
		if (SCR_InventorySlotGearInspectionUI.Cast(slot))
		{
			BaseInventoryStorageComponent inspectionStorage = storageUI.GetCurrentNavigationStorage();
			int inspectionID = storageUI.GetSlotId(slot);
			return GC_CatalogueTransfer.Insert(entry, m_Player, this, inspectionStorage, inspectionID, false);
		}
		SCR_InventorySlotWeaponSlotsUI weapon = SCR_InventorySlotWeaponSlotsUI.Cast(slot);
		if (weapon)
			return GC_CatalogueTransfer.Insert(entry, m_Player, this, m_pWeaponStorageComp, weapon.GetWeaponSlotIndex(), false);
		SCR_InventoryStoragesListUI clothing = SCR_InventoryStoragesListUI.Cast(storageUI);
		if (clothing)
		{
			if (!slot)
				return false;
			string area = clothing.GC_GetCatalogueArea(slot);
			if (!area.IsEmpty() && area == entry.m_sEquipmentSlot)
				return GC_CatalogueTransfer.Insert(entry, m_Player, this);
			// A consumable dropped on the shirt icon goes inside that shirt, not another container.
			InventoryItemComponent item = slot.GetInventoryItemComponent();
			if (!item)
				return false;
			BaseInventoryStorageComponent container = BaseInventoryStorageComponent.Cast(item.GetOwner().FindComponent(BaseInventoryStorageComponent));
			return GC_CatalogueTransfer.Insert(entry, m_Player, this, container, -1, false);
		}
		BaseInventoryStorageComponent storage = storageUI.GetCurrentNavigationStorage();
		return GC_CatalogueTransfer.Insert(entry, m_Player, this, storage, -1, false);
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		GC_ReleaseCatalogue();
		super.OnMenuClose();
	}
}
#endif
