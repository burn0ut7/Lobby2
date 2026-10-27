#ifdef WORKBENCH
//! Editor-only choices. The native picker owns presentation; these providers
//! read the current authoring sources and never create flags or change a mission.
class GC_MissionFlagChoices
{
	//------------------------------------------------------------------------------------------------
	array<string> GetChoices(WorldEditorAPI api)
	{
		array<string> values = {};
		set<IEntitySource> visited = new set<IEntitySource>();
		for (int i; i < api.GetEditorEntityCount(); i++)
			CollectEntity(api.GetEditorEntity(i), values, visited);
		return values;
	}

	//------------------------------------------------------------------------------------------------
	protected void CollectEntity(IEntitySource entity, array<string> values, set<IEntitySource> visited)
	{
		if (!entity || visited.Contains(entity))
			return;
		visited.Insert(entity);
		typename entityType = entity.GetClassName().ToType();
		if (entityType && entityType.IsInherited(GC_MissionAuthoringEntity))
			Collect(entity, values);
		for (int j; j < entity.GetComponentCount(); j++)
		{
			IEntityComponentSource component = entity.GetComponent(j);
			typename componentType = component.GetClassName().ToType();
			if (componentType && (componentType.IsInherited(GC_LobbyManager) || componentType.IsInherited(ActionsManagerComponent)))
				Collect(component, values);
		}
		for (int child; child < entity.GetNumChildren(); child++)
			CollectEntity(IEntitySource.Cast(entity.GetChild(child)), values, visited);
	}

	//------------------------------------------------------------------------------------------------
	protected void Collect(BaseContainer source, array<string> values)
	{
		if (!source)
			return;
		typename sourceType = source.GetClassName().ToType();
		if (!sourceType)
			return;
		string name;
		if (sourceType.IsInherited(GC_MissionFlag) || sourceType.IsInherited(GC_MissionFlagEntity))
			source.Get("m_sName", name);
		else if (sourceType.IsInherited(GC_SetFlagAction) || sourceType.IsInherited(GC_RandomFlagAction) || sourceType.IsInherited(GC_SetFlagInteraction)
			|| sourceType.IsInherited(GC_SetFlagActionEntity) || sourceType.IsInherited(GC_RandomFlagActionEntity))
			source.Get("m_sFlag", name);
		else if (sourceType.IsInherited(GC_ChooseFlagsAction) || sourceType.IsInherited(GC_ChooseFlagsActionEntity))
		{
			array<string> names = {};
			source.Get("m_aFlags", names);
			values.InsertAll(names);
		}
		if (!name.IsEmpty())
			values.Insert(name);

		for (int i; i < source.GetNumVars(); i++)
		{
			string field = source.GetVarName(i);
			DataVarType fieldType = source.GetDataVarType(i);
			if (fieldType == DataVarType.OBJECT)
				Collect(source.GetObject(field), values);
			else if (fieldType == DataVarType.OBJECT_ARRAY)
			{
				BaseContainerList children = source.GetObjectArray(field);
				if (!children)
					continue;
				for (int j; j < children.Count(); j++)
					Collect(children.Get(j), values);
			}
		}
	}
}

class GC_MissionFactionChoices
{
	//------------------------------------------------------------------------------------------------
	array<string> GetChoices(WorldEditorAPI api)
	{
		array<string> values = {};
		for (int i; i < api.GetEditorEntityCount(); i++)
		{
			IEntitySource entity = api.GetEditorEntity(i);
			if (!entity)
				continue;
			typename entityType = entity.GetClassName().ToType();
			if (!entityType || !entityType.IsInherited(FactionManager))
				continue;
			BaseContainerList factions = entity.GetObjectArray("Factions");
			if (!factions)
				continue;
			for (int j; j < factions.Count(); j++)
			{
				string key;
				factions.Get(j).Get("FactionKey", key);
				if (!key.IsEmpty())
					values.Insert(key);
			}
		}
		return values;
	}
}

//! Player conditions offer the keys authored on playable groups with slots.
//! Mission validation separately checks whether those factions are defined.
class GC_PlayableFactionChoices
{
	//------------------------------------------------------------------------------------------------
	array<string> GetChoices(WorldEditorAPI api)
	{
		array<string> values = {};
		for (int i; i < api.GetEditorEntityCount(); i++)
		{
			IEntitySource entity = api.GetEditorEntity(i);
			if (!entity)
				continue;
			typename entityType = entity.GetClassName().ToType();
			if (!entityType || !entityType.IsInherited(SCR_AIGroup))
				continue;
			bool playable;
			entity.Get("m_bPlayable", playable);
			if (!playable)
				continue;
			array<ResourceName> slots = {};
			entity.Get("m_aUnitPrefabSlots", slots);
			if (slots.IsEmpty())
				continue;
			string factionKey;
			entity.Get("m_faction", factionKey);
			if (!factionKey.IsEmpty())
				values.Insert(factionKey);
		}
		return values;
	}
}
#endif
