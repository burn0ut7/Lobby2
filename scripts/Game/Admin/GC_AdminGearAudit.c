//! Server-observed gear facts, formatted for the canonical session/admin log.
class GC_AdminGearAudit
{
	//------------------------------------------------------------------------------------------------
	static string Entity(IEntity entity)
	{
		if (!entity || entity.IsDeleted())
			return "unavailable";
		return string.Format("[prefab=%1 rpl=%2 position=%3]", SCR_ResourceNameUtils.GetPrefabName(entity),
			GC_LobbyPlayerController.GC_GearEntityId(entity), entity.GetOrigin());
	}

	//------------------------------------------------------------------------------------------------
	static string Storage(BaseInventoryStorageComponent storage)
	{
		if (!storage)
			return "automatic-or-unavailable";
		return string.Format("[type=%1 rpl=%2 owner=%3]", storage.Type().ToString(), Replication.FindItemId(storage), Entity(storage.GetOwner()));
	}

	//------------------------------------------------------------------------------------------------
	static void Completion(int actorId, string actorIdentity, string context, bool success, string message)
	{
		if (!Replication.IsServer())
			return;
		string outcome = "failed: " + message;
		if (success)
			outcome = "completed: " + message;
		context += string.Format(" requesting-player=%1 requesting-identity=%2", actorId, actorIdentity);
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players.IsPlayerConnected(actorId) || SCR_PlayerIdentityUtils.GetPlayerIdentityId(actorId) != actorIdentity)
			actorId = 0;
		GC_AdminActionLogger.Log(actorId, "gear-spawn", context, outcome);
	}

	//------------------------------------------------------------------------------------------------
	//! Native inventory notifications identify the affected inventory owner, not necessarily the input instigator.
	static void InventoryChange(IEntity owner, BaseInventoryStorageComponent storage, IEntity item, bool added)
	{
		if (!Replication.IsServer() || !GC_LobbyManager.GetManager() || !owner || !item)
			return;
		PlayerManager players = GetGame().GetPlayerManager();
		int playerId = players.GetPlayerIdFromControlledEntity(owner);
		if (playerId <= 0 || !players.IsPlayerConnected(playerId) || !SCR_Global.IsAdmin(playerId))
			return;
		string action = "gear-inventory-remove";
		if (added)
			action = "gear-inventory-add";
		string context = "inventory-owner=" + Entity(owner) + " item=" + Entity(item) + " storage=" + Storage(storage) + " quantity=1";
		GC_AdminActionLogger.Log(playerId, action, context, "observed-native-change; input-instigator-not-reported");
	}
}
