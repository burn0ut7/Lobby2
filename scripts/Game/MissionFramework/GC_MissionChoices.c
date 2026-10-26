#ifdef WORKBENCH
//! Editor-only choices. The native picker owns presentation; these providers
//! read the current authoring sources and never create flags or change a mission.
class GC_MissionFlagChoices
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
			for (int j; j < entity.GetComponentCount(); j++)
			{
				IEntityComponentSource component = entity.GetComponent(j);
				typename componentType = component.GetClassName().ToType();
				if (componentType && (componentType.IsInherited(GC_LobbyManager) || componentType.IsInherited(ActionsManagerComponent)))
					Collect(component, values);
			}
		}
		return values;
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
		if (sourceType.IsInherited(GC_MissionFlag))
			source.Get("m_sName", name);
		else if (sourceType.IsInherited(GC_SetFlagAction) || sourceType.IsInherited(GC_RandomFlagAction) || sourceType.IsInherited(GC_SetFlagInteraction))
			source.Get("m_sFlag", name);
		else if (sourceType.IsInherited(GC_ChooseFlagsAction))
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
#endif
