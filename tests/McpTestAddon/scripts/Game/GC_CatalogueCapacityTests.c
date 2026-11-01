#ifdef WORKBENCH
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	//! Read native limits and optionally exercise the production catalogue insertion into one garment.
	string GC_TestCatalogueCapacity(string itemName, string clothingName, bool insert)
	{
		IEntity owner = GetGame().GetPlayerController().GetControlledEntity();
		SCR_InventoryStorageManagerComponent manager = SCR_InventoryStorageManagerComponent.Cast(owner.FindComponent(SCR_InventoryStorageManagerComponent));
		array<IEntity> items = {};
		manager.GetItems(items);
		ResourceName prefab;
		foreach (IEntity item : items)
		{
			ResourceName candidate = SCR_ResourceNameUtils.GetPrefabName(item);
			if (candidate.Contains(itemName))
			{
				prefab = candidate;
				break;
			}
		}
		if (prefab.IsEmpty())
			return "item missing";
		array<BaseInventoryStorageComponent> storages = {};
		manager.GetStorages(storages);
		string report = "items=" + items.Count() + " locked=" + manager.IsInventoryLocked();
		foreach (BaseInventoryStorageComponent storage : storages)
		{
			if (!SCR_ResourceNameUtils.GetPrefabName(storage.GetOwner()).Contains(clothingName))
				continue;
			BaseInventoryStorageComponent actual = manager.FindActualStorageForItemResource(prefab, storage);
			report += "\n" + storage.Type().ToString() + " occupied=" + storage.GetOccupiedSpace() + " capacity=" + storage.GetMaxVolumeCapacity() + " dimensions=" + storage.GetMaxDimensionCapacity().ToString();
			report += " direct=" + manager.CanInsertResourceInStorage(prefab, storage) + " resolved=" + (actual != null);
			if (actual)
				report += " actual=" + actual.Type().ToString();
			if (insert && (ClothNodeStorageComponent.Cast(storage) || SCR_UniversalInventoryStorageComponent.Cast(storage)))
			{
				report += " dispatched=" + GC_CatalogueTransfer.Insert(GC_ItemCatalogue.ReadResource(prefab), owner, SCR_InventoryMenuUI.GetInventoryMenu(), storage, -1, false);
				break;
			}
		}
		return report;
	}
}
#endif
