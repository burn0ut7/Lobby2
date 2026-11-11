#ifdef WORKBENCH
//! Shared read-only discovery of authored manager definitions in the current world.
class GC_MissionReferenceChoices
{
	//------------------------------------------------------------------------------------------------
	array<string> GetChoices(WorldEditorAPI api)
	{
		array<string> values = {};
		if (!api || !api.GetWorld())
			return values;

		int entityCount = api.GetEditorEntityCount();
		for (int entityIndex; entityIndex < entityCount; entityIndex++)
		{
			IEntitySource entitySource = api.GetEditorEntity(entityIndex);
			if (!entitySource)
				continue;

			int componentCount = entitySource.GetComponentCount();
			for (int componentIndex; componentIndex < componentCount; componentIndex++)
			{
				IEntityComponentSource componentSource = entitySource.GetComponent(componentIndex);
				if (!componentSource)
					continue;

				typename componentType = componentSource.GetClassName().ToType();
				if (componentType && componentType.IsInherited(GC_LobbyManager))
					CollectNames(componentSource, values);
			}
		}

		return values;
	}

	//------------------------------------------------------------------------------------------------
	protected void CollectNames(BaseContainer manager, array<string> values)
	{
	}

	//------------------------------------------------------------------------------------------------
	protected void CollectName(BaseContainer definition, array<string> values)
	{
		string name;
		if (definition && definition.Get("m_sName", name) && !name.IsEmpty())
			values.Insert(name);
	}
}
#endif
