//! Live gear requests run on the owning controller; caller identity is never supplied by the client.
modded class GC_LobbyPlayerController
{
	protected bool m_bGC_GearPending;
	protected bool m_bGC_GearServerBusy;
	protected int m_iGC_GearRequestRevision;
	protected RplId m_GC_GearCrate;
	protected IEntity m_GC_NearestCrate;
	protected float m_fGC_NearestDistance;

	//------------------------------------------------------------------------------------------------
	static RplId GC_GearEntityId(IEntity entity)
	{
		if (!entity)
			return RplId.Invalid();
		RplComponent replication = RplComponent.Cast(entity.FindComponent(RplComponent));
		if (!replication)
			return RplId.Invalid();
		return replication.Id();
	}

	//------------------------------------------------------------------------------------------------
	static IEntity GC_GearEntity(RplId id)
	{
		RplComponent replication = RplComponent.Cast(Replication.FindItem(id));
		if (!replication)
			return null;
		return replication.GetEntity();
	}

	//------------------------------------------------------------------------------------------------
	void GC_RequestGearOpen()
	{
		if (GetGame().GetPlayerController() == this)
			Rpc(GC_RpcAskGearOpen);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	//------------------------------------------------------------------------------------------------
	protected void GC_RpcAskGearOpen()
	{
		if (!GC_AuthorizeGear("gear-open", "inventory"))
		{
			Rpc(GC_RpcDoGearOpen, false);
			return;
		}
		IEntity owner = GetControlledEntity();
		bool valid = owner && owner.FindComponent(SCR_InventoryStorageManagerComponent);
		GC_AdminActionLogger.Log(GetPlayerId(), "gear-open", "inventory", valid.ToString());
		Rpc(GC_RpcDoGearOpen, valid);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	//------------------------------------------------------------------------------------------------
	protected void GC_RpcDoGearOpen(bool valid)
	{
		if (!valid || !GC_GearSession.Open())
			GC_AdminToolsComponent.GearUnavailable();
	}

	//------------------------------------------------------------------------------------------------
	protected bool GC_AuthorizeGear(string action, string target)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (manager)
			return manager.AuthorizeAdminAction(GetPlayerId(), action, target);
		GC_AdminActionLogger.Log(GetPlayerId(), action, target, "rejected-inactive-mode");
		return false;
	}

	//------------------------------------------------------------------------------------------------
	bool GC_RequestGearItem(ResourceName prefab, BaseInventoryStorageComponent target, int slot, bool automatic)
	{
		if (m_bGC_GearPending || !GC_GearSession.IsAdmin(GetControlledEntity()))
			return false;
		RplId targetId;
		if (target)
			targetId = Replication.FindItemId(target);
		if (!automatic && !targetId.IsValid())
			return false;
		m_bGC_GearPending = true;
		m_iGC_GearRequestRevision = GC_GearSession.GetRevision();
		Rpc(GC_RpcAskGearItem, prefab, GC_GearEntityId(GetControlledEntity()), targetId, slot, automatic, GC_GearSession.s_bCrate, m_GC_GearCrate);
		return true;
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	//------------------------------------------------------------------------------------------------
	protected void GC_RpcAskGearItem(ResourceName prefab, RplId characterId, RplId storageId, int slot, bool automatic, bool toCrate, RplId crateId)
	{
		if (!GC_AuthorizeGear("gear-spawn", prefab))
		{
			Rpc(GC_RpcDoGearResult, false, "Admin login required.");
			return;
		}
		IEntity owner = GetControlledEntity();
		if (m_bGC_GearServerBusy)
		{
			GC_AdminActionLogger.Log(GetPlayerId(), "gear-spawn", prefab, "rejected-busy");
			Rpc(GC_RpcDoGearResult, false, "Inventory busy.");
			return;
		}
		if (!owner || !characterId.IsValid() || GC_GearEntityId(owner) != characterId)
		{
			GC_FinishGear(false, prefab, "Character changed or inventory busy.");
			return;
		}
		SCR_InventoryStorageManagerComponent manager = SCR_InventoryStorageManagerComponent.Cast(owner.FindComponent(SCR_InventoryStorageManagerComponent));
		GC_ItemCatalogueEntry entry = GC_ItemCatalogue.ReadResource(prefab);
		if (!manager || manager.IsInventoryLocked() || !entry || !entry.m_bPortable)
		{
			GC_FinishGear(false, prefab, "Item cannot be placed.");
			return;
		}
		BaseInventoryStorageComponent target;
		if (toCrate)
		{
			IEntity crate = GC_GearEntity(crateId);
			target = GC_GetGearCrateStorage(crate);
			slot = -1;
			automatic = false;
			if (!target || vector.Distance(owner.GetOrigin(), crate.GetOrigin()) > 5)
			{
				GC_FinishGear(false, prefab, "Select a crate within 5 m.");
				return;
			}
		}
		else if (!automatic)
		{
			target = BaseInventoryStorageComponent.Cast(Replication.FindItem(storageId));
			array<BaseInventoryStorageComponent> ownedStorages = {};
			manager.GetStorages(ownedStorages);
			if (!target || !ownedStorages.Contains(target))
			{
				GC_FinishGear(false, prefab, "That slot is no longer available.");
				return;
			}
		}
		// A real authority-owned item supplies native compatibility checks, including replacements.
		EntitySpawnParams spawn = new EntitySpawnParams();
		spawn.TransformMode = ETransformMode.WORLD;
		spawn.Transform[3] = owner.GetOrigin();
		IEntity item = GetGame().SpawnEntityPrefab(Resource.Load(prefab), GetGame().GetWorld(), spawn);
		if (!item)
		{
			GC_FinishGear(false, prefab, "Item could not be created.");
			return;
		}
		bool valid = true;
		RplComponent replication = RplComponent.Cast(item.FindComponent(RplComponent));
		if (replication && !replication.Id().IsValid())
		{
			SCR_EntityHelper.DeleteEntityAndChildren(item);
			GC_FinishGear(false, prefab, "Item is not available in this session.");
			return;
		}
		if (automatic)
			valid = GC_CatalogueTransfer.FindSlot(entry, manager, item, target, slot);
		valid = valid && target && slot >= -1 && slot < target.GetSlotsCount();
		bool replace = valid && slot >= 0 && target.GetSlot(slot).GetAttachedEntity();
		if (valid && replace)
			valid = manager.CanReplaceItem(item, target, slot);
		else if (valid)
			valid = manager.CanInsertItemInStorage(item, target, slot);
		if (!valid)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(item);
			GC_FinishGear(false, prefab, "Item does not fit.");
			return;
		}
		GC_AdminGearInsertCallback callback = new GC_AdminGearInsertCallback();
		callback.m_Controller = this;
		callback.m_Item = item;
		callback.m_sContext = prefab;
		m_bGC_GearServerBusy = true;
		bool accepted;
		if (replace)
			accepted = manager.TryReplaceItem(item, target, slot, callback);
		else
			accepted = manager.TryInsertItemInStorage(item, target, slot, callback);
		if (!accepted)
			callback.OnFailed();
	}

	//------------------------------------------------------------------------------------------------
	void GC_FinishGear(bool success, string context, string message)
	{
		m_bGC_GearServerBusy = false;
		GC_AdminActionLogger.Log(GetPlayerId(), "gear-spawn", context, message);
		Rpc(GC_RpcDoGearResult, success, message);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	//------------------------------------------------------------------------------------------------
	protected void GC_RpcDoGearResult(bool success, string message)
	{
		m_bGC_GearPending = false;
		if (!GC_GearSession.IsAdmin(GetControlledEntity()) || m_iGC_GearRequestRevision != GC_GearSession.GetRevision())
			return;
		SCR_InventoryMenuUI menu = SCR_InventoryMenuUI.GetInventoryMenu();
		if (menu)
		{
			menu.GC_CatalogueTransferResult(success);
			menu.GC_GearMessage(message);
		}
	}

	//------------------------------------------------------------------------------------------------
	void GC_RequestGearCrate(bool create)
	{
		if (!m_bGC_GearPending && GC_GearSession.IsAdmin(GetControlledEntity()))
		{
			m_bGC_GearPending = true;
			m_iGC_GearRequestRevision = GC_GearSession.GetRevision();
			Rpc(GC_RpcAskGearCrate, create);
		}
	}

	//------------------------------------------------------------------------------------------------
	bool GC_IsGearPending()
	{
		return m_bGC_GearPending;
	}

	//------------------------------------------------------------------------------------------------
	protected BaseInventoryStorageComponent GC_GetGearCrateStorage(IEntity crate)
	{
		if (!crate || ChimeraCharacter.Cast(crate) || Vehicle.Cast(crate) || crate.GetParent())
			return null;
		SCR_UniversalInventoryStorageComponent storage = SCR_UniversalInventoryStorageComponent.Cast(crate.FindComponent(SCR_UniversalInventoryStorageComponent));
		if (!storage || storage.GetParentSlot())
			return null;
		return storage;
	}

	//------------------------------------------------------------------------------------------------
	protected bool GC_CollectGearCrate(IEntity candidate)
	{
		if (!GC_GetGearCrateStorage(candidate) || !GC_GearEntityId(candidate).IsValid())
			return true;
		float distance = vector.Distance(GetControlledEntity().GetOrigin(), candidate.GetOrigin());
		if (distance < m_fGC_NearestDistance)
		{
			m_fGC_NearestDistance = distance;
			m_GC_NearestCrate = candidate;
		}
		return true;
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	//------------------------------------------------------------------------------------------------
	protected void GC_RpcAskGearCrate(bool create)
	{
		string action = "gear-select-crate";
		if (create)
			action = "gear-create-crate";
		if (!GC_AuthorizeGear(action, "nearby"))
		{
			Rpc(GC_RpcDoGearResult, false, "Admin login required.");
			return;
		}
		IEntity owner = GetControlledEntity();
		IEntity crate;
		if (owner && create)
		{
			EntitySpawnParams spawn = new EntitySpawnParams();
			spawn.TransformMode = ETransformMode.WORLD;
			owner.GetWorldTransform(spawn.Transform);
			spawn.Transform[3] = owner.GetOrigin() + owner.GetWorldTransformAxis(2) * 1.5;
			// Trace nearby ground/floors, rather than placing beneath buildings at terrain height.
			TraceParam trace = new TraceParam();
			trace.Flags = TraceFlags.ENTS | TraceFlags.WORLD;
			trace.LayerMask = EPhysicsLayerDefs.Projectile;
			trace.Exclude = owner;
			trace.Start = spawn.Transform[3] + "0 0.5 0";
			trace.End = spawn.Transform[3] - "0 2 0";
			float fraction = GetGame().GetWorld().TraceMove(trace, null);
			if (fraction >= 1)
			{
				GC_AdminActionLogger.Log(GetPlayerId(), action, "nearby", "rejected-no-ground");
				Rpc(GC_RpcDoGearResult, false, "No suitable ground nearby.");
				return;
			}
			spawn.Transform[3] = trace.Start + (trace.End - trace.Start) * fraction;
			crate = GetGame().SpawnEntityPrefab(Resource.Load("{EBBC859CCD94D96D}Prefabs/Props/EmptyStorages/USSR_AmmoBox_empty.et"), GetGame().GetWorld(), spawn);
		}
		else if (owner)
		{
			m_GC_NearestCrate = null;
			m_fGC_NearestDistance = 5;
			GetGame().GetWorld().QueryEntitiesBySphere(owner.GetOrigin(), 5, GC_CollectGearCrate);
			crate = m_GC_NearestCrate;
			m_GC_NearestCrate = null;
		}
		RplId id;
		if (crate)
			id = GC_GearEntityId(crate);
		if (create && crate && !id.IsValid())
			SCR_EntityHelper.DeleteEntityAndChildren(crate);
		GC_AdminActionLogger.Log(GetPlayerId(), action, "nearby", id.IsValid().ToString());
		Rpc(GC_RpcDoGearCrate, id);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	//------------------------------------------------------------------------------------------------
	protected void GC_RpcDoGearCrate(RplId id)
	{
		m_bGC_GearPending = false;
		if (!GC_GearSession.IsAdmin(GetControlledEntity()) || m_iGC_GearRequestRevision != GC_GearSession.GetRevision())
			return;
		m_GC_GearCrate = id;
		GC_GearSession.s_bCrate = id.IsValid();
		string message = "No crate within 5 m.";
		if (id.IsValid())
			message = "Spawning into nearby crate.";
		SCR_InventoryMenuUI menu = SCR_InventoryMenuUI.GetInventoryMenu();
		if (menu)
			menu.GC_GearMessage(message);
	}

	//------------------------------------------------------------------------------------------------
	override void ShutdownLobbyPresentation()
	{
		m_bGC_GearPending = false;
		if (GetGame().GetPlayerController() == this)
			GC_GearSession.Close();
		super.ShutdownLobbyPresentation();
	}
}

class GC_AdminGearInsertCallback : ScriptedInventoryOperationCallback
{
	GC_LobbyPlayerController m_Controller;
	IEntity m_Item;
	string m_sContext;
	protected bool m_bFinished;

	//------------------------------------------------------------------------------------------------
	override void OnComplete()
	{
		if (m_bFinished)
			return;
		m_bFinished = true;
		if (m_Controller)
			m_Controller.GC_FinishGear(true, m_sContext, "Item added.");
	}

	//------------------------------------------------------------------------------------------------
	override void OnFailed()
	{
		if (m_bFinished)
			return;
		m_bFinished = true;
		if (m_Item)
			SCR_EntityHelper.DeleteEntityAndChildren(m_Item);
		if (m_Controller)
			m_Controller.GC_FinishGear(false, m_sContext, "Item could not be placed.");
	}
}
