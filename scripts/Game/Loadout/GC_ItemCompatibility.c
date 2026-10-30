enum GC_EItemCompatibility
{
	UNVERIFIED,
	ACCEPTED,
	REJECTED
}

//! Read-only checks for the eventual arsenal-style inventory integration.
//! Results describe the supplied live target, including occupancy and capacity.
class GC_ItemCompatibility
{
	//------------------------------------------------------------------------------------------------
	static GC_EItemCompatibility CheckInsertion(ResourceName prefab, InventoryStorageManagerComponent manager, BaseInventoryStorageComponent storage, int slotID)
	{
		if (prefab.IsEmpty() || !manager || !storage || slotID < 0 || slotID >= storage.GetSlotsCount())
			return GC_EItemCompatibility.UNVERIFIED;
		array<BaseInventoryStorageComponent> storages = {};
		manager.GetStorages(storages);
		if (storages.Find(storage) < 0)
			return GC_EItemCompatibility.UNVERIFIED;
		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return GC_EItemCompatibility.UNVERIFIED;
		if (manager.CanInsertResourceInStorage(prefab, storage, slotID))
			return GC_EItemCompatibility.ACCEPTED;
		return GC_EItemCompatibility.REJECTED;
	}

	//------------------------------------------------------------------------------------------------
	//! Caller supplies existing entities; this method never spawns or attaches an item.
	static GC_EItemCompatibility CheckAttachment(IEntity item, AttachmentSlotComponent slot)
	{
		if (!item || !slot)
			return GC_EItemCompatibility.UNVERIFIED;
		if (slot.CanSetAttachment(item))
			return GC_EItemCompatibility.ACCEPTED;
		return GC_EItemCompatibility.REJECTED;
	}
}
