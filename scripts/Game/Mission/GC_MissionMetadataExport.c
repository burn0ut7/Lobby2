#ifdef WORKBENCH
//! Editor snapshot only: no runtime spawning, world edits or exhaustive dependency analysis.
class GC_MissionMetadataExport
{
	protected ref map<string, ref GC_MissionMetadataPrefab> m_mPrefabs = new map<string, ref GC_MissionMetadataPrefab>();
	protected ref map<string, ref array<string>> m_mPrefabAddons = new map<string, ref array<string>>();
	protected ref map<string, int> m_mAssets = new map<string, int>();
	protected ref array<string> m_aFactions = {};
	protected ref array<string> m_aConfiguredFactions = {};
	protected ref array<string> m_aReferencedAddons = {};
	protected int m_iEntityCount;

	//------------------------------------------------------------------------------------------------
	void Collect(WorldEditorAPI api, int subscene)
	{
		m_mPrefabs.Clear();
		m_mPrefabAddons.Clear();
		m_mAssets.Clear();
		m_aFactions.Clear();
		m_aConfiguredFactions.Clear();
		m_aReferencedAddons.Clear();
		m_iEntityCount = 0;
		// Flat editor enumeration omits some prefab-inherited children. Visit each root once.
		for (int i, count = api.GetEntityCount(subscene); i < count; i++)
		{
			IEntitySource source = api.GetEntity(subscene, i);
			if (source && !source.GetParent() && source.GetSubScene() == subscene)
				Visit(source, false);
		}
		m_aFactions.Sort();
		m_aConfiguredFactions.Sort();
		m_aReferencedAddons.Sort();
	}

	//------------------------------------------------------------------------------------------------
	int GetEntityCount()
	{
		return m_iEntityCount;
	}

	//------------------------------------------------------------------------------------------------
	protected void Visit(IEntitySource source, bool insideVehicle)
	{
		m_iEntityCount++;
		typename type = source.GetClassName().ToType();
		bool vehicle = type && type.IsInherited(BaseVehicle);
		string faction;
		BaseContainer affiliation = FindComponent(source, FactionAffiliationComponent);
		if (affiliation)
			affiliation.Get("faction affiliation", faction);
		AddUnique(m_aFactions, faction);
		if (type && type.IsInherited(FactionManager))
		{
			BaseContainerList factions = source.GetObjectArray("Factions");
			for (int f; factions && f < factions.Count(); f++)
			{
				string key;
				factions.Get(f).Get("FactionKey", key);
				AddUnique(m_aConfiguredFactions, key);
			}
		}

		string evidence;
		string asset;
		if (vehicle && !insideVehicle)
			asset = ClassifyVehicle(source, evidence);
		else if (type && type.IsInherited(ChimeraCharacter))
		{
			asset = "infantry";
			evidence = "class:ChimeraCharacter";
		}
		if (!asset.IsEmpty())
			m_mAssets.Set(asset, m_mAssets.Get(asset) + 1);

		ResourceName prefab;
		BaseContainer ancestor = source.GetAncestor();
		// Inherited child containers can return their entity name instead of a resource.
		// Advance to the first actual prefab resource before grouping or caching provenance.
		while (ancestor)
		{
			ResourceName candidate = ancestor.GetResourceName();
			if (candidate.StartsWith("{") && candidate.EndsWith(".et"))
			{
				prefab = candidate;
				break;
			}
			ancestor = ancestor.GetAncestor();
		}
		string rowKey = prefab + "\n" + source.GetClassName() + "\n" + faction + "\n" + asset + "\n" + evidence;
		GC_MissionMetadataPrefab row = m_mPrefabs.Get(rowKey);
		if (!row)
		{
			row = new GC_MissionMetadataPrefab();
			row.m_aAddonIds = {};
			row.m_sPrefab = prefab;
			row.m_sClass = source.GetClassName();
			row.m_sFaction = faction;
			row.m_sAsset = asset;
			row.m_sAssetEvidence = evidence;
			if (ancestor)
				row.m_aAddonIds = GetPrefabAddons(prefab, ancestor);
			m_mPrefabs.Set(rowKey, row);
		}
		row.m_iCount++;
		for (int i, count = source.GetNumChildren(); i < count; i++)
			Visit(source.GetChild(i), insideVehicle || vehicle);
	}

	//------------------------------------------------------------------------------------------------
	protected array<string> GetPrefabAddons(ResourceName prefab, BaseContainer ancestor)
	{
		array<string> result = m_mPrefabAddons.Get(prefab);
		if (result)
			return result;
		result = {};
		// Include addons defining/modifying any ancestor, not just the leaf prefab's addon.
		while (ancestor)
		{
			array<string> ids = {};
			ancestor.GetSourceAddons(ids);
			foreach (string id : ids)
			{
				AddUnique(result, id);
				AddUnique(m_aReferencedAddons, id);
			}
			ancestor = ancestor.GetAncestor();
		}
		result.Sort();
		m_mPrefabAddons.Set(prefab, result);
		return result;
	}

