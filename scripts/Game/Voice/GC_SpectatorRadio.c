//! Native radio discovery; never derives encryption from the viewer's faction.
class GC_SpectatorRadio
{
	static const ResourceName RECEIVER = "{D392A10030000001}Prefabs/Radio_Spectator_Receiver.et";
	static const int MAX_RECEIVERS = 16;

	static void GetRadios(IEntity character, out array<BaseRadioComponent> radios)
	{
		radios = {};
		if (!character || character.FindComponent(GC_LobbySpectatorBodyComponent))
			return;
		InventoryStorageManagerComponent inventory = InventoryStorageManagerComponent.Cast(character.FindComponent(InventoryStorageManagerComponent));
		if (!inventory)
			return;
		array<IEntity> items = {};
		inventory.GetItems(items);
		foreach (IEntity item : items)
		{
			if (!item)
				continue;
			BaseRadioComponent radio = BaseRadioComponent.Cast(item.FindComponent(BaseRadioComponent));
			if (!radio || !item.GetPrefabData() || item.GetPrefabData().GetPrefabName() == GC_LobbyRadio.PREFAB)
				continue;
			radios.Insert(radio);
		}
	}

	static RplId GetId(BaseRadioComponent radio)
	{
		RplComponent replication = RplComponent.Cast(radio.GetOwner().FindComponent(RplComponent));
		if (replication)
			return replication.Id();
		return RplId.Invalid();
	}
}

class GC_SpectatorRadioStorageComponentClass : SCR_UniversalInventoryStorageComponentClass
{
}

//! Dedicated native inventory storage, present only on the spectator body.
class GC_SpectatorRadioStorageComponent : SCR_UniversalInventoryStorageComponent
{
}
