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
	static bool IsWritable(ResourceName resourceName)
	{
		if (!resourceName.GetPath().StartsWith("Prefabs/Loadouts/"))
			return false;
		string path;
		if (!Workbench.GetAbsolutePath("$GlobalConflictsUnitsDev:" + resourceName.GetPath(), path))
			return false;
		ResourceManager manager = Workbench.GetModule(ResourceManager);
		if (!manager)
			return false;
		MetaFile meta = manager.GetMetaFile(path);
		return meta && meta.GetResourceID() == resourceName;
	}

	//------------------------------------------------------------------------------------------------
	static string Destination(string faction, string group, string name)
	{
		if (!ValidName(faction) || !ValidName(group) || !ValidName(name))
			return string.Empty;
		string root;
		if (!Workbench.GetAbsolutePath("$GlobalConflictsUnitsDev:", root))
			return string.Empty;
		root = FilePath.Concat(root, "Prefabs/Loadouts/");
		array<string> files = {};
		FileIO.FindFiles(files.Insert, root, ".et");
		string directory = ExistingDirectoryCase(FilePath.Concat(root, faction) + "/", files);
		directory = ExistingDirectoryCase(FilePath.Concat(directory, group) + "/", files);
		return FilePath.Concat(directory, name + ".et");
	}

	//------------------------------------------------------------------------------------------------
	//! Windows accepts Test/test as one directory; resource identities must use its actual spelling.
	protected static string ExistingDirectoryCase(string directory, array<string> files)
	{
		directory.Replace("\\", "/");
		string folded = directory;
		folded.ToLower();
		foreach (string filename : files)
		{
			filename.Replace("\\", "/");
			string candidate = filename;
			candidate.ToLower();
			if (candidate.StartsWith(folded))
				return filename.Substring(0, directory.Length());
		}
		return directory;
	}

	//------------------------------------------------------------------------------------------------
	static ResourceName Register(string path)
	{
		// RegisterResourceFile always opens Workbench's registration progress UI,
		// even with runtime building disabled. Author the native prefab sidecar
		// directly; the resource manager reads it without invoking the importer.
		string root;
		if (!Workbench.GetAbsolutePath("$GlobalConflictsUnitsDev:", root))
			return string.Empty;
		root.Replace("\\", "/");
		path.Replace("\\", "/");
		if (!root.EndsWith("/"))
			root += "/";
		if (!path.StartsWith(root + "Prefabs/Loadouts/") || path.Contains("/../") || !path.EndsWith(".et") || FileIO.FileExists(path + ".meta"))
			return string.Empty;
		string guid = Workbench.GenerateGloballyUniqueID64();
		guid.Replace("{", "");
		guid.Replace("}", "");
		if (guid.Length() != 16)
			return string.Empty;
		string identity = "{" + guid + "}" + path.Substring(root.Length(), path.Length() - root.Length());
		FileHandle file = FileIO.OpenFile(path + ".meta", FileMode.WRITE);
		if (!file)
			return string.Empty;
		file.WriteLine("MetaFileClass {");
		file.WriteLine(" Name " + Quote(identity));
		file.WriteLine(" Configurations {");
		file.WriteLine("  EntityTemplateResourceClass PC {}");
		array<string> platforms = {"XBOX_ONE", "XBOX_SERIES", "PS4", "PS5", "HEADLESS"};
		foreach (string platform : platforms)
			file.WriteLine("  EntityTemplateResourceClass " + platform + " : PC {}");
		file.WriteLine(" }");
		file.WriteLine("}");
		file.Close();
		ResourceManager manager = Workbench.GetModule(ResourceManager);
		if (!manager)
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
		if (!GetGame() || !GetGame().GetPlayerController() || !GC_PrefabAuthoringSession.IsAvailable(GetGame().GetPlayerController().GetControlledEntity()))
			return string.Empty;
		string root;
		if (!Workbench.GetAbsolutePath("$GlobalConflictsUnitsDev:Prefabs/Loadouts/", root, false))
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
		if (!IsWritable(resourceName))
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
	//! A new gear choice inherits the pool and uses a predictable, collision-safe filename.
	static bool CreateGearVariant(IEntity owner, ResourceName parent, int number, out ResourceName created)
	{
		if (!GC_PrefabAuthoringSession.IsAvailable(owner) || !IsWritable(parent))
			return false;
		GC_LoadoutMetadata metadata = GC_LoadoutMetadata.Read(parent);
		if (!metadata)
			return false;
		string faction = metadata.m_sFaction;
		faction.ToUpper();
		string pool = FilePath.StripExtension(FilePath.StripPath(parent.GetPath()));
		pool.ToLower();
		pool.Replace(" ", "_");
		if (!ValidName(faction) || !ValidName(pool))
			return false;
		string directory;
		if (!Workbench.GetAbsolutePath("$GlobalConflictsUnitsDev:" + FilePath.StripFileName(parent.GetPath()), directory, false))
			return false;
		string path;
		string name;
		for (int attempt; attempt < 10000; attempt++)
		{
			name = faction + "_variant_" + pool + "_" + number;
			path = FilePath.Concat(directory, name + ".et");
			if (!FileIO.FileExists(path) && !FileIO.FileExists(path + ".meta"))
				break;
			number++;
		}
		created = Create(parent, path, false);
		if (created.IsEmpty())
			return false;
		GC_LoadoutPrefab writer = new GC_LoadoutPrefab();
		metadata.m_sName = name;
		if (!writer.Open(created) || !writer.Save(owner, metadata))
			return false;
		// A choice is a concrete character, not another pool inherited from its parent.
		GC_LoadoutVariants empty = new GC_LoadoutVariants();
		return writer.SaveVariants(owner, empty);
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
				if (item && HasSlotChanges(item))
				{
					wanted = SaveVariant(item);
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
	protected ResourceName SaveVariant(IEntity item)
	{
		array<string> parts = {};
		m_sResource.GetPath().Split("/", parts, true);
		if (parts.Count() < 5)
			return string.Empty;
		ResourceName parent = SCR_ResourceNameUtils.GetPrefabName(item);
		Resource parentResource = Resource.Load(parent);
		IEntitySource source = SCR_BaseContainerTools.FindEntitySource(parentResource);
		// Use the original weapon name, not an already numbered variant's label.
		BaseContainer original = source;
		while (original && original.GetAncestor() && original.GetResourceName().GetPath().StartsWith("Prefabs/Loadouts/"))
			original = original.GetAncestor();
		string name = FilePath.StripExtension(FilePath.StripPath(parent));
		IEntityComponentSource inventory = SCR_ComponentHelper.GetInventoryItemComponentSource(IEntitySource.Cast(original));
		SCR_ItemAttributeCollection attributes = SCR_ComponentHelper.GetInventoryItemInfo(inventory);
		if (attributes && attributes.GetUIInfo())
			name = WidgetManager.Translate(attributes.GetUIInfo().GetName()).Trim();
		string stem;
		for (int ch; ch < name.Length(); ch++)
		{
			string letter = name.Substring(ch, 1);
			if ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-".Contains(letter))
				stem += letter;
			else if (letter == " ")
				stem += "_";
		}
		if (stem.IsEmpty())
			stem = "Weapon";
		string team = parts[3];
		team.Replace(" ", "_");
		string directory;
		if (!Workbench.GetAbsolutePath("$GlobalConflictsUnitsDev:Prefabs/Loadouts/" + parts[2] + "/" + parts[3] + "/Weapons/", directory, false))
			return string.Empty;
		ResourceManager manager = Workbench.GetModule(ResourceManager);
		string path;
		ResourceName variant;
		for (int number = 1; number <= 10000; number++)
		{
			path = FilePath.Concat(directory, stem + "_" + team + "_" + number + ".et");
			if (!FileIO.FileExists(path) && !FileIO.FileExists(path + ".meta"))
				break;
			MetaFile meta = manager.GetMetaFile(path);
			if (!meta)
				continue;
			Resource candidate = Resource.Load(meta.GetResourceID());
			BaseContainer candidateSource = SCR_BaseContainerTools.FindEntitySource(candidate);
			if (candidateSource && candidateSource.GetAncestor() && candidateSource.GetAncestor().GetResourceName() == parent && MatchesSlots(item, candidateSource))
				return meta.GetResourceID();
			path = string.Empty;
		}
		// Shared variants are not overwritten when another unit customizes a weapon.
		if (path.IsEmpty())
			return string.Empty;
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
	protected bool MatchesSlots(IEntity owner, BaseContainer source)
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
				if (!slot || !slot.GetParentContainer())
					continue;
				array<ref ContainerIdPathEntry> path = {};
				BaseContainer component = FindComponent(source, slot.GetParentContainer().GetComponentSource(owner).GetResourceName(), path);
				if (!component)
					return false;
				string key = "Prefab";
				BaseContainer target = component;
				if (component.GetVarIndex("WeaponTemplate") >= 0)
					key = "WeaponTemplate";
				else if (component.GetVarIndex("MagazineTemplate") >= 0)
					key = "MagazineTemplate";
				else
					target = FindSlot(component, slot.GetSourceName(), path);
				ResourceName expected;
				if (!target || !target.Get(key, expected))
					return false;
				IEntity item = slot.GetAttachedEntity();
				ResourceName actual;
				if (item)
					actual = SCR_ResourceNameUtils.GetPrefabName(item);
				if (actual == expected && (!item || !HasSlotChanges(item)))
					continue;
				if (!item || expected.IsEmpty())
					return false;
				Resource nestedResource = Resource.Load(expected);
				BaseContainer nestedSource = SCR_BaseContainerTools.FindEntitySource(nestedResource);
				if (!nestedSource)
					return false;
				// A customized attachment is itself an inherited prefab. Compare its
				// authored slots, not just its new identity, when reusing the weapon.
				if (actual != expected && (!nestedSource.GetAncestor() || nestedSource.GetAncestor().GetResourceName() != actual))
					return false;
				if (!MatchesSlots(item, nestedSource))
					return false;
			}
		}
		return true;
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
	protected string SerializeLabels(array<EEditableEntityLabel> labels)
	{
		string value;
		foreach (EEditableEntityLabel label : labels)
		{
			if (!value.IsEmpty())
				value += ",";
			value += label.ToString();
		}
		return value;
	}

	//------------------------------------------------------------------------------------------------
	protected bool WriteFactionLabels(BaseContainer info, BaseContainer inheritedInfo, array<ref ContainerIdPathEntry> path, string key, SCR_Faction faction, bool authored)
	{
		array<EEditableEntityLabel> current = {};
		if (!info.Get(key, current))
			return false;
		array<EEditableEntityLabel> wanted = {};
		bool inserted;
		foreach (EEditableEntityLabel label : current)
		{
			if (typename.EnumToString(EEditableEntityLabel, label).StartsWith("FACTION_"))
			{
				if (authored && faction && !inserted)
				{
					wanted.Insert(faction.GetFactionLabel());
					inserted = true;
				}
			}
			else
				wanted.Insert(label);
		}
		if (authored && faction && !inserted)
			wanted.Insert(faction.GetFactionLabel());
		string value = SerializeLabels(wanted);
		if (SerializeLabels(current) == value)
			return true;
		array<EEditableEntityLabel> inherited = {};
		if (inheritedInfo && inheritedInfo.Get(key, inherited) && SerializeLabels(inherited) == value)
			return m_API.ClearVariableValue(m_Source, path, key);
		return m_API.SetVariableValue(m_Source, path, key, value);
	}

	//------------------------------------------------------------------------------------------------
	//! Updates only the selected prefab's native randomizer; no gear write or Play restart.
	bool SaveVariants(IEntity owner, GC_LoadoutVariants variants)
	{
		if (!GC_PrefabAuthoringSession.IsAvailable(owner) || !m_Source || !IsWritable(m_sResource) || !variants || !variants.IsValid(m_sResource))
			return false;
		if (variants.Equals(GC_LoadoutVariants.Read(m_Source)))
			return m_API.SaveEntityTemplate(m_Source);
		array<ref ContainerIdPathEntry> path = {};
		BaseContainer editable = GC_LoadoutMetadata.FindComponent(m_Source, SCR_EditableCharacterComponent, path);
		if (!editable || !m_API.BeginEntityAction("Save random variants"))
			return false;
		bool success = WriteVariants(editable, path, variants);
		m_API.EndEntityAction();
		return success && m_API.SaveEntityTemplate(m_Source);
	}

	//------------------------------------------------------------------------------------------------
	protected bool WriteVariants(BaseContainer editable, array<ref ContainerIdPathEntry> path, GC_LoadoutVariants variants)
	{
		if (m_Source.GetAncestor() && variants.Equals(GC_LoadoutVariants.Read(m_Source.GetAncestor())))
			return m_API.ClearVariableValue(m_Source, path, "m_VariantData");
		// Replace this object's local value; removing members of an inherited
		// object array through the native editor API crashes Workbench 1.8.
		if (!m_API.CreateObjectVariableMember(m_Source, path, "m_VariantData", "SCR_EditableEntityVariantData"))
			return false;
		path.Insert(new ContainerIdPathEntry("m_VariantData"));
		if (!m_API.SetVariableValue(m_Source, path, "m_bRandomizeDefaultVariant", variants.m_bIncludeDefault.ToString()) || !m_API.SetVariableValue(m_Source, path, "m_iDefaultVariantRandomizerWeight", variants.m_iDefaultWeight.ToString()))
			return false;
		foreach (int index, SCR_EditableEntityVariant entry : variants.m_aEntries)
		{
			if (!m_API.CreateObjectArrayVariableMember(m_Source, path, "m_aVariants", "SCR_EditableEntityVariant", index))
				return false;
			path.Insert(new ContainerIdPathEntry("m_aVariants", index));
			bool success = m_API.SetVariableValue(m_Source, path, "m_sVariantPrefab", entry.m_sVariantPrefab)
				&& m_API.SetVariableValue(m_Source, path, "m_iRandomizerWeight", entry.m_iRandomizerWeight.ToString())
				&& m_API.SetVariableValue(m_Source, path, "m_bEnabled", entry.m_bEnabled.ToString());
			path.Remove(path.Count() - 1);
			if (!success)
				return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool Save(IEntity owner, GC_LoadoutMetadata metadata = null, bool writeGear = true)
	{
		if (!GC_PrefabAuthoringSession.IsAvailable(owner) || !m_Source || !IsWritable(m_sResource))
			return false;
		array<ref ContainerIdPathEntry> editablePath = {};
		array<ref ContainerIdPathEntry> factionPath = {};
		BaseContainer editable;
		BaseContainer affiliation;
		if (metadata)
		{
			editable = GC_LoadoutMetadata.FindComponent(m_Source, SCR_EditableCharacterComponent, editablePath);
			affiliation = GC_LoadoutMetadata.FindComponent(m_Source, FactionAffiliationComponent, factionPath);
			if (!metadata.IsValid() || !editable || !editable.GetObject("m_UIInfo") || !affiliation)
			{
				m_sError = "Choose a valid faction and character role icon.";
				return false;
			}
		}
		m_aPendingWeapons = {};
		EntitySpawnParams spawn = new EntitySpawnParams();
		spawn.TransformMode = ETransformMode.WORLD;
		spawn.Transform[3] = owner.GetOrigin() - "0 1000 0";
		if (writeGear)
		{
			m_Reference = GetGame().SpawnEntityPrefabLocal(m_Resource, GetGame().GetWorld(), spawn);
			if (!m_Reference)
				return false;
		}
		if (!m_API.BeginEntityAction("Save loadout"))
		{
			if (m_Reference)
				SCR_EntityHelper.DeleteEntityAndChildren(m_Reference);
			m_Reference = null;
			return false;
		}
		bool success = true;
		if (writeGear)
			success = WriteSlots(owner) && WriteContents(owner);
		if (success && metadata)
		{
			BaseContainer info = editable.GetObject("m_UIInfo");
			BaseContainer inheritedInfo;
			if (editable.GetAncestor())
				inheritedInfo = editable.GetAncestor().GetObject("m_UIInfo");
			editablePath.Insert(new ContainerIdPathEntry("m_UIInfo"));
			SCR_Faction faction = SCR_Faction.Cast(GetGame().GetFactionManager().GetFactionByKey(metadata.m_sFaction));
			success = SetResource(info, inheritedInfo, editablePath, "Name", metadata.m_sName)
				&& SetResource(info, inheritedInfo, editablePath, "Icon", metadata.m_sIcon)
				&& SetResource(info, inheritedInfo, editablePath, "IconSetName", metadata.m_sIconName)
				&& SetResource(info, inheritedInfo, editablePath, "m_sFaction", metadata.m_sFaction)
				&& SetResource(affiliation, affiliation.GetAncestor(), factionPath, "faction affiliation", metadata.m_sFaction)
				&& WriteFactionLabels(info, inheritedInfo, editablePath, "m_aAuthoredLabels", faction, true)
				&& WriteFactionLabels(info, inheritedInfo, editablePath, "m_aAutoLabels", faction, false);
		}
		m_API.EndEntityAction();
		if (m_Reference)
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
		if (!m_aPendingWeapons.IsEmpty())
			GC_ItemCatalogue.NotifyAuthoredSave();
		if (!m_API.SaveEntityTemplate(m_Source))
		{
			m_sError = "Weapon changes were saved, but the character could not be written.";
			return false;
		}
		return true;
	}
}
#endif
