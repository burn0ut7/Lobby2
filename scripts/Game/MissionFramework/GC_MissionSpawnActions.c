//! Native AI waypoint ownership is shared by spawning and reassignment.
class GC_MissionWaypoints
{
	//------------------------------------------------------------------------------------------------
	static bool Assign(AIGroup group, array<string> names, bool clear)
	{
		if (!group)
			return false;
		array<AIWaypoint> next = {};
		if (names)
		{
			foreach (string name : names)
			{
				AIWaypoint waypoint = AIWaypoint.Cast(GetGame().GetWorld().FindEntityByName(name));
				if (!waypoint)
					return false;
				next.Insert(waypoint);
			}
		}
		if (clear)
		{
			array<AIWaypoint> previous = {};
			group.GetWaypoints(previous);
			foreach (AIWaypoint waypoint : previous)
				group.RemoveWaypoint(waypoint);
		}
		foreach (AIWaypoint waypoint : next)
			group.AddWaypoint(waypoint);
		return true;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Spawn Prefab")]
class GC_SpawnPrefabAction : GC_MissionAction
{
	[Attribute("", UIWidgets.ResourceNamePicker, "Prefab to spawn.", params: "et")]
	ResourceName m_sPrefab;
	[Attribute("", UIWidgets.EditBox, "Name of an existing entity that supplies position and rotation.")]
	string m_sLocation;
	[Attribute("", UIWidgets.EditBox, "Optional unique name for the spawned entity. A name already in use rejects the spawn.")]
	string m_sNewName;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (!m_sLocation.IsEmpty() && !m_sPrefab.IsEmpty() && Resource.Load(m_sPrefab))
			return true;
		error = "Spawn Prefab: choose a valid prefab and a location entity name.";
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected IEntity Spawn(GC_LobbyMissionContext context)
	{
		if (!context.IsActive())
			return null;
		IEntity location = GetGame().GetWorld().FindEntityByName(m_sLocation);
		if (!location || (!m_sNewName.IsEmpty() && GetGame().GetWorld().FindEntityByName(m_sNewName)))
			return null;
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		location.GetWorldTransform(params.Transform);
		IEntity entity = GetGame().SpawnEntityPrefab(Resource.Load(m_sPrefab), GetGame().GetWorld(), params);
		if (entity && !m_sNewName.IsEmpty())
			entity.SetName(m_sNewName);
		return entity;
	}

	//------------------------------------------------------------------------------------------------
	override bool Execute(GC_LobbyMissionContext context)
	{
		return Spawn(context) != null;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Spawn AI Group")]
class GC_SpawnAIGroupAction : GC_SpawnPrefabAction
{
	[Attribute("", UIWidgets.EditBox, "Names of existing waypoints, in order.")]
	ref array<string> m_aWaypoints;

	//------------------------------------------------------------------------------------------------
	override bool Execute(GC_LobbyMissionContext context)
	{
		IEntity entity = Spawn(context);
		AIGroup group = AIGroup.Cast(entity);
		if (!group)
		{
			if (entity)
				SCR_EntityHelper.DeleteEntityAndChildren(entity);
			return false;
		}
		return GC_MissionWaypoints.Assign(group, m_aWaypoints, false);
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Assign Waypoints")]
class GC_AssignWaypointsAction : GC_MissionAction
{
	[Attribute("", UIWidgets.EditBox, "Name of the existing AI group.")]
	string m_sGroup;
	[Attribute("", UIWidgets.EditBox, "Names of existing waypoints, in order.")]
	ref array<string> m_aWaypoints;
	[Attribute("0", UIWidgets.CheckBox, "Remove the group's current waypoints before adding these.")]
	bool m_bClearExisting;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (!m_sGroup.IsEmpty() && (m_bClearExisting || (m_aWaypoints && !m_aWaypoints.IsEmpty())))
			return true;
		error = "Assign Waypoints: enter a group and waypoints, or choose Clear Existing.";
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool Execute(GC_LobbyMissionContext context)
	{
		if (!context.IsActive())
			return false;
		return GC_MissionWaypoints.Assign(AIGroup.Cast(GetGame().GetWorld().FindEntityByName(m_sGroup)), m_aWaypoints, m_bClearExisting);
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Crew Group")]
class GC_MissionCrewGroup
{
	[Attribute("1", UIWidgets.CheckBox, "Spawn this group.")]
	bool m_bEnabled;
	[Attribute("", UIWidgets.EditBox, "Optional unique AI group entity name. An existing AI group with this name receives the crew.")]
	string m_sName;
	[Attribute("", UIWidgets.EditBox, "Optional player-facing group name.")]
	string m_sDisplayName;
	[Attribute("", UIWidgets.Object, "Which seats and characters belong to this group. Entries consume seats in order.")]
	ref array<ref GC_MissionCrewSeats> m_aSeats;
	[Attribute("", UIWidgets.EditBox, "Existing waypoint names, in order.")]
	ref array<string> m_aWaypoints;
	[Attribute("0", UIWidgets.EditBox, "Seconds after boarding before waypoints are assigned.", params: "0 86400 0.1")]
	float m_fWaypointDelay;
	[Attribute("1", UIWidgets.CheckBox, "Keep gunners in separate idle groups when this group has waypoints.")]
	bool m_bSeparateGunners;
	[Attribute("0", UIWidgets.CheckBox, "Prevent the AI's automatic turret-investigation dismount. Explicit orders can still make crew leave. Useful for static gunners.")]
	bool m_bKeepGunnersSeated;
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Crew Seats")]
class GC_MissionCrewSeats
{
	[Attribute("1", UIWidgets.CheckBox, "Fill driver/pilot seats.")]
	bool m_bDrivers;
	[Attribute("1", UIWidgets.CheckBox, "Fill gunner seats.")]
	bool m_bGunners;
	[Attribute("1", UIWidgets.CheckBox, "Fill passenger seats.")]
	bool m_bPassengers;
	[Attribute("", UIWidgets.ResourceAssignArray, "Optional character prefabs in seat order. Empty list uses the vehicle defaults; an empty entry leaves that seat empty.", params: "et")]
	ref array<ResourceName> m_aCharacters;

	//------------------------------------------------------------------------------------------------
	bool Accepts(BaseCompartmentSlot seat)
	{
		return (m_bDrivers && seat.GetType() == ECompartmentType.PILOT)
			|| (m_bGunners && seat.GetType() == ECompartmentType.TURRET)
			|| (m_bPassengers && seat.GetType() == ECompartmentType.CARGO);
	}
}

//! Runtime-only follow-up, owned by the same cancellable scheduler as every action.
class GC_MissionCrewWaypoints : GC_MissionAction
{
	AIGroup m_Group;
	ref array<string> m_aNames;

	//------------------------------------------------------------------------------------------------
	override bool Execute(GC_LobbyMissionContext context)
	{
		return context.IsActive() && GC_MissionWaypoints.Assign(m_Group, m_aNames, false);
	}
}

class GC_MissionFillCrew : GC_MissionAction
{
	IEntity m_Vehicle;
	ref array<ref GC_MissionCrewGroup> m_aGroups;

	//------------------------------------------------------------------------------------------------
	override bool Execute(GC_LobbyMissionContext context)
	{
		if (!context.IsActive() || !m_Vehicle)
			return false;
		SCR_BaseCompartmentManagerComponent compartments = SCR_BaseCompartmentManagerComponent.Cast(m_Vehicle.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (!compartments)
			return false;
		array<BaseCompartmentSlot> seats = {};
		compartments.GetCompartments(seats);
		bool applied = true;
		foreach (GC_MissionCrewGroup definition : m_aGroups)
		{
			if (!definition.m_bEnabled)
				continue;
			AIGroup group;
			if (!definition.m_sName.IsEmpty())
			{
				IEntity existing = GetGame().GetWorld().FindEntityByName(definition.m_sName);
				group = AIGroup.Cast(existing);
				if (existing && !group)
					return false;
			}
			foreach (GC_MissionCrewSeats selection : definition.m_aSeats)
			{
				int characterIndex;
				for (int i; i < seats.Count();)
				{
					BaseCompartmentSlot seat = seats[i];
					if (!seat || seat.IsOccupied() || !seat.IsCompartmentAccessible() || !selection.Accepts(seat))
					{
						i++;
						continue;
					}
					bool custom = selection.m_aCharacters && !selection.m_aCharacters.IsEmpty();
					if (custom && characterIndex >= selection.m_aCharacters.Count())
						break;
					ResourceName prefab = seat.GetDefaultOccupantPrefab();
					if (custom)
						prefab = selection.m_aCharacters[characterIndex++];
					seats.RemoveOrdered(i);
					if (prefab.IsEmpty())
						continue;
					bool idle = definition.m_bSeparateGunners && seat.GetType() == ECompartmentType.TURRET
						&& definition.m_aWaypoints && !definition.m_aWaypoints.IsEmpty();
					AIGroup destination = group;
					if (idle)
						destination = null;
					IEntity character = seat.SpawnCharacterInCompartment(prefab, destination);
					if (!character)
						applied = false;
					else if (definition.m_bKeepGunnersSeated && seat.GetType() == ECompartmentType.TURRET)
					{
						SCR_AICombatComponent combat = SCR_AICombatComponent.Cast(character.FindComponent(SCR_AICombatComponent));
						if (combat)
							combat.GC_KeepMissionTurretSeated(context);
						else
							applied = false;
					}
					if (!idle)
						group = destination;
				}
			}
			if (!group)
				continue;
			if (!definition.m_sName.IsEmpty())
				group.SetName(definition.m_sName);
			SCR_AIGroup scripted = SCR_AIGroup.Cast(group);
			if (scripted && !definition.m_sDisplayName.IsEmpty())
				scripted.SetCustomName(definition.m_sDisplayName, 0);
			if (definition.m_aWaypoints && !definition.m_aWaypoints.IsEmpty())
			{
				GC_MissionCrewWaypoints next = new GC_MissionCrewWaypoints();
				next.m_Group = group;
				next.m_aNames = definition.m_aWaypoints;
				next.m_fDelayAfterEventStarts = definition.m_fWaypointDelay;
				context.GetRuntime().QueueFollowup(next, "Crew waypoints");
			}
		}
		return applied;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Spawn Crewed Vehicle")]
class GC_SpawnCrewedVehicleAction : GC_SpawnPrefabAction
{
	[Attribute("", UIWidgets.Object, "Crew groups and seat selection. Add at least one group.")]
	ref array<ref GC_MissionCrewGroup> m_aCrew;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (!m_aCrew || m_aCrew.IsEmpty())
		{
			error = "Spawn Crewed Vehicle: add a crew group.";
			return false;
		}
		foreach (GC_MissionCrewGroup group : m_aCrew)
		{
			if (!group || !group.m_aSeats || group.m_aSeats.IsEmpty() || group.m_fWaypointDelay < 0 || group.m_fWaypointDelay > 86400)
			{
				error = "Spawn Crewed Vehicle: each crew group needs seats and a valid waypoint delay.";
				return false;
			}
			foreach (GC_MissionCrewSeats seats : group.m_aSeats)
			{
				if (!seats)
					return false;
				if (seats.m_aCharacters)
				{
					foreach (ResourceName name : seats.m_aCharacters)
					{
						Resource prefab = Resource.Load(name);
						if (name.IsEmpty() || !prefab || !prefab.IsValid())
						{
							error = "Spawn Crewed Vehicle: invalid crew prefab " + name;
							return false;
						}
					}
				}
			}
		}
		return super.Validate(context, error);
	}

	//------------------------------------------------------------------------------------------------
	override bool Execute(GC_LobbyMissionContext context)
	{
		IEntity vehicle = Spawn(context);
		if (!vehicle)
			return false;
		GC_MissionFillCrew next = new GC_MissionFillCrew();
		next.m_Vehicle = vehicle;
		next.m_aGroups = m_aCrew;
		return context.GetRuntime().QueueFollowup(next, "Fill vehicle crew");
	}
}
