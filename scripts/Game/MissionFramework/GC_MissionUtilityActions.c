[BaseContainerProps(), BaseContainerCustomStringTitleField("Set Cleanup Protection")]
class GC_SetCleanupProtectionAction : GC_MissionAction
{
	[Attribute("", UIWidgets.EditBox, "Names of entities to protect from automatic garbage collection.")]
	ref array<string> m_aEntities;
	[Attribute("1", UIWidgets.CheckBox, "Protect from automatic cleanup. Off removes the native blacklist entry. Explicit mission deletion is unaffected.")]
	bool m_bProtected;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (m_aEntities && !m_aEntities.IsEmpty())
			return true;
		error = "Set Cleanup Protection: add entity names.";
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool Execute(GC_LobbyMissionContext context)
	{
		if (!context.IsActive())
			return false;
		GarbageSystem garbage = ChimeraWorld.CastFrom(GetGame().GetWorld()).GetGarbageSystem();
		if (!garbage)
			return false;
		bool applied = true;
		foreach (string name : m_aEntities)
		{
			IEntity entity = GetGame().GetWorld().FindEntityByName(name);
			if (!entity || !garbage.UpdateBlacklist(entity, m_bProtected))
				applied = false;
		}
		return applied;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Rebuild Navigation")]
class GC_RebuildNavigationAction : GC_MissionAction
{
	[Attribute("", UIWidgets.EditBox, "Names of changed objects. Rebuilds native navigation around each object and its children. Objects must have navigation collision layers; completion is asynchronous.")]
	ref array<string> m_aEntities;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (m_aEntities && !m_aEntities.IsEmpty())
			return true;
		error = "Rebuild Navigation: add entity names.";
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool Execute(GC_LobbyMissionContext context)
	{
		SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
		if (!context.IsActive() || !aiWorld)
			return false;
		bool applied = true;
		foreach (string name : m_aEntities)
		{
			IEntity entity = GetGame().GetWorld().FindEntityByName(name);
			if (!entity)
				applied = false;
			else
				aiWorld.RequestNavmeshRebuildEntity(entity);
		}
		return applied;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Set Deletion Protection")]
class GC_SetDeletionProtectionAction : GC_MissionAction
{
	[Attribute("", UIWidgets.EditBox, "Names of entities to protect from this mission framework's deletion actions. Their children are protected too.")]
	ref array<string> m_aEntities;
	[Attribute("1", UIWidgets.CheckBox, "Protect. Off removes this mission's protection. Does not prevent damage or automatic cleanup.")]
	bool m_bProtected;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (m_aEntities && !m_aEntities.IsEmpty())
			return true;
		error = "Set Deletion Protection: add entity names.";
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool Execute(GC_LobbyMissionContext context)
	{
		if (!context.IsActive() || !context.GetRuntime())
			return false;
		bool applied = true;
		foreach (string name : m_aEntities)
		{
			IEntity entity = GetGame().GetWorld().FindEntityByName(name);
			if (!entity)
				applied = false;
			else
				context.GetRuntime().ProtectEntity(entity, m_bProtected);
		}
		return applied;
	}
}

//! Selection is completed before deletion; never mutate the world inside a query.
class GC_MissionDeletionQuery
{
	GC_LobbyMissionContext m_Context;
	ref array<ResourceName> m_aPrefabs;
	bool m_bExcludePrefabs;
	bool m_bRootsOnly;
	ref array<IEntity> m_aSelected = {};

	//------------------------------------------------------------------------------------------------
	bool Collect(IEntity entity)
	{
		if ((m_bRootsOnly && entity.GetParent()) || !m_Context.CanDelete(entity))
			return true;
		if (m_aPrefabs && !m_aPrefabs.IsEmpty())
		{
			bool matches;
			EntityPrefabData prefab = entity.GetPrefabData();
			if (prefab)
			{
				foreach (ResourceName name : m_aPrefabs)
				{
					if (SCR_BaseContainerTools.IsKindOf(prefab.GetPrefab(), name))
					{
						matches = true;
						break;
					}
				}
			}
			if (matches == m_bExcludePrefabs)
				return true;
		}
		m_aSelected.Insert(entity);
		return true;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Delete Entities In Area")]
class GC_DeleteEntitiesInAreaAction : GC_MissionAction
{
	[Attribute("", UIWidgets.EditBox, "Name of the deletion sphere's center entity.")]
	string m_sArea;
	[Attribute("20", UIWidgets.EditBox, "Deletion radius in meters.", params: "0.1 1000 1")]
	float m_fRadius;
	[Attribute("", UIWidgets.ResourceAssignArray, "Prefab filter, including inherited variants. Empty selects all entities in the area.", params: "et")]
	ref array<ResourceName> m_aPrefabs;
	[Attribute("0", UIWidgets.CheckBox, "Exclude these prefabs instead of selecting only them.")]
	bool m_bExcludePrefabs;
	[Attribute("1", UIWidgets.CheckBox, "Only select entities with a mesh or other visual object.")]
	bool m_bVisualObjectsOnly;
	[Attribute("1", UIWidgets.CheckBox, "Select only root entities; children are deleted with their parent.")]
	bool m_bRootsOnly;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (m_sArea.IsEmpty() || m_fRadius <= 0 || m_fRadius > 1000)
		{
			error = "Delete Entities In Area: enter a center and radius up to 1000 meters.";
			return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool Execute(GC_LobbyMissionContext context)
	{
		IEntity center = GetGame().GetWorld().FindEntityByName(m_sArea);
		if (!context.IsActive() || !center)
			return false;
		GC_MissionDeletionQuery query = new GC_MissionDeletionQuery();
		query.m_Context = context;
		query.m_aPrefabs = m_aPrefabs;
		query.m_bExcludePrefabs = m_bExcludePrefabs;
		query.m_bRootsOnly = m_bRootsOnly;
		EQueryEntitiesFlags flags = EQueryEntitiesFlags.ALL;
		if (m_bVisualObjectsOnly)
			flags = EQueryEntitiesFlags.WITH_OBJECT;
		GetGame().GetWorld().QueryEntitiesBySphere(center.GetOrigin(), m_fRadius, query.Collect, null, flags);
		foreach (IEntity entity : query.m_aSelected)
		{
			if (entity && entity != center && context.CanDelete(entity))
				SCR_EntityHelper.DeleteEntityAndChildren(entity);
		}
		return true;
	}
}
