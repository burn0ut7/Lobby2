#ifdef WORKBENCH
//! Native prefab authoring for the explicitly opened local loadout workbench.
//! Inventory remains native; no runtime loadout format or spawn-time replay component.
class GC_LoadoutPrefab
{
	protected ref Resource m_Resource;
	protected IEntitySource m_Source;
	protected WorldEditorAPI m_API;
	protected ResourceName m_sResource;
	protected string m_sError;
	protected int m_iChanges;
	protected IEntity m_Reference;
	protected ref array<ref GC_LoadoutPrefab> m_aPendingWeapons;

	//------------------------------------------------------------------------------------------------
	string GetError()
	{
		return m_sError;
	}

	//------------------------------------------------------------------------------------------------
	static string Quote(string value)
	{
		value.Replace("\\", "\\\\");
		value.Replace("\"", "\\\"");
		return "\"" + value + "\"";
	}

	//------------------------------------------------------------------------------------------------
	static bool ValidName(string value)
	{
		if (value.IsEmpty() || value.Length() > 64 || value == "." || value == "..")
			return false;
		string upper = value;
		upper.ToUpper();
		array<string> reserved = {"CON", "PRN", "AUX", "NUL", "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"};
		if (reserved.Contains(upper))
			return false;
		for (int i; i < value.Length(); i++)
		{
			string ch = value.Substring(i, 1);
			if (!"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_- ".Contains(ch))
				return false;
		}
		return !value.StartsWith(" ") && !value.EndsWith(" ");
	}

	//------------------------------------------------------------------------------------------------
	static string Destination(string faction, string group, string name)
	{
		if (!ValidName(faction) || !ValidName(group) || !ValidName(name))
			return string.Empty;
		string root;
		if (!Workbench.GetAbsolutePath("$Lobby2:", root))
			return string.Empty;
		return FilePath.Concat(root, "Prefabs/Loadouts/" + faction + "/" + group + "/" + name + ".et");
	}

	//------------------------------------------------------------------------------------------------
	static ResourceName Register(string path)
	{
		ResourceManager manager = Workbench.GetModule(ResourceManager);
		// Build the runtime representation as well as registering the source.
		// Database enumeration acknowledgement is deferred during Play.
		if (!manager || !manager.RegisterResourceFile(path, true))
			return string.Empty;
		MetaFile meta = manager.GetMetaFile(path);
		if (!meta)
			return string.Empty;
		return meta.GetResourceID();
	}

	//------------------------------------------------------------------------------------------------
	//! Native duplication retains the source's existing ancestor, with a fresh resource identity.
	static ResourceName Create(ResourceName parent, string path, bool duplicate)
	{
		if (!GetGame() || !GetGame().GetPlayerController() || !GC_WorkbenchItemBrowser.IsActive(GetGame().GetPlayerController().GetControlledEntity()))
			return string.Empty;
		string root;
		if (!Workbench.GetAbsolutePath("$Lobby2:Prefabs/Loadouts/", root, false))
			return string.Empty;
		string normalized = path;
		normalized.Replace("\\", "/");
		root.Replace("\\", "/");
		if (!root.EndsWith("/"))
			root += "/";
		if (!normalized.StartsWith(root) || normalized.Contains("/../") || !normalized.EndsWith(".et"))
			return string.Empty;
		if (path.IsEmpty() || FileIO.FileExists(path) || FileIO.FileExists(path + ".meta"))
			return string.Empty;
		Resource resource = Resource.Load(parent);
		if (!resource || !resource.IsValid())
			return string.Empty;
		IEntitySource source = resource.GetResource().ToEntitySource();
		if (!source || !FileIO.MakeDirectory(FilePath.StripFileName(path)))
			return string.Empty;
		if (duplicate)
		{
			if (!BaseContainerTools.SaveContainer(source, string.Empty, path))
				return string.Empty;
		}
		else
		{
			FileHandle file = FileIO.OpenFile(path, FileMode.WRITE);
			if (!file)
				return string.Empty;
			file.WriteLine(source.GetClassName() + " : " + Quote(parent) + " {}\n");
			file.Close();
		}
		ResourceName result = Register(path);
		if (result.IsEmpty())
		{
			// Both files were created by this call; leave no unselectable name behind.
			FileIO.DeleteFile(path + ".meta");
			if (!FileIO.DeleteFile(path))
				Workbench.Dialog("Loadout", "Registration failed and the incomplete file could not be removed:", path);
		}
		return result;
	}