	//------------------------------------------------------------------------------------------------
	protected BaseContainer FindComponent(BaseContainer source, typename componentType)
	{
		BaseContainerList components = source.GetObjectArray("components");
		for (int i; components && i < components.Count(); i++)
		{
			BaseContainer component = components.Get(i);
			typename type = component.GetClassName().ToType();
			if (type && type.IsInherited(componentType))
				return component;
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected string ClassifyVehicle(IEntitySource source, out string evidence)
	{
		// RHIB uses wheeled simulation. Its verified ancestor identifies a boat, not buoyancy alone.
		BaseContainer ancestor = source.GetAncestor();
		while (ancestor)
		{
			if (ancestor.GetResourceName() == "{A295678AF2AE1205}Prefabs/Vehicles/Naval/RHIB/RHIB_Base.et")
			{
				evidence = "ancestor:RHIB_Base";
				return "boat";
			}
			ancestor = ancestor.GetAncestor();
		}
		if (FindComponent(source, VehicleHelicopterSimulation))
		{
			evidence = "component:VehicleHelicopterSimulation";
			return "helicopter";
		}
		// Optional mod enum members are resolved by name, without adding a mod dependency.
		BaseContainer editable = FindComponent(source, SCR_EditableVehicleComponent);
		BaseContainer info;
		if (editable)
			info = editable.GetObject("m_UIInfo");
		array<int> labels = {};
		if (info)
			info.Get("m_aAuthoredLabels", labels);
		foreach (int label : labels)
		{
			string labelName = typename.EnumToString(EEditableEntityLabel, label);
			if (labelName == "VEHICLE_BOAT" || labelName == "VEHICLE_TANK")
			{
				evidence = "authoredLabel:" + labelName;
				if (labelName == "VEHICLE_BOAT")
					return "boat";
				return "tank";
			}
		}
		int vehicleType;
		if (source.Get("m_eVehicleType", vehicleType) && vehicleType != EVehicleType.VEHICLE)
		{
			evidence = "vehicleType:" + typename.EnumToString(EVehicleType, vehicleType);
			switch (vehicleType)
			{
				case EVehicleType.APC: return "apc";
				case EVehicleType.CAR: return "car";
				case EVehicleType.MORTAR: return "mortar";
				case EVehicleType.TRUCK: return "truck";
				case EVehicleType.FUEL_TRUCK: return "fuel_truck";
				case EVehicleType.COMM_TRUCK: return "command_truck";
				case EVehicleType.SUPPLY_TRUCK: return "supply_truck";
				case EVehicleType.REPAIR: return "repair_vehicle";
				case EVehicleType.ARSENAL: return "arsenal_vehicle";
			}
		}
		BaseContainer usage = FindComponent(source, SCR_AIVehicleUsageComponent);
		int aiType;
		if (usage && usage.Get("m_eVehicleType", aiType))
		{
			evidence = "aiVehicleType:" + typename.EnumToString(EAIVehicleType, aiType);
			switch (aiType)
			{
				case EAIVehicleType.AIRCRAFT_HELICOPTER: return "helicopter";
				case EAIVehicleType.AIRCRAFT_PLANE: return "airplane";
				case EAIVehicleType.STATIC_ARTILLERY: return "artillery";
				case EAIVehicleType.STATIC_WEAPON: return "static_weapon";
				case EAIVehicleType.GROUND_VEHICLE_TRACKED: return "tracked_vehicle";
			}
		}
		evidence = "class:BaseVehicle";
		return "vehicle";
	}

	//------------------------------------------------------------------------------------------------
	protected void AddUnique(array<string> values, string value)
	{
		if (!value.IsEmpty() && !values.Contains(value))
			values.Insert(value);
	}

	//------------------------------------------------------------------------------------------------
	GC_GeneratedMissionMetadata CreateSnapshot(ResourceName world)
	{
		GC_GeneratedMissionMetadata snapshot = new GC_GeneratedMissionMetadata();
		snapshot.m_iSchemaVersion = 1;
		snapshot.m_aFactions = {};
		snapshot.m_aConfiguredFactions = {};
		snapshot.m_aReferencedAddonIds = {};
		snapshot.m_aAssets = {};
		snapshot.m_aPrefabs = {};
		snapshot.m_aLoadedAddons = {};
		snapshot.m_sWorld = world;
		snapshot.m_sCaptureScope = "Editor snapshot; active subscene, including unsaved edits and prefab children. Other subscenes, resource-valued slots and runtime spawns excluded. This snapshot does not refresh automatically. Asset presence does not imply player availability; source addons are not a complete dependency list.";
		snapshot.m_iEntityCount = m_iEntityCount;
		snapshot.m_aFactions.Copy(m_aFactions);
		snapshot.m_aConfiguredFactions.Copy(m_aConfiguredFactions);
		snapshot.m_aReferencedAddonIds.Copy(m_aReferencedAddons);
		array<string> keys = {};
		foreach (string assetKey, int assetCount : m_mAssets)
			keys.Insert(assetKey);
		keys.Sort();
		foreach (string key : keys)
		{
			GC_MissionMetadataAsset asset = new GC_MissionMetadataAsset();
			asset.m_sCategory = key;
			asset.m_iCount = m_mAssets.Get(key);
			snapshot.m_aAssets.Insert(asset);
		}
		keys.Clear();
		foreach (string prefabKey, GC_MissionMetadataPrefab row : m_mPrefabs)
			keys.Insert(prefabKey);
		keys.Sort();
		foreach (string prefab : keys)
			snapshot.m_aPrefabs.Insert(m_mPrefabs.Get(prefab));
		array<string> addons = {};
		GameProject.GetLoadedAddons(addons);
		addons.Sort();
		foreach (string guid : addons)
		{
			GC_MissionMetadataAddon addon = new GC_MissionMetadataAddon();
			addon.m_sId = GameProject.GetAddonID(guid);
			addon.m_sGuid = guid;
			addon.m_sTitle = GameProject.GetAddonTitle(guid);
			addon.m_bVanilla = GameProject.IsVanillaAddon(guid);
			addon.m_bReferencedByPlacedPrefab = m_aReferencedAddons.Contains(addon.m_sId);
			snapshot.m_aLoadedAddons.Insert(addon);
		}
		return snapshot;
	}

	//------------------------------------------------------------------------------------------------
	bool Save(string output, string worldPath)
	{
		PrettyJsonSaveContext context = CreateJson(worldPath);
		return context && context.SaveToFile(output);
	}

	//------------------------------------------------------------------------------------------------
	bool SaveIfChanged(string output, string worldPath, inout string previous, out bool changed)
	{
		changed = false;
		PrettyJsonSaveContext context = CreateJson(worldPath);
		if (!context)
			return false;
		string current = context.SaveToString();
		if (current == previous && FileIO.FileExists(output))
			return true;
		if (!context.SaveToFile(output))
			return false;
		previous = current;
		changed = true;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected PrettyJsonSaveContext CreateJson(string worldPath)
	{
		PrettyJsonSaveContext context = new PrettyJsonSaveContext(false);
		array<string> limitations = {"Captured world subscene only; inherited/other world subscenes excluded.", "Current editor snapshot, including unsaved edits.", "Placed entities only; resource-valued slots, loadouts and runtime spawners are not expanded.", "Prefab source addons are evidence of content use, not a complete dependency list.", "Asset categories describe presence, not player availability; mod configuration may be inaccurate."};
		bool valid = context.WriteValue("schemaVersion", 1)
			&& context.WriteValue("world", worldPath)
			&& context.WriteValue("scope", "world_subscene")
			&& context.WriteValue("limitations", limitations)
			&& context.WriteValue("entityCount", m_iEntityCount)
			&& context.WriteValue("factions", m_aFactions)
			&& context.WriteValue("configuredFactions", m_aConfiguredFactions)
			&& context.WriteValue("referencedAddonIds", m_aReferencedAddons);

		array<string> keys = {};
		foreach (string assetKey, int assetCount : m_mAssets)
			keys.Insert(assetKey);
		keys.Sort();
		valid = context.StartObject("assets") && valid;
		foreach (string asset : keys)
			valid = context.WriteValue(asset, m_mAssets.Get(asset)) && valid;
		valid = context.EndObject() && valid;

		keys.Clear();
		foreach (string prefabKey, GC_MissionMetadataPrefab row : m_mPrefabs)
			keys.Insert(prefabKey);
		keys.Sort();
		int count = keys.Count();
		valid = context.StartArray("prefabs", count) && valid;
		foreach (string key : keys)
		{
			valid = context.StartObject() && valid;
			valid = m_mPrefabs.Get(key).Write(context) && valid;
			valid = context.EndObject() && valid;
		}
		valid = context.EndArray() && valid;

		array<string> addons = {};
		GameProject.GetLoadedAddons(addons);
		addons.Sort();
		count = addons.Count();
		valid = context.StartArray("loadedAddons", count) && valid;
		foreach (string guid : addons)
		{
			string id = GameProject.GetAddonID(guid);
			valid = context.StartObject() && valid;
			valid = context.WriteValue("id", id) && valid;
			valid = context.WriteValue("guid", guid) && valid;
			valid = context.WriteValue("title", GameProject.GetAddonTitle(guid)) && valid;
			valid = context.WriteValue("vanilla", GameProject.IsVanillaAddon(guid)) && valid;
			valid = context.WriteValue("referencedByPlacedPrefab", m_aReferencedAddons.Contains(id)) && valid;
			valid = context.EndObject() && valid;
		}
		valid = context.EndArray() && valid;
		if (!valid)
			return null;
		return context;
	}
}
#endif
