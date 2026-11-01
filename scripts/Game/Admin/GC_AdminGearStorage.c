//! Only crates explicitly selected by an authenticated admin gain unlimited storage.
modded class SCR_UniversalInventoryStorageComponent
{
	[RplProp()]
	protected bool m_bGC_AdminGearCapacity;

	//------------------------------------------------------------------------------------------------
	void GC_SetAdminGearCapacity(bool enabled)
	{
		if (!Replication.IsServer() || m_bGC_AdminGearCapacity == enabled)
			return;
		m_bGC_AdminGearCapacity = enabled;
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	bool GC_HasAdminGearCapacity()
	{
		IEntity owner = GetOwner();
		return m_bGC_AdminGearCapacity && GC_LobbyManager.GetManager() && owner && !owner.GetParent() && !GetParentSlot()
			&& !ChimeraCharacter.Cast(owner) && !Vehicle.Cast(owner);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanStoreItem(IEntity item, int slotID)
	{
		if (!GC_HasAdminGearCapacity())
			return super.CanStoreItem(item, slotID);
		// The stock universal handler adds volume/dimension/weight limits to the base
		// permissive storage event. Native manager insertion still owns slot/transfer validity.
		return item && item != GetOwner() && item.FindComponent(InventoryItemComponent);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanStoreResource(ResourceName resourceName, int slotID)
	{
		if (!GC_HasAdminGearCapacity())
			return super.CanStoreResource(resourceName, slotID);
		return !resourceName.IsEmpty();
	}
}

modded class GC_LobbyManager
{
	protected ref array<SCR_UniversalInventoryStorageComponent> m_aGC_AdminGearCrates = {};

	//------------------------------------------------------------------------------------------------
	void GC_EnableGearCrateCapacity(SCR_UniversalInventoryStorageComponent storage)
	{
		if (!Replication.IsServer() || GetManager() != this || !storage)
			return;
		if (!m_aGC_AdminGearCrates.Contains(storage))
			m_aGC_AdminGearCrates.Insert(storage);
		storage.GC_SetAdminGearCapacity(true);
	}

	//------------------------------------------------------------------------------------------------
	override protected void Shutdown()
	{
		foreach (SCR_UniversalInventoryStorageComponent storage : m_aGC_AdminGearCrates)
		{
			if (storage)
				storage.GC_SetAdminGearCapacity(false);
		}
		m_aGC_AdminGearCrates.Clear();
		super.Shutdown();
	}
}
