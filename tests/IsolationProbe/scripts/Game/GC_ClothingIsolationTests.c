//! Addon-independent native inventory checks, identical in baseline and loaded runs.
#ifdef WORKBENCH
modded class GC_IsolationProbeComponent
{
	protected IEntity m_GC_ProbeClothingItem;
	protected BaseInventoryStorageComponent m_GC_ProbeClothingStorage;
	protected int m_iGC_ProbeClothingSlot;

	//------------------------------------------------------------------------------------------------
	string Clothing(string action)
	{
		IEntity character = GetGame().GetPlayerController().GetControlledEntity();
		SCR_CharacterInventoryStorageComponent storage = SCR_CharacterInventoryStorageComponent.Cast(character.FindComponent(SCR_CharacterInventoryStorageComponent));
		InventoryStorageManagerComponent manager = InventoryStorageManagerComponent.Cast(character.FindComponent(InventoryStorageManagerComponent));
		if (!storage || !manager)
			return "UNAVAILABLE inventory";
		IEntity pants = storage.GetClothFromArea(LoadoutPantsArea);
		bool accepted;
		if (action == "remove" || action == "invalid-replace")
		{
			InventoryItemComponent inventory = InventoryItemComponent.Cast(pants.FindComponent(InventoryItemComponent));
			m_GC_ProbeClothingItem = pants;
			m_GC_ProbeClothingStorage = inventory.GetParentSlot().GetStorage();
			for (int i; i < m_GC_ProbeClothingStorage.GetSlotsCount(); i++)
			{
				if (m_GC_ProbeClothingStorage.GetSlot(i).GetAttachedEntity() == pants)
					m_iGC_ProbeClothingSlot = i;
			}
			if (action == "remove")
				accepted = manager.TryRemoveItemFromStorage(pants, m_GC_ProbeClothingStorage);
			else
				accepted = manager.TryReplaceItem(storage.GetClothFromArea(LoadoutHeadCoverArea), m_GC_ProbeClothingStorage, m_iGC_ProbeClothingSlot);
		}
		if (action == "restore")
			accepted = manager.TryInsertItemInStorage(m_GC_ProbeClothingItem, m_GC_ProbeClothingStorage, m_iGC_ProbeClothingSlot);
		if (action == "delete")
		{
			m_GC_ProbeClothingItem = storage.GetClothFromArea(LoadoutBootsArea);
			accepted = manager.TryDeleteItem(m_GC_ProbeClothingItem);
		}
		string result = string.Format("action=%1 accepted=%2 originalExists=%3", action, accepted, m_GC_ProbeClothingItem != null);
		array<typename> areas = {LoadoutJacketArea, LoadoutPantsArea, LoadoutBootsArea, LoadoutHeadCoverArea};
		foreach (typename area : areas)
		{
			IEntity item = storage.GetClothFromArea(area);
			result += " | " + area.ToString();
			if (!item)
			{
				result += " empty";
				continue;
			}
			InventoryItemComponent inventory = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
			result += " canRemove=" + manager.CanRemoveItemFromStorage(item, inventory.GetParentSlot().GetStorage()).ToString();
		}
		return result;
	}
}
#endif