	//------------------------------------------------------------------------------------------------
	bool Open(ResourceName resourceName)
	{
		if (!resourceName.GetPath().StartsWith("Prefabs/Loadouts/"))
			return false;
		m_sResource = resourceName;
		m_Resource = Resource.Load(resourceName);
		WorldEditor editor = Workbench.GetModule(WorldEditor);
		if (!editor || !m_Resource || !m_Resource.IsValid())
			return false;
		m_API = editor.GetApi();
		m_Source = m_Resource.GetResource().ToEntitySource();
		return m_API && m_Source;
	}

	//------------------------------------------------------------------------------------------------
	//! Resolve component identities into paths rooted at the destination, never mutate a supplier.
	protected BaseContainer FindComponent(BaseContainer container, string identity, array<ref ContainerIdPathEntry> path)
	{
		if (!container)
			return null;
		BaseContainerList components = container.GetObjectArray("components");
		if (!components)
			return null;
		for (int i; i < components.Count(); i++)
		{
			BaseContainer component = components.Get(i);
			path.Insert(new ContainerIdPathEntry("components", i));
			if (component.GetResourceName() == identity)
				return component;
			BaseContainer found = FindComponent(component, identity, path);
			if (found)
				return found;
			path.Remove(path.Count() - 1);
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected BaseContainer FindSlot(BaseContainer component, string name, array<ref ContainerIdPathEntry> path)
	{
		for (int v; v < component.GetNumVars(); v++)
		{
			string key = component.GetVarName(v);
			DataVarType type = component.GetDataVarType(v);
			if (type == DataVarType.OBJECT)
			{
				BaseContainer child = component.GetObject(key);
				if (child && child.GetName() == name && child.GetVarIndex("Prefab") >= 0)
				{
					path.Insert(new ContainerIdPathEntry(key));
					return child;
				}
			}
			else if (type == DataVarType.OBJECT_ARRAY && key != "components")
			{
				BaseContainerList entries = component.GetObjectArray(key);
				for (int i; entries && i < entries.Count(); i++)
				{
					BaseContainer entry = entries.Get(i);
					if (entry.GetName() == name && entry.GetVarIndex("Prefab") >= 0)
					{
						path.Insert(new ContainerIdPathEntry(key, i));
						return entry;
					}
				}
			}
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected bool SetResource(BaseContainer container, BaseContainer inheritedContainer, array<ref ContainerIdPathEntry> path, string key, ResourceName value)
	{
		string current;
		if (!container.Get(key, current))
			return false;
		if (current == value)
			return true;
		string inherited;
		bool changed;
		if (inheritedContainer && inheritedContainer.Get(key, inherited) && inherited == value)
			changed = m_API.ClearVariableValue(m_Source, path, key);
		else
			changed = m_API.SetVariableValue(m_Source, path, key, value);
		if (changed)
			m_iChanges++;
		return changed;
	}

	//------------------------------------------------------------------------------------------------
	protected bool WriteSlots(IEntity owner)
	{
		set<BaseInventoryStorageComponent> storages = new set<BaseInventoryStorageComponent>();
		SCR_PlayerArsenalLoadout.FindStorageComponents(owner, storages);
		foreach (BaseInventoryStorageComponent storage : storages)
		{
			// Health and the temporary hand slot are runtime state, not loadout authoring.
			if (storage.IsInherited(SCR_HandSlotStorageComponent) || storage.IsInherited(SCR_SalineStorageComponent) || storage.IsInherited(SCR_TourniquetStorageComponent))
				continue;
			if (storage.GetPurpose() == EStoragePurpose.PURPOSE_DEPOSIT)
				continue;
			for (int i; i < storage.GetSlotsCount(); i++)
			{
				InventoryStorageSlot slot = storage.GetSlot(i);
				if (!slot || !slot.GetParentContainer())
					continue;
				IEntity item = slot.GetAttachedEntity();
				ResourceName wanted;
				if (item)
					wanted = SCR_ResourceNameUtils.GetPrefabName(item);
				string ownPrefix = FilePath.Concat(FilePath.StripFileName(m_sResource.GetPath()), "Weapons/" + FilePath.StripExtension(FilePath.StripPath(m_sResource.GetPath())) + "_Slot" + i.ToString() + "_");
				bool ownVariant = wanted.GetPath().StartsWith(ownPrefix);
				if (item && (HasSlotChanges(item) || ownVariant))
				{
					wanted = SaveVariant(item, i);
					if (wanted.IsEmpty())
						return false;
				}
				array<ref ContainerIdPathEntry> path = {};
				string identity = slot.GetParentContainer().GetComponentSource(owner).GetResourceName();
				BaseContainer component = FindComponent(m_Source, identity, path);
				array<ref ContainerIdPathEntry> inheritedPath = {};
				BaseContainer inheritedComponent = FindComponent(m_Source.GetAncestor(), identity, inheritedPath);
				if (!component)
				{
					m_sError = "Cannot resolve slot " + slot.GetSourceName();
					return false;
				}
				string key = "Prefab";
				BaseContainer target;
				if (component.GetVarIndex("WeaponTemplate") >= 0)
				{
					target = component;
					key = "WeaponTemplate";
				}
				else if (component.GetVarIndex("MagazineTemplate") >= 0)
				{
					target = component;
					key = "MagazineTemplate";
				}
				else
					target = FindSlot(component, slot.GetSourceName(), path);
				if (!target)
				{
					if (wanted == slot.GetSlotTemplate())
						continue;
					m_sError = "Unsupported authored slot: " + slot.GetSourceName();
					return false;
				}
				BaseContainer inheritedTarget = inheritedComponent;
				if (key == "Prefab" && inheritedComponent)
					inheritedTarget = FindSlot(inheritedComponent, slot.GetSourceName(), inheritedPath);
				if (!SetResource(target, inheritedTarget, path, key, wanted))
					return false;
			}
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool HasSlotChanges(IEntity owner)
	{
		set<BaseInventoryStorageComponent> storages = new set<BaseInventoryStorageComponent>();
		SCR_PlayerArsenalLoadout.FindStorageComponents(owner, storages);
		foreach (BaseInventoryStorageComponent storage : storages)
		{
			if (storage.GetPurpose() == EStoragePurpose.PURPOSE_DEPOSIT)
				continue;
			for (int i; i < storage.GetSlotsCount(); i++)
			{
				InventoryStorageSlot slot = storage.GetSlot(i);
				if (!slot)
					continue;
				IEntity child = slot.GetAttachedEntity();
				ResourceName current;
				if (child)
					current = SCR_ResourceNameUtils.GetPrefabName(child);
				if (current != slot.GetSlotTemplate() || (child && HasSlotChanges(child)))
					return true;
			}
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected ResourceName SaveVariant(IEntity item, int slotIndex)
	{
		string absolute;
		if (!Workbench.GetAbsolutePath(m_sResource.GetPath(), absolute))
			return string.Empty;
		string stem = FilePath.StripExtension(FilePath.StripPath(absolute));
		ResourceName parent = SCR_ResourceNameUtils.GetPrefabName(item);
		string prefix = stem + "_Slot" + slotIndex.ToString() + "_";
		string path = FilePath.Concat(FilePath.StripFileName(absolute), "Weapons/" + prefix + parent.Substring(1, 16) + ".et");
		ResourceName variant;
		// Keep authored weapon identities stable so child attachment variants continue
		// inheriting future base changes. A replacement weapon gets its own identity.
		if (parent.GetPath().StartsWith(FilePath.Concat(FilePath.StripFileName(m_sResource.GetPath()), "Weapons/" + prefix)))
			variant = parent;
		else if (FileIO.FileExists(path))
		{
			ResourceManager manager = Workbench.GetModule(ResourceManager);
			MetaFile meta = manager.GetMetaFile(path);
			if (meta)
				variant = meta.GetResourceID();
		}
		else
			variant = Create(parent, path, false);
		GC_LoadoutPrefab writer = new GC_LoadoutPrefab();
		if (variant.IsEmpty() || !writer.Open(variant))
			return string.Empty;
		if (variant != parent && (!writer.m_Source.GetAncestor() || writer.m_Source.GetAncestor().GetResourceName() != parent))
		{
			m_sError = "Incompatible weapon variant: " + variant.GetPath();
			return string.Empty;
		}
		writer.m_aPendingWeapons = m_aPendingWeapons;
		bool valid = writer.WriteSlots(item);
		writer.m_aPendingWeapons = null;
		if (!valid)
		{
			m_sError = "Could not save weapon: " + writer.GetError();
			return string.Empty;
		}
		m_aPendingWeapons.Insert(writer);
		return variant;
	}

	//------------------------------------------------------------------------------------------------
	protected string StoragePath(IEntity item, IEntity character)
	{
		if (!item || item == character)
			return string.Empty;
		InventoryItemComponent inventory = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
		if (!inventory || !inventory.GetParentSlot())
			return string.Empty;
		InventoryStorageSlot slot = inventory.GetParentSlot();
		string prefix = StoragePath(slot.GetStorage().GetOwner(), character);
		string filename = FilePath.StripPath(SCR_ResourceNameUtils.GetPrefabName(item).GetPath());
		if (!prefix.IsEmpty())
			prefix += "/";
		return prefix + slot.GetSourceName() + "/" + filename;
	}

	//------------------------------------------------------------------------------------------------
	protected bool SameItems(array<ResourceName> a, array<ResourceName> b)
	{
		if (a.Count() != b.Count())
			return false;
		array<ResourceName> left = {};
		array<ResourceName> right = {};
		left.Copy(a);
		right.Copy(b);
		left.Sort();
		right.Sort();
		for (int i; i < left.Count(); i++)
		{
			if (left[i] != right[i])
				return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected ref map<string, ref array<ResourceName>> ReadContents(IEntity owner)
	{
		InventoryStorageManagerComponent manager = InventoryStorageManagerComponent.Cast(owner.FindComponent(InventoryStorageManagerComponent));
		map<string, ref array<ResourceName>> contents = new map<string, ref array<ResourceName>>();
		if (!manager)
			return contents;
		array<BaseInventoryStorageComponent> storages = {};
		manager.GetStorages(storages, EStoragePurpose.PURPOSE_DEPOSIT);
		foreach (BaseInventoryStorageComponent storage : storages)
		{
			string target = StoragePath(storage.GetOwner(), owner);
			if (target.IsEmpty())
				continue;
			array<ResourceName> items;
			if (!contents.Find(target, items))
			{
				items = {};
				contents.Insert(target, items);
			}
			array<InventoryItemComponent> ownedItems = {};
			storage.GetOwnedItems(ownedItems, false);
			foreach (InventoryItemComponent item : ownedItems)
				items.Insert(SCR_ResourceNameUtils.GetPrefabName(item.GetOwner()));
		}
		return contents;
	}

	//------------------------------------------------------------------------------------------------
	protected bool WriteContents(IEntity owner)
	{
		InventoryStorageManagerComponent manager = InventoryStorageManagerComponent.Cast(owner.FindComponent(InventoryStorageManagerComponent));
		if (!manager)
			return true;
		array<ref ContainerIdPathEntry> managerPath = {};
		BaseContainer managerSource = FindComponent(m_Source, manager.GetComponentSource(owner).GetResourceName(), managerPath);
		if (!managerSource || !m_Reference)
			return false;
		map<string, ref array<ResourceName>> contents = ReadContents(owner);
		map<string, ref array<ResourceName>> baseline = ReadContents(m_Reference);
		BaseContainerList configurations = managerSource.GetObjectArray("InitialInventoryItems");
		map<string, ref array<int>> indexes = new map<string, ref array<int>>();
		for (int c; configurations && c < configurations.Count(); c++)
		{
			BaseContainer config = configurations.Get(c);
			string target;
			config.Get("TargetStorage", target);
			array<int> entries;
			if (!indexes.Find(target, entries))
			{
				entries = {};
				indexes.Insert(target, entries);
			}
			entries.Insert(c);
			if (!contents.Contains(target))
				contents.Insert(target, new array<ResourceName>());
		}
		foreach (string target, array<ResourceName> wanted : contents)
		{
			array<ResourceName> original;
			if (!baseline.Find(target, original))
				original = {};
			if (SameItems(wanted, original))
				continue;
			array<int> entries;
			array<ResourceName> existing = {};
			indexes.Find(target, entries);
			if (entries)
			{
				foreach (int entry : entries)
				{
					array<ResourceName> values = {};
					configurations.Get(entry).Get("PrefabsToSpawn", values);
					existing.InsertAll(values);
				}
			}
			if (SameItems(wanted, existing))
				continue;
			if (!entries)
			{
				int index;
				if (configurations)
					index = configurations.Count();
				if (!m_API.CreateObjectArrayVariableMember(m_Source, managerPath, "InitialInventoryItems", "ItemsInitConfigurationItem", index))
					return false;
				configurations = managerSource.GetObjectArray("InitialInventoryItems");
				entries = { index };
				array<ref ContainerIdPathEntry> newPath = {};
				foreach (ContainerIdPathEntry part : managerPath)
					newPath.Insert(part);
				newPath.Insert(new ContainerIdPathEntry("InitialInventoryItems", index));
				if (!m_API.SetVariableValue(m_Source, newPath, "TargetStorage", target) || !m_API.SetVariableValue(m_Source, newPath, "TargetPurpose", EStoragePurpose.PURPOSE_DEPOSIT.ToString()))
					return false;
			}
			string serialized;
			foreach (ResourceName prefab : wanted)
			{
				if (!serialized.IsEmpty())
					serialized += ",";
				serialized += prefab;
			}
			foreach (int position, int entry : entries)
			{
				array<ref ContainerIdPathEntry> entryPath = {};
				foreach (ContainerIdPathEntry part : managerPath)
					entryPath.Insert(part);
				entryPath.Insert(new ContainerIdPathEntry("InitialInventoryItems", entry));
				string value;
				if (position == 0)
					value = serialized;
				if (!m_API.SetVariableValue(m_Source, entryPath, "PrefabsToSpawn", value))
					return false;
				m_iChanges++;
			}
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool Save(IEntity owner)
	{
		if (!GC_WorkbenchItemBrowser.IsActive(owner) || !m_Source)
			return false;
		m_aPendingWeapons = {};
		EntitySpawnParams spawn = new EntitySpawnParams();
		spawn.TransformMode = ETransformMode.WORLD;
		spawn.Transform[3] = owner.GetOrigin() - "0 1000 0";
		m_Reference = GetGame().SpawnEntityPrefabLocal(m_Resource, GetGame().GetWorld(), spawn);
		if (!m_Reference)
			return false;
		if (!m_API.BeginEntityAction("Save loadout"))
		{
			SCR_EntityHelper.DeleteEntityAndChildren(m_Reference);
			m_Reference = null;
			return false;
		}
		bool success = WriteSlots(owner) && WriteContents(owner);
		m_API.EndEntityAction();
		SCR_EntityHelper.DeleteEntityAndChildren(m_Reference);
		m_Reference = null;
		if (!success)
			return false;
		// Validate all slots and contents before publishing any existing dependency.
		foreach (GC_LoadoutPrefab weapon : m_aPendingWeapons)
		{
			if (!weapon.m_API.SaveEntityTemplate(weapon.m_Source))
			{
				m_sError = "Some weapon changes may have been saved. Could not write " + weapon.m_sResource.GetPath();
				return false;
			}
		}
		if (!m_API.SaveEntityTemplate(m_Source))
		{
			m_sError = "Weapon changes were saved, but the character could not be written.";
			return false;
		}
		return true;
	}
}
#endif
