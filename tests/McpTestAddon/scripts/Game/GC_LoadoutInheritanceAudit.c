#ifdef WORKBENCH
//! Explicit disposable-fixture audit. Does not overwrite user loadouts.
modded class GC_LobbyManager
{
	protected ResourceName m_GCAuditBase;
	protected ResourceName m_GCAuditChild;
	protected IEntity m_GCAuditProbe;

	string GC_TestInheritanceResume(string basePrefab, string childPrefab)
	{
		ResourceName baseResource = basePrefab;
		ResourceName childResource = childPrefab;
		if (baseResource.GetPath() != "Prefabs/Loadouts/US/InheritanceAudit/base.et")
			return "not an audit base";
		array<string> children = {"child", "freshChild", "duplicate"};
		bool valid;
		foreach (string name : children)
		{
			if (childResource.GetPath() == "Prefabs/Loadouts/US/InheritanceAudit/" + name + ".et")
				valid = true;
		}
		if (!valid)
			return "not an audit child";
		m_GCAuditBase = basePrefab;
		m_GCAuditChild = childPrefab;
		return "audit references set";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestInheritanceAudit(string action)
	{
		IEntity owner = GetGame().GetPlayerController().GetControlledEntity();
		if (!GC_WorkbenchItemBrowser.IsActive(owner))
			return "browser inactive";
		if (action == "base" || action == "child" || action == "freshChild" || action == "duplicate")
		{
			ResourceName parent = m_GCAuditBase;
			if (action == "base")
				parent = GC_WorkbenchItemBrowser.GetEditingPrefab();
			string path = GC_LoadoutPrefab.Destination("US", "InheritanceAudit", action);
			ResourceName created = GC_LoadoutPrefab.Create(parent, path, action == "duplicate");
			if (created.IsEmpty())
				return "create failed; existing fixtures are not overwritten";
			if (action == "base")
				m_GCAuditBase = created;
			else
				m_GCAuditChild = created;
			GC_LoadoutPrefab writer = new GC_LoadoutPrefab();
			bool saved = writer.Open(created) && writer.Save(owner);
			return string.Format("saved=%1 resource=%2 error=%3", saved, created, writer.GetError());
		}
		if (action == "saveChild")
		{
			GC_LoadoutPrefab writer = new GC_LoadoutPrefab();
			bool saved = writer.Open(m_GCAuditChild) && writer.Save(owner);
			return string.Format("saved=%1 error=%2", saved, writer.GetError());
		}
		if (action == "probeBase" || action == "probeChild")
		{
			if (m_GCAuditProbe)
				SCR_EntityHelper.DeleteEntityAndChildren(m_GCAuditProbe);
			ResourceName resource = m_GCAuditChild;
			if (action == "probeBase")
				resource = m_GCAuditBase;
			EntitySpawnParams spawn = new EntitySpawnParams();
			spawn.Transform[3] = owner.GetOrigin() - "0 1000 0";
			m_GCAuditProbe = GetGame().SpawnEntityPrefabLocal(Resource.Load(resource), GetGame().GetWorld(), spawn);
			return string.Format("spawned=%1 resource=%2", m_GCAuditProbe != null, resource);
		}
		if (action == "probeCount")
			owner = m_GCAuditProbe;
		if (!owner)
			return "owner absent";
		InventoryStorageManagerComponent manager = InventoryStorageManagerComponent.Cast(owner.FindComponent(InventoryStorageManagerComponent));
		array<IEntity> items = {};
		manager.GetItems(items);
		int count;
		foreach (IEntity item : items)
		{
			if (SCR_ResourceNameUtils.GetPrefabName(item) != "{A81F501D3EF6F38E}Prefabs/Items/Medicine/FieldDressing_01/FieldDressing_US_01.et")
				continue;
			if (action == "removeOne")
				return string.Format("removed=%1", GC_CatalogueTransfer.Drop(item, owner, SCR_InventoryMenuUI.GetInventoryMenu()));
			count++;
		}
		return string.Format("bandages=%1", count);
	}
}
#endif
