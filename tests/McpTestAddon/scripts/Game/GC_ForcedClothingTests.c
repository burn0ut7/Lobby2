//! Explicit native inventory probes. No permission grants or automatic execution.
modded class GC_LobbyManager
{
	protected IEntity m_GC_ClothingTestItem;
	protected IEntity m_GC_ClothingTestCandidate;
	protected BaseInventoryStorageComponent m_GC_ClothingTestStorage;
	protected int m_iGC_ClothingTestSlot;

	//------------------------------------------------------------------------------------------------
	string GC_TestClothingShutdown()
	{
		if (!Replication.IsServer() || GetManager() != this)
			return "UNAVAILABLE active authority";
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		array<GC_LobbyPlayerController> controllers = {};
		int allowedBefore;
		foreach (int playerId : players)
		{
			GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
			if (controller)
			{
				controllers.Insert(controller);
				if (controller.GC_TestRawClothingPermission())
					allowedBefore++;
			}
		}
		Shutdown();
		int allowedAfter;
		foreach (GC_LobbyPlayerController controller : controllers)
		{
			if (controller && controller.GC_TestRawClothingPermission())
				allowedAfter++;
		}
		return string.Format("controllers=%1 rawAllowedBefore=%2 rawAllowedAfter=%3 managerReleased=%4", controllers.Count(), allowedBefore, allowedAfter, GetManager() == null);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestClothingRequest(int scope, int targetId, bool allowed, int radiusMeters, string factionKey, bool staleIdentity)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return "UNAVAILABLE controller";
		array<int> players = {};
		array<string> identities = {};
		if (scope == GC_EAdminHealScope.SELECTED)
		{
			players.Insert(targetId);
			string identity = GetPlayerVoiceIdentity(targetId);
			if (staleIdentity)
				identity = "stale-clothing-test";
			identities.Insert(identity);
		}
		controller.GC_RequestForcedClothing(scope, players, identities, controller.ReserveAdminPlayerToolRequest(), radiusMeters, factionKey, allowed);
		return "DISPATCHED; inspect permission and central audit";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestClothingDenied()
	{
		array<int> targets = {};
		array<string> identities = {};
		return GC_ExecuteForcedClothing(-1, GC_EAdminHealScope.EVERYONE, targets, identities, -1, 15, "", true);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestClothingSnapshot(int playerId)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!controller || !controller.GetControlledEntity())
			return "UNAVAILABLE player/character";
		SCR_CharacterInventoryStorageComponent storage = SCR_CharacterInventoryStorageComponent.Cast(controller.GetControlledEntity().FindComponent(SCR_CharacterInventoryStorageComponent));
		InventoryStorageManagerComponent manager = InventoryStorageManagerComponent.Cast(controller.GetControlledEntity().FindComponent(InventoryStorageManagerComponent));
		if (!storage || !manager)
			return "UNAVAILABLE inventory";
		string result = string.Format("player=%1 server=%2 allowed=%3", playerId, Replication.IsServer(), controller.GC_AreClothingChangesAllowed());
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
			BaseInventoryStorageComponent source = inventory.GetParentSlot().GetStorage();
			result += string.Format(" locked=%1 canRemove=%2 source=%3 id=%4", storage.GC_IsClothingLocked(item), manager.CanRemoveItemFromStorage(item, source), source.Type(), Replication.FindItemId(item));
		}
		return result;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestClothingMutation(string action, string areaName)
	{
		PlayerController controller = GetGame().GetPlayerController();
		IEntity character = controller.GetControlledEntity();
		SCR_CharacterInventoryStorageComponent storage = SCR_CharacterInventoryStorageComponent.Cast(character.FindComponent(SCR_CharacterInventoryStorageComponent));
		SCR_InventoryStorageManagerComponent manager = SCR_InventoryStorageManagerComponent.Cast(character.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!storage || !manager)
			return "UNAVAILABLE inventory";
		if (action == "observe")
			return string.Format("originalExists=%1 sameSlot=%2 candidateExists=%3 | %4", m_GC_ClothingTestItem != null,
				m_GC_ClothingTestItem && m_GC_ClothingTestStorage.GetSlot(m_iGC_ClothingTestSlot).GetAttachedEntity() == m_GC_ClothingTestItem,
				m_GC_ClothingTestCandidate != null, GC_TestClothingSnapshot(controller.GetPlayerId()));
		if (action == "restore")
			return "restoreDispatched=" + manager.TryInsertItemInStorage(m_GC_ClothingTestItem, m_GC_ClothingTestStorage, m_iGC_ClothingTestSlot).ToString();
		typename area = areaName.ToType();
		IEntity item = storage.GetClothFromArea(area);
		if (!item)
			return "UNAVAILABLE garment";
		InventoryItemComponent inventory = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
		BaseInventoryStorageComponent source = inventory.GetParentSlot().GetStorage();
		m_GC_ClothingTestItem = item;
		m_GC_ClothingTestStorage = source;
		for (int index; index < source.GetSlotsCount(); index++)
		{
			if (source.GetSlot(index).GetAttachedEntity() == item)
				m_iGC_ClothingTestSlot = index;
		}
		bool accepted;
		if (action == "remove")
			accepted = manager.TryRemoveItemFromStorage(item, source);
		else if (action == "delete")
			accepted = manager.TryDeleteItem(item);
		else if (action == "drop")
			accepted = manager.TryRemoveItemFromInventory(item, source);
		else if (action == "replace")
		{
			if (m_GC_ClothingTestCandidate)
				return "REFUSED previous candidate requires cleanup";
			EntitySpawnParams spawn = new EntitySpawnParams();
			spawn.Transform[3] = character.GetOrigin();
			m_GC_ClothingTestCandidate = GetGame().SpawnEntityPrefab(Resource.Load(item.GetPrefabData().GetPrefabName()), GetGame().GetWorld(), spawn);
			accepted = manager.TryReplaceItem(m_GC_ClothingTestCandidate, source, m_iGC_ClothingTestSlot);
		}
		else if (action == "cleanup-candidate")
		{
			if (m_GC_ClothingTestCandidate && !manager.Contains(m_GC_ClothingTestCandidate))
				SCR_EntityHelper.DeleteEntityAndChildren(m_GC_ClothingTestCandidate);
		}
		return string.Format("action=%1 dispatch=%2 | %3", action, accepted, GC_TestClothingSnapshot(controller.GetPlayerId()));
	}
}

modded class GC_LobbyPlayerController
{
	//------------------------------------------------------------------------------------------------
	bool GC_TestRawClothingPermission()
	{
		return m_bGC_ClothingChangesAllowed;
	}
}
