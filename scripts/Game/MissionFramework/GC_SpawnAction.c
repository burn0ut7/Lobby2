//! One action, with only the fields belonging to the selected spawn setup.
[BaseContainerProps(), BaseContainerCustomStringTitleField("Spawn Action")]
class GC_SpawnAction : GC_ActionBase
{
	[Attribute("", UIWidgets.Object, "Choose Prefab, AI Group or Crewed Vehicle. Spawns once each time this action runs. Use the event's conditions, repeatability and Delay to control when it happens.")]
	ref GC_PrefabSpawn m_Spawn;

	//------------------------------------------------------------------------------------------------
	override GC_RuleAction BuildAction()
	{
		GC_RuleAction action = new GC_RuleAction();
		action.m_eDo = GC_ERuleAction.SPAWN;
		action.m_Spawn = m_Spawn;
		if (m_Spawn)
			action.m_sTextOrName = m_Spawn.m_sLocation;
		return action;
	}
}

//! This parent is also the useful plain-prefab choice. Configurations hold no runtime entities.
[BaseContainerProps(), BaseContainerCustomStringTitleField("Prefab")]
class GC_PrefabSpawn
{
	[Attribute("", UIWidgets.ResourceNamePicker, "Prefab to create. Its own components control its normal behavior. Playable groups are lobby slots and cannot be spawned by this action.", "et")]
	ResourceName m_sPrefab;
	[Attribute("", UIWidgets.EditBox, "Exact name of an existing entity. Its position and direction are used for the spawn. No automatic ground placement or clear-space search is performed.")]
	string m_sLocation;
	[Attribute("", UIWidgets.EditBox, "Optional unique name for the new entity. Leave empty when spawning repeatedly. If this name is already in use, nothing spawns.")]
	string m_sEntityName;

