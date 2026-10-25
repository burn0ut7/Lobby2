enum GC_EAOBehavior
{
	HARD_RETURN,
	SOFT_KILL
}

enum GC_EAOListMode
{
	IGNORE,
	ONLY_MATCHING,
	EXCEPT_MATCHING
}

enum GC_EAOVehicleState
{
	ANY,
	ON_FOOT,
	IN_VEHICLE,
	DRIVER,
	PASSENGER
}

enum GC_EAOItemCondition
{
	IGNORE,
	HAS_ANY,
	HAS_ALL,
	HAS_NONE,
	MISSING_ANY
}

[EntityEditorProps(category: "Lobby2/Mission", description: "Closed XZ polygon AO. Applicable limits combine. Highest priority breached limit acts; equal priorities prefer hard return, then earliest soft deadline. Exact ties retain the first evaluated limit.", color: "255 70 70 255")]
class GC_AOLimitEntityClass : PolylineShapeEntityClass
{
}

class GC_AOLimitEntity : PolylineShapeEntity
{
	[Attribute("1", UIWidgets.CheckBox, "Enable this restriction. Trusted server scripts can call SetEnabled later.", category: "AO")]
	protected bool m_bEnabled;
	[Attribute("0", UIWidgets.ComboBox, "Hard: return the character or whole occupied vehicle. Soft: kill the affected character after the grace period.", enums: ParamEnumArray.FromEnum(GC_EAOBehavior), category: "AO")]
	protected GC_EAOBehavior m_eBehavior;
	[Attribute("0", UIWidgets.CheckBox, "Off: stay INSIDE the polygon. On: stay OUTSIDE the polygon. Height is ignored.", category: "AO")]
	protected bool m_bInverted;
	[Attribute("0", UIWidgets.EditBox, "Higher numbers win among breached applicable limits. Equal priorities are allowed: hard wins, then earliest soft deadline; exact ties retain the first evaluated limit. Other limits keep their timers. Use distinct priorities to explicitly choose a winner.", category: "AO")]
	protected int m_iPriority;
	[Attribute("20", UIWidgets.EditBox, "Continuous seconds outside before death. Re-entry or exemption clears this limit's timer. Range 0.1-3600.", category: "AO")]
	protected float m_fGraceSeconds;
	[Attribute("RETURN TO AO", UIWidgets.EditBox, "Heading displayed above this AO warning countdown. Keep it short to fit the compact HUD.", category: "AO")]
	protected string m_sWarningText;
	[Attribute("", UIWidgets.EditBox, "Optional safe return transform for already enrolled characters without valid return history. Must satisfy ALL applicable hard limits. Units that have never entered the permitted area remain exempt.", category: "AO")]
	protected string m_sReturnEntity;
	[Attribute("", UIWidgets.Auto, "Native player faction keys. Empty applies to everyone; otherwise only the listed factions are affected.", category: "Eligibility")]
	protected ref array<string> m_aFactionKeys;
	[Attribute("0", UIWidgets.ComboBox, "Required vehicle state. Passenger includes turret occupants. All eligibility conditions must match.", enums: ParamEnumArray.FromEnum(GC_EAOVehicleState), category: "Eligibility")]
	protected GC_EAOVehicleState m_eVehicleState;
	[Attribute("0", UIWidgets.ComboBox, "ONLY_MATCHING affects occupants of listed vehicle prefabs. EXCEPT_MATCHING exempts those occupants. Prefab inheritance is included.", enums: ParamEnumArray.FromEnum(GC_EAOListMode), category: "Eligibility")]
	protected GC_EAOListMode m_eVehicleMode;
	[Attribute("", UIWidgets.Auto, "Vehicle prefabs (including derived prefabs)", params: "et", category: "Eligibility")]
	protected ref array<ResourceName> m_aVehiclePrefabs;
	[Attribute("0", UIWidgets.ComboBox, "Restrict only players satisfying this inventory condition. HAS_NONE exempts item holders; HAS_ANY exempts players without the items. Includes equipped/nested managed inventory, excludes vehicle cargo.", enums: ParamEnumArray.FromEnum(GC_EAOItemCondition), category: "Eligibility")]
	protected GC_EAOItemCondition m_eItemCondition;
	[Attribute("", UIWidgets.Auto, "Item prefabs (including derived prefabs). Required when item condition is enabled.", params: "et", category: "Eligibility")]
	protected ref array<ResourceName> m_aItemPrefabs;
	protected ref array<vector> m_aPolygon = {};
	protected GC_AOLimitSystem m_System;
	protected bool m_bValid;
	// Per-unit enrollment survives filter changes, freezes and temporary loss of possession.
	protected ref map<IEntity, bool> m_mEnrollment = new map<IEntity, bool>();

