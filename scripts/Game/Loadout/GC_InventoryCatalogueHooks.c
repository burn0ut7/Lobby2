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
		if (m_GCCataloguePanel)
			m_GCCataloguePanel.Detach();
		m_GCCataloguePanel = null;
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		GC_ReleaseCatalogue();
		super.OnMenuClose();
	}
}
#endif
