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
	protected bool m_bGCSuppressClick;

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
		SCR_InventorySlotWeaponSlotsUI weapon = SCR_InventorySlotWeaponSlotsUI.Cast(slot);
		if (weapon)
		{
			m_GCCataloguePanel.SetSlot(weapon.GC_GetCatalogueWeaponType());
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
		if (m_GCCataloguePanel)
			m_GCCataloguePanel.Detach();
		m_GCCataloguePanel = null;
	}

	//------------------------------------------------------------------------------------------------
	void GC_ClickCatalogueItem(GC_ItemCatalogueEntry entry)
	{
		if (m_bGCSuppressClick || !GC_WorkbenchItemBrowser.IsActive(m_Player))
			return;
		if (!GC_CatalogueTransfer.Insert(entry, m_Player, this))
			GC_CatalogueTransferResult(false);
	}

	//------------------------------------------------------------------------------------------------
	void GC_CatalogueTransferResult(bool success)
	{
		if (!m_GCCataloguePanel || !GC_WorkbenchItemBrowser.IsActive(m_Player))
			return;
		m_GCCataloguePanel.TransferResult(success);
		if (success)
			UpdatePreview();
		else
			SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_INV_DROP_ERROR);
	}

	//------------------------------------------------------------------------------------------------
	//! Native drag input, threshold and cursor preview; catalogue items have no live source slot.
	override void Action_DragDown()
	{
		if (m_GCCataloguePanel && GC_WorkbenchItemBrowser.IsActive(m_Player))
		{
			GC_ItemCatalogueEntry entry = m_GCCataloguePanel.EntryAt(WidgetManager.GetWidgetUnderCursor());
			if (entry)
			{
				m_GCGestureItem = entry;
				m_bGCSuppressClick = false;
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
			super.Action_OnDrag();
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
			m_bGCSuppressClick = true;
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
	protected bool GC_DropCatalogueItem(GC_ItemCatalogueEntry entry)
	{
		if (!GC_WorkbenchItemBrowser.IsActive(m_Player))
			return false;
		Widget cursor = WidgetManager.GetWidgetUnderCursor();
		if (cursor == m_wPlayerRender)
			return GC_CatalogueTransfer.Insert(entry, m_Player, this);
		if (!m_pFocusedSlotUI || cursor != m_pFocusedSlotUI.GetButtonWidget())
			return false;
		SCR_InventorySlotWeaponSlotsUI weapon = SCR_InventorySlotWeaponSlotsUI.Cast(m_pFocusedSlotUI);
		if (weapon)
			return GC_CatalogueTransfer.Insert(entry, m_Player, this, m_pWeaponStorageComp, weapon.GetWeaponSlotIndex(), false);
		SCR_InventoryStoragesListUI clothing = SCR_InventoryStoragesListUI.Cast(m_pFocusedSlotUI.GetStorageUI());
		if (clothing)
		{
			if (clothing.GC_GetCatalogueArea(m_pFocusedSlotUI) != entry.m_sEquipmentSlot)
				return false;
			// The engine resolves the actual slot behind occupied and empty loadout icons.
			return GC_CatalogueTransfer.Insert(entry, m_Player, this);
		}
		if (SCR_InventoryStorageQuickSlotsUI.Cast(m_pFocusedSlotUI.GetStorageUI()))
			return false;
		BaseInventoryStorageComponent storage = m_pFocusedSlotUI.GetStorageUI().GetCurrentNavigationStorage();
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
