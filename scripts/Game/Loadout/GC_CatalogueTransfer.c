#ifdef WORKBENCH
//! Local workbench transfers. Preview entities are never moved into the player's inventory.
class GC_CatalogueTransfer
{
	//------------------------------------------------------------------------------------------------
	//! Only the selected weapon's attachment storage, never another weapon or a deposit container.
	static bool InsertWeapon(GC_ItemCatalogueEntry entry, IEntity owner, SCR_InventoryMenuUI menu, IEntity weapon)
	{
		if (!entry || !weapon || !GC_WorkbenchItemBrowser.IsActive(owner))
			return false;
		SCR_InventoryStorageManagerComponent manager = SCR_InventoryStorageManagerComponent.Cast(owner.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!manager || manager.IsInventoryLocked() || !manager.Contains(weapon))
			return false;
		array<BaseInventoryStorageComponent> ownedStorages = {};
		manager.GetStorages(ownedStorages);
		array<BaseInventoryStorageComponent> weaponStorages = {};
		foreach (BaseInventoryStorageComponent candidate : ownedStorages)
		{
			IEntity parent = candidate.GetOwner();
			while (parent && parent != weapon)
			{
				InventoryItemComponent inventory = InventoryItemComponent.Cast(parent.FindComponent(InventoryItemComponent));
				if (!inventory || !inventory.GetParentSlot())
					break;
				parent = inventory.GetParentSlot().GetStorage().GetOwner();
			}
			if (parent == weapon)
				weaponStorages.Insert(candidate);
		}
		ChimeraWorld world = GetGame().GetWorld();
		IEntity preview = world.GetItemPreviewManager().ResolvePreviewEntityForPrefab(entry.m_sPrefab);
		if (!preview)
			return false;
		for (int replace = 0; replace < 2; replace++)
		{
			foreach (BaseInventoryStorageComponent storage : weaponStorages)
			{
				for (int i; i < storage.GetSlotsCount(); i++)
				{
					bool occupied = storage.GetSlot(i).GetAttachedEntity() != null;
					if (occupied != (replace == 1))
						continue;
					if ((!occupied && manager.CanInsertResourceInStorage(entry.m_sPrefab, storage, i)) || (occupied && manager.CanReplaceItem(preview, storage, i)))
						return Insert(entry, owner, menu, storage, i, false);
				}
			}
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	static bool Delete(IEntity item, IEntity owner, SCR_InventoryMenuUI menu)
	{
		if (!item || item == owner || !GC_WorkbenchItemBrowser.IsActive(owner))
			return false;
		SCR_InventoryStorageManagerComponent manager = SCR_InventoryStorageManagerComponent.Cast(owner.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!manager || manager.IsInventoryLocked() || !manager.Contains(item))
			return false;
		InventoryItemComponent inventory = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
		if (!inventory || !inventory.GetParentSlot())
			return false;
		array<BaseInventoryStorageComponent> ownedStorages = {};
		manager.GetStorages(ownedStorages);
		if (!ownedStorages.Contains(inventory.GetParentSlot().GetStorage()))
			return false;
		GC_CatalogueTransferCallback callback = new GC_CatalogueTransferCallback();
		callback.m_Owner = owner;
		callback.m_Manager = manager;
		callback.m_pItem = item;
		callback.m_pMenu = menu;
		callback.m_bDeleting = true;
		manager.SetInventoryLocked(true);
		bool accepted = manager.TryDeleteItem(item, callback);
		if (!accepted)
			callback.InternalFailed();
		return accepted;
	}

	//------------------------------------------------------------------------------------------------
	static bool FindSlot(GC_ItemCatalogueEntry entry, SCR_InventoryStorageManagerComponent manager, IEntity preview, out BaseInventoryStorageComponent target, out int slotID)
	{
		array<EStoragePurpose> purposes = {EStoragePurpose.PURPOSE_LOADOUT_PROXY, EStoragePurpose.PURPOSE_WEAPON_PROXY, EStoragePurpose.PURPOSE_GADGET_PROXY, EStoragePurpose.PURPOSE_EQUIPMENT_ATTACHMENT};
		foreach (EStoragePurpose purpose : purposes)
		{
			array<BaseInventoryStorageComponent> storages = {};
			manager.GetStorages(storages, purpose);
			// Prefer an empty compatible equipment slot before replacing equipped gear.
			for (int replace = 0; replace < 2; replace++)
			{
				foreach (BaseInventoryStorageComponent storage : storages)
				{
					for (int i; i < storage.GetSlotsCount(); i++)
					{
						bool occupied = storage.GetSlot(i).GetAttachedEntity() != null;
						if (occupied != (replace == 1))
							continue;
						if ((!occupied && manager.CanInsertResourceInStorage(entry.m_sPrefab, storage, i)) || (occupied && manager.CanReplaceItem(preview, storage, i)))
						{
							target = storage;
							slotID = i;
							return true;
						}
					}
				}
			}
		}
		target = manager.FindStorageForResource(entry.m_sPrefab, EStoragePurpose.PURPOSE_DEPOSIT);
		slotID = -1;
		return target != null;
	}

	//------------------------------------------------------------------------------------------------
	static bool Insert(GC_ItemCatalogueEntry entry, IEntity owner, SCR_InventoryMenuUI menu, BaseInventoryStorageComponent target = null, int slotID = -1, bool automatic = true)
	{
		if (!entry || !entry.m_bPortable || !GC_WorkbenchItemBrowser.IsActive(owner))
			return false;
		SCR_InventoryStorageManagerComponent manager = SCR_InventoryStorageManagerComponent.Cast(owner.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!manager || manager.IsInventoryLocked())
			return false;
		ChimeraWorld world = GetGame().GetWorld();
		IEntity preview = world.GetItemPreviewManager().ResolvePreviewEntityForPrefab(entry.m_sPrefab);
		if (!preview)
			return false;
		if (automatic && !FindSlot(entry, manager, preview, target, slotID))
			return false;
		array<BaseInventoryStorageComponent> ownedStorages = {};
		manager.GetStorages(ownedStorages);
		if (!target || !ownedStorages.Contains(target) || slotID < -1 || slotID >= target.GetSlotsCount())
			return false;
		bool replace = slotID >= 0 && target.GetSlot(slotID).GetAttachedEntity() != null;
		if (replace)
		{
			if (!manager.CanReplaceItem(preview, target, slotID))
				return false;
		}
		else if (!manager.CanInsertResourceInStorage(entry.m_sPrefab, target, slotID))
			return false;

		// Spawn only after the destination is validated; the callback removes a failed insertion.
		EntitySpawnParams spawn = new EntitySpawnParams();
		spawn.Transform[3] = owner.GetOrigin();
		IEntity item = GetGame().SpawnEntityPrefab(Resource.Load(entry.m_sPrefab), world, spawn);
		if (!item)
			return false;
		GC_CatalogueTransferCallback callback = new GC_CatalogueTransferCallback();
		callback.m_Owner = owner;
		callback.m_Manager = manager;
		callback.m_pItem = item;
		callback.m_pMenu = menu;
		manager.SetInventoryLocked(true);
		bool accepted;
		if (replace)
			accepted = manager.TryReplaceItem(item, target, slotID, callback);
		else
			accepted = manager.TryInsertItemInStorage(item, target, slotID, callback);
		if (!accepted)
			callback.InternalFailed();
		return accepted;
	}
}

class GC_CatalogueTransferCallback : SCR_InvCallBack
{
	IEntity m_Owner;
	SCR_InventoryStorageManagerComponent m_Manager;
	bool m_bDeleting;
	protected bool m_bFinished;

	//------------------------------------------------------------------------------------------------
	override protected void OnFailed()
	{
		if (m_bFinished)
			return;
		m_bFinished = true;
		if (m_Manager)
			m_Manager.SetInventoryLocked(false);
		if (!m_bDeleting && m_pItem)
		{
			InventoryItemComponent item = InventoryItemComponent.Cast(m_pItem.FindComponent(InventoryItemComponent));
			if (!item || !item.GetParentSlot())
				SCR_EntityHelper.DeleteEntityAndChildren(m_pItem);
		}
		if (GC_WorkbenchItemBrowser.IsActive(m_Owner) && m_pMenu)
			m_pMenu.GC_CatalogueTransferResult(false, m_bDeleting);
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnComplete()
	{
		if (m_bFinished)
			return;
		m_bFinished = true;
		if (m_Manager)
			m_Manager.SetInventoryLocked(false);
		if (!GC_WorkbenchItemBrowser.IsActive(m_Owner) || !m_pMenu)
			return;
		m_pMenu.GC_PrepareCatalogueRefresh();
		super.OnComplete();
		m_pMenu.GC_CatalogueTransferResult(true, m_bDeleting);
	}
}
#endif
