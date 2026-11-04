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
	protected ref array<ref GC_MissionMetadataSlot> m_aPlayerSlots = {};
	protected ref map<ResourceName, ref GC_LoadoutMetadata> m_mLoadouts = new map<ResourceName, ref GC_LoadoutMetadata>();
	protected int m_iEntityCount;
	protected int m_iPlayableGroupCount;
	protected BaseContainer m_TimeHandler;
	protected bool m_bAmbiguousTime;
	protected bool m_bAllowHeaderTime;
	protected bool m_bRandomStart;
	protected ref array<int> m_aStartHours = {};
	protected string m_sError;
	protected bool m_bComplete;
	protected int m_iStarted;
	protected WBProgressDialog m_Progress;

	//------------------------------------------------------------------------------------------------
	bool Collect(WorldEditorAPI api, int subscene, WBProgressDialog progress = null)
	{
		m_bComplete = false;
		m_sError = string.Empty;
		m_iStarted = System.GetTickCount();
		m_Progress = progress;
		m_mPrefabs.Clear();
		m_mPrefabAddons.Clear();
		m_mAssets.Clear();
		m_aFactions.Clear();
		m_aConfiguredFactions.Clear();
		m_aReferencedAddons.Clear();
		m_aPlayerSlots.Clear();
		m_mLoadouts.Clear();
		m_iEntityCount = 0;
		m_iPlayableGroupCount = 0;
		m_TimeHandler = null;
		m_bAmbiguousTime = false;
		m_bAllowHeaderTime = false;
		m_bRandomStart = false;
		m_aStartHours.Clear();
		// Flat editor enumeration omits some prefab-inherited children. Visit each root once.
		for (int i, count = api.GetEntityCount(subscene); i < count; i++)
		{
			if (count > 100000 || System.GetTickCount(m_iStarted) > 30000)
			{
				m_sError = "Mission scan exceeded its safety limit (100,000 entities or 30 seconds).";
				break;
			}
			IEntitySource source = api.GetEntity(subscene, i);
			if (source && !source.GetParent() && source.GetSubScene() == subscene)
			{
				typename type = source.GetClassName().ToType();
				if (type && type.IsInherited(GenericTerrainEntity))
				{
					m_sError = "Select the mission subscene, not the base terrain.";
					break;
				}
				Visit(source, false, 0);
			}
			if (!m_sError.IsEmpty())
				break;
			if (progress && (i % 32 == 0 || i == count - 1))
				progress.SetProgress((i + 1.0) / count);
		}
		m_Progress = null;
		if (!m_sError.IsEmpty())
			return false;
		CollectStartTime(api);
		m_aFactions.Sort();
		m_aConfiguredFactions.Sort();
		m_aReferencedAddons.Sort();
		m_bComplete = true;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	string GetError()
	{
		return m_sError;
	}

	//------------------------------------------------------------------------------------------------
	int GetEntityCount()
	{
		return m_iEntityCount;
	}

	//------------------------------------------------------------------------------------------------
	protected void Visit(IEntitySource source, bool insideVehicle, int depth)
	{
		if (!source || !m_sError.IsEmpty())
			return;
		if (depth > 128 || m_iEntityCount >= 100000 || System.GetTickCount(m_iStarted) > 30000)
		{
			m_sError = "Mission scan exceeded its entity, nesting or time limit.";
			return;
		}
		m_iEntityCount++;
		if (m_Progress && m_iEntityCount % 32 == 0)
			m_Progress.SetActionTitle(string.Format("%1 mission entities checked", m_iEntityCount));
		typename type = source.GetClassName().ToType();
		if (type && type.IsInherited(BaseGameMode))
			ObserveTimeHandler(source);
		bool vehicle = type && type.IsInherited(BaseVehicle);
		string faction;
		BaseContainer affiliation = FindComponent(source, FactionAffiliationComponent);
		if (affiliation)
			affiliation.Get("faction affiliation", faction);
		AddUnique(m_aFactions, faction);
		if (type && type.IsInherited(SCR_AIGroup))
			CollectPlayerSlots(source);
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
		int ancestorDepth;
		while (ancestor && ancestorDepth++ < 128)
		{
			ResourceName candidate = ancestor.GetResourceName();
			if (candidate.StartsWith("{") && candidate.EndsWith(".et"))
			{
				prefab = candidate;
				break;
			}
			ancestor = ancestor.GetAncestor();
		}
		if (ancestorDepth > 128)
		{
			m_sError = "Prefab inheritance exceeded 128 levels.";
			return;
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
		{
			Visit(source.GetChild(i), insideVehicle || vehicle, depth + 1);
			if (!m_sError.IsEmpty())
				break;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected array<string> GetPrefabAddons(ResourceName prefab, BaseContainer ancestor)
	{
		array<string> result = m_mPrefabAddons.Get(prefab);
		if (result)
			return result;
		result = {};
		// Include addons defining/modifying any ancestor, not just the leaf prefab's addon.
		int ancestorDepth;
		while (ancestor && ancestorDepth++ < 128)
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
		if (ancestor)
			m_sError = "Prefab inheritance exceeded 128 levels.";
		result.Sort();
		m_mPrefabAddons.Set(prefab, result);
		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! Read exactly the authored entries used by GC_LobbyManager.RegisterGroupMembers.
	//! This never spawns characters or chooses a random variant.
	protected void CollectPlayerSlots(IEntitySource source)
	{
		bool playable;
		source.Get("m_bPlayable", playable);
		if (!playable)
			return;
		m_iPlayableGroupCount++;
		string groupName = source.GetName();
		if (groupName.IsEmpty())
			groupName = string.Format("Group %1", m_iPlayableGroupCount);
		string faction;
		source.Get("m_faction", faction);
		AddUnique(m_aFactions, faction);
		array<ResourceName> prefabs = {};
		source.Get("m_aUnitPrefabSlots", prefabs);
		foreach (int index, ResourceName prefab : prefabs)
		{
			if (m_aPlayerSlots.Count() >= 10000 || System.GetTickCount(m_iStarted) > 30000)
			{
				m_sError = "Player-slot scan exceeded its slot or time limit.";
				return;
			}
			GC_MissionMetadataSlot slot = new GC_MissionMetadataSlot();
			slot.m_sGroup = groupName;
			slot.m_iIndex = index;
			slot.m_sFaction = faction;
			slot.m_sPrefab = prefab;
			GC_LoadoutMetadata loadout;
			if (!m_mLoadouts.Find(prefab, loadout))
			{
				loadout = GC_LoadoutMetadata.Read(prefab);
				m_mLoadouts.Set(prefab, loadout);
			}
			if (loadout)
			{
				slot.m_sName = loadout.m_sName;
				slot.m_sLoadoutFaction = loadout.m_sFaction;
				AddUnique(m_aFactions, loadout.m_sFaction);
			}
			m_aPlayerSlots.Insert(slot);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ObserveTimeHandler(IEntitySource source)
	{
		BaseContainer handler = FindComponent(source, SCR_TimeAndWeatherHandlerComponent);
		if (!handler)
			return;
		if (m_TimeHandler && handler != m_TimeHandler)
			m_bAmbiguousTime = true;
		m_TimeHandler = handler;
	}

	//------------------------------------------------------------------------------------------------
	protected void CollectStartTime(WorldEditorAPI api)
	{
		// The game mode may live in a parent subscene. Use its resolved source if available.
		BaseGameMode mode = GetGame().GetGameMode();
		if (mode && mode.GetWorld() == api.GetWorld())
		{
			IEntitySource modeSource = api.EntityToSource(mode);
			if (modeSource)
				ObserveTimeHandler(modeSource);
		}
		if (m_bAmbiguousTime)
			return;
		int hour;
		if (!m_TimeHandler)
		{
			ChimeraWorld world = ChimeraWorld.CastFrom(api.GetWorld());
			if (!world || !world.GetTimeAndWeatherManager())
				return;
			IEntitySource manager = api.EntityToSource(world.GetTimeAndWeatherManager());
			float initialHour;
			if (manager && manager.Get("Initial Day Time", initialHour) && initialHour >= 0 && initialHour < 24)
				m_aStartHours.Insert(Math.Floor(initialHour));
			return;
		}

		bool presets, range;
		m_TimeHandler.Get("m_bUsePredefineStartingTimeAndWeather", presets);
		BaseContainerList states = m_TimeHandler.GetObjectArray("m_aStartingWeatherAndTime");
		if (presets && states && states.Count() > 0)
		{
			for (int i; i < states.Count(); i++)
			{
				if (!states.Get(i).Get("m_iStartingHour", hour) || hour < 0 || hour > 23)
				{
					m_aStartHours.Clear();
					return;
				}
				if (!m_aStartHours.Contains(hour))
					m_aStartHours.Insert(hour);
			}
			return;
		}
		m_TimeHandler.Get("m_bRandomStartingDaytimeRange", range);
		if (range)
		{
			int start, end, startMinute, endMinute;
			if (!m_TimeHandler.Get("m_iRangeHoursStart", start) || !m_TimeHandler.Get("m_iRangeHoursEnd", end)
				|| !m_TimeHandler.Get("m_iRangeMinutesStart", startMinute) || !m_TimeHandler.Get("m_iRangeMinutesEnd", endMinute)
				|| start < 0 || end > 23 || start > end || startMinute > endMinute)
				return;
			// The handler passes negative hours/minutes for wrapped ranges. Their
			// normalization is native, so omit those instead of guessing.
			int endExclusive = Math.Max(start + 1, end);
			for (hour = start; hour < endExclusive; hour++)
				m_aStartHours.Insert(hour);
			return;
		}
		m_TimeHandler.Get("m_bAllowHeaderSettings", m_bAllowHeaderTime);
		m_TimeHandler.Get("m_bRandomStartingDaytime", m_bRandomStart);
		if (!m_bRandomStart && m_TimeHandler.Get("m_iStartingHours", hour) && hour >= 0 && hour < 24)
			m_aStartHours.Insert(hour);
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
		int ancestorDepth;
		while (ancestor && ancestorDepth++ < 128)
		{
			if (ancestor.GetResourceName() == "{A295678AF2AE1205}Prefabs/Vehicles/Naval/RHIB/RHIB_Base.et")
			{
				evidence = "ancestor:RHIB_Base";
				return "boat";
			}
			ancestor = ancestor.GetAncestor();
		}
		if (ancestor)
			m_sError = "Prefab inheritance exceeded 128 levels.";
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
		snapshot.m_sCaptureScope = "Editor snapshot; active subscene, including unsaved edits and prefab children. Factions include authored playable groups and loadout keys. Slot rows are exported only to the JSON sidecar. Other subscenes and runtime spawns excluded. This snapshot does not refresh automatically. Asset presence does not imply player availability; source addons are not a complete dependency list.";
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
		if (!m_bComplete)
			return false;
		PrettyJsonSaveContext context = CreateJson(worldPath);
		return context && context.SaveToFile(output);
	}

	//------------------------------------------------------------------------------------------------
	protected PrettyJsonSaveContext CreateJson(string worldPath)
	{
		PrettyJsonSaveContext context = new PrettyJsonSaveContext(false);
		array<string> limitations = {"Captured world subscene only; inherited/other world subscenes excluded.", "Current editor snapshot, including unsaved edits.", "Player slots describe authored playable groups and their loadout prefabs; random variants, runtime spawns and inventory contents are not expanded. Group and loadout faction keys are separate authored values; runtime changes are not inferred.", "Prefab source addons are evidence of content use, not a complete dependency list.", "Asset categories describe presence, not player availability; mod configuration may be inaccurate."};
		bool valid = context.WriteValue("schemaVersion", 1)
			&& context.WriteValue("world", worldPath)
			&& context.WriteValue("scope", "world_subscene")
			&& context.WriteValue("limitations", limitations)
			&& context.WriteValue("entityCount", m_iEntityCount)
			&& context.WriteValue("factions", m_aFactions)
			&& context.WriteValue("configuredFactions", m_aConfiguredFactions)
			&& context.WriteValue("referencedAddonIds", m_aReferencedAddons);
		if (m_bRandomStart || !m_aStartHours.IsEmpty())
		{
			if (m_bRandomStart)
				valid = context.WriteValue("randomStartTime", true) && valid;
			else
				valid = context.WriteValue("startHours", m_aStartHours) && valid;
			valid = context.WriteValue("allowHeaderTime", m_bAllowHeaderTime) && valid;
		}
		int slotCount = m_aPlayerSlots.Count();
		valid = context.StartArray("playerSlots", slotCount) && valid;
		foreach (GC_MissionMetadataSlot slot : m_aPlayerSlots)
		{
			valid = context.StartObject() && valid;
			valid = slot.Write(context) && valid;
			valid = context.EndObject() && valid;
		}
		valid = context.EndArray() && valid;

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

//! Export-only derived data; never appears in a mission header's property grid.
class GC_MissionMetadataSlot
{
	string m_sGroup;
	int m_iIndex;
	string m_sFaction;
	ResourceName m_sPrefab;
	string m_sName;
	string m_sLoadoutFaction;

	bool Write(SaveContext context)
	{
		return context.WriteValue("group", m_sGroup)
			&& context.WriteValue("index", m_iIndex)
			&& context.WriteValue("faction", m_sFaction)
			&& context.WriteValue("prefab", m_sPrefab)
			&& context.WriteValue("name", m_sName)
			&& context.WriteValue("loadoutFaction", m_sLoadoutFaction);
	}
}
#endif
