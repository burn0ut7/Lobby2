#ifdef WORKBENCH
//! Discovers entity identities and existing names without naming or selecting entities.
class GC_EntityReferenceChoices
{
	//------------------------------------------------------------------------------------------------
	//! Identity discovery includes unnamed entities and never authors a name.
	array<IEntitySource> GetChoiceEntities(WorldEditorAPI api)
	{
		array<IEntitySource> sources = {};
		if (!api || !api.GetWorld())
			return sources;

		int entityCount = api.GetEditorEntityCount();
		for (int entityIndex; entityIndex < entityCount; entityIndex++)
		{
			IEntitySource source = api.GetEditorEntity(entityIndex);
			if (!source)
				continue;
			IEntity entity = api.SourceToEntity(source);
			if (entity && AcceptEntity(entity))
				sources.Insert(source);
		}
		return sources;
	}

	//------------------------------------------------------------------------------------------------
	array<string> GetChoices(WorldEditorAPI api)
	{
		array<string> values = {};
		if (!api || !api.GetWorld())
			return values;

		BaseWorld world = api.GetWorld();
		foreach (IEntitySource source : GetChoiceEntities(api))
		{
			IEntity entity = api.SourceToEntity(source);
			if (!entity)
				continue;

			string name = source.GetName();
			// A hierarchy's nice name is not evidence of a serialized reference.
			if (name.IsEmpty() || entity.GetName() != name)
				continue;
			if (api.FindEntityByName(name) != source || world.FindEntityByName(name) != entity)
				continue;
			values.Insert(name);
		}

		return values;
	}

	//------------------------------------------------------------------------------------------------
	protected bool AcceptEntity(IEntity entity)
	{
		return true;
	}
}
#endif
