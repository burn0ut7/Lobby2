#ifdef WORKBENCH
//! Explicit source/runtime comparison; never changes the player's character or saves a file.
modded class GC_LobbyManager
{
	string GC_TestLoadoutResource(string prefab, bool sourceFile)
	{
		IEntity owner = GetGame().GetPlayerController().GetControlledEntity();
		ResourceName name = prefab;
		if (!GC_WorkbenchItemBrowser.IsActive(owner) || !name.GetPath().StartsWith("Prefabs/Loadouts/"))
			return "inactive or outside loadouts";
		Resource resource;
		if (sourceFile)
			resource = BaseContainerTools.LoadContainer(name);
		else
			resource = Resource.Load(name);
		if (!resource || !resource.IsValid())
			return "resource invalid";
		IEntitySource source = resource.GetResource().ToEntitySource();
		if (!source)
			return "not entity source";
		EntitySpawnParams spawn = new EntitySpawnParams();
		spawn.Transform[3] = owner.GetOrigin() - "0 1000 0";
		IEntity probe = GetGame().SpawnEntityPrefabLocal(resource, GetGame().GetWorld(), spawn);
		if (!probe)
			return "spawn failed";
		RplComponent replication = RplComponent.Cast(probe.FindComponent(RplComponent));
		string result = string.Format("sourceFile=%1 replication=%2", sourceFile, replication && replication.Id().IsValid());
		InventoryStorageManagerComponent manager = InventoryStorageManagerComponent.Cast(probe.FindComponent(InventoryStorageManagerComponent));
		array<IEntity> items = {};
		if (manager)
			manager.GetItems(items);
		foreach (IEntity item : items)
			result += "\n" + SCR_ResourceNameUtils.GetPrefabName(item);
		SCR_EntityHelper.DeleteEntityAndChildren(probe);
		return result;
	}
}
#endif