	//------------------------------------------------------------------------------------------------
	bool IsValid()
	{
		return !m_sPrefab.IsEmpty() && !m_sLocation.IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	protected bool Prepare(IEntitySource source)
	{
		if (!source)
			return false;
		if (source.GetClassName().ToType().IsInherited(SCR_AIGroup))
		{
			bool playable;
			source.Get("m_bPlayable", playable);
			if (playable)
				return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool Configure(IEntity entity, GC_SpawnAttempt attempt)
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Called only through the manager's current-engine/server authority gate. No callbacks or polling.
	bool Execute()
	{
		if (!IsValid())
			return false;
		BaseWorld world = GetGame().GetWorld();
		IEntity location = world.FindEntityByName(m_sLocation);
		if (!location || (!m_sEntityName.IsEmpty() && world.FindEntityByName(m_sEntityName)))
			return false;
		Resource resource = Resource.Load(m_sPrefab);
		if (!resource || !resource.IsValid() || !Prepare(resource.GetResource().ToEntitySource()))
			return false;
		EntitySpawnParams parameters = new EntitySpawnParams();
		parameters.TransformMode = ETransformMode.WORLD;
		location.GetWorldTransform(parameters.Transform);
		IEntity entity = GetGame().SpawnEntityPrefab(resource, world, parameters);
		if (!entity)
			return false;
		GC_SpawnAttempt attempt = new GC_SpawnAttempt();
		SCR_AIGroup group = SCR_AIGroup.Cast(entity);
		if ((group && group.IsPlayable()) || !Configure(entity, attempt))
		{
			attempt.Rollback();
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
			return false;
		}
		if (!m_sEntityName.IsEmpty())
			entity.SetName(m_sEntityName);
		return true;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("AI Group")]
class GC_AIGroupSpawn : GC_PrefabSpawn
{
	[Attribute("", UIWidgets.EditBox, "Names of existing AI waypoints, in order. Added after the prefab's existing orders. Leave empty to use only the prefab's orders. Choose a non-playable AI group prefab configured to spawn its members.")]
	ref array<string> m_aWaypoints;

	//------------------------------------------------------------------------------------------------
	override bool IsValid()
	{
		return super.IsValid() && GC_RuleAction.ValidTargetNames(m_aWaypoints);
	}

	//------------------------------------------------------------------------------------------------
	override protected bool Prepare(IEntitySource source)
	{
		return super.Prepare(source) && source.GetClassName().ToType().IsInherited(AIGroup)
			&& GC_WaypointOrders.Check(m_aWaypoints);
	}

	//------------------------------------------------------------------------------------------------
	override protected bool Configure(IEntity entity, GC_SpawnAttempt attempt)
	{
		AIGroup group = AIGroup.Cast(entity);
		if (!group)
			return false;
		return GC_WaypointOrders.Apply(group, m_aWaypoints);
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Crewed Vehicle")]
class GC_CrewedVehicleSpawn : GC_PrefabSpawn
{
	[Attribute("", UIWidgets.Object, "Optional crew groups, processed from top to bottom. Empty: fill available seats with the vehicle's default crew in one AI group. Add groups to separate driver/passengers from gunners or give them different waypoints. Each seat is used at most once.")]
	ref array<ref GC_SpawnCrewGroup> m_aCrewGroups;

	//------------------------------------------------------------------------------------------------
	override bool IsValid()
	{
		if (!super.IsValid())
			return false;
		if (!m_aCrewGroups)
			return true;
		if (m_aCrewGroups.Count() > 16)
			return false;
		foreach (GC_SpawnCrewGroup crew : m_aCrewGroups)
		{
			if (!crew || !crew.IsValid())
				return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override protected bool Prepare(IEntitySource source)
	{
		if (!super.Prepare(source))
			return false;
		if (m_aCrewGroups)
		{
			foreach (GC_SpawnCrewGroup crew : m_aCrewGroups)
			{
				if (!crew.Prepare())
					return false;
			}
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override protected bool Configure(IEntity entity, GC_SpawnAttempt attempt)
	{
		SCR_BaseCompartmentManagerComponent manager = SCR_BaseCompartmentManagerComponent.Cast(entity.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (!manager)
			return false;
		array<BaseCompartmentSlot> slots = {};
		manager.GetCompartments(slots);
		if (slots.IsEmpty() || slots.Count() > 128)
			return false;
		if (!m_aCrewGroups || m_aCrewGroups.IsEmpty())
		{
			GC_SpawnCrewGroup defaults = new GC_SpawnCrewGroup();
			return defaults.Fill(slots, attempt) && attempt.HasCrew();
		}
		foreach (GC_SpawnCrewGroup crew : m_aCrewGroups)
		{
			if (!crew.Fill(slots, attempt))
				return false;
		}
		return attempt.HasCrew();
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Crew Group")]
class GC_SpawnCrewGroup
{
	[Attribute("1", UIWidgets.CheckBox, "Fill available driver or pilot seats.")]
	bool m_bDriver = true;
	[Attribute("1", UIWidgets.CheckBox, "Fill available gunner seats. For stationary gunners, put them in their own crew group without waypoints.")]
	bool m_bGunners = true;
	[Attribute("1", UIWidgets.CheckBox, "Fill available passenger seats.")]
	bool m_bPassengers = true;
	[Attribute("", UIWidgets.ResourceNamePicker, "Optional characters in the vehicle's seat order. Empty list: use default crew. With a list, a blank entry leaves that seat empty; when the list ends, this group stops filling seats.", "et")]
	ref array<ResourceName> m_aCharacters;
	[Attribute("", UIWidgets.EditBox, "Existing AI waypoint names in order. These orders apply only to this crew group. Empty means no added orders.")]
	ref array<string> m_aWaypoints;
	[Attribute("0", UIWidgets.CheckBox, "Keep this group's gunners in their turrets rather than voluntarily dismounting to fight.")]
	bool m_bKeepGunnersSeated;

	//------------------------------------------------------------------------------------------------
	bool IsValid()
	{
		return (m_bDriver || m_bGunners || m_bPassengers) && GC_RuleAction.ValidTargetNames(m_aWaypoints)
			&& (!m_aCharacters || m_aCharacters.Count() <= 128);
	}

	//------------------------------------------------------------------------------------------------
	bool Prepare()
	{
		if (!GC_WaypointOrders.Check(m_aWaypoints))
			return false;
		if (m_aCharacters)
		{
			foreach (ResourceName name : m_aCharacters)
			{
				if (name.IsEmpty())
					continue;
				Resource resource = Resource.Load(name);
				if (!resource || !resource.IsValid())
					return false;
			}
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool Fill(array<BaseCompartmentSlot> slots, GC_SpawnAttempt attempt)
	{
		AIGroup group;
		int characterIndex;
		bool custom = m_aCharacters && !m_aCharacters.IsEmpty();
		array<BaseCompartmentSlot> available = {};
		available.Copy(slots);
		foreach (BaseCompartmentSlot slot : available)
		{
			if (custom && characterIndex >= m_aCharacters.Count())
				break;
			if (!slot || slot.IsOccupied() || !slot.IsCompartmentAccessible())
				continue;
			ECompartmentType type = slot.GetType();
			if (!((type == ECompartmentType.PILOT && m_bDriver) || (type == ECompartmentType.TURRET && m_bGunners)
				|| (type == ECompartmentType.CARGO && m_bPassengers)))
				continue;
			ResourceName character;
			if (custom)
				character = m_aCharacters[characterIndex++];
			else
				character = slot.GetDefaultOccupantPrefab();
			if (!custom && character.IsEmpty())
				continue;
			slots.RemoveItemOrdered(slot);
			if (character.IsEmpty())
				continue;
			IEntity occupant = slot.SpawnCharacterInCompartment(character, group);
			attempt.Track(occupant, group);
			if (!occupant || !group)
				return false;
			if (m_bKeepGunnersSeated && type == ECompartmentType.TURRET)
			{
				SCR_AICombatComponent combat = SCR_AICombatComponent.Cast(occupant.FindComponent(SCR_AICombatComponent));
				if (combat)
					combat.m_neverDismountTurret = true;
			}
		}
		if (group)
			return GC_WaypointOrders.Apply(group, m_aWaypoints);
		return true;
	}
}

//! Local attempt bookkeeping only; native groups own members after success.
class GC_SpawnAttempt
{
	protected ref array<IEntity> m_aCrew = {};
	protected ref array<AIGroup> m_aGroups = {};

	//------------------------------------------------------------------------------------------------
	void Track(IEntity crew, AIGroup group)
	{
		if (crew)
			m_aCrew.Insert(crew);
		if (group && !m_aGroups.Contains(group))
			m_aGroups.Insert(group);
	}

	//------------------------------------------------------------------------------------------------
	bool HasCrew()
	{
		return !m_aCrew.IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	void Rollback()
	{
		foreach (IEntity crew : m_aCrew)
		{
			if (crew)
				SCR_EntityHelper.DeleteEntityAndChildren(crew);
		}
		foreach (AIGroup group : m_aGroups)
		{
			if (group)
				SCR_EntityHelper.DeleteEntityAndChildren(group);
		}
	}

}
