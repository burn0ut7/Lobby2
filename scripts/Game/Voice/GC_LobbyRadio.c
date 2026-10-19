//! Lobby2 transport helpers. Room selection remains owned by GC_LobbyManager.
class GC_LobbyRadio
{
	static const ResourceName PREFAB = "{A437C5A45B797D2B}Prefabs/Radio_Lobby_Spectator.et";
	static const int ADMIN = 0;
	static const int CHAT = 1;
	static const int FACTION = 2;
	static const int GLOBAL_FREQUENCY = 1;
	// Reserved upper endpoint, outside the sequential active-room allocation.
	static const int INACTIVE_FREQUENCY = 999999999;

	static BaseRadioComponent FindRadio(IEntity body)
	{
		if (!body)
			return null;
		InventoryStorageManagerComponent inventory = InventoryStorageManagerComponent.Cast(body.FindComponent(InventoryStorageManagerComponent));
		if (!inventory)
			return null;
		array<IEntity> items = {};
		inventory.GetItems(items);
		foreach (IEntity item : items)
		{
			if (!item || !item.GetPrefabData() || item.GetPrefabData().GetPrefabName() != PREFAB)
				continue;
			return BaseRadioComponent.Cast(item.FindComponent(BaseRadioComponent));
		}
		return null;
	}
}