	//------------------------------------------------------------------------------------------------
	//! Trusted authority API. Disabling clears active violations immediately.
	bool SetEnabled(bool enabled)
	{
		if (!Replication.IsServer() || !m_System || !m_bValid)
			return false;
		m_bEnabled = enabled;
		m_System.ResetLimit(this);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Trusted authority API. Equal priorities are allowed.
	bool SetBehavior(GC_EAOBehavior behavior, float graceSeconds, int priority)
	{
		if (!Replication.IsServer() || !m_System || behavior < 0 || behavior > GC_EAOBehavior.SOFT_KILL
			|| graceSeconds < 0.1 || graceSeconds > 3600)
			return false;
		m_eBehavior = behavior;
		m_fGraceSeconds = graceSeconds;
		m_iPriority = priority;
		m_System.ResetLimit(this);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool IsEnabled()
	{
		return m_bEnabled && m_bValid;
	}

	//------------------------------------------------------------------------------------------------
	GC_EAOBehavior GetBehavior()
	{
		return m_eBehavior;
	}

	//------------------------------------------------------------------------------------------------
	int GetPriority()
	{
		return m_iPriority;
	}

	//------------------------------------------------------------------------------------------------
	float GetGraceSeconds()
	{
		return m_fGraceSeconds;
	}

	//------------------------------------------------------------------------------------------------
	//! Runtime geometry is world-space. The caller owns safe, non-self-intersecting authoring.
	bool SetPolygon(array<vector> worldPoints, bool inverted, string returnEntity)
	{
		if (!Replication.IsServer() || !m_System || !ValidatePolygon(worldPoints) || !ValidateSettings())
			return false;
		m_aPolygon.Copy(worldPoints);
		m_bInverted = inverted;
		m_sReturnEntity = returnEntity;
		m_bValid = true;
		if (!m_System.Register(this))
		{
			m_bValid = false;
			return false;
		}
		m_System.ResetLimit(this);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool ValidatePolygon(array<vector> points)
	{
		if (!points || points.Count() < 3)
			return false;
		float twiceArea;
		for (int i; i < points.Count(); i++)
		{
			vector first = points[i];
			vector second = points[(i + 1) % points.Count()];
			twiceArea += first[0] * second[2] - second[0] * first[2];
		}
		return Math.AbsFloat(twiceArea) > 0.01;
	}

	//------------------------------------------------------------------------------------------------
	protected bool ValidateSettings()
	{
		if (m_fGraceSeconds < 0.1 || m_fGraceSeconds > 3600 || m_eBehavior < 0 || m_eBehavior > GC_EAOBehavior.SOFT_KILL
			|| m_eVehicleMode < 0 || m_eVehicleMode > GC_EAOListMode.EXCEPT_MATCHING
			|| m_eVehicleState < 0 || m_eVehicleState > GC_EAOVehicleState.PASSENGER
			|| m_eItemCondition < 0 || m_eItemCondition > GC_EAOItemCondition.MISSING_ANY)
			return false;
		if (m_eItemCondition != GC_EAOItemCondition.IGNORE && (!m_aItemPrefabs || m_aItemPrefabs.IsEmpty()))
			return false;
		if (m_eVehicleMode != GC_EAOListMode.IGNORE && (!m_aVehiclePrefabs || m_aVehiclePrefabs.IsEmpty()))
			return false;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Trusted server scripts can replace the filters atomically. Lists are copied.
	bool SetEligibility(array<string> factions, GC_EAOVehicleState vehicleState,
		GC_EAOListMode vehicleMode, array<ResourceName> vehicles, GC_EAOItemCondition itemCondition, array<ResourceName> items)
	{
		if (!Replication.IsServer() || !m_System
			|| vehicleMode < 0 || vehicleMode > GC_EAOListMode.EXCEPT_MATCHING
			|| vehicleState < 0 || vehicleState > GC_EAOVehicleState.PASSENGER
			|| itemCondition < 0 || itemCondition > GC_EAOItemCondition.MISSING_ANY)
			return false;
		if (vehicleMode != GC_EAOListMode.IGNORE && (!vehicles || vehicles.IsEmpty()))
			return false;
		if (itemCondition != GC_EAOItemCondition.IGNORE && (!items || items.IsEmpty()))
			return false;
		m_eVehicleState = vehicleState;
		m_eVehicleMode = vehicleMode;
		m_eItemCondition = itemCondition;
		m_aFactionKeys = {};
		m_aVehiclePrefabs = {};
		m_aItemPrefabs = {};
		if (factions)
			m_aFactionKeys.Copy(factions);
		if (vehicles)
			m_aVehiclePrefabs.Copy(vehicles);
		if (items)
			m_aItemPrefabs.Copy(items);
		m_System.ResetLimit(this);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool Permits(vector position)
	{
		bool inside = Math2D.IsPointInPolygonXZ(m_aPolygon, position);
		return inside != m_bInverted;
	}

	//------------------------------------------------------------------------------------------------
	//! First entry into the permitted side arms this AO for the unit's lifetime.
	string GetWarningText()
	{
		return m_sWarningText;
	}

	//------------------------------------------------------------------------------------------------
	//! First entry into the permitted side arms this AO for the unit's lifetime.
	void ObserveCharacter(IEntity character, IEntity vehicle)
	{
		if (!Replication.IsServer() || !IsEnabled() || !character || m_mEnrollment.Contains(character))
			return;
		IEntity mover = character;
		if (vehicle)
			mover = vehicle;
		if (Permits(mover.GetOrigin()))
			m_mEnrollment.Set(character, true);
	}

	//------------------------------------------------------------------------------------------------
	bool GetReturnTransform(out vector transform[4])
	{
		if (m_sReturnEntity.IsEmpty())
			return false;
		IEntity anchor = GetWorld().FindEntityByName(m_sReturnEntity);
		if (!anchor)
			return false;
		anchor.GetWorldTransform(transform);
		return Permits(transform[3]);
	}

	//------------------------------------------------------------------------------------------------
	protected bool MatchesPrefab(IEntity entity, ResourceName prefab)
	{
		if (!entity || !entity.GetPrefabData())
			return false;
		return SCR_BaseContainerTools.IsKindOf(entity.GetPrefabData().GetPrefab(), prefab);
	}

	//------------------------------------------------------------------------------------------------
	protected bool MatchesFaction(int playerId)
	{
		if (!m_aFactionKeys || m_aFactionKeys.IsEmpty())
			return true;
		Faction faction = GC_LobbyManager.GetManager().GetPlayerFaction(playerId);
		return faction && m_aFactionKeys.Contains(faction.GetFactionKey());
	}

	//------------------------------------------------------------------------------------------------
	//! Shared map component reads the enforcement polygon without owning a second copy.
	void GetMapPolygon(array<vector> points)
	{
		if (m_bValid)
			points.Copy(m_aPolygon);
	}

	//------------------------------------------------------------------------------------------------
	//! Native player affiliation and current possession are queried on each evaluation.
	bool AppliesTo(int playerId, IEntity character, IEntity vehicle)
	{
		if (!IsEnabled())
			return false;
		bool enrolled;
		if (!m_mEnrollment.Find(character, enrolled) || !enrolled)
			return false;
		if (!MatchesFaction(playerId))
			return false;
		bool driver;
		if (vehicle)
		{
			BaseVehicleControllerComponent controller = BaseVehicleControllerComponent.Cast(vehicle.FindComponent(BaseVehicleControllerComponent));
			if (controller && controller.GetPilotCompartmentSlot())
				driver = controller.GetPilotCompartmentSlot().GetOccupant() == character;
		}
		if (m_eVehicleState == GC_EAOVehicleState.ON_FOOT && vehicle)
			return false;
		if (m_eVehicleState == GC_EAOVehicleState.IN_VEHICLE && !vehicle)
			return false;
		if (m_eVehicleState == GC_EAOVehicleState.DRIVER && !driver)
			return false;
		if (m_eVehicleState == GC_EAOVehicleState.PASSENGER && (!vehicle || driver))
			return false;
		if (m_eVehicleMode != GC_EAOListMode.IGNORE)
		{
			bool vehicleMatches;
			foreach (ResourceName vehiclePrefab : m_aVehiclePrefabs)
			{
				if (MatchesPrefab(vehicle, vehiclePrefab))
					vehicleMatches = true;
			}
			if ((m_eVehicleMode == GC_EAOListMode.ONLY_MATCHING) != vehicleMatches)
				return false;
		}
		if (m_eItemCondition == GC_EAOItemCondition.IGNORE)
			return true;
		InventoryStorageManagerComponent inventory = InventoryStorageManagerComponent.Cast(character.FindComponent(InventoryStorageManagerComponent));
		array<IEntity> items = {};
		if (inventory)
			inventory.GetItems(items);
		int matches;
		foreach (ResourceName itemPrefab : m_aItemPrefabs)
		{
			foreach (IEntity item : items)
			{
				if (!MatchesPrefab(item, itemPrefab))
					continue;
				matches++;
				break;
			}
		}
		if (m_eItemCondition == GC_EAOItemCondition.HAS_ANY)
			return matches > 0;
		if (m_eItemCondition == GC_EAOItemCondition.HAS_ALL)
			return matches == m_aItemPrefabs.Count();
		if (m_eItemCondition == GC_EAOItemCondition.HAS_NONE)
			return matches == 0;
		return matches < m_aItemPrefabs.Count();
	}

	//------------------------------------------------------------------------------------------------
	protected void RegisterLimit()
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!mode || !Replication.IsServer())
			return;
		m_System = mode.GetAOLimits();
		GetPointsPositions(m_aPolygon);
		for (int i; i < m_aPolygon.Count(); i++)
			m_aPolygon[i] = CoordToParent(m_aPolygon[i]);
		m_bValid = IsClosed() && ValidatePolygon(m_aPolygon) && ValidateSettings();
		if (!m_bValid || !m_System || !m_System.Register(this))
		{
			m_bValid = false;
			PrintFormat("GC | AO | invalid configuration: %1", GetName(), level: LogLevel.ERROR);
		}
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		if (!SCR_Global.IsEditMode() && Replication.IsServer())
			GetGame().GetCallqueue().CallLater(RegisterLimit, 0);
	}

	//------------------------------------------------------------------------------------------------
	void GC_AOLimitEntity(IEntitySource src, IEntity parent)
	{
		SetEventMask(EntityEvent.INIT);
	}

	//------------------------------------------------------------------------------------------------
	void ~GC_AOLimitEntity()
	{
		if (GetGame())
			GetGame().GetCallqueue().Remove(RegisterLimit);
		if (m_System)
			m_System.Unregister(this);
	}
}
