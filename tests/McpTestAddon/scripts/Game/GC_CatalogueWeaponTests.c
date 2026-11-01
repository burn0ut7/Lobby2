#ifdef WORKBENCH
modded class GC_LobbyManager
{
	string GC_TestCatalogueTrailingClick(int row)
	{
		return SCR_InventoryMenuUI.GetInventoryMenu().GC_TestCatalogueTrailingClick(row);
	}

	string GC_TestCatalogueWeaponContents(int slotID)
	{
		return SCR_InventoryMenuUI.GetInventoryMenu().GC_TestCatalogueWeaponContents(slotID);
	}

	string GC_TestCatalogueWeapon(int slotID, bool remove)
	{
		return SCR_InventoryMenuUI.GetInventoryMenu().GC_TestCatalogueWeapon(slotID, remove);
	}

	string GC_TestCatalogueWeaponMatches()
	{
		return SCR_InventoryMenuUI.GetInventoryMenu().GC_TestCatalogueWeaponMatches();
	}
}

modded class SCR_InventoryMenuUI
{
	string GC_TestCatalogueTrailingClick(int row)
	{
		Widget widget = m_GCCataloguePanel.GC_TestRow(row);
		bool suppressed = m_bGCDragged;
		SCR_ButtonBaseComponent.Cast(widget.FindHandler(SCR_ButtonBaseComponent)).OnClick(widget, 0, 0, 0);
		return string.Format("suppressed=%1 consumed=%2", suppressed, !m_bGCDragged);
	}

	string GC_TestCatalogueWeaponContents(int slotID)
	{
		IEntity weapon = m_pWeaponStorageComp.GetSlot(slotID).GetAttachedEntity();
		if (!weapon)
			return "empty";
		BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(weapon.FindComponent(WeaponAttachmentsStorageComponent));
		string result = weapon.GetPrefabData().GetPrefabName();
		for (int i; storage && i < storage.GetSlotsCount(); i++)
		{
			IEntity item = storage.GetSlot(i).GetAttachedEntity();
			if (item)
				result += "\n" + i.ToString() + " | " + item.GetPrefabData().GetPrefabName();
		}
		return result;
	}

	string GC_TestCatalogueWeapon(int slotID, bool remove)
	{
		array<SCR_InventorySlotUI> slots = {};
		m_pWeaponStorage.GetSlots(slots);
		foreach (SCR_InventorySlotUI slot : slots)
		{
			SCR_InventorySlotWeaponSlotsUI weapon = SCR_InventorySlotWeaponSlotsUI.Cast(slot);
			if (!weapon || weapon.GetWeaponSlotIndex() != slotID)
				continue;
			GC_TestMouse(slot.GetButtonWidget());
			if (remove)
				GC_ActionCatalogueShortcut();
			else
				Action_SelectItem();
			return m_GCCataloguePanel.State();
		}
		return "weapon slot absent";
	}

	string GC_TestCatalogueWeaponMatches()
	{
		return m_GCCataloguePanel.GC_TestCatalogueWeaponMatches();
	}
}

modded class GC_InventoryCataloguePanel
{
	string GC_TestCatalogueWeaponMatches()
	{
		string result = State() + " context=" + m_sWeaponSignature;
		array<GC_ItemCatalogueEntry> matches = {};
		int total = m_Catalogue.Search(m_Query, 0, 100, matches);
		foreach (GC_ItemCatalogueEntry entry : matches)
			result += "\n" + entry.m_sCategory + " | " + entry.m_sName + " | " + entry.m_sPrefab;
		return result;
	}
}
#endif
