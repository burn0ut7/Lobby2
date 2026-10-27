//! Applies an arsenal loadout to a client-local killed-from preview.
//! The base arsenal loader uses TrySpawnPrefabToStorage, which is intended for
//! gameplay inventory creation. Preview actors and every item added to them are
//! local-only, so changed gear is spawned locally and then inserted into the
//! captured storage slot instead.
class GC_KilledFromLoadoutApplicator : SCR_PlayerArsenalLoadout
{
	static bool Apply(IEntity owner, LoadContext context, string role)
	{
		if (!owner || !context.StartObject(ARSENALLOADOUT_KEY))
		{
			PrintFormat("GC | Killed From | event=preview-loadout result=failed role=%1 stage=root", role, level: LogLevel.ERROR);
			return false;
		}

		InventoryStorageManagerComponent manager = InventoryStorageManagerComponent.Cast(owner.FindComponent(InventoryStorageManagerComponent));
		if (!manager)
		{
			PrintFormat("GC | Killed From | event=preview-loadout result=failed role=%1 stage=manager", role, level: LogLevel.ERROR);
			return false;
		}

		if (!ApplyEntity(owner, context, manager, role, "character"))
			return false;

		if (!ApplyCharacterDataLoadoutString(owner, context))
		{
			PrintFormat("GC | Killed From | event=preview-loadout result=failed role=%1 stage=character-data", role, level: LogLevel.ERROR);
			return false;
		}

		if (!context.EndObject())
		{
			PrintFormat("GC | Killed From | event=preview-loadout result=failed role=%1 stage=root-end", role, level: LogLevel.ERROR);
			return false;
		}

		PrintFormat("GC | Killed From | event=preview-loadout result=success role=%1", role);
		return true;
	}

	protected static bool ApplyEntity(
		IEntity owner,
		LoadContext context,
		InventoryStorageManagerComponent manager,
		string role,
		string path,
		BaseInventoryStorageComponent parentStorage = null,
		int slotId = -1)
	{
		if (parentStorage && slotId != -1)
		{
			ResourceName prefab;
			if (!context.Read(prefab))
			{
				PrintFormat("GC | Killed From | event=preview-loadout result=failed role=%1 stage=prefab-read path=%2 slot=%3", role, path, slotId, level: LogLevel.ERROR);
				return false;
			}

			ResourceName currentPrefab;
			if (owner)
				currentPrefab = SCR_ResourceNameUtils.GetPrefabName(owner);

			if (prefab != currentPrefab)
			{
				// These are exclusively local preview entities. Gameplay inventory
				// deletion can acknowledge a request without deleting on a client.
				if (owner)
					delete owner;

				Resource resource = Resource.Load(prefab);
				if (!resource || !resource.IsValid())
				{
					PrintFormat("GC | Killed From | event=preview-loadout result=failed role=%1 stage=resource path=%2 slot=%3 prefab=%4", role, path, slotId, prefab, level: LogLevel.ERROR);
					return false;
				}

				IEntity spawnedItem = GetGame().SpawnEntityPrefabLocal(resource, manager.GetOwner().GetWorld());
				if (!spawnedItem)
				{
					PrintFormat("GC | Killed From | event=preview-loadout result=failed role=%1 stage=local-spawn path=%2 slot=%3 prefab=%4", role, path, slotId, prefab, level: LogLevel.ERROR);
					return false;
				}

				if (!manager.TryInsertItemInStorage(spawnedItem, parentStorage, slotId))
				{
					PrintFormat("GC | Killed From | event=preview-loadout result=failed role=%1 stage=insert path=%2 slot=%3 prefab=%4", role, path, slotId, prefab, level: LogLevel.ERROR);
					delete spawnedItem;
					return false;
				}

				owner = parentStorage.Get(slotId);
				if (!owner)
				{
					PrintFormat("GC | Killed From | event=preview-loadout result=failed role=%1 stage=insert-readback path=%2 slot=%3 prefab=%4", role, path, slotId, prefab, level: LogLevel.ERROR);
					return false;
				}
			}
		}

		if (!owner || !ApplyEntityCustomDataString(owner, context))
		{
			PrintFormat("GC | Killed From | event=preview-loadout result=failed role=%1 stage=custom-data path=%2 slot=%3", role, path, slotId, level: LogLevel.ERROR);
			return false;
		}

		return ApplyStorages(owner, context, manager, role, path);
	}

	protected static bool ApplyStorages(IEntity owner, LoadContext context, InventoryStorageManagerComponent manager, string role, string path)
	{
		int storageCount;
		// The stock writer omits this member for leaf items such as optics.
		// It is a valid end of the inventory tree, not a corrupt loadout.
		if (!context.StartArray("storages", storageCount))
			return true;
		if (storageCount == 0)
			return true;

		set<BaseInventoryStorageComponent> storageCandidates();
		FindStorageComponents(owner, storageCandidates);

		for (int storageIndex = 0; storageIndex < storageCount; ++storageIndex)
		{
			if (!context.StartObject())
				return Fail(role, "storage-object-start", path, storageIndex);

			string id;
			if (!context.Read(id))
				return Fail(role, "storage-id-read", path, storageIndex);

			BaseInventoryStorageComponent storage;
			foreach (BaseInventoryStorageComponent candidate : storageCandidates)
			{
				if (GetComponentIdentifier(owner, candidate) == id)
				{
					storage = candidate;
					break;
				}
			}
			if (!storage)
			{
				PrintFormat("GC | Killed From | event=preview-loadout result=failed role=%1 stage=storage-match path=%2 index=%3 id=%4", role, path, storageIndex, id, level: LogLevel.ERROR);
				return false;
			}

			map<int, IEntity> existingItems = new map<int, IEntity>();
			array<InventoryItemComponent> itemComponents = {};
			storage.GetOwnedItems(itemComponents, false);
			foreach (InventoryItemComponent item : itemComponents)
				existingItems.Insert(item.GetParentSlot().GetID(), item.GetOwner());

			int slotCount;
			if (!context.StartMap("slots", slotCount))
				return Fail(role, "slots-start", path, storageIndex);

			for (int slotIndex = 0; slotIndex < slotCount; ++slotIndex)
			{
				string slotKey;
				if (!context.ReadMapKey(slotIndex, slotKey))
					return Fail(role, "slot-key-read", path, slotIndex);

				int childSlotId = slotKey.ToInt(-1);
				if (childSlotId == -1)
					return Fail(role, "slot-id", path, slotIndex);

				IEntity existing;
				existingItems.Take(childSlotId, existing);
				if (!context.StartObject(slotKey))
					return Fail(role, "slot-object-start", path, childSlotId);

				string childPath = path + "/" + id + "[" + slotKey + "]";
				if (!ApplyEntity(existing, context, manager, role, childPath, storage, childSlotId))
					return false;

				if (!context.EndObject())
					return Fail(role, "slot-object-end", childPath, childSlotId);
			}

			if (!context.EndMap())
				return Fail(role, "slots-end", path, storageIndex);

			foreach (int existingSlotId, IEntity existingEntity : existingItems)
			{
				if (existingEntity)
					delete existingEntity;
			}

			if (!context.EndObject())
				return Fail(role, "storage-object-end", path, storageIndex);
		}

		if (!context.EndArray())
			return Fail(role, "storages-end", path, storageCount);
		return true;
	}

	protected static bool Fail(string role, string stage, string path, int index)
	{
		PrintFormat("GC | Killed From | event=preview-loadout result=failed role=%1 stage=%2 path=%3 index=%4", role, stage, path, index, level: LogLevel.ERROR);
		return false;
	}
}
